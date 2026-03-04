#include "application.hpp"
#include "nodes/node_builder.hpp"
#include "nodes/pin_icons.hpp"
#include <stdexcept>
#include <iostream>

#include "imgui_internal.h"
#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_sdlrenderer3.h"
#include "misc/freetype/imgui_freetype.h"

namespace ed = ax::NodeEditor;

application::application(const std::string& title, int width, int height)
    : m_window(nullptr),
      m_renderer(nullptr),
      m_running(true),
      m_title(title),
      m_width(width),
      m_height(height) {
    init_sdl();
    init_imgui();
    init_node_editor();
    apply_theme();
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
    auto displayMode = SDL_GetCurrentDisplayMode(display[0]);
    if (!displayMode)
        throw std::runtime_error("SDL_GetCurrentDisplayMode Error: " + std::string(SDL_GetError()));
    m_ui_scale = displayMode->pixel_density;

    m_window = SDL_CreateWindow(m_title.c_str(), m_width, m_height, SDL_WINDOW_RESIZABLE);
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

    const float ui_scale = m_ui_scale;
    const float font_size = 16.0f;
    ImFontConfig config;
    config.OversampleH = 8;
    config.OversampleV = 8;
    io.Fonts->AddFontFromFileTTF("../resources/selawk.ttf", font_size * ui_scale, &config);

    ImGui::GetStyle().ScaleAllSizes(ui_scale);
}

void application::init_node_editor() {
    ax::NodeEditor::Config config;
    config.SettingsFile = "NodeEditor.json";
    config.EnableSmoothZoom = false;
    config.CanvasSizeMode = ax::NodeEditor::CanvasSizeMode::CenterOnly;
    m_node_editor_context = ed::CreateEditor(&config);
    ed::SetCurrentEditor(m_node_editor_context);
}

void application::run() {
    while (m_running) {
        process_events();
        update();
        render();
    }
}

void application::process_events() {
    while (SDL_PollEvent(&m_event)) {
        ImGui_ImplSDL3_ProcessEvent(&m_event);

        if (m_event.type == SDL_EVENT_QUIT) {
            m_running = false;
        }

        if (m_event.type == SDL_EVENT_WINDOW_RESIZED) {
            m_width = m_event.window.data1;
            m_height = m_event.window.data2;
        }
    }
}

void application::update() {
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    // Fullscreen node editor window
    ImGui::SetNextWindowSize(ImVec2(m_width, m_height), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);

    ImGui::Begin("Level Synth", nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoBringToFrontOnFocus);

    node_editor();

    ImGui::End();
    ImGui::PopStyleVar(2);

    // Floating toolbar overlay
    toolbar();
}

void application::render() {
    ImGui::Render();

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
    ax::NodeEditor::DestroyEditor(m_node_editor_context);

    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(m_renderer);
    SDL_DestroyWindow(m_window);
    SDL_Quit();
}

void application::node_editor() {
    ed::SetCurrentEditor(m_node_editor_context);

    ed::Begin("My Editor", ImVec2(0.0, 0.0f));

    int uniqueId = 1;
    editor::NodeBuilder builder;
    const ImVec2 iconSize(16, 16);

    // --- Node A: Input node (green header) ---
    ed::NodeId nodeA_Id = uniqueId++;
    ed::PinId  nodeA_InputPinId = uniqueId++;
    ed::PinId  nodeA_OutputPinId = uniqueId++;

    if (m_first_frame)
        ed::SetNodePosition(nodeA_Id, ImVec2(10, 10));

    builder.Begin(nodeA_Id);
    {
        builder.Header(ImVec4(0.30f, 0.60f, 0.30f, 1.0f));
            ImGui::TextUnformatted("Input");
            ImGui::Dummy(ImVec2(80, 0));
        builder.EndHeader();

        builder.BeginColumns();
        {
            // Inputs column
            ImGui::BeginGroup();
            {
                ed::BeginPin(nodeA_InputPinId, ed::PinKind::Input);
                    editor::DrawPinIcon(iconSize, editor::PinIconType::Circle, true,
                        ImVec4(0.25f, 0.75f, 0.85f, 1.0f));
                    ImGui::SameLine();
                    ImGui::TextUnformatted("Value");
                ed::EndPin();
            }
            ImGui::EndGroup();

            ImGui::SameLine(0, 20);

            // Outputs column
            ImGui::BeginGroup();
            {
                ed::BeginPin(nodeA_OutputPinId, ed::PinKind::Output);
                    ImGui::TextUnformatted("Result");
                    ImGui::SameLine();
                    editor::DrawPinIcon(iconSize, editor::PinIconType::Circle, true,
                        ImVec4(0.45f, 0.78f, 0.45f, 1.0f));
                ed::EndPin();
            }
            ImGui::EndGroup();
        }
        builder.EndColumns();
    }
    builder.End();

    // --- Node B: Process node (orange header) ---
    ed::NodeId nodeB_Id = uniqueId++;
    ed::PinId  nodeB_InputPinId1 = uniqueId++;
    ed::PinId  nodeB_InputPinId2 = uniqueId++;
    ed::PinId  nodeB_OutputPinId = uniqueId++;

    if (m_first_frame)
        ed::SetNodePosition(nodeB_Id, ImVec2(310, 60));

    builder.Begin(nodeB_Id);
    {
        builder.Header(ImVec4(0.75f, 0.45f, 0.20f, 1.0f));
            ImGui::TextUnformatted("Process");
            ImGui::Dummy(ImVec2(100, 0));
        builder.EndHeader();

        builder.BeginColumns();
        {
            // Inputs column
            ImGui::BeginGroup();
            {
                ed::BeginPin(nodeB_InputPinId1, ed::PinKind::Input);
                    editor::DrawPinIcon(iconSize, editor::PinIconType::Circle, true,
                        ImVec4(0.45f, 0.78f, 0.45f, 1.0f));
                    ImGui::SameLine();
                    ImGui::TextUnformatted("Grid");
                ed::EndPin();

                ed::BeginPin(nodeB_InputPinId2, ed::PinKind::Input);
                    editor::DrawPinIcon(iconSize, editor::PinIconType::Circle, true,
                        ImVec4(0.25f, 0.75f, 0.85f, 1.0f));
                    ImGui::SameLine();
                    ImGui::TextUnformatted("Scale");
                ed::EndPin();
            }
            ImGui::EndGroup();

            ImGui::SameLine(0, 20);

            // Outputs column
            ImGui::BeginGroup();
            {
                ed::BeginPin(nodeB_OutputPinId, ed::PinKind::Output);
                    ImGui::TextUnformatted("Output");
                    ImGui::SameLine();
                    editor::DrawPinIcon(iconSize, editor::PinIconType::Square, true,
                        ImVec4(0.65f, 0.40f, 0.85f, 1.0f));
                ed::EndPin();
            }
            ImGui::EndGroup();
        }
        builder.EndColumns();
    }
    builder.End();

    // Submit Links
    for (auto& linkInfo : m_links)
        ed::Link(linkInfo.Id, linkInfo.InputId, linkInfo.OutputId);

    // Handle creation
    if (ed::BeginCreate())
    {
        ed::PinId inputPinId, outputPinId;
        if (ed::QueryNewLink(&inputPinId, &outputPinId))
        {
            if (inputPinId && outputPinId)
            {
                if (ed::AcceptNewItem())
                {
                    m_links.push_back({ ed::LinkId(m_next_link_id++), inputPinId, outputPinId });
                    ed::Link(m_links.back().Id, m_links.back().InputId, m_links.back().OutputId);
                }
            }
        }
    }
    ed::EndCreate();

    // Handle deletion
    if (ed::BeginDelete())
    {
        ed::LinkId deletedLinkId;
        while (ed::QueryDeletedLink(&deletedLinkId))
        {
            if (ed::AcceptDeletedItem())
            {
                for (auto it = m_links.begin(); it != m_links.end(); ++it)
                {
                    if (it->Id == deletedLinkId)
                    {
                        m_links.erase(it);
                        break;
                    }
                }
            }
        }
    }
    ed::EndDelete();

    ed::End();

    ed::SetCurrentEditor(nullptr);

    m_first_frame = false;
}

void application::toolbar() {
    auto& io = ImGui::GetIO();
    const float scale = m_ui_scale;

    // Toolbar dimensions
    const float toolbar_height = 36.0f * scale;
    const float toolbar_padding = 6.0f * scale;
    const float button_size = 24.0f * scale;

    // Position at top-center
    float toolbar_width = 220.0f * scale;
    float toolbar_x = (m_width - toolbar_width) * 0.5f;
    float toolbar_y = 12.0f * scale;

    ImGui::SetNextWindowPos(ImVec2(toolbar_x, toolbar_y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(toolbar_width, toolbar_height), ImGuiCond_Always);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f * scale);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(toolbar_padding, toolbar_padding));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg,
        m_dark_theme ? ImVec4(0.18f, 0.18f, 0.19f, 0.92f)
                     : ImVec4(0.95f, 0.95f, 0.96f, 0.92f));
    ImGui::PushStyleColor(ImGuiCol_Border,
        m_dark_theme ? ImVec4(0.30f, 0.30f, 0.32f, 0.60f)
                     : ImVec4(0.70f, 0.70f, 0.72f, 0.60f));

    ImGui::Begin("##toolbar", nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoCollapse);

    // Hamburger menu button
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
        m_dark_theme ? ImVec4(0.30f, 0.30f, 0.32f, 0.60f)
                     : ImVec4(0.80f, 0.80f, 0.82f, 0.60f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,
        m_dark_theme ? ImVec4(0.35f, 0.35f, 0.37f, 0.80f)
                     : ImVec4(0.70f, 0.70f, 0.72f, 0.80f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f * scale);

    if (ImGui::Button("=", ImVec2(button_size, button_size)))
    {
        ImGui::OpenPopup("MainMenu");
    }

    if (ImGui::BeginPopup("MainMenu"))
    {
        if (ImGui::BeginMenu("File"))
        {
            ImGui::MenuItem("New");
            ImGui::MenuItem("Open...");
            ImGui::MenuItem("Save");
            ImGui::MenuItem("Save As...");
            ImGui::Separator();
            if (ImGui::MenuItem("Quit"))
                m_running = false;
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit"))
        {
            ImGui::MenuItem("Undo");
            ImGui::MenuItem("Redo");
            ImGui::Separator();
            ImGui::MenuItem("Cut");
            ImGui::MenuItem("Copy");
            ImGui::MenuItem("Paste");
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help"))
        {
            ImGui::MenuItem("About Level Synth");
            ImGui::EndMenu();
        }
        ImGui::EndPopup();
    }

    ImGui::SameLine();

    // Separator
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine();

    // Theme toggle
    const char* theme_label = m_dark_theme ? "D" : "L";
    if (ImGui::Button(theme_label, ImVec2(button_size, button_size)))
    {
        m_dark_theme = !m_dark_theme;
        apply_theme();
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Toggle theme");

    ImGui::SameLine();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine();

    // FPS display
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4.0f * scale);
    ImGui::TextDisabled("%.0f fps", io.Framerate);

    ImGui::PopStyleVar(1); // FrameRounding
    ImGui::PopStyleColor(3); // Button colors

    ImGui::End();

    ImGui::PopStyleColor(2); // WindowBg, Border
    ImGui::PopStyleVar(3); // WindowRounding, WindowPadding, WindowBorderSize
}

void application::apply_theme() {
    if (m_dark_theme)
        set_dark_theme();
    else
        set_light_theme();
}

void application::set_light_theme() {
    auto& style = ImGui::GetStyle();
    style.WindowBorderSize = 0.0f;
    style.FrameBorderSize = 0.0f;
    style.FrameRounding = 5.0f;
    style.ChildRounding = 5.0f;
    style.GrabRounding = 5.0f;
    style.WindowShadowSize = 35.0f;
    style.WindowShadowOffsetAngle = 0.0f;
    style.WindowShadowOffsetDist = 0.0f;

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
    colors[ImGuiCol_WindowShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.40f);

    // Node editor style
    ed::Style& edStyle = ed::GetStyle();
    edStyle.NodePadding              = ImVec4(8, 8, 8, 8);
    edStyle.NodeRounding             = 12.0f;
    edStyle.NodeBorderWidth          = 1.0f;
    edStyle.HoveredNodeBorderWidth   = 2.0f;
    edStyle.HoverNodeBorderOffset    = 0.0f;
    edStyle.SelectedNodeBorderWidth  = 2.0f;
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

    using namespace ax::NodeEditor;
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

void application::set_dark_theme() {
    auto& style = ImGui::GetStyle();
    style.WindowBorderSize = 0.0f;
    style.FrameBorderSize = 0.0f;
    style.FrameRounding = 5.0f;
    style.ChildRounding = 5.0f;
    style.GrabRounding = 5.0f;
    style.WindowShadowSize = 35.0f;
    style.WindowShadowOffsetAngle = 0.0f;
    style.WindowShadowOffsetDist = 0.0f;

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
    colors[ImGuiCol_WindowShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.70f);

    // Node editor style
    ed::Style& edStyle = ed::GetStyle();
    edStyle.NodePadding              = ImVec4(8, 8, 8, 8);
    edStyle.NodeRounding             = 12.0f;
    edStyle.NodeBorderWidth          = 0.0f;
    edStyle.HoveredNodeBorderWidth   = 1.5f;
    edStyle.HoverNodeBorderOffset    = 0.0f;
    edStyle.SelectedNodeBorderWidth  = 2.0f;
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

    using namespace ax::NodeEditor;
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
