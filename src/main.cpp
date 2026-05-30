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
    app->initWebGPU();

    while (app->isRunning()) {
        app->mainLoop();
    }

    return 0;
}
