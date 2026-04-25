#pragma once

#include <SDL3/SDL.h>
#include <string>
#include <memory>
#include "editor.hpp"

class application {
public:
    application(const std::string& title, int width, int height);
    ~application();

    void run();

    // Thread-safe: pushes a custom SDL event to wake the main loop.
    static void request_redraw();

private:
    void init_sdl();
    void init_imgui();
    void process_events();
    void dispatch_event(const SDL_Event& event);
    void update();
    void render();
    void cleanup();

    SDL_Window* m_window;
    SDL_Renderer* m_renderer;
    SDL_Event m_event;

    bool m_running;
    std::string m_title;
    std::string m_pref_dir;
    std::string m_imgui_ini_path; // must outlive the ImGui context
    int m_width;
    int m_height;
    float m_ui_scale;

    int m_redraw_frames = 0;
    static constexpr int k_cooldown_frames = 3;
    static Uint32 s_redraw_event_type;

    std::unique_ptr<editor> m_editor;
};
