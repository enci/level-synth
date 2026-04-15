#include "application.hpp"
#include "fluent_glyph.hpp"
#include <imgui_node_editor_node_builder.h>
#include <imgui_node_editor_pin_icons.h>
#include <stdexcept>
#include <iostream>

#include "imgui_internal.h"
#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_sdlrenderer3.h"
#include "misc/freetype/imgui_freetype.h"

#include <level_synth/nodes/node_create_grid.hpp>
#include <level_synth/nodes/node_cellular_automata.hpp>
#include <level_synth/nodes/node_input_number.hpp>
#include <level_synth/nodes/node_noise_grid.hpp>
#include <level_synth/nodes/node_output_grid.hpp>
#include <level_synth/nodes/node_output_number.hpp>
#include <level_synth/node_registry.hpp>
#include <level_synth/attribute_grid.hpp>
#include <map>
#include <algorithm>

namespace ed = ax::NodeEditor;

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

    // Merge Fluent System Icons into the main font.
    // OversampleH/V = 1: FreeType handles its own antialiasing, so oversampling
    // only wastes atlas space.
    // GlyphOffset.y: the Fluent font has ascender=upem and descent=0, meaning
    // all icon geometry sits entirely above the baseline. Without an offset the
    // icons hug the top of every button. Shifting down by ~font_size/4 centres
    // them against Selawik's actual cap-height. This offset is per-glyph only
    // and does not affect ImFont::Ascent/Descent or LineSpacing.
    {
        ImFontConfig icon_config;
        icon_config.MergeMode = true;
        icon_config.OversampleH = 1;
        icon_config.OversampleV = 1;
        icon_config.GlyphOffset = ImVec2(0.0f, k_font_size / 4.0f);
        io.Fonts->AddFontFromFileTTF("../resources/FluentSystemIcons-Regular.ttf",
            k_font_size, &icon_config, xs::tools::get_fluent_glyph_ranges());
    }
}

void application::init_node_editor() {
    ax::NodeEditor::Config config;
    config.SettingsFile = "NodeEditor.json";
    config.EnableSmoothZoom = false;
    config.NavigateButtonIndex = 2; // Middle mouse button for panning
    config.CustomZoomLevels.push_back(1.0f);
    config.CanvasSizeMode = ax::NodeEditor::CanvasSizeMode::CenterOnly;
    m_node_editor_context = ed::CreateEditor(&config);
    ed::SetCurrentEditor(m_node_editor_context);

    // Test graph: Create Grid -> Noise Grid -> Cellular Automata -> Output Grid
    auto& eng = m_generator.engine();
    int create_id = eng.add_node(std::make_unique<ls::node_create_grid>());
    int noise_id  = eng.add_node(std::make_unique<ls::node_noise_grid>());
    int ca_id     = eng.add_node(std::make_unique<ls::node_cellular_automata>());
    int out_id    = eng.add_node(std::make_unique<ls::node_output_grid>());

    eng.add_wire({ create_id, "grid", noise_id, "grid"   });
    eng.add_wire({ noise_id,  "grid", ca_id,    "input"  });
    eng.add_wire({ ca_id,  "output", out_id,    "value"  });

    m_link_to_wire[m_next_link_id++] = { create_id, "grid", noise_id, "grid"   };
    m_link_to_wire[m_next_link_id++] = { noise_id,  "grid", ca_id,    "input"  };
    m_link_to_wire[m_next_link_id++] = { ca_id,  "output", out_id,    "value"  };
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
        // Still cooling down — drain events without blocking
        while (SDL_PollEvent(&m_event)) {
            ImGui_ImplSDL3_ProcessEvent(&m_event);
            dispatch_event(m_event);
            had_event = true;
        }
    } else {
        // Idle — block until something happens (input, resize, worker thread, etc.)
        if (SDL_WaitEvent(&m_event)) {
            ImGui_ImplSDL3_ProcessEvent(&m_event);
            dispatch_event(m_event);
            had_event = true;

            // Drain remaining queued events
            while (SDL_PollEvent(&m_event)) {
                ImGui_ImplSDL3_ProcessEvent(&m_event);
                dispatch_event(m_event);
            }
        }
    }

    if (had_event) {
        // Reset cooldown so ImGui animations/hovers have a few frames to settle
        m_redraw_frames = k_cooldown_frames;
    } else if (m_redraw_frames > 0) {
        m_redraw_frames--;
    }
}

void application::dispatch_event(const SDL_Event& event) {
    if (event.type == SDL_EVENT_QUIT) {
        m_running = false;
    }

    if (event.type == SDL_EVENT_WINDOW_RESIZED) {
        m_width = event.window.data1;
        m_height = event.window.data2;
    }
}

void application::update() {
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    // --- Layout: Node Editor (top) and Details (bottom) ---
    ImGuiViewport* vp = ImGui::GetMainViewport();
    const float details_h = vp->WorkSize.y * 0.28f;
    const float editor_h  = vp->WorkSize.y - details_h;

    ImGui::SetNextWindowPos(vp->WorkPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(vp->WorkSize.x, editor_h), ImGuiCond_Always);
    ImGui::Begin("Node Editor", nullptr,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoTitleBar  | ImGuiWindowFlags_NoResize          |
        ImGuiWindowFlags_NoMove      | ImGuiWindowFlags_NoBringToFrontOnFocus);
    node_editor();
    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(vp->WorkPos.x, vp->WorkPos.y + editor_h), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(vp->WorkSize.x, details_h), ImGuiCond_Always);
    // --- Details panel (always open) ---
    inspector();

    // --- Floating toolbar overlay ---
    toolbar();

    // ImGui demo window
    if (m_show_demo_window)
        ImGui::ShowDemoWindow(&m_show_demo_window);
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
    ax::NodeEditor::DestroyEditor(m_node_editor_context);

    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(m_renderer);
    SDL_DestroyWindow(m_window);
    SDL_Quit();
}

ed::PinId application::make_pin_id(int node_id, int pin_index) {
    return ed::PinId(static_cast<uintptr_t>(node_id) << 16 | static_cast<uintptr_t>(pin_index));
}

std::pair<int, int> application::unpack_pin_id(ed::PinId pin_id) {
    auto val = pin_id.Get();
    return { static_cast<int>(val >> 16), static_cast<int>(val & 0xFFFF) };
}

ImVec4 application::pin_color(ls::pin_type type) const {
    switch (type) {
        case ls::pin_type::number: return m_colors[editor::Color_PinNumber];
        case ls::pin_type::grid:   return m_colors[editor::Color_PinGrid];
    }
    return ImVec4(1, 1, 1, 1);
}

ImVec4 application::category_color(const std::string& category) const {
    if (category == "IO")         return m_colors[editor::Color_HeaderInput];
    if (category == "Generation") return m_colors[editor::Color_HeaderProcess];
    if (category == "Analysis")   return m_colors[editor::Color_HeaderProcess];
    return m_colors[editor::Color_HeaderOutput];
}

application::pin_info application::resolve_pin(ed::PinId pin_id) const {
    auto [node_id, pin_index] = unpack_pin_id(pin_id);
    auto* n = m_generator.engine().find_node(node_id);
    if (!n) return { node_id, pin_index, nullptr };
    const auto& desc = n->descriptor();
    if (pin_index < 0 || pin_index >= static_cast<int>(desc.pins.size()))
        return { node_id, pin_index, nullptr };
    return { node_id, pin_index, &desc.pins[pin_index] };
}

void application::node_editor() {
    ed::SetCurrentEditor(m_node_editor_context);
    ed::Begin("Level Synth", ImVec2(0.0, 0.0f));

    auto& eng = m_generator.engine();
    ed::NodeBuilder builder;
    const ImVec2 icon_size(16, 16);

    // --- Render all nodes from engine ---
    for (int node_id : eng.node_ids()) {
        auto* n = eng.find_node(node_id);
        if (!n) continue;

        const auto& desc = n->descriptor();
        ImVec4 header_color = category_color(desc.category);

        if (m_first_frame) {
            // Stagger initial positions so nodes don't overlap
            ed::SetNodePosition(ed::NodeId(node_id),
                ImVec2(10.0f + (node_id - 1) * 300.0f, 10.0f));
        }

        builder.Begin(ed::NodeId(node_id));
        {
            builder.Header(header_color);
                ImGui::TextUnformatted(desc.name.c_str());
                ImGui::Dummy(ImVec2(80, 0));
            builder.EndHeader();

            builder.BeginColumns();
            {
                // Input pins (left column)
                ImGui::BeginGroup();
                for (int i = 0; i < static_cast<int>(desc.pins.size()); i++) {
                    const auto& pin = desc.pins[i];
                    if (pin.direction != ls::pin_direction::input) continue;

                    auto pid = make_pin_id(node_id, i);
                    ImVec4 color = pin_color(pin.type);

                    ed::BeginPin(pid, ed::PinKind::Input);
                        ed::DrawPinIcon(icon_size, ed::PinIconType::Circle, true, color);
                        ImGui::SameLine();
                        ImGui::TextUnformatted(pin.name.c_str());
                    ed::EndPin();
                }
                ImGui::EndGroup();

                ImGui::SameLine(0, 20);

                // Output pins (right column)
                ImGui::BeginGroup();
                for (int i = 0; i < static_cast<int>(desc.pins.size()); i++) {
                    const auto& pin = desc.pins[i];
                    if (pin.direction != ls::pin_direction::output) continue;

                    auto pid = make_pin_id(node_id, i);
                    ImVec4 color = pin_color(pin.type);

                    ed::BeginPin(pid, ed::PinKind::Output);
                        ImGui::TextUnformatted(pin.name.c_str());
                        ImGui::SameLine();
                        ed::DrawPinIcon(icon_size, ed::PinIconType::Circle, true, color);
                    ed::EndPin();
                }
                ImGui::EndGroup();
            }
            builder.EndColumns();
        }
        builder.End();
    }

    // --- Render links from side map ---
    for (const auto& [link_id, wv] : m_link_to_wire) {
        // Find output pin ID
        auto* from_node = eng.find_node(wv.from_node);
        auto* to_node = eng.find_node(wv.to_node);
        if (!from_node || !to_node) continue;

        const auto& from_desc = from_node->descriptor();
        const auto& to_desc = to_node->descriptor();

        ed::PinId from_pin_id, to_pin_id;
        ImVec4 link_color(1, 1, 1, 1);

        // Find output pin index by name
        for (int i = 0; i < static_cast<int>(from_desc.pins.size()); i++) {
            if (from_desc.pins[i].name == wv.from_pin &&
                from_desc.pins[i].direction == ls::pin_direction::output) {
                from_pin_id = make_pin_id(wv.from_node, i);
                link_color = pin_color(from_desc.pins[i].type);
                break;
            }
        }

        // Find input pin index by name
        for (int i = 0; i < static_cast<int>(to_desc.pins.size()); i++) {
            if (to_desc.pins[i].name == wv.to_pin &&
                to_desc.pins[i].direction == ls::pin_direction::input) {
                to_pin_id = make_pin_id(wv.to_node, i);
                break;
            }
        }

        ed::Link(ed::LinkId(link_id), from_pin_id, to_pin_id, link_color, 2.0f);
    }

    // --- Handle new connections ---
    if (ed::BeginCreate()) {
        ed::PinId input_pin_id, output_pin_id;
        if (ed::QueryNewLink(&input_pin_id, &output_pin_id)) {
            if (input_pin_id && output_pin_id) {
                auto input_info = resolve_pin(input_pin_id);
                auto output_info = resolve_pin(output_pin_id);

                // Determine which is input and which is output
                // (user might drag from either direction)
                const ls::pin_descriptor* src = nullptr;
                const ls::pin_descriptor* dst = nullptr;
                int src_node, dst_node;
                std::string src_pin, dst_pin;

                if (input_info.desc && input_info.desc->direction == ls::pin_direction::output &&
                    output_info.desc && output_info.desc->direction == ls::pin_direction::input) {
                    src = input_info.desc;  src_node = input_info.node_id;  src_pin = src->name;
                    dst = output_info.desc; dst_node = output_info.node_id; dst_pin = dst->name;
                } else if (input_info.desc && input_info.desc->direction == ls::pin_direction::input &&
                           output_info.desc && output_info.desc->direction == ls::pin_direction::output) {
                    dst = input_info.desc;  dst_node = input_info.node_id;  dst_pin = dst->name;
                    src = output_info.desc; src_node = output_info.node_id; src_pin = src->name;
                }

                bool can_connect = src && dst && src->type == dst->type;

                if (can_connect) {
                    if (ed::AcceptNewItem()) {
                        ls::wire w{ src_node, src_pin, dst_node, dst_pin };
                        eng.add_wire(w);

                        int link_id = m_next_link_id++;
                        m_link_to_wire[link_id] = { src_node, src_pin, dst_node, dst_pin };
                    }
                } else {
                    ed::RejectNewItem(ImVec4(1, 0, 0, 1), 2.0f);
                }
            }
        }
    }
    ed::EndCreate();

    // --- Handle deletions ---
    if (ed::BeginDelete()) {
        // Link deletion
        ed::LinkId deleted_link_id;
        while (ed::QueryDeletedLink(&deleted_link_id)) {
            if (ed::AcceptDeletedItem()) {
                int lid = static_cast<int>(deleted_link_id.Get());
                auto it = m_link_to_wire.find(lid);
                if (it != m_link_to_wire.end()) {
                    const auto& wv = it->second;
                    eng.remove_wire(wv.from_node, wv.from_pin,
                                    wv.to_node, wv.to_pin);
                    m_link_to_wire.erase(it);
                }
            }
        }

        // Node deletion
        ed::NodeId deleted_node_id;
        while (ed::QueryDeletedNode(&deleted_node_id)) {
            if (ed::AcceptDeletedItem()) {
                int nid = static_cast<int>(deleted_node_id.Get());

                // Remove associated links
                std::erase_if(m_link_to_wire, [&](const auto& pair) {
                    const auto& wv = pair.second;
                    return wv.from_node == nid || wv.to_node == nid;
                });

                eng.remove_node(nid);
            }
        }
    }
    ed::EndDelete();

    // --- Right-click context menu for adding nodes ---
    // Detect right-click before Suspend so we have the canvas-space position,
    // but defer OpenPopup until after Suspend so ImGui anchors the popup in
    // normal screen coordinates (not the canvas-transformed space).
    bool open_add_node_popup = ed::ShowBackgroundContextMenu();
    if (open_add_node_popup)
        m_popup_canvas_pos = ed::ScreenToCanvas(ImGui::GetMousePos());

    ed::Suspend();
    if (open_add_node_popup)
        ImGui::OpenPopup("##add_node");
    if (ImGui::BeginPopup("##add_node")) {
        ImGui::TextUnformatted("Add Node");
        ImGui::Separator();

        auto& reg = ls::node_registry::instance();
        auto types = reg.registered_types();

        // Group by category, sorted for stable ordering
        std::map<std::string, std::vector<std::string>> by_category;
        for (const auto& type_name : types)
            by_category[reg.descriptor(type_name).category].push_back(type_name);

        for (auto& [category, type_names] : by_category) {
            std::sort(type_names.begin(), type_names.end(), [&](const auto& a, const auto& b) {
                return reg.descriptor(a).name < reg.descriptor(b).name;
            });

            if (ImGui::BeginMenu(category.c_str())) {
                for (const auto& type_name : type_names) {
                    const auto& desc = reg.descriptor(type_name);
                    if (ImGui::MenuItem(desc.name.c_str())) {
                        int nid = eng.add_node(reg.create(type_name));
                        ed::SetNodePosition(ed::NodeId(nid), m_popup_canvas_pos);
                    }
                }
                ImGui::EndMenu();
            }
        }

        ImGui::EndPopup();
    }
    ed::Resume();

    ed::End();

    // Update inspector selection (query after End so all interaction is settled)
    {
        int count = ed::GetSelectedObjectCount();
        std::vector<ed::NodeId> sel(count);
        count = ed::GetSelectedNodes(sel.data(), static_cast<int>(sel.size()));
        m_inspector_node_id = (count == 1) ? static_cast<int>(sel[0].Get()) : -1;
    }

    ed::SetCurrentEditor(nullptr);
    m_first_frame = false;
}

void application::toolbar() {
    auto& io = ImGui::GetIO();
    const float scale = 1.0f;

    // Toolbar dimensions
    const float toolbar_height = 36.0f * scale;
    const float toolbar_padding = 6.0f * scale;
    const float button_size = 24.0f * scale;

    // Position at top-center
    float toolbar_width = 420.0f * scale;
    float toolbar_x = (m_width - toolbar_width) * 0.5f;
    float toolbar_y = 12.0f * scale;

    draw_window_shadow(ImVec2(toolbar_x, toolbar_y), ImVec2(toolbar_width, toolbar_height));
    ImGui::SetNextWindowPos(ImVec2(toolbar_x, toolbar_y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(toolbar_width, toolbar_height), ImGuiCond_Always);

    // ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f * scale);
    // ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(toolbar_padding, toolbar_padding));
    // ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, m_colors[editor::Color_ToolbarBg]);
    ImGui::PushStyleColor(ImGuiCol_Border, m_colors[editor::Color_ToolbarBorder]);

    ImGui::Begin("##toolbar", nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoCollapse);

    // Transparent button style
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, m_colors[editor::Color_ToolbarButtonHovered]);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, m_colors[editor::Color_ToolbarButtonActive]);
    // ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f * scale);

    // Hamburger menu button
    if (ImGui::Button(ICON_FI_BARS, ImVec2(button_size, button_size)))
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
            if (ImGui::MenuItem("ImGui Demo", nullptr, m_show_demo_window))
                m_show_demo_window = !m_show_demo_window;
            ImGui::Separator();
            ImGui::MenuItem("About Level Synth");
            ImGui::EndMenu();
        }
        ImGui::EndPopup();
    }

    ImGui::SameLine();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine();

    // Theme toggle
    const char* theme_label = m_dark_theme ? ICON_FI_DARK_THEME : ICON_FI_BRIGHTNESS_HIGH;
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

    // Zoom controls.
    // GetCurrentZoom() returns InvScale (= 1/Scale). SetCurrentZoom(v) sets
    // Scale = 1/v. So a *smaller* InvScale value means more zoomed in.
    // Multiply InvScale by > 1 to zoom out, by < 1 to zoom in.
    // Apparent magnification shown to the user = 1/InvScale * 100 %.
    ed::SetCurrentEditor(m_node_editor_context);
    float inv_scale = ed::GetCurrentZoom();

    if (ImGui::Button(ICON_FI_ZOOM_OUT, ImVec2(button_size, button_size)))
        ed::SetCurrentZoom(inv_scale * 1.25f);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Zoom out");

    ImGui::SameLine();

    // Zoom percentage display (click to reset to 100%)
    char zoom_buf[16];
    snprintf(zoom_buf, sizeof(zoom_buf), "%d%%", (int)(100.0f / inv_scale + 0.5f));
    if (ImGui::Button(zoom_buf, ImVec2(0, button_size)))
        ed::SetCurrentZoom(1.0f);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Reset zoom to 100%%");

    ImGui::SameLine();

    if (ImGui::Button(ICON_FI_ZOOM_IN, ImVec2(button_size, button_size)))
        ed::SetCurrentZoom(inv_scale * 0.8f);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Zoom in");

    ImGui::SameLine();

    // Fit to content
    if (ImGui::Button(ICON_FI_ZOOM_FIT, ImVec2(button_size, button_size)))
        ed::NavigateToContent();
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Fit to content");

    ed::SetCurrentEditor(nullptr);

    ImGui::SameLine();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine();

    // Evaluate button
    if (ImGui::Button(ICON_FI_PLAY, ImVec2(button_size, button_size)))
        m_generator.engine().evaluate(0);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Evaluate graph");

    ImGui::SameLine();

    // ImGui demo window toggle
    if (ImGui::Button(ICON_FI_WINDOW, ImVec2(button_size, button_size)))
        m_show_demo_window = !m_show_demo_window;
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("ImGui demo");

    ImGui::SameLine();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine();

    // FPS display
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4.0f * scale);
    ImGui::TextDisabled("%.0f fps", io.Framerate);

    // ImGui::PopStyleVar(1); // FrameRounding
    ImGui::PopStyleColor(3); // Button colors

    ImGui::End();

    ImGui::PopStyleColor(2); // WindowBg, Border
    // ImGui::PopStyleVar(3); // WindowRounding, WindowPadding, WindowBorderSize
}

void application::set_node_editor_style() {
    ed::Style& edStyle = ed::GetStyle();
    edStyle.NodePadding              = ImVec4(8, 8, 8, 8);
    edStyle.NodeRounding             = 12.0f;
    edStyle.HoveredNodeBorderWidth   = 3.0f;
    edStyle.HoverNodeBorderOffset    = 0.0f;
    edStyle.SelectedNodeBorderWidth  = 4.0f;
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
}

void application::apply_theme() {
    ed::SetCurrentEditor(m_node_editor_context);
    if (m_dark_theme)
        set_dark_theme();
    else
        set_light_theme();
    ed::SetCurrentEditor(nullptr);
}

void application::set_light_theme() {
    auto& style = ImGui::GetStyle();
    style.WindowBorderSize = 0.0f;
    style.FrameBorderSize = 0.0f;
    style.FrameRounding = 5.0f;
    style.ChildRounding = 5.0f;
    style.GrabRounding = 5.0f;

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

    // Editor custom colors
    m_colors[editor::Color_PinNumber]          = ImVec4(0.25f, 0.75f, 0.85f, 1.0f);
    m_colors[editor::Color_PinGrid]             = ImVec4(0.65f, 0.40f, 0.85f, 1.0f);
    m_colors[editor::Color_HeaderInput]         = ImVec4(0.30f, 0.60f, 0.30f, 1.0f);
    m_colors[editor::Color_HeaderProcess]       = ImVec4(0.75f, 0.45f, 0.20f, 1.0f);
    m_colors[editor::Color_HeaderOutput]        = ImVec4(0.30f, 0.45f, 0.70f, 1.0f);
    m_colors[editor::Color_ToolbarBg]           = ImVec4(0.95f, 0.95f, 0.96f, 0.92f);
    m_colors[editor::Color_ToolbarBorder]       = ImVec4(0.70f, 0.70f, 0.72f, 0.60f);
    m_colors[editor::Color_ToolbarButtonHovered]= ImVec4(0.80f, 0.80f, 0.82f, 0.60f);
    m_colors[editor::Color_ToolbarButtonActive] = ImVec4(0.70f, 0.70f, 0.72f, 0.80f);

    // Node editor style
    set_node_editor_style();

    using namespace ax::NodeEditor;
    ed::Style& edStyle = ed::GetStyle();
    edStyle.NodeBorderWidth = 1.0f;
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

    // Editor custom colors
    m_colors[editor::Color_PinNumber]          = ImVec4(0.25f, 0.75f, 0.85f, 1.0f);
    m_colors[editor::Color_PinGrid]             = ImVec4(0.65f, 0.40f, 0.85f, 1.0f);
    m_colors[editor::Color_HeaderInput]         = ImVec4(0.30f, 0.60f, 0.30f, 1.0f);
    m_colors[editor::Color_HeaderProcess]       = ImVec4(0.75f, 0.45f, 0.20f, 1.0f);
    m_colors[editor::Color_HeaderOutput]        = ImVec4(0.30f, 0.45f, 0.70f, 1.0f);
    m_colors[editor::Color_ToolbarBg]           = ImVec4(0.18f, 0.18f, 0.19f, 0.92f);
    m_colors[editor::Color_ToolbarBorder]       = ImVec4(0.30f, 0.30f, 0.32f, 0.60f);
    m_colors[editor::Color_ToolbarButtonHovered]= ImVec4(0.30f, 0.30f, 0.32f, 0.60f);
    m_colors[editor::Color_ToolbarButtonActive] = ImVec4(0.35f, 0.35f, 0.37f, 0.80f);

    // Node editor style
    set_node_editor_style();

    using namespace ax::NodeEditor;
    ed::Style& edStyle = ed::GetStyle();
    edStyle.NodeBorderWidth = 0.0f;
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

void application::inspector() {
    ImGui::Begin("Details");

    if (m_inspector_node_id == -1) {
        ImGui::TextDisabled("Select a node to inspect.");
        ImGui::End();
        return;
    }

    auto& eng = m_generator.engine();
    auto* n = eng.find_node(m_inspector_node_id);
    if (!n) { m_inspector_node_id = -1; return; }

    const auto& desc = n->descriptor();

    // Header: category badge + node name
    ImGui::TextColored(category_color(desc.category), "%s", desc.category.c_str());
    ImGui::Separator();

    // Node-specific property controls
    ImGui::PushID(m_inspector_node_id);
    n->draw_ui();
    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
        (ImGui::IsAnyItemActive() || ImGui::IsItemDeactivatedAfterEdit()))
        eng.invalidate(m_inspector_node_id);
    ImGui::PopID();

    // Grid preview: show the first cached grid output, if any
    for (const auto& pin : desc.pins) {
        if (pin.direction != ls::pin_direction::output || pin.type != ls::pin_type::grid)
            continue;
        const auto* val = eng.get_output(m_inspector_node_id, pin.name);
        if (!val || !std::holds_alternative<std::shared_ptr<ls::attribute_grid>>(*val))
            continue;
        const auto& grid = std::get<std::shared_ptr<ls::attribute_grid>>(*val);
        if (!grid || grid->width() == 0 || grid->height() == 0)
            continue;

        const auto attrs = grid->attribute_names();
        if (attrs.empty()) continue;
        const std::string& attr = attrs[0];

        ImGui::Separator();
        ImGui::TextDisabled("Preview: %s (%dx%d)", attr.c_str(), grid->width(), grid->height());

        const float preview_w = ImGui::GetContentRegionAvail().x;
        const float cell = preview_w / static_cast<float>(grid->width());
        const float preview_h = cell * static_cast<float>(grid->height());

        const auto cells = grid->data(attr);
        const int min_v = *std::min_element(cells.begin(), cells.end());
        const int max_v = *std::max_element(cells.begin(), cells.end());
        const int range = std::max(1, max_v - min_v);

        const ImVec2 origin = ImGui::GetCursorScreenPos();
        ImDrawList* dl = ImGui::GetWindowDrawList();

        for (int y = 0; y < grid->height(); y++) {
            for (int x = 0; x < grid->width(); x++) {
                float t = static_cast<float>(grid->get(attr, x, y) - min_v)
                          / static_cast<float>(range);
                auto v = static_cast<int>(t * 210 + 20);
                ImU32 col = IM_COL32(v, v, v, 255);
                ImVec2 p0(origin.x + x * cell,        origin.y + y * cell);
                ImVec2 p1(p0.x    + cell + 0.5f,      p0.y    + cell + 0.5f);
                dl->AddRectFilled(p0, p1, col);
            }
        }
        ImGui::Dummy(ImVec2(preview_w, preview_h));
        break;
    }

    ImGui::End();
}

void application::draw_window_shadow(ImVec2 pos, ImVec2 size, float rounding) {
    ImDrawList* bg = ImGui::GetBackgroundDrawList();
    const float offset = 8.0f;
    // Four passes: outermost (most transparent) to innermost (most opaque)
    for (int i = 4; i >= 1; i--) {
        float spread = offset * static_cast<float>(i) * 0.35f;
        int   alpha  = static_cast<int>(255 * 0.055f * static_cast<float>(5 - i));
        bg->AddRectFilled(
            ImVec2(pos.x - spread + offset, pos.y - spread + offset),
            ImVec2(pos.x + size.x + spread + offset, pos.y + size.y + spread + offset),
            IM_COL32(0, 0, 0, alpha), rounding + spread);
    }
}
