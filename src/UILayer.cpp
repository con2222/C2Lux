#include "UILayer.hpp"

#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_wgpu.h"
#include "imgui.h"
#include "nfd.h"

// std
#include <iostream>

namespace C2Lux {

UILayer::~UILayer() { destroy(); }

bool UILayer::init(GLFWwindow* window, wgpu::Device device,
                   wgpu::TextureFormat targetFormat) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    nfdInitialized = NFD_Init() == NFD_OKAY;
    if (!nfdInitialized) {
        destroy();
        return false;
    }
    glfwInitialized = ImGui_ImplGlfw_InitForOther(window, true);
    if (!glfwInitialized) {
        destroy();
        return false;
    }

    ImGui_ImplWGPU_InitInfo initInfo{};
    initInfo.Device = device.Get();
    initInfo.NumFramesInFlight = 3;
    initInfo.RenderTargetFormat = static_cast<WGPUTextureFormat>(targetFormat);
    webgpuInitialized = ImGui_ImplWGPU_Init(&initInfo);
    if (!webgpuInitialized) {
        destroy();
        return false;
    }

    ImGuiIO& io = ImGui::GetIO();
    ImGui::StyleColorsDark();  // TODO: Customize theme

    float xScale, yScale;
    glfwGetWindowContentScale(window, &xScale, &yScale);
    currentScale = xScale;

    float perfectFontSize = fontSize * currentScale;

    const std::string fontPath = std::string(FONT_DIR) + "/Roboto-Black.ttf";
    ImFont* font =
        io.Fonts->AddFontFromFileTTF(fontPath.c_str(), perfectFontSize);
    if (font == nullptr) {
        std::cerr << "Can't load user's font. Use standard font" << std::endl;
        io.Fonts->AddFontDefault();
    }

    ImGui::GetStyle().ScaleAllSizes(currentScale * 2.f);
    ImGui::GetStyle().ButtonTextAlign = ImVec2(0.5f, 0.5f);

    return true;
}

void UILayer::destroy() {
    if (ImGui::GetCurrentContext()) {
        if (webgpuInitialized) ImGui_ImplWGPU_Shutdown();
        if (glfwInitialized) ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }
    webgpuInitialized = false;
    glfwInitialized = false;
    if (nfdInitialized) NFD_Quit();
    nfdInitialized = false;
}

void UILayer::beginFrame() {
    ImGui_ImplWGPU_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void UILayer::endFrame() { ImGui::EndFrame(); }

std::string UILayer::buildUI() {
    std::string selectedPath = "";

    ImGui::Begin("Viewer Controls");

    ImGui::Text("Options");
    ImGui::Separator();

    if (ImGui::Button("Open Model (.obj)")) {
        nfdchar_t* outPath = NULL;
        nfdfilteritem_t filterItem[1] = {{"Wavefront OBJ", "obj"}};

        nfdresult_t result = NFD_OpenDialog(&outPath, filterItem, 1, NULL);
        if (result == NFD_OKAY) {
            selectedPath = outPath;
            NFD_FreePath(outPath);
        } else if (result == NFD_CANCEL) {
            std::cout << "User canceled file dialog." << std::endl;
        } else {
            std::cerr << "NFD Error: " << NFD_GetError() << std::endl;
        }
    }

    ImGui::Spacing();
    ImGui::SeparatorText("Light Data");
    ImGui::DragFloat3("Light Direction", lightDirection, 0.01f);
    ImGui::DragFloat("Ambient Occlusion", &ambientOcclusion, 0.001f);
    ImGui::DragFloat("Intensity", &intensity, 0.001f);
    ImGui::DragFloat("Shininess", &shininess, 1.f, 0.f, 128.f);
    ImGui::DragFloat("Specular Strength", &specularStrength, 0.001f);
    ImGui::SeparatorText("Material & Environment");
    ImGui::ColorEdit3("Background Color", &bgColor.x);
    ImGui::ColorEdit3("Model Color", &modelColor.x);

    ImVec2 oldCursorPos = ImGui::GetCursorPos();
    const float buttonWidth = 120.0f;
    const float buttonHeight = 40.0f;

    const float availableWidth = ImGui::GetContentRegionAvail().x;

    if (availableWidth > buttonWidth) {
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + availableWidth -
                             buttonWidth);
    }

    if (ImGui::Button("Exit To Menu", ImVec2(buttonWidth, buttonHeight))) {
        exitMenu = true;
    }

    ImGui::End();

    return selectedPath;
}

std::string UILayer::buildStartupUI() {
    std::string selectedPath = "";

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);

    ImGuiWindowFlags window_flags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::Begin("MainMenu", nullptr, window_flags);

    ImVec2 menuBlockSize = ImVec2(800.0f, 250.0f);
    ImVec2 windowSize = ImGui::GetWindowSize();

    float startX = (windowSize.x - menuBlockSize.x) * 0.5f;
    float startY = (windowSize.y - menuBlockSize.y) * 0.5f;

    ImGui::SetCursorPos(ImVec2(startX, startY));

    ImGui::BeginChild("MenuContent", menuBlockSize, false,
                      ImGuiWindowFlags_NoBackground);

    std::string titleText = "Welcome to C2Lux 3D Viewer!";
    float titleWidth = ImGui::CalcTextSize(titleText.c_str()).x;
    ImGui::SetCursorPosX((menuBlockSize.x - titleWidth) * 0.5f);
    ImGui::Text("%s", titleText.c_str());

    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::Spacing();

    float buttonWidth = 250.0f;
    float buttonHeight = 50.0f;
    ImGui::SetCursorPosX((menuBlockSize.x - buttonWidth) * 0.5f);

    if (ImGui::Button("Open Model (.obj)", ImVec2(buttonWidth, buttonHeight))) {
        nfdchar_t* outPath = NULL;
        nfdfilteritem_t filterItem[1] = {{"Wavefront OBJ", "obj"}};

        nfdresult_t result = NFD_OpenDialog(&outPath, filterItem, 1, NULL);
        if (result == NFD_OKAY) {
            selectedPath = outPath;
            NFD_FreePath(outPath);
        }
    }

    ImGui::EndChild();

    ImGui::SetCursorPos(ImVec2{5.f, 5.f});

    static bool switcher = false;
    if (ImGui::Button("Settings", ImVec2{0.f, 0.f})) {
        switcher = !switcher;
    }

    if (switcher) {
        buildSettingBlock();
    }

    // Exit

    ImVec2 availableSpace = ImGui::GetContentRegionAvail();

    float posX = ImGui::GetCursorPosX() + availableSpace.x - buttonWidth;
    float posY = ImGui::GetCursorPosY() + availableSpace.y - buttonHeight;

    ImGui::SetCursorPos(ImVec2(posX, posY));

    if (ImGui::Button("Exit", ImVec2(buttonWidth, buttonHeight))) {
        exit = true;
    }

    ImGui::End();

    return selectedPath;
}

void UILayer::rebuildFontsAndScale() {
    bNeedsRebuild = false;
    ImGuiIO& io = ImGui::GetIO();

    ImGui_ImplWGPU_InvalidateDeviceObjects();

    io.Fonts->Clear();

    float perfectFontSize = fontSize * currentScale;

    const std::string fontPath = std::string(FONT_DIR) + "/Roboto-Black.ttf";
    ImFont* font =
        io.Fonts->AddFontFromFileTTF(fontPath.c_str(), perfectFontSize);
    if (font == nullptr) {
        io.Fonts->AddFontDefault();
    }

    ImGui_ImplWGPU_CreateDeviceObjects();

    ImGui::GetStyle() = ImGuiStyle();
    ImGui::StyleColorsDark();
    ImGui::GetStyle().ScaleAllSizes(currentScale);
}

void UILayer::buildSettingBlock() {
    ImVec2 settingMenuBlockSize = ImVec2{800.0f, 400.0f};
    ImGui::BeginChild("SettingsChild", settingMenuBlockSize, false,
                      ImGuiWindowFlags_NoBackground);
    ImGui::Text("UI Settings");

    unsigned int step = 1;
    unsigned int step_fast = 10;
    ImGui::InputScalar("Font size", ImGuiDataType_U32, &fontSize, &step,
                       &step_fast, "%u");

    ImGui::Spacing();
    ImGui::SliderFloat("Interface scale", &currentScale, 0.5f, 3.0f, "%.1f");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // ImGui::Checkbox("FullScreen", &fullScreenFlag);

    if (ImGui::Button("Confirm")) {
        bNeedsRebuild = true;
        fullScreen = fullScreenFlag;
    };

    ImGui::EndChild();
}

void UILayer::draw(wgpu::RenderPassEncoder renderPass) {
    ImGui::Render();
    ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), renderPass.Get());
}

}  // namespace C2Lux
