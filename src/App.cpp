#include "App.hpp"
#include "glfw3webgpu.h"
#include "imgui.h"
#include "ResourceManager.hpp"
#include "nfd.h"

// std
#include <iostream>
#include <iomanip> 


void printMatrix(const std::string& name, const glm::mat4& m) {
    std::cout << "=== " << name << " ===" << std::endl;
    for (int row = 0; row < 4; ++row) {
        std::cout << "[ ";
        for (int col = 0; col < 4; ++col) {
            std::cout << std::setw(8) << std::fixed << std::setprecision(2) << m[col][row] << " ";
        }
        std::cout << "]" << std::endl;
    }
    std::cout << "==================\n" << std::endl;
}

namespace C2Lux {
    
App::~App() {
    uiLayer.destroy();
    glfwTerminate();
}

App::App(std::unique_ptr<Window> window) : mainWindow(std::move(window)), appState(AppState::Startup), bisRunning(true) {}

bool App::initWebGPU() {
    if (!renderer.initInstance()) return false;
    if (!renderer.setSurface(glfwGetWGPUSurface(renderer.getInstance().Get(), mainWindow->getGLFWwindow()))) return false;
    if (!renderer.initAdapter()) return false;
    if (!renderer.initDevice()) return false;
    if (!renderer.initQueue()) return false;

    renderer.setupSurfaceConfig(mainWindow->getFramebufferSize().first, mainWindow->getFramebufferSize().second);

    ResourceManager resourceManager;
    wgpu::ShaderModule shader = resourceManager.loadShaderModule("shader.wgsl", renderer.getDevice());

    renderer.initBuffers();
    renderer.initBindGroups();
    renderer.initRenderPipeline(shader);

    if (!uiLayer.init(mainWindow->getGLFWwindow(), renderer.getDevice(), renderer.getSurfaceFormat())) {
        std::cerr << "Could not initialize UI Layer!" << std::endl;
        return false;
    }

    return true;
}

// TODO: add new class to process input
void App::processInput(float deltaTime) {
    ImGuiIO& io = ImGui::GetIO();

    if (!io.WantCaptureKeyboard) {
        if (glfwGetKey(mainWindow->getGLFWwindow(), GLFW_KEY_W) == GLFW_PRESS) camera.moveForward(deltaTime);
        if (glfwGetKey(mainWindow->getGLFWwindow(), GLFW_KEY_S) == GLFW_PRESS) camera.moveBackward(deltaTime);
        if (glfwGetKey(mainWindow->getGLFWwindow(), GLFW_KEY_A) == GLFW_PRESS) camera.moveLeft(deltaTime);
        if (glfwGetKey(mainWindow->getGLFWwindow(), GLFW_KEY_D) == GLFW_PRESS) camera.moveRight(deltaTime);
        if (glfwGetKey(mainWindow->getGLFWwindow(), GLFW_KEY_P) == GLFW_PRESS) {
            printMatrix("\nView Matrix", camera.getViewMatrix());
            printMatrix("Projection", camera.getProjectionMatrix(1920.0f / 1080.0f));
        }


        static bool f11WasPressed = false;
        bool f11IsPressed = glfwGetKey(mainWindow->getGLFWwindow(), GLFW_KEY_F11) == GLFW_PRESS;

        if (f11IsPressed && !f11WasPressed) {
            mainWindow->toggleFullscreen();
        }
        f11WasPressed = f11IsPressed;
    }

    if (!io.WantCaptureMouse) {
        float scroll = mainWindow->getAndResetScrollDelta();
        if (scroll != 0.0f) {
            camera.zoom(scroll);
        }

        static double lastX = 0.0, lastY = 0.0;
        static bool isDragging = false;

        if (glfwGetMouseButton(mainWindow->getGLFWwindow(), GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
            double xpos, ypos;
            glfwGetCursorPos(mainWindow->getGLFWwindow(), &xpos, &ypos);

            if (!isDragging) {
                lastX = xpos;
                lastY = ypos;
                isDragging = true;
            }
            else {
                camera.rotate(static_cast<float>(xpos - lastX), static_cast<float>(ypos - lastY));
                lastX = xpos;
                lastY = ypos;
            }
        }
        else {
            isDragging = false;
        }
    }
}

void App::renderFrame()
{
    float deltaTime = getDeltaTime();

    if (uiLayer.needsRebuild()) {
        uiLayer.rebuildFontsAndScale();
    }

    if (uiLayer.exitMenu) {
        appState = AppState::Startup;
        uiLayer.exitMenu = false;
    }

    if (uiLayer.exit) {
        bisRunning = false;
    }

    uiLayer.beginFrame();

    switch (appState) {
    case AppState::Startup: {
        std::string path = uiLayer.buildStartupUI();
        if (!path.empty()) {
            appState = AppState::Viewing;
            ResourceManager resourceManager;
            renderer.updateMeshBuffers(resourceManager.loadObj(path));
        }
        renderer.drawJustUI(uiLayer);
        break;
    }
    case AppState::Viewing: {
        processInput(deltaTime);

        std::string path = uiLayer.buildUI();
        if (!path.empty()) {
            ResourceManager resourceManager;
            renderer.updateMeshBuffers(resourceManager.loadObj(path));
        }
        renderer.draw(uiLayer, camera);
        break;
    }
    }
}

std::unique_ptr<App> App::create() {
    if (!glfwInit()) {
        std::cerr << "Could not initialize GLFW!" << std::endl;
        return nullptr;
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    return std::unique_ptr<App>(new App(Window::create(1920, 1080, "Main window")));
}

bool App::isRunning() {
    return !(!bisRunning || mainWindow->shouldClose());
}

void App::mainLoop() {
    if (!handleWindowEvents()) return;
    renderFrame();
}

bool App::handleWindowEvents()
{
    mainWindow->pollEvents();
    auto [width, height] = mainWindow->getFramebufferSize();
    if (width <= 0 || height <= 0) { return false; }

    if (mainWindow->hasResized()) {
        renderer.resizeSwapchain(width, height);
        mainWindow->setResized(false);
        renderFrame();
    }

    return true;
}

float App::getDeltaTime() const
{
    static double lastFrame = 0.0;
    double currentFrame = glfwGetTime();
    float deltaTime = static_cast<float>(currentFrame - lastFrame);
    lastFrame = currentFrame;

    return deltaTime;
}

} // namespace C2Lux
