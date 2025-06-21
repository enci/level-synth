#include "application.hpp"
#include <stdexcept>
#include <iostream>
#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_sdlrenderer3.h"
#include "misc/freetype/imgui_freetype.h"

namespace ed = ax::NodeEditor;

void ImGuiEx_BeginColumn()
{
    ImGui::BeginGroup();
}

void ImGuiEx_NextColumn()
{
    ImGui::EndGroup();
    ImGui::SameLine();
    ImGui::BeginGroup();
}

void ImGuiEx_EndColumn()
{
    ImGui::EndGroup();
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
    init_node_editor();
    set_dark_theme();
}

application::~application() {
    cleanup();
}

void application::init_sdl() {
    if (!SDL_Init(SDL_INIT_VIDEO))
        throw std::runtime_error("SDL_Init Error: " + std::string(SDL_GetError()));

    // Get the display resolution and ui scaling
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

    // Get the display scale
    m_ui_scale = SDL_GetWindowDisplayScale(m_window);

    m_renderer = SDL_CreateRenderer(m_window, nullptr);
    if (!m_renderer) {
        SDL_DestroyWindow(m_window);
        SDL_Quit();
        throw std::runtime_error("SDL_CreateRenderer Error: " + std::string(SDL_GetError()));
    }
}

void application::init_imgui() {
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;      // Enable Docking

    // Setup ImGui style
    // ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForSDLRenderer(m_window, m_renderer);
    ImGui_ImplSDLRenderer3_Init(m_renderer);

    // Setup FreeType
    const ImFontBuilderIO* builder = ImGuiFreeType::GetBuilderForFreeType();
    io.Fonts->FontBuilderIO = builder;
    io.Fonts->FontBuilderFlags = ImGuiFreeTypeBuilderFlags_ForceAutoHint;

    // Load additional fonts
    const float ui_scale = m_ui_scale;
    const float font_size = 16.0f;
    // const float iconSize = 12.0f;
    ImFontConfig config;
    config.OversampleH = 8;
    config.OversampleV = 8;
    io.Fonts->AddFontFromFileTTF("../resources/selawk.ttf", font_size * ui_scale, &config);

    // Scale all ImGui styles by the window scale factor
    ImGui::GetStyle().ScaleAllSizes(ui_scale);
}

void application::init_node_editor() {
    ax::NodeEditor::Config config;
    config.SettingsFile = "NodeEditor.json";
    //config.BeginSaveSession = [](void* userPointer) {
    //    std::cout << "BeginSaveSession" << std::endl;
    //};
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

        /*
        if (m_event.type == SDL_EVENT_KEY_DOWN) {
            if (m_event.key.keysym.sym == SDLK_ESCAPE) {
                m_running = false;
            }
        }
        */
    }
}

void application::update() {
    // Start the Dear ImGui frame
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    // Create a fullscreen window for the editor
    ImGui::SetNextWindowSize(ImVec2(m_width, m_height), ImGuiCond_Always);
    // ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);

    // Push no border
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);

    ImGui::Begin("Game Tool", nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoBringToFrontOnFocus);

    node_editor();

    ImGui::End();

    // Pop no border
    ImGui::PopStyleVar(2);

    // Create a demo window
    ImGui::ShowDemoWindow();
}

void application::render() {
    ImGui::Render();

    SDL_SetRenderDrawColor(m_renderer, 255, 0, 0, 255);
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

    auto& io = ImGui::GetIO();

        ImGui::Text("FPS: %.2f (%.2gms)", io.Framerate, io.Framerate ? 1000.0f / io.Framerate : 0.0f);

        ImGui::Separator();

        ed::SetCurrentEditor(m_node_editor_context);

        // Start interaction with editor.
        ed::Begin("My Editor", ImVec2(0.0, 0.0f));

        int uniqueId = 1;

        //
        // 1) Commit known data to editor
        //

        // Submit Node A
        ed::NodeId nodeA_Id = uniqueId++;
        ed::PinId  nodeA_InputPinId = uniqueId++;
        ed::PinId  nodeA_OutputPinId = uniqueId++;

        if (m_first_frame)
            ed::SetNodePosition(nodeA_Id, ImVec2(10, 10));
        ed::BeginNode(nodeA_Id);
            ImGui::Text("Node A");
            ed::BeginPin(nodeA_InputPinId, ed::PinKind::Input);
                ImGui::Text("-> In");
            ed::EndPin();
            ImGui::SameLine();
            ed::BeginPin(nodeA_OutputPinId, ed::PinKind::Output);
                ImGui::Text("Out ->");
            ed::EndPin();
        ed::EndNode();

        // Submit Node B
        ed::NodeId nodeB_Id = uniqueId++;
        ed::PinId  nodeB_InputPinId1 = uniqueId++;
        ed::PinId  nodeB_InputPinId2 = uniqueId++;
        ed::PinId  nodeB_OutputPinId = uniqueId++;

        if (m_first_frame)
            ed::SetNodePosition(nodeB_Id, ImVec2(210, 60));
        ed::BeginNode(nodeB_Id);
            ImGui::Text("Node B");
            ImGuiEx_BeginColumn();
                ed::BeginPin(nodeB_InputPinId1, ed::PinKind::Input);
                    ImGui::Text("-> In1");
                ed::EndPin();
                ed::BeginPin(nodeB_InputPinId2, ed::PinKind::Input);
                    ImGui::Text("-> In2");
                ed::EndPin();
            ImGuiEx_NextColumn();
                ed::BeginPin(nodeB_OutputPinId, ed::PinKind::Output);
                    ImGui::Text("Out ->");
                ed::EndPin();
            ImGuiEx_EndColumn();
        ed::EndNode();

        // Submit Links
        for (auto& linkInfo : m_links)
            ed::Link(linkInfo.Id, linkInfo.InputId, linkInfo.OutputId);

        //
        // 2) Handle interactions
        //

        // Handle creation action, returns true if editor want to create new object (node or link)
        if (ed::BeginCreate())
        {
            ed::PinId inputPinId, outputPinId;
            if (ed::QueryNewLink(&inputPinId, &outputPinId))
            {
                // QueryNewLink returns true if editor want to create new link between pins.
                //
                // Link can be created only for two valid pins, it is up to you to
                // validate if connection make sense. Editor is happy to make any.
                //
                // Link always goes from input to output. User may choose to drag
                // link from output pin or input pin. This determine which pin ids
                // are valid and which are not:
                //   * input valid, output invalid - user started to drag new ling from input pin
                //   * input invalid, output valid - user started to drag new ling from output pin
                //   * input valid, output valid   - user dragged link over other pin, can be validated

                if (inputPinId && outputPinId) // both are valid, let's accept link
                {
                    // ed::AcceptNewItem() return true when user release mouse button.
                    if (ed::AcceptNewItem())
                    {
                        // Since we accepted new link, lets add one to our list of links.
                        m_links.push_back({ ed::LinkId(m_next_link_id++), inputPinId, outputPinId });

                        // Draw new link.
                        ed::Link(m_links.back().Id, m_links.back().InputId, m_links.back().OutputId);
                    }

                    // You may choose to reject connection between these nodes
                    // by calling ed::RejectNewItem(). This will allow editor to give
                    // visual feedback by changing link thickness and color.
                }
            }
        }
        ed::EndCreate(); // Wraps up object creation action handling.


        // Handle deletion action
        if (ed::BeginDelete())
        {
            // There may be many links marked for deletion, let's loop over them.
            ed::LinkId deletedLinkId;
            while (ed::QueryDeletedLink(&deletedLinkId))
            {
                // If you agree that link can be deleted, accept deletion.
                if (ed::AcceptDeletedItem())
                {
                    // Then remove link from your data.
                    for (auto& link : m_links)
                    {
                        if (link.Id == deletedLinkId)
                        {
                            //m_links.erase(&link);
                            break;
                        }
                    }
                }

                // You may reject link deletion by calling:
                // ed::RejectDeletedItem();
            }
        }
        ed::EndDelete(); // Wrap up deletion action



        // End of interaction with editor.
        ed::End();

    // Show current zoom
    float z = ed::GetCurrentZoom();

        //if (m_first_frame)
        //    ed::NavigateToContent(0.0f);

        ed::SetCurrentEditor(nullptr);

        m_first_frame = false;


    /*
    // Set the current editor context
    ed::SetCurrentEditor(m_node_editor_context);

    // Begin the node editor
    ed::Begin("Node Editor");

    // Create a node
    ed::BeginNode(1);
    ImGui::Text("Node A");
    ed::BeginPin(2, ed::PinKind::Input);
    ImGui::Text("-> In");
    ed::EndPin();
    ImGui::SameLine();
    ed::BeginPin(3, ed::PinKind::Output);
    ImGui::Text("Out ->");
    ed::EndPin();
    ed::EndNode();

    // End the node editor
    ed::End();
    */
}

void application::set_light_theme() {
}

void application::set_dark_theme() {

    // Set metrics
    ImGui::GetStyle().WindowBorderSize = 0.0f;
    ImGui::GetStyle().FrameBorderSize = 0.0f;
    //ImGui::GetStyle().GrabMinSize = 8.0f;
    //ImGui::GetStyle().WindowRounding = 0.0f;
    ImGui::GetStyle().FrameRounding = 5.0f;
    ImGui::GetStyle().ChildRounding = 5.0f;
    //ImGui::GetStyle().ScrollbarRounding = 0.0f;
    ImGui::GetStyle().GrabRounding = 5.0f;
    //ImGui::GetStyle().PopupRounding = 0.0f;
    //ImGui::GetStyle().TabRounding = 0.0f;
    //ImGui::GetStyle().IndentSpacing = 8.0f;
    //ImGui::GetStyle().ItemSpacing = ImVec2(8.0f, 4.0f);
    //ImGui::GetStyle().ItemInnerSpacing = ImVec2(4.0f, 4.0f);
    //ImGui::GetStyle().WindowPadding = ImVec2(8.0f, 8.0f);
    //ImGui::GetStyle().FramePadding = ImVec2(4.0f, 4.0f);
    //ImGui::GetStyle().ScrollbarSize = 16.0f;
    //ImGui::GetStyle().WindowTitleAlign = ImVec2(0.5f, 0.5f);
    ImGui::GetStyle().WindowShadowSize = 35.0f;
    ImGui::GetStyle().WindowShadowOffsetAngle = 0.0f;
    ImGui::GetStyle().WindowShadowOffsetDist = 0.0f;

    // Set dark theme colors
    ImVec4* colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_Text]                   = ImVec4(0.78f, 0.78f, 0.78f, 1.00f);
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg]               = ImVec4(0.13f, 0.13f, 0.14f, 1.00f);
    colors[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.22f);
    colors[ImGuiCol_PopupBg]                = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
    colors[ImGuiCol_Border]                 = ImVec4(0.43f, 0.43f, 0.50f, 0.00f);
    colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]                = ImVec4(0.25f, 0.26f, 0.26f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.32f, 0.33f, 0.34f, 1.00f);
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.39f, 0.40f, 0.41f, 1.00f);
    colors[ImGuiCol_TitleBg]                = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.16f, 0.29f, 0.48f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg]              = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.13f, 0.13f, 0.14f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.25f, 0.26f, 0.26f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.32f, 0.33f, 0.34f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.39f, 0.40f, 0.41f, 1.00f);
    colors[ImGuiCol_CheckMark]              = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_SliderGrab]             = ImVec4(0.38f, 0.39f, 0.39f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.45f, 0.46f, 0.47f, 1.00f);
    colors[ImGuiCol_Button]                 = ImVec4(0.25f, 0.26f, 0.26f, 1.00f);
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.32f, 0.33f, 0.34f, 1.00f);
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.39f, 0.40f, 0.41f, 1.00f);
    colors[ImGuiCol_Header]                 = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_HeaderActive]           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Separator]              = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
    colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
    colors[ImGuiCol_SeparatorActive]        = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
    colors[ImGuiCol_ResizeGrip]             = ImVec4(0.26f, 0.59f, 0.98f, 0.20f);
    colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_TabHovered]             = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_Tab]                    = ImVec4(0.18f, 0.35f, 0.58f, 0.86f);
    colors[ImGuiCol_TabSelected]            = ImVec4(0.20f, 0.41f, 0.68f, 1.00f);
    colors[ImGuiCol_TabSelectedOverline]    = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_TabDimmed]              = ImVec4(0.07f, 0.10f, 0.15f, 0.97f);
    colors[ImGuiCol_TabDimmedSelected]      = ImVec4(0.14f, 0.26f, 0.42f, 1.00f);
    colors[ImGuiCol_TabDimmedSelectedOverline]  = ImVec4(0.50f, 0.50f, 0.50f, 0.00f);
    colors[ImGuiCol_PlotLines]              = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
    colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
    colors[ImGuiCol_TableBorderLight]       = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
    colors[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt]          = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    colors[ImGuiCol_TextLink]               = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    colors[ImGuiCol_DragDropTarget]         = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavCursor]              = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
    colors[ImGuiCol_WindowShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);

    // Set the style for the node editor
    ed::Style& style = ed::GetStyle();
    style.NodePadding              = ImVec4(8, 8, 8, 8);
    style.NodeRounding             = 12.0f;
    style.NodeBorderWidth          = 0.0f;
    style.HoveredNodeBorderWidth   = 1.0f;
    style.HoverNodeBorderOffset    = 0.0f;
    style.SelectedNodeBorderWidth  = 2.0f;
    style.SelectedNodeBorderOffset = 0.0f;
    style.PinRounding              = 4.0f;
    style.PinBorderWidth           = 0.0f;
    style.LinkStrength             = 100.0f;
    style.SourceDirection          = ImVec2(1.0f, 0.0f);
    style.TargetDirection          = ImVec2(-1.0f, 0.0f);
    style.ScrollDuration           = 0.35f;
    style.FlowMarkerDistance       = 30.0f;
    style.FlowSpeed                = 150.0f;
    style.FlowDuration             = 2.0f;
    style.PivotAlignment           = ImVec2(0.5f, 0.5f);
    style.PivotSize                = ImVec2(0.0f, 0.0f);
    style.PivotScale               = ImVec2(1, 1);
    style.PinCorners               = ImDrawFlags_RoundCornersAll;
    style.PinRadius                = 0.0f;
    style.PinArrowSize             = 0.0f;
    style.PinArrowWidth            = 0.0f;
    style.GroupRounding            = 6.0f;
    style.GroupBorderWidth         = 1.0f;
    style.HighlightConnectedLinks  = 0.0f;
    style.SnapLinkToPinDir         = 0.0f;

    using namespace ax::NodeEditor;
    //style.Colors[StyleColor_Bg]                 = ImColor(0xFF252829);
    style.Colors[StyleColor_Bg]                 = ImColor(0xFF292825);
    style.Colors[StyleColor_Grid]               = ImColor(120, 120, 120,  40);
    style.Colors[StyleColor_NodeBg]             = ImColor( 0xFF1A1B1D);
    style.Colors[StyleColor_NodeBorder]         = ImColor(255, 255, 255,  96);
    style.Colors[StyleColor_HovNodeBorder]      = ImColor( 50, 176, 255, 255);
    style.Colors[StyleColor_SelNodeBorder]      = ImColor(255, 176,  50, 255);
    style.Colors[StyleColor_NodeSelRect]        = ImColor(  5, 130, 255,  64);
    style.Colors[StyleColor_NodeSelRectBorder]  = ImColor(  5, 130, 255, 128);
    style.Colors[StyleColor_HovLinkBorder]      = ImColor( 50, 176, 255, 255);
    style.Colors[StyleColor_SelLinkBorder]      = ImColor(255, 176,  50, 255);
    style.Colors[StyleColor_HighlightLinkBorder]= ImColor(204, 105,   0, 255);
    style.Colors[StyleColor_LinkSelRect]        = ImColor(  5, 130, 255,  64);
    style.Colors[StyleColor_LinkSelRectBorder]  = ImColor(  5, 130, 255, 128);
    style.Colors[StyleColor_PinRect]            = ImColor( 60, 180, 255, 100);
    style.Colors[StyleColor_PinRectBorder]      = ImColor( 60, 180, 255, 128);
    style.Colors[StyleColor_Flow]               = ImColor(255, 128,  64, 255);
    style.Colors[StyleColor_FlowMarker]         = ImColor(255, 128,  64, 255);
    style.Colors[StyleColor_GroupBg]            = ImColor(  0,   0,   0, 160);
    style.Colors[StyleColor_GroupBorder]        = ImColor(255, 255, 255,  32);
}

