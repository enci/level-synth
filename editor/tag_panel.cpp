//
// Created by Bojan Endrovski on 10/05/2026.
//

#include "tag_panel.hpp"
#include "tag_panel.hpp"

#include <algorithm>
#include <map>

namespace ls {

// ---------------------------------------------------------------------------
// Palette
// ---------------------------------------------------------------------------
static constexpr uint32_t k_palette[] = {
    IM_COL32( 82, 120, 200, 255),
    IM_COL32( 82, 200, 110, 255),
    IM_COL32(200,  82, 116, 255),
    IM_COL32(200, 160,  82, 255),
    IM_COL32(140,  82, 200, 255),
    IM_COL32( 82, 190, 176, 255),
    IM_COL32(200, 120,  82, 255),
};
static constexpr int k_palette_count = (int)(sizeof(k_palette) / sizeof(k_palette[0]));

/*static*/ uint32_t tag_panel::palette_color(int idx) {
    return k_palette[idx % k_palette_count];
}

// ---------------------------------------------------------------------------
// resolve_color
// ---------------------------------------------------------------------------
tag_panel::resolved_color tag_panel::resolve_color(
        ls::tag_registry& reg,
        const std::string& full_id,
        int l0_pal_idx) const {
    std::string id = full_id;
    bool inherited = false;
    while (true) {
        if (auto c = reg.get_color(id))
            return { *c, inherited };
        auto dot = id.rfind('.');
        if (dot == std::string::npos) break;
        id = id.substr(0, dot);
        inherited = true;
    }
    return { palette_color(l0_pal_idx), true };
}

// ---------------------------------------------------------------------------
// draw_swatch
// ---------------------------------------------------------------------------
bool tag_panel::draw_swatch(uint32_t color, bool inherited) {
    constexpr float sz = 11.0f, r = 2.5f;
    ImVec2 p  = ImGui::GetCursorScreenPos();
    ImVec2 p1 = { p.x + sz, p.y + sz };
    ImDrawList* dl = ImGui::GetWindowDrawList();
    if (inherited)
        dl->AddRect(p, p1, color, r, 0, 1.5f);
    else {
        dl->AddRectFilled(p, p1, color, r);
        dl->AddRect(p, p1, IM_COL32(0, 0, 0, 40), r, 0, 0.5f);
    }
    return ImGui::InvisibleButton("##sw", { sz, sz });
}

// ---------------------------------------------------------------------------
// draw_color_popup
// ---------------------------------------------------------------------------
void tag_panel::draw_color_popup(ls::tag_registry& reg,
                                  const std::string& full_id,
                                  uint32_t current_color) {
    if (m_color_target != full_id) return;
    ImGui::SetNextWindowSize({ 220, 0 }, ImGuiCond_Always);
    if (ImGui::BeginPopup("##tag_color_picker")) {
        ImVec4 col = ImGui::ColorConvertU32ToFloat4(current_color);
        if (ImGui::ColorPicker4("##cp", &col.x,
                ImGuiColorEditFlags_NoAlpha        |
                ImGuiColorEditFlags_DisplayHex     |
                ImGuiColorEditFlags_NoSidePreview))
            reg.set_color(full_id, ImGui::ColorConvertFloat4ToU32(col));
        ImGui::Separator();
        if (ImGui::SmallButton("Clear (inherit)")) {
            reg.clear_color(full_id);
            ImGui::CloseCurrentPopup();
            m_color_target.clear();
        }
        ImGui::EndPopup();
    } else {
        m_color_target.clear();
    }
}

// ---------------------------------------------------------------------------
// draw_inline_add
// Shared between root level (prefix="") and non-root (prefix=full_id).
// Shows the input widget when active, plus the muted "+ add tag" button.
// ---------------------------------------------------------------------------
void tag_panel::draw_inline_add(const std::string& prefix) {
    // Active input widget.
    if (m_adding_under == prefix) {
        ImGui::SetNextItemWidth(130.0f);
        if (m_add_buf[0] == '\0') ImGui::SetKeyboardFocusHere();
        bool enter = ImGui::InputText("##add", m_add_buf, sizeof(m_add_buf),
            ImGuiInputTextFlags_EnterReturnsTrue);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Name, then \xe2\x86\xb5  \xe2\x80\xa2  Esc to cancel");
        if (enter && m_add_buf[0] != '\0') {
            m_pending_add  = prefix.empty()
                ? m_add_buf
                : prefix + "." + m_add_buf;
            m_add_buf[0]   = '\0';
            m_adding_under = k_idle;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            m_add_buf[0]   = '\0';
            m_adding_under = k_idle;
        }
    }

    // Muted button — hidden while filter is active.
    if (!m_filter.IsActive()) {
        ImGui::PushStyleColor(ImGuiCol_Text,
            ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
        if (ImGui::SmallButton(("+ add tag##btn_" + prefix).c_str())) {
            m_adding_under = prefix;
            m_add_buf[0]   = '\0';
        }
        ImGui::PopStyleColor();
    }
}

// ---------------------------------------------------------------------------
// build_tree
// ---------------------------------------------------------------------------
void tag_panel::build_tree(ls::tag_registry& reg) {
    m_nodes.clear();
    m_roots.clear();

    std::map<std::string, int> id_to_idx;
    int l0_pal_counter = 0;
    std::map<std::string, int> l0_pal_map;

    for (const auto& full_id : reg.all_identifiers()) {
        if (m_filter.IsActive() && !m_filter.PassFilter(full_id.c_str()))
            continue;

        int depth = (int)std::count(full_id.begin(), full_id.end(), '.');
        std::string label = full_id.substr(full_id.rfind('.') + 1);
        std::string l0_name = full_id.substr(0, full_id.find('.'));

        if (l0_pal_map.find(l0_name) == l0_pal_map.end())
            l0_pal_map[l0_name] = l0_pal_counter++;
        int pal = l0_pal_map[l0_name];

        int idx = (int)m_nodes.size();
        m_nodes.push_back({ full_id, label, depth, pal, {} });
        id_to_idx[full_id] = idx;

        if (depth == 0) {
            m_roots.push_back(idx);
        } else {
            std::string parent_id = full_id.substr(0, full_id.rfind('.'));
            auto it = id_to_idx.find(parent_id);
            if (it != id_to_idx.end())
                m_nodes[it->second].children.push_back(idx);
        }
    }
}

// ---------------------------------------------------------------------------
// draw_row — identical for all depths
// ---------------------------------------------------------------------------
void tag_panel::draw_row(ls::tag_registry& reg, int idx) {
    // Copy the fields we need — m_nodes is stable during rendering but we
    // want to avoid holding a reference across recursive calls.
    const std::string full_id = m_nodes[idx].full_id;
    const std::string label   = m_nodes[idx].label;
    const int         depth   = m_nodes[idx].depth;
    const int         pal     = m_nodes[idx].l0_pal_idx;
    const bool        is_leaf = (depth == 2) || m_nodes[idx].children.empty();

    auto [color, inherited] = resolve_color(reg, full_id, pal);

    ImGui::PushID(full_id.c_str());

    // ---- Swatch ------------------------------------------------------------
    if (draw_swatch(color, inherited)) {
        m_color_target = full_id;
        ImGui::OpenPopup("##tag_color_picker");
    }
    draw_color_popup(reg, full_id, color);
    ImGui::SameLine(0, 5);

    // ---- Tree node ---------------------------------------------------------
    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_SpanFullWidth |
        ImGuiTreeNodeFlags_FramePadding  |
        (depth == 0 ? ImGuiTreeNodeFlags_DefaultOpen : 0) |
        (is_leaf    ? ImGuiTreeNodeFlags_Leaf         : 0);

    bool open = false;
    if (m_renaming == full_id) {
        ImGui::SetNextItemWidth(130.0f);
        if (ImGui::InputText("##ren", m_rename_buf, sizeof(m_rename_buf),
                ImGuiInputTextFlags_EnterReturnsTrue |
                ImGuiInputTextFlags_AutoSelectAll)) {
            if (m_rename_buf[0] != '\0') {
                std::string prefix = depth == 0
                    ? ""
                    : full_id.substr(0, full_id.rfind('.') + 1);
                m_pending_rename_from = full_id;
                m_pending_rename_to   = prefix + m_rename_buf;
            }
            m_renaming.clear();
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) m_renaming.clear();
        open = true;
    } else {
        open = ImGui::TreeNodeEx("##n", flags, "%s", label.c_str());
    }

    // ---- Tooltip -----------------------------------------------------------
    if (ImGui::IsItemHovered()) {
        if (auto t = reg.find(full_id))
            ImGui::SetTooltip("0x%06X\xc2\xb7%06X\xc2\xb7%06X\n%s",
                t->l0(), t->l1(), t->l2(), full_id.c_str());
    }

    // ---- Context menu ------------------------------------------------------
    if (ImGui::BeginPopupContextItem("##ctx")) {
        if (depth < 2 && ImGui::MenuItem("Add tag")) {
            m_adding_under = full_id;
            m_add_buf[0]   = '\0';
        }
        if (ImGui::MenuItem("Rename")) {
            m_renaming = full_id;
            strncpy(m_rename_buf, label.c_str(), sizeof(m_rename_buf) - 1);
            m_rename_buf[sizeof(m_rename_buf) - 1] = '\0';
        }
        if (ImGui::MenuItem("Edit color")) {
            m_color_target = full_id;
            ImGui::OpenPopup("##tag_color_picker");
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Delete")) m_pending_remove = full_id;
        ImGui::EndPopup();
    }

    // ---- Children ----------------------------------------------------------
    if (open) {
        for (int child_idx : m_nodes[idx].children)
            draw_row(reg, child_idx);

        if (depth < 2)
            draw_inline_add(full_id);

        ImGui::TreePop();
    }

    ImGui::PopID();
}

// ---------------------------------------------------------------------------
// draw
// ---------------------------------------------------------------------------
void tag_panel::draw(ls::tag_registry& reg) {
    ImGui::Begin("Tags");

    // ---- Toolbar -----------------------------------------------------------
    m_filter.Draw("##filter", ImGui::GetContentRegionAvail().x - 26.0f);
    ImGui::SameLine();
    if (ImGui::SmallButton("+")) {
        m_adding_under = "";
        m_add_buf[0]   = '\0';
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Add root category");
    ImGui::Separator();

    // ---- Build tree --------------------------------------------------------
    build_tree(reg);

    // ---- Render ------------------------------------------------------------
    for (int root_idx : m_roots)
        draw_row(reg, root_idx);

    // Root-level add — same widget as inside draw_row, prefix is just "".
    draw_inline_add("");

    // ---- Apply pending mutations -------------------------------------------
    if (!m_pending_add.empty())
        reg.add(m_pending_add);

    if (!m_pending_remove.empty())
        reg.remove(m_pending_remove);

    if (!m_pending_rename_from.empty())
        reg.rename(m_pending_rename_from, m_pending_rename_to);

    ImGui::End();
}

}