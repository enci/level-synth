#include "application.hpp"
#include <stdexcept>

#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_sdlrenderer3.h"
#include "misc/freetype/imgui_freetype.h"
#include "phosphor_icons.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace {

SDL_Surface* load_icon_from_file(const char* filename) {
    int width, height, channels;
    unsigned char* data = stbi_load(filename, &width, &height, &channels, 4);
    if (!data)
        return nullptr;

    SDL_Surface* surface = SDL_CreateSurfaceFrom(
        width, height, SDL_PIXELFORMAT_RGBA32, data, width * 4);

    if (!surface)
        stbi_image_free(data);

    return surface;
}

} // namespace

Uint32 application::s_redraw_event_type = 0;

void application::request_redraw() {
    if (s_redraw_event_type == 0)
        return;
    SDL_Event event = {};
    event.type = s_redraw_event_type;
    SDL_PushEvent(&event);
}

application::application(const std::string& title, int width, int height)
    : m_window(nullptr),
      m_renderer(nullptr),
      m_running(true),
      m_title(title),
      m_width(width),
      m_height(height) {
    init_sdl();
    init_imgui();
    m_editor = std::make_unique<editor>();
    m_editor->init();
}

application::~application() {
    cleanup();
}

void application::init_sdl() {
    if (!SDL_Init(SDL_INIT_VIDEO))
        throw std::runtime_error("SDL_Init Error: " + std::string(SDL_GetError()));

    auto display_count = 0;
    auto display = SDL_GetDisplays(&display_count);
    if (!display)
        throw std::runtime_error("SDL_GetDisplays Error: " + std::string(SDL_GetError()));
    auto display_mode = SDL_GetCurrentDisplayMode(display[0]);
    if (!display_mode)
        throw std::runtime_error("SDL_GetCurrentDisplayMode Error: " + std::string(SDL_GetError()));
    m_ui_scale = display_mode->pixel_density;

    m_window = SDL_CreateWindow(m_title.c_str(), m_width, m_height, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!m_window) {
        SDL_Quit();
        throw std::runtime_error("SDL_CreateWindow Error: " + std::string(SDL_GetError()));
    }

    m_ui_scale = SDL_GetWindowDisplayScale(m_window);

    m_renderer = SDL_CreateRenderer(m_window, nullptr);
    if (!m_renderer) {
        SDL_DestroyWindow(m_window);
        SDL_Quit();
        throw std::runtime_error("SDL_CreateRenderer Error: " + std::string(SDL_GetError()));
    }

    SDL_SetRenderVSync(m_renderer, 1);

    SDL_Surface* icon = load_icon_from_file("../resources/icon.png");
    if (icon) {
        SDL_SetWindowIcon(m_window, icon);
        SDL_DestroySurface(icon);
    }

    s_redraw_event_type = SDL_RegisterEvents(1);
}

void application::init_imgui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui_ImplSDL3_InitForSDLRenderer(m_window, m_renderer);
    ImGui_ImplSDLRenderer3_Init(m_renderer);

    const auto* loader = ImGuiFreeType::GetFontLoader();
    io.Fonts->SetFontLoader(loader);
    io.Fonts->FontLoaderFlags = ImGuiFreeTypeLoaderFlags_ForceAutoHint;

    constexpr float k_font_size = 16.0f;
    ImFontConfig config;
    config.OversampleH = 8;
    config.OversampleV = 8;
    io.Fonts->AddFontFromFileTTF("../resources/selawk.ttf", k_font_size, &config);

    {
        ImFontConfig icon_config;
        icon_config.MergeMode = true;
        icon_config.OversampleH = 1;
        icon_config.OversampleV = 1;
        static const ImWchar phosphor_ranges[] = {
            (ImWchar)phosphor::PH_RANGE_BEGIN, (ImWchar)phosphor::PH_RANGE_END, 0
        };
        io.Fonts->AddFontFromFileTTF("../resources/Phosphor-Bold.woff",
            k_font_size, &icon_config, phosphor_ranges);
    }
}

void application::run() {
    while (m_running) {
        process_events();
        update();
        render();
    }
}

void application::process_events() {
    bool had_event = false;

    if (m_redraw_frames > 0) {
        while (SDL_PollEvent(&m_event)) {
            ImGui_ImplSDL3_ProcessEvent(&m_event);
            dispatch_event(m_event);
            had_event = true;
        }
    } else {
        if (SDL_WaitEvent(&m_event)) {
            ImGui_ImplSDL3_ProcessEvent(&m_event);
            dispatch_event(m_event);
            had_event = true;

            while (SDL_PollEvent(&m_event)) {
                ImGui_ImplSDL3_ProcessEvent(&m_event);
                dispatch_event(m_event);
            }
        }
    }

    if (had_event) {
        m_redraw_frames = k_cooldown_frames;
    } else if (m_redraw_frames > 0) {
        m_redraw_frames--;
    }
}

void application::dispatch_event(const SDL_Event& event) {
    if (event.type == SDL_EVENT_QUIT)
        m_running = false;

    if (event.type == SDL_EVENT_WINDOW_RESIZED) {
        m_width = event.window.data1;
        m_height = event.window.data2;
    }
}

void application::update() {
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    m_editor->draw();
}

void application::render() {
    ImGui::Render();

    auto& io = ImGui::GetIO();
    SDL_SetRenderScale(m_renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);

    auto& style = ImGui::GetStyle();
    auto bg = style.Colors[ImGuiCol_WindowBg];
    SDL_SetRenderDrawColor(m_renderer,
        (Uint8)(bg.x * 255), (Uint8)(bg.y * 255),
        (Uint8)(bg.z * 255), (Uint8)(bg.w * 255));
    SDL_RenderClear(m_renderer);

    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), m_renderer);
    SDL_RenderPresent(m_renderer);
}

void application::cleanup() {
    m_editor.reset();  // must die before ImGui context is destroyed

    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(m_renderer);
    SDL_DestroyWindow(m_window);
    SDL_Quit();
}
