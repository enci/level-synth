#pragma once

#include <imgui.h>
#include <imgui_node_editor.h>
#include "nodes/node_colors.hpp"
#include "command_history.hpp"
#include <level_synth/generator.hpp>
#include <level_synth/node.hpp>
#include <level_synth/pin.hpp>
#include <filesystem>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <vector>

class editor {
public:
    editor() = default;
    ~editor();

    void init(const std::string& pref_dir);
    void draw();

    /// Returns the string that should appear in the OS window title bar.
    /// Format: ["• "] <filename|"Untitled"> " — Level Synth"
    std::string window_title() const;

private:
    void draw_node_editor();
    void draw_toolbar();
    void draw_details_panel();

    void set_light_theme();
    void set_dark_theme();
    void set_node_editor_style();
    void apply_theme();
    void draw_window_shadow(ImVec2 pos, ImVec2 size, float rounding = 10.0f);

    void new_graph();
    void load_graph();                                    // opens file dialog
    void load_graph(const std::filesystem::path& path);  // loads directly
    void save_graph();
    void save_graph_as();
    std::string build_save_json();
    void rebuild_links_from_graph();

    void load_preferences();
    void save_preferences();
    void push_recent_file(const std::filesystem::path& path);

    // Snapshot-based undo/redo: call begin_edit before mutating the graph,
    // commit_edit after. Any change — structural, property, custom node UI —
    // is covered without needing a dedicated command class.
    void begin_edit(std::string description);
    void commit_edit();

    void draw_history_panel();

    ax::NodeEditor::EditorContext* m_node_editor_context = nullptr;
    ls::generator m_generator;
    command_history m_history;

    struct wire_visual {
        int from_node;
        std::string from_pin;
        int to_node;
        std::string to_pin;
    };
    int m_next_link_id = 1;
    std::unordered_map<int, wire_visual> m_link_to_wire;
    ImVec2 m_popup_canvas_pos = {};
    // Nodes that have been placed on the canvas at least once.
    // Any node not in this set gets its initial position applied on first render.
    std::unordered_set<int> m_positioned_nodes;

    // Pending edit state for begin_edit / commit_edit
    std::string m_edit_description;
    std::string m_edit_before_json;
    // Before-state captured on mouse press for node drag detection.
    // m_drag_before_json holds positions-only JSON for change detection.
    // m_drag_before_full_json holds the full graph snapshot for the undo before-state.
    std::string m_drag_before_json;
    std::string m_drag_before_full_json;

    ImVec4 m_colors[editor_colors::Color_COUNT];
    bool m_dark_theme = true;
    bool m_show_demo_window   = false;
    bool m_show_history_panel = true;
    std::filesystem::path m_current_file;
    std::string m_pref_dir;
    std::string m_node_editor_settings_path; // must outlive the editor context
    std::vector<std::string> m_recent_files; // most-recent first, max 10

    // Node editor IDs share a flat namespace — tag each type with distinct high bits
    // to prevent collisions between node IDs, pin IDs, and link IDs.
    static constexpr uintptr_t k_node_tag = uintptr_t(0x10000000);
    static constexpr uintptr_t k_pin_tag  = uintptr_t(0x20000000);
    static constexpr uintptr_t k_link_tag = uintptr_t(0x40000000);

    static ax::NodeEditor::NodeId make_node_id(int node_id);
    static ax::NodeEditor::PinId  make_pin_id(int node_id, int pin_index);
    static ax::NodeEditor::LinkId make_link_id(int link_id);
    static std::pair<int, int> unpack_pin_id(ax::NodeEditor::PinId pin_id);

    ImVec4 pin_color(ls::pin_type type) const;
    ImVec4 category_color(const std::string& category) const;

    struct pin_info {
        int node_id;
        int pin_index;
        const ls::pin_descriptor* desc;
    };
    pin_info resolve_pin(ax::NodeEditor::PinId pin_id) const;
};
