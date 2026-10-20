#include "App.hpp"

// std
#include <iostream>
#include <memory>


int main() {
    std::unique_ptr<C2Lux::App> app = C2Lux::App::create();

    if (!app) {
        std::cerr << "Could not start app" << std::endl;
        return 1;
    }
    if (!app->initWebGPU()) {
        std::cerr << "Could not initialize WebGPU" << std::endl;
        app.reset();
        glfwTerminate();
        return 1;
    }

    while (app->isRunning()) {
        app->mainLoop();
    }

    // Destroy the renderer, UI and window before shutting down GLFW.
    app.reset();
    glfwTerminate();
    return 0;
}
