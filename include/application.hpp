#pragma once

#include <SDL3/SDL.h>
#include <string>
#include "imgui.h"
#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_sdlrenderer3.h"
#include "misc/freetype/imgui_freetype.h"

// Ensure we have the correct ImGuiFreeType declarations
namespace ImGuiFreeType { const ImFontBuilderIO* GetBuilderForFreeType(); }
namespace ax { namespace NodeEditor { class EditorContext; } }

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
};