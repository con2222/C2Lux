#include "Window.hpp"
#include "GLFW/glfw3.h"


namespace C2Lux {

Window::Window(GLFWwindow* window_) {
    window = window_;
}
    
Window::~Window() {
    glfwDestroyWindow(window);
    
}

Window::Window(Window&& other) noexcept {
    if (this != &other) {
        window = other.window;
        height = other.height;
        width = other.width;
        name = other.name;

        other.window = nullptr;
        other.height = -1;
        other.width = -1;
        other.name = "";
    }
}

Window& Window::operator=(Window&& other) noexcept {
    if (this != &other) {
        glfwDestroyWindow(window);
        window = other.window;
        height = other.height;
        width = other.width;
        name = other.name;

        other.window = nullptr;
        other.height = -1;
        other.width = -1;
        other.name = "";
    }
    return *this;
}

std::unique_ptr<Window> Window::create(int width_, int height_, std::string name_) {
    //glfwWindowHintString(GLFW_WAYLAND_APP_ID, "c2lux.viewer");

    GLFWwindow* glfwWindow = glfwCreateWindow(width_, height_, name_.c_str(), nullptr, nullptr);
    if (!glfwWindow) {
        return nullptr;
    } else {
        std::unique_ptr<Window> wPtr(new Window(glfwWindow));
        glfwSetWindowUserPointer(glfwWindow, wPtr.get());
        glfwSetFramebufferSizeCallback(
            wPtr->getGLFWwindow(), 
            [](GLFWwindow* window, int, int) {
                auto that = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
                if (that != nullptr) that->setResized(true);
            }
        );
        glfwSetScrollCallback(
            wPtr->getGLFWwindow(),
            [](GLFWwindow* window, double xoffset, double yoffset) {
                auto that = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
                if (that != nullptr) {
                    that->setScrollDelta(static_cast<float>(yoffset));
                }
            }
        );
        return wPtr;
    }
}

void Window::pollEvents() {
    glfwPollEvents();
}

void Window::toggleFullscreen()
{
    if (!isFullscreen) {
        glfwGetWindowPos(window, &savedX, &savedY);
        glfwGetWindowSize(window, &savedWidth, &savedHeight);

        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);

        glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
        isFullscreen = true;
    }
    else {
        glfwSetWindowMonitor(window, nullptr, savedX, savedY, savedWidth, savedHeight, 0);
        isFullscreen = false;
    }
}

float Window::getAndResetScrollDelta()
{
    float d = scrollDelta;
    scrollDelta = 0.0f;
    return d;   
}

bool Window::shouldClose() {
    return glfwWindowShouldClose(window);
}

std::pair<int, int> Window::getFramebufferSize() {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    return std::pair<int, int>(width, height);
}

} // namespace C2Lux
