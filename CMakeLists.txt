cmake_minimum_required(VERSION 3.16)
project(level-synth VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
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

# Fetch ImGui
FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG features/shadows
)
FetchContent_MakeAvailable(imgui)

# Fetch ImGui Node Editor
FetchContent_Declare(
        imgui_node_editor
        GIT_REPOSITORY  https://github.com/enci/imgui-node-editor.git
        GIT_TAG master
)
FetchContent_MakeAvailable(imgui_node_editor)

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

# Project structure
set(LEVEL_SYNTH_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/source)
set(LEVEL_SYNTH_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Create directories if they don't exist
file(MAKE_DIRECTORY ${LEVEL_SYNTH_SOURCE_DIR})
file(MAKE_DIRECTORY ${LEVEL_SYNTH_INCLUDE_DIR})

# Set source files
set(SOURCES
        ${LEVEL_SYNTH_SOURCE_DIR}/main.cpp
        ${LEVEL_SYNTH_SOURCE_DIR}/application.cpp
)

# ImGui files
set(IMGUI_DIR ${imgui_SOURCE_DIR})
set(IMGUI_SOURCES
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_demo.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
)

# ImGui Node Editor files
set(IMGUI_NODE_EDITOR_DIR ${imgui_node_editor_SOURCE_DIR})
set(IMGUI_NODE_EDITOR_SOURCES
        ${IMGUI_NODE_EDITOR_DIR}/imgui_node_editor.cpp
        ${IMGUI_NODE_EDITOR_DIR}/imgui_node_editor.h
        ${IMGUI_NODE_EDITOR_DIR}/imgui_node_editor_internal.h
        ${IMGUI_NODE_EDITOR_DIR}/crude_json.cpp
        ${IMGUI_NODE_EDITOR_DIR}/crude_json.h
        ${IMGUI_NODE_EDITOR_DIR}/imgui_node_editor_api.cpp
        ${IMGUI_NODE_EDITOR_DIR}/imgui_canvas.cpp
        ${IMGUI_NODE_EDITOR_DIR}/imgui_canvas.h
)

# ImGui SDL3 backend files
set(IMGUI_SDL3_SOURCES
        ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
        ${IMGUI_DIR}/backends/imgui_impl_sdlrenderer3.cpp
)

# ImGui FreeType integration
set(IMGUI_FREETYPE_SOURCES
        ${IMGUI_DIR}/misc/freetype/imgui_freetype.cpp
)

# Set header files
set(HEADERS
        ${LEVEL_SYNTH_INCLUDE_DIR}/application.hpp
)

# Create executable
add_executable(${PROJECT_NAME}
        ${SOURCES}
        ${HEADERS}
        ${IMGUI_SOURCES}
        ${IMGUI_NODE_EDITOR_SOURCES}
        ${IMGUI_SDL3_SOURCES}
        ${IMGUI_FREETYPE_SOURCES}
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
        ${LEVEL_SYNTH_INCLUDE_DIR}
        ${IMGUI_DIR}
        ${IMGUI_DIR}/misc/freetype
        ${IMGUI_NODE_EDITOR_DIR}
        ${freetype_SOURCE_DIR}/include
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
        SDL3::SDL3-static
        freetype
)