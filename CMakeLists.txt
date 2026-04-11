cmake_minimum_required(VERSION 3.16)
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

# ---- Library target (header-only for now) ----
add_library(level_synth_library INTERFACE)
target_include_directories(level_synth_library INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/library
)

# ---- ImGui sources ----
set(IMGUI_DIR ${imgui_SOURCE_DIR})
set(IMGUI_SOURCES
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_demo.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
)

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
        library/level_synth/attribute_grid.cpp
        library/level_synth/attribute_grid.hpp
)

set(EDITOR_HEADERS
        ${EDITOR_DIR}/application.hpp
        ${EDITOR_DIR}/nodes/node_colors.hpp
        library/level_synth/node.hpp
        library/level_synth/attribute_grid.cpp
        library/level_synth/attribute_grid.hpp
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
        ${freetype_SOURCE_DIR}/include
)

target_compile_definitions(level_synth_editor PRIVATE
        IMGUI_USE_WCHAR32
)

target_link_libraries(level_synth_editor PRIVATE
        level_synth_library
        SDL3::SDL3-static
        freetype
)
