cmake_minimum_required(VERSION 3.24)
project(level-synth VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include FetchContent for dependencies
include(FetchContent)

# SDL3 options
set(SDL_SHARED OFF CACHE BOOL "Build a SDL shared library (if available)" FORCE)
set(SDL_STATIC ON CACHE BOOL "Build a SDL static library (if available)" FORCE)
set(SDL_TEST OFF CACHE BOOL "Build the SDL test programs" FORCE)

# Fetch SDL3
FetchContent_Declare(
        sdl3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG main  # Using main branch for SDL3
)
FetchContent_MakeAvailable(sdl3)

# Fetch ImGui (shadows branch for node drop shadows)
FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG features/shadows
)
FetchContent_MakeAvailable(imgui)

# ---- ImGui core library ----
# Compiled separately so that level_synth_library (and any downstream targets
# like sample_cave_gen) can resolve the ImGui symbols used in draw_ui() without
# each executable having to list ImGui sources itself.
add_library(imgui_lib STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
)
target_include_directories(imgui_lib PUBLIC ${imgui_SOURCE_DIR})
target_compile_definitions(imgui_lib PUBLIC IMGUI_USE_WCHAR32)

# ImGui Node Editor (git submodule in external/)
set(IMGUI_NODE_EDITOR_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui-node-editor)

# Fetch FreeType
FetchContent_Declare(
        freetype
        GIT_REPOSITORY https://github.com/freetype/freetype.git
        GIT_TAG master
)
set(FT_DISABLE_ZLIB ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BZIP2 ON CACHE BOOL "" FORCE)
set(FT_DISABLE_PNG ON CACHE BOOL "" FORCE)
set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BROTLI ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(freetype)

# ---- Library target ----
set(LIBRARY_SOURCES
        library/level_synth/attribute_grid.cpp
        library/level_synth/eval_context.cpp
        library/level_synth/eval_engine.cpp
        library/level_synth/node_registry.cpp
        library/level_synth/generator.cpp
        library/level_synth/nodes/node_create_grid.cpp
        library/level_synth/nodes/node_cellular_automata.cpp
        library/level_synth/nodes/node_input_number.cpp
        library/level_synth/nodes/node_output_grid.cpp
        library/level_synth/nodes/node_output_number.cpp
        library/level_synth/nodes/node_noise_grid.cpp
)

set(LIBRARY_HEADERS
        library/level_synth/level_synth.hpp
        library/level_synth/pin.hpp
        library/level_synth/node.hpp
        library/level_synth/attribute_grid.hpp
        library/level_synth/eval_task.hpp
        library/level_synth/eval_context.hpp
        library/level_synth/eval_engine.hpp
        library/level_synth/node_registry.hpp
        library/level_synth/generator.hpp
        library/level_synth/nodes/node_create_grid.hpp
        library/level_synth/nodes/node_cellular_automata.hpp
        library/level_synth/nodes/node_input_number.hpp
        library/level_synth/nodes/node_output_grid.hpp
        library/level_synth/nodes/node_output_number.hpp
        library/level_synth/nodes/node_noise_grid.hpp
)

add_library(level_synth_library STATIC
        ${LIBRARY_SOURCES}
        ${LIBRARY_HEADERS}
)
target_include_directories(level_synth_library PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/library
)

target_compile_definitions(level_synth_library PUBLIC
        LS_EDITOR
)

target_link_libraries(level_synth_library PUBLIC imgui_lib)

# ---- ImGui editor-only sources (core is in imgui_lib) ----
set(IMGUI_DIR ${imgui_SOURCE_DIR})
set(IMGUI_SOURCES)

set(IMGUI_NODE_EDITOR_SOURCES
        ${IMGUI_NODE_EDITOR_DIR}/imgui_node_editor.cpp
        ${IMGUI_NODE_EDITOR_DIR}/imgui_node_editor.h
        ${IMGUI_NODE_EDITOR_DIR}/imgui_node_editor_internal.h
        ${IMGUI_NODE_EDITOR_DIR}/crude_json.cpp
        ${IMGUI_NODE_EDITOR_DIR}/crude_json.h
        ${IMGUI_NODE_EDITOR_DIR}/imgui_node_editor_api.cpp
        ${IMGUI_NODE_EDITOR_DIR}/imgui_canvas.cpp
        ${IMGUI_NODE_EDITOR_DIR}/imgui_canvas.h
        ${IMGUI_NODE_EDITOR_DIR}/imgui_node_editor_node_builder.h
        ${IMGUI_NODE_EDITOR_DIR}/imgui_node_editor_node_builder.cpp
        ${IMGUI_NODE_EDITOR_DIR}/imgui_node_editor_pin_icons.h
        ${IMGUI_NODE_EDITOR_DIR}/imgui_node_editor_pin_icons.cpp
)

set(IMGUI_SDL3_SOURCES
        ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
        ${IMGUI_DIR}/backends/imgui_impl_sdlrenderer3.cpp
)

set(IMGUI_FREETYPE_SOURCES
        ${IMGUI_DIR}/misc/freetype/imgui_freetype.cpp
)

# ---- Editor executable ----
set(EDITOR_DIR ${CMAKE_CURRENT_SOURCE_DIR}/editor)

set(EDITOR_SOURCES
        ${EDITOR_DIR}/main.cpp
        ${EDITOR_DIR}/application.cpp
)

set(EDITOR_HEADERS
        ${EDITOR_DIR}/application.hpp
        ${EDITOR_DIR}/nodes/node_colors.hpp
        library/level_synth/node.hpp
        library/level_synth/attribute_grid.hpp
        library/level_synth/eval_context.hpp
        library/level_synth/eval_task.hpp
)

add_executable(level_synth_editor
        ${EDITOR_SOURCES}
        ${EDITOR_HEADERS}
        ${IMGUI_SOURCES}
        ${IMGUI_NODE_EDITOR_SOURCES}
        ${IMGUI_SDL3_SOURCES}
        ${IMGUI_FREETYPE_SOURCES}
)

target_include_directories(level_synth_editor PRIVATE
        ${EDITOR_DIR}
        ${IMGUI_DIR}
        ${IMGUI_DIR}/misc/freetype
        ${IMGUI_NODE_EDITOR_DIR}
        ${IMGUI_NODE_EDITOR_DIR}/external/stb_image
        ${freetype_SOURCE_DIR}/include
)

target_compile_definitions(level_synth_editor PRIVATE
        IMGUI_USE_WCHAR32
)

target_link_libraries(level_synth_editor PRIVATE
        $<LINK_LIBRARY:WHOLE_ARCHIVE,level_synth_library>
        SDL3::SDL3-static
        freetype
)

# ---- Tests ----
FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.8.0
)
FetchContent_MakeAvailable(Catch2)

add_executable(level_synth_tests
        tests/test_attribute_grid.cpp
)

target_link_libraries(level_synth_tests PRIVATE
        level_synth_library
        Catch2::Catch2WithMain
)

enable_testing()
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)
catch_discover_tests(level_synth_tests)

# ---- Samples ----
add_executable(sample_cave_gen samples/cave_gen.cpp)
target_link_libraries(sample_cave_gen PRIVATE level_synth_library)
