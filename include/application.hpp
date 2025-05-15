#pragma once

#include <SDL3/SDL.h>
#include <string>
#include <vector>
#include <imgui.h>
#include <imgui_node_editor.h>

// Ensure we have the correct ImGuiFreeType declarations
namespace ImGuiFreeType { const ImFontBuilderIO* GetBuilderForFreeType(); }

// Struct to hold basic information about connection between
// pins. Note that connection (aka. link) has its own ID.
// This is useful later with dealing with selections, deletion
// or other operations.
struct LinkInfo
{
    ax::NodeEditor::LinkId Id;
    ax::NodeEditor::PinId InputId;
    ax::NodeEditor::PinId OutputId;
};

class application {
public:
    application(const std::string& title, int width, int height);
    ~application();

    void run();

private:
    void init_sdl();
    void init_imgui();
    void init_node_editor();
    void process_events();
    void update();
    void render();
    void cleanup();
    void node_editor();

    void set_light_theme();
    void set_dark_theme();

    SDL_Window* m_window;
    SDL_Renderer* m_renderer;
    SDL_Event m_event;

    bool m_running;
    std::string m_title;
    int m_width;
    int m_height;
    float m_resolution_scale;
    float m_ui_scale;

    ax::NodeEditor::EditorContext* m_node_editor_context;
    bool m_first_frame = true;
    std::vector<LinkInfo> m_links;
    int m_next_link_id = 100; // Counter to help generate link ids. In real application this will probably based on pointer to user data structure.

};