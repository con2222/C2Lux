#pragma once

#include "webgpu/webgpu_cpp.h"
#include "GLFW/glfw3.h"
#include "glm/glm.hpp"

// std
#include <string>

namespace C2Lux {

class UILayer {
public:
    UILayer() = default;
    ~UILayer();

    bool init(GLFWwindow* window, wgpu::Device device, wgpu::TextureFormat targetFormat);

    void destroy();

    void beginFrame();

    std::string buildUI();
    std::string buildStartupUI();
    void rebuildFontsAndScale();
    void buildSettingBlock();

    void draw(wgpu::RenderPassEncoder renderPass);

    glm::vec4 bgColor = { 0.1f, 0.1f, 0.1f, 1.0f };
    glm::vec4 modelColor = { 0.8f, 0.8f, 0.8f, 1.0f };

    // Light data
    float lightDirection[3] = { 1.0f, 1.0f, 1.0f };
    float ambientOcclusion = 0.1f;
    float intensity = 0.2f;
    float shininess = 10.f;
    float specularStrength = 0.1f;

    bool fullScreen = false;
    bool exitMenu = false;
    bool exit = false;
    bool fullScreenFlag = false;

private:
    unsigned int fontSize = 16.f;
    float currentScale = 2.1f;
    bool bNeedsRebuild = false;

public:
    bool needsRebuild() const { return bNeedsRebuild; }
};

} // namespace C2Lux