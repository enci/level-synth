# Using modern CMake
cmake_minimum_required(VERSION 3.26)

# Set the project name
project(level-synth
    VERSION 0.1
    DESCRIPTION "A level synthesizer"
    LANGUAGES CXX)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 20)

# Set the build shared libraries
set(BUILD_SHARED_LIBS OFF)

# Add SDL3 support
add_subdirectory(external/sdl EXCLUDE_FROM_ALL)

# Add the executable
add_executable(level-synth source/main.cpp)

# Add include directories
target_include_directories(level-synth
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/external/SDL/include
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/backends
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/external/ImNodeFlow/include)

# ImGui does not come with a CMakeLists.txt file, so we have to add the source files manually as a library
add_library(imgui STATIC
        external/imgui/imgui.cpp
        external/imgui/imgui_demo.cpp
        external/imgui/imgui_draw.cpp
        external/imgui/imgui_widgets.cpp
        external/imgui/imgui_tables.cpp
        external/imgui/backends/imgui_impl_sdl3.cpp
        external/imgui/backends/imgui_impl_sdlrenderer3.cpp)

# Add include directories
target_include_directories(imgui
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/external/SDL/include
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/backends)

# Add ImNodeFlow directly as a library
add_compile_definitions(IMGUI_DEFINE_MATH_OPERATORS)
add_library(ImNodeFlow STATIC
        external/ImNodeFlow/src/ImNodeFlow.cpp)

# Add include directories for ImNodeFlow
target_include_directories(ImNodeFlow
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/external/ImNodeFlow/include)

# Link to the actual SDL3 library. SDL3::SDL3 is the shared SDL library, SDL2::SDL2-static is the static SDL libarary.
target_link_libraries(level-synth PRIVATE
        SDL3::SDL3
        imgui
        ImNodeFlow)
