#pragma once

#include <imgui.h>
#include <imgui_node_editor.h>
#include "nodes/node_colors.hpp"
#include <level_synth/generator.hpp>
#include <level_synth/node.hpp>
#include <level_synth/pin.hpp>
#include <filesystem>
#include <unordered_map>
#include <string>

class editor {
public:
    editor() = default;
    ~editor();

    void init();
    void draw();

private:
    void draw_node_editor();
    void draw_toolbar();
    void draw_details_panel();

    void set_light_theme();
    void set_dark_theme();
    void set_node_editor_style();
    void apply_theme();
    void draw_window_shadow(ImVec2 pos, ImVec2 size, float rounding = 10.0f);

    void save_graph();
    void save_graph_as();

    ax::NodeEditor::EditorContext* m_node_editor_context = nullptr;
    ls::generator m_generator;
    bool m_first_frame = true;

    struct wire_visual {
        int from_node;
        std::string from_pin;
        int to_node;
        std::string to_pin;
    };
    int m_next_link_id = 1;
    std::unordered_map<int, wire_visual> m_link_to_wire;
    ImVec2 m_popup_canvas_pos = {};

    ImVec4 m_colors[editor_colors::Color_COUNT];
    bool m_dark_theme = true;
    bool m_show_demo_window = false;
    int m_seed = 42;
    std::filesystem::path m_current_file;

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
