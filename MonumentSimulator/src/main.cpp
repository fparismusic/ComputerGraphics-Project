// This is the main: probably you do not need to touch this!
#include <iostream>
#include <exception>
#include <memory>

#include "modules/droneSimulator.hpp"

//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------
/*
 * MAIN FUNCTION: do not touch this
 */
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