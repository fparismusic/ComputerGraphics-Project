#include <iostream>
#include <exception>
#include <memory>

#include "modules/droneSimulator.hpp"

int main() {
    DroneSimulator app;

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}