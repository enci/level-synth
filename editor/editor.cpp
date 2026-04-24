#include "editor.hpp"
#include "imgui_visitor.hpp"
#include "phosphor_icons.hpp"
#include <imgui_node_editor_node_builder.h>
#include <imgui_node_editor_pin_icons.h>
#include <fstream>

#define NOMINMAX
#include <portable-file-dialogs.h>

#include "imgui_internal.h"

#include <level_synth/nodes/node_create_grid.hpp>
#include <level_synth/nodes/node_cellular_automata.hpp>
#include <level_synth/nodes/node_input_number.hpp>
#include <level_synth/nodes/node_noise_grid.hpp>
#include <level_synth/nodes/node_output_grid.hpp>
#include <level_synth/nodes/node_output_number.hpp>
#include <level_synth/node_registry.hpp>
#include <level_synth/grid.hpp>
#include <map>
#include <set>
#include <algorithm>

namespace ed = ax::NodeEditor;

editor::~editor() {
    if (m_node_editor_context) {
        ed::DestroyEditor(m_node_editor_context);
        m_node_editor_context = nullptr;
    }
}

void editor::init() {
    ax::NodeEditor::Config config;
    config.SettingsFile = "NodeEditor.json";
    config.EnableSmoothZoom = false;
    config.NavigateButtonIndex = 2;
    config.CustomZoomLevels.push_back(1.0f);
    config.CanvasSizeMode = ax::NodeEditor::CanvasSizeMode::CenterOnly;
    m_node_editor_context = ed::CreateEditor(&config);
    ed::SetCurrentEditor(m_node_editor_context);

    // Test graph: Create Grid -> Noise Grid -> Cellular Automata -> Output Grid
    auto& graph = m_generator.graph();
    int create_id = graph.add_node(std::make_unique<ls::node_create_grid>());
    int noise_id  = graph.add_node(std::make_unique<ls::node_noise_grid>());
    int ca_id     = graph.add_node(std::make_unique<ls::node_cellular_automata>());
    int out_id    = graph.add_node(std::make_unique<ls::node_output_grid>());

    graph.add_wire({ create_id, "grid", noise_id, "grid"   });
    graph.add_wire({ noise_id,  "grid", ca_id,    "input"  });
    graph.add_wire({ ca_id,  "output", out_id,    "value"  });

    m_link_to_wire[m_next_link_id++] = { create_id, "grid", noise_id, "grid"   };
    m_link_to_wire[m_next_link_id++] = { noise_id,  "grid", ca_id,    "input"  };
    m_link_to_wire[m_next_link_id++] = { ca_id,  "output", out_id,    "value"  };

    m_colors[editor_colors::Color_PinNumber]           = ImVec4(0.25f, 0.75f, 0.85f, 1.0f);
    m_colors[editor_colors::Color_PinGrid]             = ImVec4(0.65f, 0.40f, 0.85f, 1.0f);
    m_colors[editor_colors::Color_HeaderInput]         = ImVec4(0.30f, 0.60f, 0.30f, 1.0f);
    m_colors[editor_colors::Color_HeaderProcess]       = ImVec4(0.75f, 0.45f, 0.20f, 1.0f);
    m_colors[editor_colors::Color_HeaderOutput]        = ImVec4(0.30f, 0.45f, 0.70f, 1.0f);
    m_colors[editor_colors::Color_ToolbarBg]           = ImVec4(0.95f, 0.95f, 0.96f, 0.92f);
    m_colors[editor_colors::Color_ToolbarBorder]       = ImVec4(0.70f, 0.70f, 0.72f, 0.60f);
    m_colors[editor_colors::Color_ToolbarButtonHovered]= ImVec4(0.80f, 0.80f, 0.82f, 0.60f);
    m_colors[editor_colors::Color_ToolbarButtonActive] = ImVec4(0.70f, 0.70f, 0.72f, 0.80f);

    m_generator.evaluate();
}

void editor::draw() {
    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(vp->WorkSize, ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::Begin("Node Editor", nullptr,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoTitleBar  | ImGuiWindowFlags_NoResize          |
        ImGuiWindowFlags_NoMove      | ImGuiWindowFlags_NoBringToFrontOnFocus);
    draw_node_editor();
    ImGui::End();
    ImGui::PopStyleVar();

    draw_toolbar();
    draw_details_panel();

    if (m_show_demo_window)
        ImGui::ShowDemoWindow(&m_show_demo_window);
}

ed::NodeId editor::make_node_id(int node_id) {
    return ed::NodeId(k_node_tag | static_cast<uintptr_t>(node_id));
}

ed::PinId editor::make_pin_id(int node_id, int pin_index) {
    return ed::PinId(k_pin_tag | (static_cast<uintptr_t>(node_id) << 8) | static_cast<uintptr_t>(pin_index));
}

ed::LinkId editor::make_link_id(int link_id) {
    return ed::LinkId(k_link_tag | static_cast<uintptr_t>(link_id));
}

std::pair<int, int> editor::unpack_pin_id(ed::PinId pin_id) {
    auto val = pin_id.Get() & ~k_pin_tag;
    return { static_cast<int>(val >> 8), static_cast<int>(val & 0xFF) };
}

ImVec4 editor::pin_color(ls::pin_type type) const {
    switch (type) {
        case ls::pin_type::number: return m_colors[editor_colors::Color_PinNumber];
        case ls::pin_type::grid:   return m_colors[editor_colors::Color_PinGrid];
    }
    return ImVec4(1, 1, 1, 1);
}

ImVec4 editor::category_color(const std::string& category) const {
    if (category == "IO")         return m_colors[editor_colors::Color_HeaderInput];
    if (category == "Generation") return m_colors[editor_colors::Color_HeaderProcess];
    if (category == "Analysis")   return m_colors[editor_colors::Color_HeaderProcess];
    return m_colors[editor_colors::Color_HeaderOutput];
}

editor::pin_info editor::resolve_pin(ed::PinId pin_id) const {
    auto [node_id, pin_index] = unpack_pin_id(pin_id);
    auto* n = m_generator.graph().find_node(node_id);
    if (!n) return { node_id, pin_index, nullptr };
    const auto& desc = n->descriptor();
    if (pin_index < 0 || pin_index >= static_cast<int>(desc.pins.size()))
        return { node_id, pin_index, nullptr };
    return { node_id, pin_index, &desc.pins[pin_index] };
}

void editor::draw_node_editor() {
    ed::SetCurrentEditor(m_node_editor_context);
    ed::Begin("Level Synth", ImVec2(0.0, 0.0f));

    auto& eng = m_generator.engine();
    auto& graph = m_generator.graph();
    ed::NodeBuilder builder;
    const ImVec2 icon_size(16, 16);
    auto& reg = ls::node_registry::instance();

    // Build set of connected input pins for this frame
    std::set<std::pair<int, std::string>> connected_inputs;
    for (const auto& [lid, wv] : m_link_to_wire)
        connected_inputs.emplace(wv.to_node, wv.to_pin);

    // --- Render all nodes ---
    for (int node_id : graph.node_ids()) {
        auto* n = graph.find_node(node_id);
        if (!n) continue;

        const auto& desc = n->descriptor();
        const auto* entry = reg.find(*n);
        assert(entry && "Node registry entry not found for node");
        ImVec4 header_color = category_color(entry->category);

        if (m_first_frame) {
            ed::SetNodePosition(make_node_id(node_id),
                ImVec2(10.0f + (node_id - 1) * 300.0f, 10.0f));
        }

        builder.Begin(make_node_id(node_id));
        {
            builder.Header(header_color);
                ImGui::TextUnformatted(entry->display_name.c_str());
                ImGui::Dummy(ImVec2(80, 0));
            builder.EndHeader();

            builder.BeginColumns();
            {
                // Input pins (left column)
                ImGui::BeginGroup();
                for (int i = 0; i < static_cast<int>(desc.pins.size()); i++) {
                    const auto& pin = desc.pins[i];
                    if (pin.direction != ls::pin_direction::input) continue;

                    auto pid = make_pin_id(node_id, i);
                    ImVec4 color = pin_color(pin.type);
                    bool is_connected = connected_inputs.count({node_id, pin.name}) > 0;

                    ed::BeginPin(pid, ed::PinKind::Input);
                        ed::DrawPinIcon(icon_size, ed::PinIconType::Circle, is_connected, color);
                        ImGui::SameLine();
                        if (is_connected) {
                            ImGui::TextDisabled("%s", pin.name.c_str());
                        } else {
                            ImGui::TextUnformatted(pin.name.c_str());
                        }
                    ed::EndPin();
                }
                ImGui::EndGroup();

                ImGui::SameLine(0, 20);

                // Output pins (right column)
                ImGui::BeginGroup();
                for (int i = 0; i < static_cast<int>(desc.pins.size()); i++) {
                    const auto& pin = desc.pins[i];
                    if (pin.direction != ls::pin_direction::output) continue;

                    auto pid = make_pin_id(node_id, i);
                    ImVec4 color = pin_color(pin.type);

                    ed::BeginPin(pid, ed::PinKind::Output);
                        ImGui::TextUnformatted(pin.name.c_str());
                        ImGui::SameLine();
                        ed::DrawPinIcon(icon_size, ed::PinIconType::Circle, true, color);
                    ed::EndPin();
                }
                ImGui::EndGroup();
            }
            builder.EndColumns();

            // Grid preview — only on sink nodes (all pins are inputs, e.g. Output Grid)
            const bool is_sink = std::none_of(desc.pins.begin(), desc.pins.end(),
                [](const ls::pin_descriptor& p){ return p.direction == ls::pin_direction::output; });

            if (is_sink) {
                for (const auto& pin : desc.pins) {
                    if (pin.type != ls::pin_type::grid) continue;
                    const auto* val = eng.get_output(node_id, pin.name);
                    if (!val || !std::holds_alternative<std::shared_ptr<ls::grid>>(*val))
                        continue;
                    const auto& g = std::get<std::shared_ptr<ls::grid>>(*val);
                    if (!g || g->width() == 0 || g->height() == 0) continue;

                    constexpr float k_preview_w = 150.0f;
                    const float cell      = k_preview_w / static_cast<float>(g->width());
                    const float preview_h = cell * static_cast<float>(g->height());

                    int min_v = g->get(0, 0), max_v = g->get(0, 0);
                    for (int py = 0; py < g->height(); py++)
                        for (int px = 0; px < g->width(); px++) {
                            int cv = g->get(px, py);
                            min_v = std::min(min_v, cv);
                            max_v = std::max(max_v, cv);
                        }
                    const int range = std::max(1, max_v - min_v);

                    ImVec2 origin = ImGui::GetCursorScreenPos();
                    ImDrawList* dl = ImGui::GetWindowDrawList();
                    for (int py = 0; py < g->height(); py++) {
                        for (int px = 0; px < g->width(); px++) {
                            float t = static_cast<float>(g->get(px, py) - min_v)
                                      / static_cast<float>(range);
                            int v = static_cast<int>(t * 210 + 20);
                            ImVec2 p0(origin.x + px * cell,  origin.y + py * cell);
                            ImVec2 p1(p0.x + cell + 0.5f,   p0.y + cell + 0.5f);
                            dl->AddRectFilled(p0, p1, IM_COL32(v, v, v, 255));
                        }
                    }
                    ImGui::Dummy(ImVec2(k_preview_w, preview_h));
                    break;
                }
            }
        }
        builder.End();
    }

    // --- Render links from side map ---
    for (const auto& [link_id, wv] : m_link_to_wire) {
        auto* from_node = graph.find_node(wv.from_node);
        auto* to_node = graph.find_node(wv.to_node);
        if (!from_node || !to_node) continue;

        const auto& from_desc = from_node->descriptor();
        const auto& to_desc = to_node->descriptor();

        ed::PinId from_pin_id, to_pin_id;
        ImVec4 link_color(1, 1, 1, 1);

        for (int i = 0; i < static_cast<int>(from_desc.pins.size()); i++) {
            if (from_desc.pins[i].name == wv.from_pin &&
                from_desc.pins[i].direction == ls::pin_direction::output) {
                from_pin_id = make_pin_id(wv.from_node, i);
                link_color = pin_color(from_desc.pins[i].type);
                break;
            }
        }

        for (int i = 0; i < static_cast<int>(to_desc.pins.size()); i++) {
            if (to_desc.pins[i].name == wv.to_pin &&
                to_desc.pins[i].direction == ls::pin_direction::input) {
                to_pin_id = make_pin_id(wv.to_node, i);
                break;
            }
        }

        ed::Link(make_link_id(link_id), from_pin_id, to_pin_id, link_color, 2.0f);
    }

    // --- Handle new connections ---
    if (ed::BeginCreate()) {
        ed::PinId input_pin_id, output_pin_id;
        if (ed::QueryNewLink(&input_pin_id, &output_pin_id)) {
            if (input_pin_id && output_pin_id) {
                auto input_info = resolve_pin(input_pin_id);
                auto output_info = resolve_pin(output_pin_id);

                const ls::pin_descriptor* src = nullptr;
                const ls::pin_descriptor* dst = nullptr;
                int src_node, dst_node;
                std::string src_pin, dst_pin;

                if (input_info.desc && input_info.desc->direction == ls::pin_direction::output &&
                    output_info.desc && output_info.desc->direction == ls::pin_direction::input) {
                    src = input_info.desc;  src_node = input_info.node_id;  src_pin = src->name;
                    dst = output_info.desc; dst_node = output_info.node_id; dst_pin = dst->name;
                } else if (input_info.desc && input_info.desc->direction == ls::pin_direction::input &&
                           output_info.desc && output_info.desc->direction == ls::pin_direction::output) {
                    dst = input_info.desc;  dst_node = input_info.node_id;  dst_pin = dst->name;
                    src = output_info.desc; src_node = output_info.node_id; src_pin = src->name;
                }

                bool can_connect = src && dst && src->type == dst->type;

                if (can_connect) {
                    if (ed::AcceptNewItem()) {
                        ls::wire w{ src_node, src_pin, dst_node, dst_pin };
                        graph.add_wire(w);
                        int link_id = m_next_link_id++;
                        m_link_to_wire[link_id] = { src_node, src_pin, dst_node, dst_pin };
                    }
                } else {
                    ed::RejectNewItem(ImVec4(1, 0, 0, 1), 2.0f);
                }
            }
        }
    }
    ed::EndCreate();

    // --- Handle deletions ---
    if (ed::BeginDelete()) {
        ed::LinkId deleted_link_id;
        while (ed::QueryDeletedLink(&deleted_link_id)) {
            if (ed::AcceptDeletedItem()) {
                int lid = static_cast<int>(deleted_link_id.Get() & ~k_link_tag);
                auto it = m_link_to_wire.find(lid);
                if (it != m_link_to_wire.end()) {
                    const auto& wv = it->second;
                    graph.remove_wire(wv.from_node, wv.from_pin,
                                      wv.to_node, wv.to_pin);
                    m_link_to_wire.erase(it);
                }
            }
        }

        ed::NodeId deleted_node_id;
        while (ed::QueryDeletedNode(&deleted_node_id)) {
            if (ed::AcceptDeletedItem()) {
                int nid = static_cast<int>(deleted_node_id.Get() & ~k_node_tag);
                std::erase_if(m_link_to_wire, [&](const auto& pair) {
                    const auto& wv = pair.second;
                    return wv.from_node == nid || wv.to_node == nid;
                });
                graph.remove_node(nid);
            }
        }
    }
    ed::EndDelete();

    // --- Right-click context menu for adding nodes ---
    // Detect right-click before Suspend so we have the canvas-space position,
    // but defer OpenPopup until after Suspend so ImGui anchors the popup in
    // normal screen coordinates (not the canvas-transformed space).
    bool open_add_node_popup = ed::ShowBackgroundContextMenu();
    if (open_add_node_popup)
        m_popup_canvas_pos = ed::ScreenToCanvas(ImGui::GetMousePos());

    ed::Suspend();
    if (open_add_node_popup)
        ImGui::OpenPopup("##add_node");
    if (ImGui::BeginPopup("##add_node")) {
        ImGui::TextUnformatted("Add Node");
        ImGui::Separator();

        auto types = reg.entries();

        std::map<std::string, std::vector<const ls::node_registration*>> by_category;
        for (const auto& type : types)
            by_category[type.second.category].push_back(&type.second);

        for (auto& [category, cat_types] : by_category) {
            std::sort(cat_types.begin(), cat_types.end(), [](const auto& a, const auto& b) {
                return a->display_name < b->display_name;
            });

            if (ImGui::BeginMenu(category.c_str())) {
                for (const auto& type : cat_types) {
                    if (ImGui::MenuItem(type->display_name.c_str())) {
                        int nid = graph.add_node(reg.create(type->type_name));
                        ed::SetNodePosition(make_node_id(nid), m_popup_canvas_pos);
                    }
                }
                ImGui::EndMenu();
            }
        }

        ImGui::EndPopup();
    }
    ed::Resume();

    ed::End();
    ed::SetCurrentEditor(nullptr);
    m_first_frame = false;
}

void editor::draw_toolbar() {
    auto& io = ImGui::GetIO();
    const float scale = 1.0f;

    const float toolbar_height = 36.0f * scale;
    const float button_size = 24.0f * scale;

    float toolbar_width = 540.0f * scale;
    float toolbar_x = (io.DisplaySize.x - toolbar_width) * 0.5f;
    float toolbar_y = 12.0f * scale;

    draw_window_shadow(ImVec2(toolbar_x, toolbar_y), ImVec2(toolbar_width, toolbar_height));
    ImGui::SetNextWindowPos(ImVec2(toolbar_x, toolbar_y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(toolbar_width, toolbar_height), ImGuiCond_Always);

    ImGui::Begin("##toolbar", nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoCollapse);

    // Hamburger menu button
    if (ImGui::Button(phosphor::PH_LIST, ImVec2(button_size, button_size)))
        ImGui::OpenPopup("MainMenu");

    if (ImGui::BeginPopup("MainMenu")) {
        if (ImGui::BeginMenu("File")) {
            ImGui::MenuItem("New");
            ImGui::MenuItem("Open...");
            if (ImGui::MenuItem("Save"))
                save_graph();
            if (ImGui::MenuItem("Save As..."))
                save_graph_as();
            ImGui::Separator();
            if (ImGui::MenuItem("Quit"))
                ImGui::GetIO().AddKeyEvent(ImGuiKey_Escape, true); // handled by application
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit")) {
            ImGui::MenuItem("Undo");
            ImGui::MenuItem("Redo");
            ImGui::Separator();
            ImGui::MenuItem("Cut");
            ImGui::MenuItem("Copy");
            ImGui::MenuItem("Paste");
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("ImGui Demo", nullptr, m_show_demo_window))
                m_show_demo_window = !m_show_demo_window;
            ImGui::Separator();
            ImGui::MenuItem("About Level Synth");
            ImGui::EndMenu();
        }
        ImGui::EndPopup();
    }

    ImGui::SameLine();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine();

    // Theme toggle
    const char* theme_label = m_dark_theme ? phosphor::PH_MOON : phosphor::PH_SUN;
    if (ImGui::Button(theme_label, ImVec2(button_size, button_size))) {
        m_dark_theme = !m_dark_theme;
        // apply_theme();
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Toggle theme");

    ImGui::SameLine();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine();

    // Zoom controls
    ed::SetCurrentEditor(m_node_editor_context);
    float inv_scale = ed::GetCurrentZoom();

    if (ImGui::Button(phosphor::PH_MAGNIFYING_GLASS_MINUS, ImVec2(button_size, button_size)))
        ed::SetCurrentZoom(inv_scale * 1.25f);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Zoom out");

    ImGui::SameLine();

    char zoom_buf[16];
    snprintf(zoom_buf, sizeof(zoom_buf), "%d%%", (int)(100.0f / inv_scale + 0.5f));
    if (ImGui::Button(zoom_buf, ImVec2(0, button_size)))
        ed::SetCurrentZoom(1.0f);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Reset zoom to 100%%");

    ImGui::SameLine();

    if (ImGui::Button(phosphor::PH_MAGNIFYING_GLASS_PLUS, ImVec2(button_size, button_size)))
        ed::SetCurrentZoom(inv_scale * 0.8f);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Zoom in");

    ImGui::SameLine();

    if (ImGui::Button(phosphor::PH_FRAME_CORNERS, ImVec2(button_size, button_size)))
        ed::NavigateToContent();
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Fit to content");

    ed::SetCurrentEditor(nullptr);

    ImGui::SameLine();

    // Seed input + Evaluate button
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine();
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4.0f * scale);
    ImGui::TextUnformatted("Seed");
    ImGui::SameLine();
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 4.0f * scale);
    ImGui::SetNextItemWidth(64.0f * scale);
    ImGui::DragInt("##seed", &m_seed);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Global generation seed");
    ImGui::SameLine();

    if (ImGui::Button(phosphor::PH_PLAY, ImVec2(button_size, button_size))) {
        m_generator.set_seed(m_seed);
        m_generator.evaluate();
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Evaluate graph");

    ImGui::SameLine();

    if (ImGui::Button(phosphor::PH_APP_WINDOW, ImVec2(button_size, button_size)))
        m_show_demo_window = !m_show_demo_window;
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("ImGui demo");

    ImGui::SameLine();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine();

    // FPS display
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4.0f * scale);
    ImGui::TextDisabled("%.0f fps", io.Framerate);

    ImGui::End();
}

void editor::save_graph_as() {
    auto dialog = pfd::save_file(
        "Save Graph",
        m_current_file.empty() ? "graph.json" : m_current_file.string(),
        { "Level Synth Graph", "*.json", "All Files", "*" }
    );
    std::filesystem::path path = dialog.result();
    if (path.empty()) return;

    std::string json = m_generator.graph().save();
    std::filesystem::path tmp = path;
    tmp += ".tmp";
    {
        std::ofstream f(tmp);
        if (!f || !(f << json) || !f.flush()) {
            std::filesystem::remove(tmp);
            pfd::message("Save failed", "Could not write to file:\n" + tmp.string(), pfd::choice::ok, pfd::icon::error);
            return;
        }
    }
    std::error_code ec;
    std::filesystem::rename(tmp, path, ec);
    if (ec) {
        std::filesystem::remove(tmp);
        pfd::message("Save failed", "Could not replace file:\n" + path.string(), pfd::choice::ok, pfd::icon::error);
        return;
    }
    m_current_file = path;
}

void editor::save_graph() {
    if (m_current_file.empty()) {
        save_graph_as();
        return;
    }
    std::string json = m_generator.graph().save();
    std::filesystem::path tmp = m_current_file;
    tmp += ".tmp";
    {
        std::ofstream f(tmp);
        if (!f || !(f << json) || !f.flush()) {
            std::filesystem::remove(tmp);
            pfd::message("Save failed", "Could not write to file:\n" + tmp.string(), pfd::choice::ok, pfd::icon::error);
            return;
        }
    }
    std::error_code ec;
    std::filesystem::rename(tmp, m_current_file, ec);
    if (ec) {
        std::filesystem::remove(tmp);
        pfd::message("Save failed", "Could not replace file:\n" + m_current_file.string(), pfd::choice::ok, pfd::icon::error);
    }
}

void editor::set_node_editor_style() {
    ed::Style& edStyle = ed::GetStyle();
    edStyle.NodePadding              = ImVec4(8, 8, 8, 8);
    edStyle.NodeRounding             = 12.0f;
    edStyle.HoveredNodeBorderWidth   = 3.0f;
    edStyle.HoverNodeBorderOffset    = 0.0f;
    edStyle.SelectedNodeBorderWidth  = 4.0f;
    edStyle.SelectedNodeBorderOffset = 0.0f;
    edStyle.PinRounding              = 4.0f;
    edStyle.PinBorderWidth           = 0.0f;
    edStyle.LinkStrength             = 100.0f;
    edStyle.SourceDirection          = ImVec2(1.0f, 0.0f);
    edStyle.TargetDirection          = ImVec2(-1.0f, 0.0f);
    edStyle.ScrollDuration           = 0.35f;
    edStyle.FlowMarkerDistance       = 30.0f;
    edStyle.FlowSpeed                = 150.0f;
    edStyle.FlowDuration             = 2.0f;
    edStyle.PivotAlignment           = ImVec2(0.5f, 0.5f);
    edStyle.PivotSize                = ImVec2(0.0f, 0.0f);
    edStyle.PivotScale               = ImVec2(1, 1);
    edStyle.PinCorners               = ImDrawFlags_RoundCornersAll;
    edStyle.PinRadius                = 0.0f;
    edStyle.PinArrowSize             = 0.0f;
    edStyle.PinArrowWidth            = 0.0f;
    edStyle.GroupRounding            = 6.0f;
    edStyle.GroupBorderWidth         = 1.0f;
    edStyle.HighlightConnectedLinks  = 0.0f;
    edStyle.SnapLinkToPinDir         = 0.0f;
}

void editor::apply_theme() {
    ed::SetCurrentEditor(m_node_editor_context);
    if (m_dark_theme)
        set_dark_theme();
    else
        set_light_theme();
    ed::SetCurrentEditor(nullptr);
}

void editor::set_light_theme() {
    auto& style = ImGui::GetStyle();
    style.WindowBorderSize = 0.0f;
    style.FrameBorderSize = 0.0f;
    style.FrameRounding = 5.0f;
    style.ChildRounding = 5.0f;
    style.GrabRounding = 5.0f;

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Text]                   = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg]               = ImVec4(0.90f, 0.90f, 0.91f, 1.00f);
    colors[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.05f);
    colors[ImGuiCol_PopupBg]                = ImVec4(0.95f, 0.95f, 0.96f, 0.98f);
    colors[ImGuiCol_Border]                 = ImVec4(0.70f, 0.70f, 0.72f, 0.40f);
    colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]                = ImVec4(0.82f, 0.82f, 0.83f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.75f, 0.75f, 0.77f, 1.00f);
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.68f, 0.68f, 0.70f, 1.00f);
    colors[ImGuiCol_TitleBg]                = ImVec4(0.82f, 0.82f, 0.83f, 1.00f);
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.75f, 0.75f, 0.77f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.90f, 0.90f, 0.91f, 0.75f);
    colors[ImGuiCol_MenuBarBg]              = ImVec4(0.86f, 0.86f, 0.87f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.86f, 0.86f, 0.87f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.70f, 0.70f, 0.72f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.60f, 0.60f, 0.62f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.50f, 0.50f, 0.52f, 1.00f);
    colors[ImGuiCol_CheckMark]              = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_SliderGrab]             = ImVec4(0.55f, 0.55f, 0.57f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.45f, 0.45f, 0.47f, 1.00f);
    colors[ImGuiCol_Button]                 = ImVec4(0.78f, 0.78f, 0.80f, 1.00f);
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.70f, 0.70f, 0.72f, 1.00f);
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.62f, 0.62f, 0.64f, 1.00f);
    colors[ImGuiCol_Header]                 = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.26f, 0.59f, 0.98f, 0.50f);
    colors[ImGuiCol_HeaderActive]           = ImVec4(0.26f, 0.59f, 0.98f, 0.70f);
    colors[ImGuiCol_Separator]              = ImVec4(0.70f, 0.70f, 0.72f, 0.50f);
    colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.26f, 0.59f, 0.98f, 0.60f);
    colors[ImGuiCol_SeparatorActive]        = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_ResizeGrip]             = ImVec4(0.26f, 0.59f, 0.98f, 0.20f);
    colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.26f, 0.59f, 0.98f, 0.50f);
    colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_TabHovered]             = ImVec4(0.26f, 0.59f, 0.98f, 0.60f);
    colors[ImGuiCol_Tab]                    = ImVec4(0.78f, 0.78f, 0.80f, 0.86f);
    colors[ImGuiCol_TabSelected]            = ImVec4(0.70f, 0.70f, 0.72f, 1.00f);
    colors[ImGuiCol_TabSelectedOverline]    = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_TabDimmed]              = ImVec4(0.86f, 0.86f, 0.87f, 0.97f);
    colors[ImGuiCol_TabDimmedSelected]      = ImVec4(0.78f, 0.78f, 0.80f, 1.00f);
    colors[ImGuiCol_TabDimmedSelectedOverline] = ImVec4(0.50f, 0.50f, 0.50f, 0.00f);
    colors[ImGuiCol_PlotLines]              = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.78f, 0.78f, 0.80f, 1.00f);
    colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.70f, 0.70f, 0.72f, 1.00f);
    colors[ImGuiCol_TableBorderLight]       = ImVec4(0.78f, 0.78f, 0.80f, 1.00f);
    colors[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt]          = ImVec4(0.00f, 0.00f, 0.00f, 0.04f);
    colors[ImGuiCol_TextLink]               = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
    colors[ImGuiCol_DragDropTarget]         = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavCursor]              = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);

    m_colors[editor_colors::Color_PinNumber]           = ImVec4(0.25f, 0.75f, 0.85f, 1.0f);
    m_colors[editor_colors::Color_PinGrid]             = ImVec4(0.65f, 0.40f, 0.85f, 1.0f);
    m_colors[editor_colors::Color_HeaderInput]         = ImVec4(0.30f, 0.60f, 0.30f, 1.0f);
    m_colors[editor_colors::Color_HeaderProcess]       = ImVec4(0.75f, 0.45f, 0.20f, 1.0f);
    m_colors[editor_colors::Color_HeaderOutput]        = ImVec4(0.30f, 0.45f, 0.70f, 1.0f);
    m_colors[editor_colors::Color_ToolbarBg]           = ImVec4(0.95f, 0.95f, 0.96f, 0.92f);
    m_colors[editor_colors::Color_ToolbarBorder]       = ImVec4(0.70f, 0.70f, 0.72f, 0.60f);
    m_colors[editor_colors::Color_ToolbarButtonHovered]= ImVec4(0.80f, 0.80f, 0.82f, 0.60f);
    m_colors[editor_colors::Color_ToolbarButtonActive] = ImVec4(0.70f, 0.70f, 0.72f, 0.80f);

    set_node_editor_style();

    using namespace ax::NodeEditor;
    ed::Style& edStyle = ed::GetStyle();
    edStyle.NodeBorderWidth = 1.0f;
    edStyle.Colors[StyleColor_Bg]                  = ImColor(0xFFE5E5E7);
    edStyle.Colors[StyleColor_Grid]                = ImColor(180, 180, 182, 40);
    edStyle.Colors[StyleColor_NodeBg]              = ImColor(0xFFF5F5F6);
    edStyle.Colors[StyleColor_NodeBorder]          = ImColor(180, 180, 185, 128);
    edStyle.Colors[StyleColor_HovNodeBorder]       = ImColor(50, 150, 250, 255);
    edStyle.Colors[StyleColor_SelNodeBorder]       = ImColor(250, 160, 50, 255);
    edStyle.Colors[StyleColor_NodeSelRect]         = ImColor(5, 130, 255, 48);
    edStyle.Colors[StyleColor_NodeSelRectBorder]   = ImColor(5, 130, 255, 96);
    edStyle.Colors[StyleColor_HovLinkBorder]       = ImColor(50, 150, 250, 255);
    edStyle.Colors[StyleColor_SelLinkBorder]       = ImColor(250, 160, 50, 255);
    edStyle.Colors[StyleColor_HighlightLinkBorder] = ImColor(200, 100, 0, 255);
    edStyle.Colors[StyleColor_LinkSelRect]         = ImColor(5, 130, 255, 48);
    edStyle.Colors[StyleColor_LinkSelRectBorder]   = ImColor(5, 130, 255, 96);
    edStyle.Colors[StyleColor_PinRect]             = ImColor(50, 150, 250, 80);
    edStyle.Colors[StyleColor_PinRectBorder]       = ImColor(50, 150, 250, 128);
    edStyle.Colors[StyleColor_Flow]                = ImColor(255, 128, 64, 255);
    edStyle.Colors[StyleColor_FlowMarker]          = ImColor(255, 128, 64, 255);
    edStyle.Colors[StyleColor_GroupBg]             = ImColor(0, 0, 0, 24);
    edStyle.Colors[StyleColor_GroupBorder]         = ImColor(0, 0, 0, 32);
}

void editor::set_dark_theme() {
    auto& style = ImGui::GetStyle();
    style.WindowBorderSize = 0.0f;
    style.FrameBorderSize = 0.0f;
    style.FrameRounding = 5.0f;
    style.ChildRounding = 5.0f;
    style.GrabRounding = 5.0f;

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Text]                   = ImVec4(0.85f, 0.85f, 0.85f, 1.00f);
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg]               = ImVec4(0.16f, 0.16f, 0.17f, 1.00f);
    colors[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.15f);
    colors[ImGuiCol_PopupBg]                = ImVec4(0.12f, 0.12f, 0.13f, 0.96f);
    colors[ImGuiCol_Border]                 = ImVec4(0.35f, 0.35f, 0.38f, 0.40f);
    colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]                = ImVec4(0.22f, 0.22f, 0.23f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.28f, 0.28f, 0.30f, 1.00f);
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.34f, 0.34f, 0.36f, 1.00f);
    colors[ImGuiCol_TitleBg]                = ImVec4(0.10f, 0.10f, 0.11f, 1.00f);
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.16f, 0.29f, 0.48f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg]              = ImVec4(0.14f, 0.14f, 0.15f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.14f, 0.14f, 0.15f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.28f, 0.28f, 0.30f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.34f, 0.34f, 0.36f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.40f, 0.40f, 0.42f, 1.00f);
    colors[ImGuiCol_CheckMark]              = ImVec4(0.40f, 0.68f, 0.98f, 1.00f);
    colors[ImGuiCol_SliderGrab]             = ImVec4(0.38f, 0.39f, 0.40f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.48f, 0.49f, 0.50f, 1.00f);
    colors[ImGuiCol_Button]                 = ImVec4(0.22f, 0.22f, 0.23f, 1.00f);
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.28f, 0.28f, 0.30f, 1.00f);
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.34f, 0.34f, 0.36f, 1.00f);
    colors[ImGuiCol_Header]                 = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.26f, 0.59f, 0.98f, 0.60f);
    colors[ImGuiCol_HeaderActive]           = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_Separator]              = ImVec4(0.35f, 0.35f, 0.38f, 0.50f);
    colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
    colors[ImGuiCol_SeparatorActive]        = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
    colors[ImGuiCol_ResizeGrip]             = ImVec4(0.26f, 0.59f, 0.98f, 0.20f);
    colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.26f, 0.59f, 0.98f, 0.50f);
    colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_TabHovered]             = ImVec4(0.26f, 0.59f, 0.98f, 0.60f);
    colors[ImGuiCol_Tab]                    = ImVec4(0.18f, 0.35f, 0.58f, 0.86f);
    colors[ImGuiCol_TabSelected]            = ImVec4(0.20f, 0.41f, 0.68f, 1.00f);
    colors[ImGuiCol_TabSelectedOverline]    = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_TabDimmed]              = ImVec4(0.07f, 0.10f, 0.15f, 0.97f);
    colors[ImGuiCol_TabDimmedSelected]      = ImVec4(0.14f, 0.26f, 0.42f, 1.00f);
    colors[ImGuiCol_TabDimmedSelectedOverline] = ImVec4(0.50f, 0.50f, 0.50f, 0.00f);
    colors[ImGuiCol_PlotLines]              = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
    colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
    colors[ImGuiCol_TableBorderLight]       = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
    colors[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt]          = ImVec4(1.00f, 1.00f, 1.00f, 0.04f);
    colors[ImGuiCol_TextLink]               = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
    colors[ImGuiCol_DragDropTarget]         = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavCursor]              = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

    m_colors[editor_colors::Color_PinNumber]           = ImVec4(0.25f, 0.75f, 0.85f, 1.0f);
    m_colors[editor_colors::Color_PinGrid]             = ImVec4(0.65f, 0.40f, 0.85f, 1.0f);
    m_colors[editor_colors::Color_HeaderInput]         = ImVec4(0.30f, 0.60f, 0.30f, 1.0f);
    m_colors[editor_colors::Color_HeaderProcess]       = ImVec4(0.75f, 0.45f, 0.20f, 1.0f);
    m_colors[editor_colors::Color_HeaderOutput]        = ImVec4(0.30f, 0.45f, 0.70f, 1.0f);
    m_colors[editor_colors::Color_ToolbarBg]           = ImVec4(0.18f, 0.18f, 0.19f, 0.92f);
    m_colors[editor_colors::Color_ToolbarBorder]       = ImVec4(0.30f, 0.30f, 0.32f, 0.60f);
    m_colors[editor_colors::Color_ToolbarButtonHovered]= ImVec4(0.30f, 0.30f, 0.32f, 0.60f);
    m_colors[editor_colors::Color_ToolbarButtonActive] = ImVec4(0.35f, 0.35f, 0.37f, 0.80f);

    set_node_editor_style();

    using namespace ax::NodeEditor;
    ed::Style& edStyle = ed::GetStyle();
    edStyle.NodeBorderWidth = 0.0f;
    edStyle.Colors[StyleColor_Bg]                  = ImColor(0xFF2A2A2C);
    edStyle.Colors[StyleColor_Grid]                = ImColor(100, 100, 102, 35);
    edStyle.Colors[StyleColor_NodeBg]              = ImColor(0xFF1E1E20);
    edStyle.Colors[StyleColor_NodeBorder]          = ImColor(255, 255, 255, 48);
    edStyle.Colors[StyleColor_HovNodeBorder]       = ImColor(80, 180, 255, 255);
    edStyle.Colors[StyleColor_SelNodeBorder]       = ImColor(255, 170, 50, 255);
    edStyle.Colors[StyleColor_NodeSelRect]         = ImColor(5, 130, 255, 48);
    edStyle.Colors[StyleColor_NodeSelRectBorder]   = ImColor(5, 130, 255, 96);
    edStyle.Colors[StyleColor_HovLinkBorder]       = ImColor(80, 180, 255, 255);
    edStyle.Colors[StyleColor_SelLinkBorder]       = ImColor(255, 170, 50, 255);
    edStyle.Colors[StyleColor_HighlightLinkBorder] = ImColor(200, 100, 0, 255);
    edStyle.Colors[StyleColor_LinkSelRect]         = ImColor(5, 130, 255, 48);
    edStyle.Colors[StyleColor_LinkSelRectBorder]   = ImColor(5, 130, 255, 96);
    edStyle.Colors[StyleColor_PinRect]             = ImColor(60, 180, 255, 80);
    edStyle.Colors[StyleColor_PinRectBorder]       = ImColor(60, 180, 255, 128);
    edStyle.Colors[StyleColor_Flow]                = ImColor(255, 128, 64, 255);
    edStyle.Colors[StyleColor_FlowMarker]          = ImColor(255, 128, 64, 255);
    edStyle.Colors[StyleColor_GroupBg]             = ImColor(0, 0, 0, 120);
    edStyle.Colors[StyleColor_GroupBorder]         = ImColor(255, 255, 255, 24);
}

void editor::draw_details_panel() {
    ed::SetCurrentEditor(m_node_editor_context);
    int total = ed::GetSelectedObjectCount();
    std::vector<ed::NodeId> selected(total);
    int node_count = ed::GetSelectedNodes(selected.data(), total);
    ed::SetCurrentEditor(nullptr);

    const float panel_width = 220.0f;
    const float margin = 12.0f;
    ImVec2 panel_pos(ImGui::GetIO().DisplaySize.x - panel_width - margin, margin);

    ImGui::SetNextWindowPos(panel_pos, ImGuiCond_Always);
    ImGui::SetNextWindowSizeConstraints(
        ImVec2(panel_width, 0.0f),
        ImVec2(panel_width, FLT_MAX));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    ImGui::Begin("##details", nullptr,
        ImGuiWindowFlags_NoTitleBar  |
        ImGuiWindowFlags_NoMove      |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::PopStyleVar();

    ImGui::TextUnformatted("Details");
    ImGui::Separator();

    if (node_count == 1) {
        int nid = static_cast<int>(selected[0].Get() & ~k_node_tag);
        auto* n = m_generator.graph().find_node(nid);
        if (n) {
            const auto* entry = ls::node_registry::instance().find(*n);
            if (entry)
                ImGui::TextUnformatted(entry->display_name.c_str());
            ImGui::Separator();

            imgui_visitor vis;
            n->accept(vis);
            if (vis.changed)
                m_generator.evaluate();
        }
    } else if (node_count == 0) {
        ImGui::TextDisabled("No node selected");
    } else {
        ImGui::TextDisabled("%d nodes selected", node_count);
    }

    ImGui::End();
}

void editor::draw_window_shadow(ImVec2 pos, ImVec2 size, float rounding) {
    ImDrawList* bg = ImGui::GetBackgroundDrawList();
    const float offset = 8.0f;
    for (int i = 4; i >= 1; i--) {
        float spread = offset * static_cast<float>(i) * 0.35f;
        int   alpha  = static_cast<int>(255 * 0.055f * static_cast<float>(5 - i));
        bg->AddRectFilled(
            ImVec2(pos.x - spread + offset, pos.y - spread + offset),
            ImVec2(pos.x + size.x + spread + offset, pos.y + size.y + spread + offset),
            IM_COL32(0, 0, 0, alpha), rounding + spread);
    }
}
