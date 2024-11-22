# Using modern CMake
cmake_minimum_required(VERSION 3.26)

# Set the project name
project(level-synth
    VERSION 0.1
    DESCRIPTION "A level synthesizer"
    LANGUAGES CXX)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 20)

# Add the executable
add_executable(level-synth source/main.cpp)
