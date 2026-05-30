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

    bool initInstance();
    bool initAdapter();
    bool initDevice();
    bool initQueue();

    void initBuffers();
    void initRenderPipeline(wgpu::ShaderModule shader);
    void initSampler();
    void initBindGroups();

    bool setSurface(WGPUSurface c_surface);
    void setupSurfaceConfig(int width, int height);
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

    wgpu::RequiredLimits getRequiredLimits() const;
    void setDefault(wgpu::Limits& limits) const;
    void setDefault(wgpu::DepthStencilState& depthStencilState);
    void setDefault(wgpu::StencilFaceState& stencilFaceState);
    void setDefault(wgpu::BindGroupLayoutEntry &bindingLayout); 
    
    wgpu::Buffer vertexBuffer;
    wgpu::Buffer indexBuffer;
    uint32_t indexCount;
    
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

    wgpu::SurfaceConfiguration surfaceConfig;

};

} // namespace C2Lux
