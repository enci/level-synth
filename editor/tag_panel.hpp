#pragma once

#include <imgui.h>
#include <level_synth/tag_registry.hpp>
#include <string>
#include <vector>

namespace ls {

// ---------------------------------------------------------------------------
// tag_panel
//
// Docked "Tags" panel. Derives the L0/L1/L2 hierarchy each frame from the
// flat sorted list returned by tag_registry::all_identifiers().
// No tree state is stored here — the registry is the single source of truth.
//
// Usage (in editor):
//   tag_panel m_tag_panel;
//   // in draw():
//   m_tag_panel.draw(ls::tag_registry::instance());
// ---------------------------------------------------------------------------
class tag_panel {
public:
    void draw(ls::tag_registry& reg);

private:
    // Tree node built once per frame.
    struct tree_node {
        std::string      full_id;     // "Geometry.Wall.Damaged"
        std::string      label;       // "Damaged"
        int              depth;       // 0 / 1 / 2
        int              l0_pal_idx; // stable palette slot for this L0
        std::vector<int> children;   // indices into m_nodes
    };

    std::vector<tree_node> m_nodes;       // flat storage, roots are depth-0 entries
    std::vector<int>       m_roots;       // indices of depth-0 nodes

    void build_tree(ls::tag_registry& reg);
    void draw_row(ls::tag_registry& reg, int idx);

    // Color resolution: walks up ancestors, falls back to palette.
    struct resolved_color { uint32_t value; bool inherited; };
    resolved_color resolve_color(ls::tag_registry& reg,
                                 const std::string& full_id,
                                 int l0_pal_idx) const;

    // 11×11 swatch; filled=explicit, outlined=inherited. Returns true if clicked.
    // Shared inline-add widget + muted button. prefix="" means root level.
    void draw_inline_add(const std::string& prefix);

    bool draw_swatch(uint32_t color, bool inherited);

    // Draws the color picker popup if m_color_target == full_id.
    void draw_color_popup(ls::tag_registry& reg,
                          const std::string& full_id,
                          uint32_t current_color);

    static uint32_t palette_color(int index);

    // Pending mutations — applied after the render loop.
    std::string m_pending_add;
    std::string m_pending_remove;
    std::string m_pending_rename_from;
    std::string m_pending_rename_to;

    // Inline add state. k_idle = not currently adding.
    static constexpr const char* k_idle = "\x01";
    std::string m_adding_under { k_idle };
    char        m_add_buf[128]  = {};

    // Inline rename state. Empty = not renaming.
    std::string m_renaming;
    char        m_rename_buf[128] = {};

    // Color picker state. Empty = closed.
    std::string m_color_target;

    ImGuiTextFilter m_filter;
};

} // namespace editor