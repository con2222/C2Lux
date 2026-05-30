#pragma once

#include "Window.hpp"
#include "Renderer.hpp"
#include "UILayer.hpp"
#include "Camera.hpp"

// std
#include <memory>


namespace C2Lux {

class App {
public:
    enum class AppState {
        Startup,
        Viewing
    };

    ~App();
    App(std::unique_ptr<Window> window);

    static std::unique_ptr<App> create();
    bool initWebGPU();
    bool isRunning();
    void mainLoop();

    bool handleWindowEvents();

private:
    std::unique_ptr<Window> mainWindow;
    Renderer renderer;
    UILayer uiLayer;
    Camera camera;
    AppState appState;
    bool bisRunning;

    void processInput(float deltaTime);
    void renderFrame();
public:
    float getDeltaTime() const;

    AppState getAppState() const { return appState; }
    void setAppState(AppState newState) { appState = newState; }
};



} // namespace C2Lux
