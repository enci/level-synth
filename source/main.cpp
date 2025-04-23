#include "application.hpp"
#include <iostream>

int main(int argc, char** argv) {
    try {
        application app("Game Tool", 1280, 720);
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}