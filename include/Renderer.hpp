#pragma once

#include "webgpu/webgpu_cpp.h"
#include "glfw3webgpu.h"
#include "Mesh.hpp"
#include "Camera.hpp"


namespace C2Lux {

class UILayer;

class Renderer {
public:
    Renderer();
    // Persistent device callbacks refer to this renderer's address.
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer(Renderer&&) = delete;
    Renderer& operator=(Renderer&&) = delete;

    bool initInstance();
    bool initAdapter();
    bool initDevice();
    bool initQueue();

    void initBuffers();
    void initRenderPipeline(wgpu::ShaderModule shader);
    void initSampler();
    void initBindGroups();

    bool setSurface(WGPUSurface c_surface);
    bool setupSurfaceConfig(int width, int height);
    void resizeSwapchain(int width, int height);

    void updateMeshBuffers(const Mesh& model);

    wgpu::TextureView getNextSurfaceViewData();

    void draw(UILayer& ui, const Camera& camera);

    void drawJustUI(UILayer& ui);

private:
    wgpu::Instance instance;
    wgpu::Device device;
    wgpu::Adapter adapter;
    wgpu::Queue queue;
    wgpu::Surface surface;
    wgpu::TextureFormat surfaceFormat;

    bool failed = false;

    wgpu::Buffer vertexBuffer;
    size_t vertexBufferSize = 1024;

    wgpu::Buffer indexBuffer;
    size_t indexBufferSize = 1024;
    uint32_t indexCount = 0;
    
    wgpu::Buffer uniformBuffer;
    wgpu::Buffer lightBuffer;
    std::vector<wgpu::BindGroupLayout> bindGroupLayouts;
    wgpu::PipelineLayout pipelineLayout;
    std::vector<wgpu::BindGroup> bindGroups;

    wgpu::RenderPipeline renderPipeline;
   
    wgpu::Sampler sampler;

    wgpu::Texture depthTexture;
    wgpu::Texture msaaTexture;
    wgpu::TextureView msaaTextureView;
    wgpu::TextureView depthTextureView;
    wgpu::TextureDescriptor depthTextureDescriptor;
    wgpu::TextureViewDescriptor depthTextureViewDescriptor;
    wgpu::TextureFormat depthTextureFormat;

    

public:
    const wgpu::Instance& getInstance() const { return instance; }
    const wgpu::Device& getDevice() const { return device; }
    wgpu::TextureFormat getSurfaceFormat() const { return surfaceFormat; }

    // Process asynchronous callbacks even when the window is minimized.
    void processEvents() { if (instance) instance.ProcessEvents(); }
    bool hasFailed() const { return failed; }

    wgpu::SurfaceConfiguration surfaceConfig{};

};

} // namespace C2Lux
