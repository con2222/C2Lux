#pragma once

#include "GLFW/glfw3.h"

// std
#include <string>
#include <memory>

namespace C2Lux {

class Window {
public:
    ~Window();
    Window(const Window& other) = delete;
    Window& operator=(const Window& other) = delete;

    Window(Window&& other) noexcept;
    Window& operator=(Window&& other) noexcept;

    static std::unique_ptr<Window> create(int width_, int height_, std::string name_);
    bool shouldClose();
    void pollEvents();
    void toggleFullscreen();

    float getAndResetScrollDelta();

private:
    Window(GLFWwindow* window_);

    GLFWwindow* window;
    int height;
    int width;

    bool isFullscreen = false;
    int savedX = 0, savedY = 0;
    int savedWidth = 0, savedHeight = 0;

    bool framebufferResized = false;
    std::string name;
    float scrollDelta = 0.0f;
   

public:
    GLFWwindow* getGLFWwindow() const { return window; }
    int getWidth() const { return width; }
    int getHeight() const { return height; }

    bool hasResized() const { return framebufferResized; }
    void setResized(bool resized) { framebufferResized = resized; } 
    void setScrollDelta(float delta) { scrollDelta = delta; }

    std::pair<int, int> getFramebufferSize();

};

} // namespace C2Lux
