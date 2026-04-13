#pragma once

#include <SDL3/SDL.h>
#include <string>
#include <vector>
#include <unordered_map>

#include <imgui.h>
#include <imgui_node_editor.h>
#include "nodes/node_colors.hpp"

#include <level_synth/generator.hpp>
#include <level_synth/eval_engine.hpp>
#include <level_synth/node.hpp>
#include <level_synth/pin.hpp>

class application {
public:
    application(const std::string& title, int width, int height);
    ~application();

    void run();

    // Thread-safe: pushes a custom SDL event to wake the main loop.
    // Call from any worker thread when results are ready to display.
    static void request_redraw();

private:
    void init_sdl();
    void init_imgui();
    void init_node_editor();
    void process_events();
    void dispatch_event(const SDL_Event& event);
    void update();
    void render();
    void cleanup();
    void node_editor();
    void toolbar();

    void set_light_theme();
    void set_dark_theme();
    void set_node_editor_style();
    void apply_theme();

    SDL_Window* m_window;
    SDL_Renderer* m_renderer;
    SDL_Event m_event;

    bool m_running;
    std::string m_title;
    int m_width;
    int m_height;
    float m_resolution_scale;
    float m_ui_scale;

    bool m_dark_theme = true;
    bool m_show_demo_window = false;
    int m_redraw_frames = 0;           // extra frames to render after activity
    static constexpr int k_cooldown_frames = 3;  // frames to keep rendering after last event
    static Uint32 s_redraw_event_type; // custom SDL event for cross-thread wake-up

    ImVec4 m_colors[editor::Color_COUNT];

    ax::NodeEditor::EditorContext* m_node_editor_context;

    ls::generator m_generator;
    bool m_first_frame = true;

    // Editor-only state
    struct wire_visual {
        int from_node;
        std::string from_pin;
        int to_node;
        std::string to_pin;
    };
    int m_next_link_id = 1;
    std::unordered_map<int, wire_visual> m_link_to_wire;  // link_id -> wire info

    // Helper functions
    static ax::NodeEditor::PinId make_pin_id(int node_id, int pin_index);
    static std::pair<int, int> unpack_pin_id(ax::NodeEditor::PinId pin_id);
    ImVec4 pin_color(ls::pin_type type) const;
    ImVec4 category_color(const std::string& category) const;

    // Find pin descriptor from packed pin ID
    struct pin_info {
        int node_id;
        int pin_index;
        const ls::pin_descriptor* desc;
    };
    pin_info resolve_pin(ax::NodeEditor::PinId pin_id) const;
};
