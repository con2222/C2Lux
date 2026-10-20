#include "Renderer.hpp"
#include "CoreData.hpp"
#include "glm/ext.hpp"
#include "UILayer.hpp"
#include "c2m/c2m.hpp"

// std
#include <iostream>
#include <cmath>
#include <chrono>
#include <string_view>
#include <thread>


namespace C2Lux {

Renderer::Renderer() = default;


bool Renderer::initInstance() {
    wgpu::InstanceDescriptor instanceDesc{};
    instance = wgpu::CreateInstance(&instanceDesc);
    if (!instance) {
        std::cerr << "Could not intialize WebGPU!" << std::endl;
        return false;
    }
    return true;
}

bool Renderer::setSurface(WGPUSurface c_surface) {
    if (!c_surface) {
        std::cerr << "Could not set surface!" << std::endl;
        return false;
    }
    surface = wgpu::Surface::Acquire(c_surface);
    return true;
}

bool Renderer::initAdapter() {
    wgpu::RequestAdapterOptions adapterOpts{};
    adapterOpts.nextInChain = nullptr;
    adapterOpts.compatibleSurface = surface;
    
    bool requestEnded = false;

    instance.RequestAdapter(
        &adapterOpts,
        wgpu::CallbackMode::AllowProcessEvents,
        [this, &requestEnded](wgpu::RequestAdapterStatus status, wgpu::Adapter adapter_, wgpu::StringView message) {
            if (status == wgpu::RequestAdapterStatus::Success) {
                adapter = adapter_;
                wgpu::AdapterInfo info{};
                if (adapter.GetInfo(&info) == wgpu::Status::Success) {
                    std::cout << "WebGPU adapter: " << std::string_view(info.device) << std::endl;
                }
            } else {
                std::cerr << "Could not get WebGPU adapter: ";
                std::cerr << std::string_view(message);
                std::cerr << std::endl;
            }
            requestEnded = true;
        }
    );

    while (!requestEnded) {
        instance.ProcessEvents();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    return adapter != nullptr;
}

bool Renderer::initDevice() {
    wgpu::DeviceDescriptor deviceDesc{};
    deviceDesc.nextInChain = nullptr;
    deviceDesc.label = "My device";
    deviceDesc.requiredFeatureCount = 0;
    deviceDesc.requiredLimits = nullptr;
    deviceDesc.defaultQueue.nextInChain = nullptr;
    deviceDesc.defaultQueue.label = "The default queue";

    // Register callbacks on the descriptor before requesting the device.
    deviceDesc.SetDeviceLostCallback(
        wgpu::CallbackMode::AllowProcessEvents,
        [this](const wgpu::Device&, wgpu::DeviceLostReason reason, wgpu::StringView message) {
            if (reason == wgpu::DeviceLostReason::Destroyed ||
                reason == wgpu::DeviceLostReason::CallbackCancelled) return;
            failed = true;
            std::cerr << "Device lost (" << static_cast<uint32_t>(reason) << "): "
                      << std::string_view(message) << std::endl;
        }
    );
    deviceDesc.SetUncapturedErrorCallback(
        [](const wgpu::Device&, wgpu::ErrorType type, wgpu::StringView message) {
            std::cerr << "WebGPU error (" << static_cast<uint32_t>(type) << "): "
                      << std::string_view(message) << std::endl;
        }
    );
    // WebGPU's default limits cover the current viewer. Do not cap meshes at
    // 150 KB or render targets at 2048 pixels as the old manual limits did.

    bool requestEnded = false;

    adapter.RequestDevice(
        &deviceDesc,
        wgpu::CallbackMode::AllowProcessEvents,
        [this, &requestEnded](wgpu::RequestDeviceStatus status, wgpu::Device device_, wgpu::StringView message) {
            if (status == wgpu::RequestDeviceStatus::Success) {
                device = device_;
            } else {
                std::cerr << "Could not get WebGPU device: ";
                std::cerr << std::string_view(message);
                std::cerr << std::endl;
            }
            requestEnded = true;
        }
    );

    while (!requestEnded) {
        instance.ProcessEvents();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    return device != nullptr;
}

bool Renderer::initQueue() {
    queue = device.GetQueue();
    
    return queue != nullptr;
}

void Renderer::initBindGroups() {
    std::vector<wgpu::BindGroupLayoutEntry> bindingLayoutEntries(1);
    
    // Uniforms
    wgpu::BindGroupLayoutEntry& uniformBindingLayout = bindingLayoutEntries[0];
    uniformBindingLayout.binding = 0;
    uniformBindingLayout.visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
    uniformBindingLayout.buffer.type = wgpu::BufferBindingType::Uniform;
    uniformBindingLayout.buffer.minBindingSize = sizeof(Uniforms);
    uniformBindingLayout.buffer.hasDynamicOffset = false; // NOTE: Need change in future

    // Light
    wgpu::BindGroupLayoutEntry lightBindingLayout = {};
    lightBindingLayout.binding = 0;
    lightBindingLayout.visibility = wgpu::ShaderStage::Fragment;
    lightBindingLayout.buffer.type = wgpu::BufferBindingType::Uniform;
    lightBindingLayout.buffer.minBindingSize = sizeof(LightData);
    lightBindingLayout.buffer.hasDynamicOffset = false;

    // Uniforms
    wgpu::BindGroupLayoutDescriptor bindGroupLayoutDescriptor = {};
    bindGroupLayoutDescriptor.nextInChain = nullptr;
    bindGroupLayoutDescriptor.entryCount = static_cast<uint32_t>(bindingLayoutEntries.size());
    bindGroupLayoutDescriptor.entries = bindingLayoutEntries.data();
    bindGroupLayouts.push_back(device.CreateBindGroupLayout(&bindGroupLayoutDescriptor));

    // Light
    wgpu::BindGroupLayoutDescriptor lightbindGroupLayoutDescriptor = {};
    lightbindGroupLayoutDescriptor.nextInChain = nullptr;
    lightbindGroupLayoutDescriptor.entryCount = 1;
    lightbindGroupLayoutDescriptor.entries = &lightBindingLayout;
    bindGroupLayouts.push_back(device.CreateBindGroupLayout(&lightbindGroupLayoutDescriptor));

    wgpu::PipelineLayoutDescriptor layoutDescriptor{};
    layoutDescriptor.nextInChain = nullptr;
    layoutDescriptor.bindGroupLayoutCount = 2;
    layoutDescriptor.bindGroupLayouts = bindGroupLayouts.data();
    pipelineLayout = device.CreatePipelineLayout(&layoutDescriptor);

    std::vector<wgpu::BindGroupEntry> bindings(2);

    bindings[0].binding = 0; // The index of the binding (the entries in bindGroupDesc can be in any order)
    bindings[0].buffer = uniformBuffer; // The buffer it is actually bound to
    bindings[0].offset = 0; // We can specify an offset within the buffer, so that a single buffer can hold, multiple uniform blocks.
    bindings[0].size = sizeof(Uniforms);  // And we specify again the size of the buffer.

    bindings[1].binding = 0;
    bindings[1].buffer = lightBuffer;
    bindings[1].offset = 0;
    bindings[1].size = sizeof(LightData);

    wgpu::BindGroupDescriptor bindGroupDescriptor{};
    bindGroupDescriptor.nextInChain = nullptr;
    bindGroupDescriptor.layout = bindGroupLayouts[0];
    bindGroupDescriptor.entryCount = (uint32_t)bindings.size() - 1;
    bindGroupDescriptor.entries = &bindings[0];
    bindGroups.push_back(device.CreateBindGroup(&bindGroupDescriptor));

    bindGroupDescriptor.layout = bindGroupLayouts[1];
    bindGroupDescriptor.entryCount = 1;
    bindGroupDescriptor.entries = &bindings[1];
    bindGroups.push_back(device.CreateBindGroup(&bindGroupDescriptor));
}

bool Renderer::setupSurfaceConfig(int width, int height) {
    if (width <= 0 || height <= 0) return false;
    surfaceConfig.nextInChain = nullptr;
    surfaceConfig.device = device;
    surfaceConfig.usage = wgpu::TextureUsage::RenderAttachment;
    surfaceConfig.presentMode = wgpu::PresentMode::Fifo;
    surfaceConfig.viewFormatCount = 0;
    surfaceConfig.viewFormats = nullptr;
    
    wgpu::SurfaceCapabilities capabilities{};
    if (surface.GetCapabilities(adapter, &capabilities) != wgpu::Status::Success ||
        capabilities.formatCount == 0 || capabilities.alphaModeCount == 0) {
        std::cerr << "Could not query surface capabilities." << std::endl;
        return false;
    }
    surfaceConfig.format = capabilities.formats[0];
    surfaceConfig.alphaMode = capabilities.alphaModes[0];
    surfaceFormat = surfaceConfig.format;
    surfaceConfig.width = static_cast<uint32_t>(width);
    surfaceConfig.height = static_cast<uint32_t>(height);
    surface.Configure(&surfaceConfig);
    return true;
}

void Renderer::draw(UILayer& ui, const Camera& camera) {
    

    Uniforms uniforms{};
    uniforms.time = static_cast<float>(glfwGetTime());
    uniforms.modelMatrix = glm::mat4(1.0f);
    uniforms.viewMatrix = camera.getViewMatrix();
    float aspect = static_cast<float>(surfaceConfig.width) / static_cast<float>(surfaceConfig.height);
    uniforms.projectionMatrix = camera.getProjectionMatrix(aspect);
    uniforms.cameraPosition = camera.getPosition();
    uniforms.color = { ui.modelColor.x, ui.modelColor.y, ui.modelColor.z, ui.modelColor.w };


    LightData lightData{};
    lightData.direction = { ui.lightDirection[0], ui.lightDirection[1], ui.lightDirection[2] };
    lightData.ambient = ui.ambientOcclusion;
    lightData.intensity = ui.intensity;
    lightData.shininess = ui.shininess;
    lightData.specularStrength = ui.specularStrength;

    queue.WriteBuffer(lightBuffer, 0, &lightData, sizeof(LightData));
    queue.WriteBuffer(uniformBuffer, 0, &uniforms, sizeof(Uniforms));

    wgpu::TextureView targetView = getNextSurfaceViewData();
    if (!targetView) {
        ui.endFrame();
        return;
    }

    wgpu::CommandEncoderDescriptor encoderDesc = {};
    encoderDesc.nextInChain = nullptr;
    encoderDesc.label = "Command encoder";
    wgpu::CommandEncoder encoder = device.CreateCommandEncoder(&encoderDesc);

    wgpu::RenderPassColorAttachment colorAttachment = {};
    colorAttachment.view = msaaTextureView;
    colorAttachment.resolveTarget = targetView;
    colorAttachment.loadOp = wgpu::LoadOp::Clear;
    colorAttachment.storeOp = wgpu::StoreOp::Discard;
    colorAttachment.clearValue = wgpu::Color{ ui.bgColor.x, ui.bgColor.y, ui.bgColor.z, ui.bgColor.w };

    wgpu::RenderPassDepthStencilAttachment depthAttachment{};
    depthAttachment.view = depthTextureView;
    depthAttachment.depthClearValue = 1.0f;
    depthAttachment.depthLoadOp = wgpu::LoadOp::Clear;
    depthAttachment.depthStoreOp = wgpu::StoreOp::Discard;
    depthAttachment.depthReadOnly = false;
    depthAttachment.stencilClearValue = 0;
    depthAttachment.stencilLoadOp = wgpu::LoadOp::Undefined;
    depthAttachment.stencilStoreOp = wgpu::StoreOp::Undefined;
    depthAttachment.stencilReadOnly = true;

    wgpu::RenderPassDescriptor pass1Desc = {};
    pass1Desc.colorAttachmentCount = 1;
    pass1Desc.colorAttachments = &colorAttachment;
    pass1Desc.depthStencilAttachment = &depthAttachment;

    wgpu::RenderPassEncoder pass1 = encoder.BeginRenderPass(&pass1Desc);
    pass1.SetPipeline(renderPipeline);
    if (indexCount > 0 && vertexBuffer && indexBuffer) {
        pass1.SetVertexBuffer(0, vertexBuffer);
        pass1.SetIndexBuffer(indexBuffer, wgpu::IndexFormat::Uint32, 0,
                             static_cast<uint64_t>(indexCount) * sizeof(uint32_t));
        pass1.SetBindGroup(0, bindGroups[0]);
        pass1.SetBindGroup(1, bindGroups[1]);
        pass1.DrawIndexed(indexCount);
    }
    pass1.End();

    /* Render imGui UI */
    wgpu::RenderPassColorAttachment uiAttachment = {};
    uiAttachment.view = targetView;
    uiAttachment.resolveTarget = nullptr;
    uiAttachment.loadOp = wgpu::LoadOp::Load;
    uiAttachment.storeOp = wgpu::StoreOp::Store;

    wgpu::RenderPassDescriptor pass2Desc = {};
    pass2Desc.colorAttachmentCount = 1;
    pass2Desc.colorAttachments = &uiAttachment;
    pass2Desc.depthStencilAttachment = nullptr;

    wgpu::RenderPassEncoder pass2 = encoder.BeginRenderPass(&pass2Desc);
    ui.draw(pass2);
    pass2.End();

    wgpu::CommandBufferDescriptor cmdBufferDesc = {};
    cmdBufferDesc.nextInChain = nullptr;
    cmdBufferDesc.label = "Command buffer";
    wgpu::CommandBuffer command = encoder.Finish(&cmdBufferDesc);

    //std::cout << "Submitting command..." << std::endl;
    queue.Submit(1, &command);

    targetView = nullptr;

    surface.Present();


    //std::cout << "Command submitted." << std::endl;
}

void Renderer::drawJustUI(UILayer& ui)
{
    wgpu::TextureView targetView = getNextSurfaceViewData();
    if (!targetView) {
        ui.endFrame();
        return;
    }
    
    wgpu::CommandEncoderDescriptor encoderDesc = {};
    encoderDesc.nextInChain = nullptr;
    encoderDesc.label = "Command encoder";
    wgpu::CommandEncoder encoder = device.CreateCommandEncoder(&encoderDesc);

    wgpu::RenderPassColorAttachment uiAttachment = {};
    uiAttachment.view = targetView;
    uiAttachment.resolveTarget = nullptr;
    uiAttachment.loadOp = wgpu::LoadOp::Clear;
    uiAttachment.storeOp = wgpu::StoreOp::Store;

    wgpu::RenderPassDescriptor uiPassDesc = {};
    uiPassDesc.colorAttachmentCount = 1;
    uiPassDesc.colorAttachments = &uiAttachment;
    uiPassDesc.depthStencilAttachment = nullptr;

    wgpu::RenderPassEncoder uiPass = encoder.BeginRenderPass(&uiPassDesc);
    ui.draw(uiPass);
    uiPass.End();

    wgpu::CommandBufferDescriptor cmdBufferDesc = {};
    cmdBufferDesc.nextInChain = nullptr;
    cmdBufferDesc.label = "Command buffer";
    wgpu::CommandBuffer command = encoder.Finish(&cmdBufferDesc);

    //std::cout << "Submitting command..." << std::endl;
    queue.Submit(1, &command);

    targetView = nullptr;

    surface.Present();


    //std::cout << "Command submitted." << std::endl;
}

wgpu::TextureView Renderer::getNextSurfaceViewData() {
    wgpu::SurfaceTexture surfaceTexture{};
    surface.GetCurrentTexture(&surfaceTexture);

    switch (surfaceTexture.status) {
    case wgpu::SurfaceGetCurrentTextureStatus::SuccessOptimal:
    case wgpu::SurfaceGetCurrentTextureStatus::SuccessSuboptimal:
        break;
    case wgpu::SurfaceGetCurrentTextureStatus::Timeout:
        return nullptr; // A temporary failure: skip this frame.
    case wgpu::SurfaceGetCurrentTextureStatus::Outdated:
    case wgpu::SurfaceGetCurrentTextureStatus::Lost:
        // Reconfigure before acquiring a texture on the next frame.
        surface.Configure(&surfaceConfig);
        return nullptr;
    default:
        failed = true;
        std::cerr << "Could not acquire surface texture (status "
                  << static_cast<uint32_t>(surfaceTexture.status) << ")." << std::endl;
        return nullptr;
    }
    if (!surfaceTexture.texture) return nullptr;

    wgpu::TextureViewDescriptor viewDescriptor;
    viewDescriptor.nextInChain = nullptr;
    viewDescriptor.label = "Surface texture view";
    viewDescriptor.format = surfaceTexture.texture.GetFormat();
    viewDescriptor.dimension = wgpu::TextureViewDimension::e2D;
    viewDescriptor.baseMipLevel = 0;
    viewDescriptor.mipLevelCount = 1;
    viewDescriptor.baseArrayLayer = 0;
    viewDescriptor.arrayLayerCount = 1;
    viewDescriptor.aspect = wgpu::TextureAspect::All;

    return surfaceTexture.texture.CreateView(&viewDescriptor);
}

void Renderer::resizeSwapchain(int width, int height) {
    if (width <= 0 || height <= 0) return;

    msaaTextureView = nullptr;
    depthTextureView = nullptr;
    msaaTexture = nullptr;
    depthTexture = nullptr;

    surfaceConfig.width = static_cast<uint32_t>(width);
    surfaceConfig.height = static_cast<uint32_t>(height);
    surface.Configure(&surfaceConfig);

    wgpu::TextureDescriptor msaaDesc{};
    msaaDesc.size = { surfaceConfig.width, surfaceConfig.height, 1 };
    msaaDesc.sampleCount = 4;
    msaaDesc.format = surfaceFormat;
    msaaDesc.usage = wgpu::TextureUsage::RenderAttachment;
    msaaTexture = device.CreateTexture(&msaaDesc);
    msaaTextureView = msaaTexture.CreateView();

    depthTextureDescriptor.size = { surfaceConfig.width, surfaceConfig.height, 1 };
    depthTextureDescriptor.sampleCount = 4;
    depthTexture = device.CreateTexture(&depthTextureDescriptor);
    depthTextureView = depthTexture.CreateView();
}

void Renderer::updateMeshBuffers(const Mesh& model) {
    if (!vertexBuffer || model.getVertexData().size() >= vertexBufferSize) {
        if (model.getVertexData().size() >= vertexBufferSize) {
            vertexBufferSize = model.getVertexData().size() + model.getVertexData().size() / 2;
        }
        if (vertexBuffer) {
            vertexBuffer.Destroy();
        }

        wgpu::BufferDescriptor bufferDesc{};
        bufferDesc.nextInChain = nullptr;
        bufferDesc.size = vertexBufferSize * sizeof(VertexAttributes);
        bufferDesc.label = "Vertex buffer";
        bufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex;
        bufferDesc.mappedAtCreation = false;
        vertexBuffer = device.CreateBuffer(&bufferDesc);
    }

    if (!indexBuffer || model.getIndexData().size() >= indexBufferSize) {
        if (model.getIndexData().size() >= indexBufferSize) {
            indexBufferSize = model.getIndexData().size() + model.getIndexData().size() / 2;
        }
        if (indexBuffer) {
            indexBuffer.Destroy();
        }

        wgpu::BufferDescriptor indexBufferDesc{};
        indexBufferDesc.nextInChain = nullptr;
        indexBufferDesc.size = indexBufferSize * sizeof(uint32_t); // default indexBufferSize
        indexBufferDesc.label = "Index buffer";
        indexBufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Index;
        indexBufferDesc.mappedAtCreation = false;
        indexBuffer = device.CreateBuffer(&indexBufferDesc);
    }

    queue.WriteBuffer(vertexBuffer, 0, model.getVertexData().data(), model.getVertexData().size() * sizeof(VertexAttributes));

    queue.WriteBuffer(indexBuffer, 0, model.getIndexData().data(), model.getIndexData().size() * sizeof(uint32_t));
    indexCount = static_cast<uint32_t>(model.getIndexData().size());
}

void Renderer::initBuffers() {   
    wgpu::BufferDescriptor uniformBufferDesc{};
    uniformBufferDesc.nextInChain = nullptr;
    uniformBufferDesc.size = sizeof(Uniforms);
    uniformBufferDesc.label = "Uniforms buffer";
    uniformBufferDesc.mappedAtCreation = false;
    uniformBufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform;
    uniformBuffer = device.CreateBuffer(&uniformBufferDesc);

    wgpu::BufferDescriptor lightBufferDesc{};
    lightBufferDesc.nextInChain = nullptr;
    lightBufferDesc.mappedAtCreation = false;
    lightBufferDesc.label = "Light buffer";
    lightBufferDesc.size = c2m::ceilToMultipleOf16(sizeof(LightData));
    lightBufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform;
    lightBuffer = device.CreateBuffer(&lightBufferDesc);
}

void Renderer::initRenderPipeline(wgpu::ShaderModule shader) {

    shader.GetCompilationInfo(wgpu::CallbackMode::AllowProcessEvents,
        [](wgpu::CompilationInfoRequestStatus status, const wgpu::CompilationInfo* compilationInfo) {
            if (status != wgpu::CompilationInfoRequestStatus::Success || !compilationInfo) {
                std::cerr << "Could not retrieve WGSL compilation info." << std::endl;
                return;
            }
            if (compilationInfo->messageCount > 0) {
                std::cout << "--- WGSL Compilation Info ---" << std::endl;
                for (size_t i = 0; i < compilationInfo->messageCount; ++i) {
                    const auto& msg = compilationInfo->messages[i];
                    std::cout << "Line " << msg.lineNum << ":" << msg.linePos
                              << " - " << std::string_view(msg.message) << std::endl;
                }
                std::cout << "-----------------------------" << std::endl;
            }
        });

    wgpu::RenderPipelineDescriptor pipelineDesc{};
    pipelineDesc.nextInChain = nullptr;
    pipelineDesc.layout = pipelineLayout;

    wgpu::VertexBufferLayout vertexBufferLayout{};

    std::vector<wgpu::VertexAttribute> vertexAttributes(4);

    vertexAttributes[0].shaderLocation = 0;
    vertexAttributes[0].format = wgpu::VertexFormat::Float32x3;
    vertexAttributes[0].offset = offsetof(VertexAttributes, position);

    vertexAttributes[1].shaderLocation = 1;
    vertexAttributes[1].format = wgpu::VertexFormat::Float32x3;
    vertexAttributes[1].offset = offsetof(VertexAttributes, normal);

    vertexAttributes[2].shaderLocation = 2;
    vertexAttributes[2].format = wgpu::VertexFormat::Float32x3;
    vertexAttributes[2].offset = offsetof(VertexAttributes, color);

    vertexAttributes[3].shaderLocation = 3;
    vertexAttributes[3].format = wgpu::VertexFormat::Float32x2;
    vertexAttributes[3].offset = offsetof(VertexAttributes, uv);

    vertexBufferLayout.arrayStride = sizeof(VertexAttributes);
    vertexBufferLayout.attributeCount = static_cast<uint32_t>(vertexAttributes.size());
    vertexBufferLayout.attributes = vertexAttributes.data();
    vertexBufferLayout.stepMode = wgpu::VertexStepMode::Vertex;

    pipelineDesc.vertex.bufferCount = 1;
    pipelineDesc.vertex.buffers = &vertexBufferLayout;
    pipelineDesc.vertex.module = shader;
    pipelineDesc.vertex.entryPoint = "vs_main";
    pipelineDesc.vertex.constantCount = 0;
    pipelineDesc.vertex.constants = nullptr;

    pipelineDesc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
    pipelineDesc.primitive.stripIndexFormat = wgpu::IndexFormat::Undefined;
    pipelineDesc.primitive.frontFace = wgpu::FrontFace::CCW;
    pipelineDesc.primitive.cullMode = wgpu::CullMode::None;

    wgpu::FragmentState fragmentState{};
    fragmentState.module = shader;
    fragmentState.entryPoint = "fs_main";
    fragmentState.constantCount = 0;
    fragmentState.constants = nullptr;

    wgpu::BlendState blendState{};
    blendState.color.srcFactor = wgpu::BlendFactor::SrcAlpha;
    blendState.color.dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha;
    blendState.color.operation= wgpu::BlendOperation::Add;
    // Similar a = srcFactor * srcA [operation] dstFactor * destA
    blendState.alpha.srcFactor = wgpu::BlendFactor::Zero;
    blendState.alpha.dstFactor = wgpu::BlendFactor::One;
    blendState.alpha.operation = wgpu::BlendOperation::Add;
    wgpu::ColorTargetState colorTarget{};
    colorTarget.format = surfaceFormat;
    colorTarget.blend = &blendState;
    colorTarget.writeMask = wgpu::ColorWriteMask::All;
    
    fragmentState.targetCount = 1;
    fragmentState.targets = &colorTarget;
    
    pipelineDesc.fragment = &fragmentState;

    wgpu::DepthStencilState depthStencilState{};
    depthStencilState.depthCompare = wgpu::CompareFunction::Less; // compare two z-buffer values
    depthStencilState.depthWriteEnabled = true; // overwrite value = a if a less than b. Can deactivate when rendering user interface for example
    depthTextureFormat = wgpu::TextureFormat::Depth24Plus;
    depthStencilState.format = depthTextureFormat;
    depthStencilState.stencilReadMask = 0;
    depthStencilState.stencilWriteMask = 0;


    depthTextureDescriptor = {};
    depthTextureDescriptor.dimension = wgpu::TextureDimension::e2D;
    depthTextureDescriptor.format = depthTextureFormat;
    depthTextureDescriptor.mipLevelCount = 1;
    depthTextureDescriptor.sampleCount = 4;
    depthTextureDescriptor.size = {surfaceConfig.width, surfaceConfig.height, 1};
    depthTextureDescriptor.usage = wgpu::TextureUsage::RenderAttachment;
    depthTextureDescriptor.viewFormatCount = 1;
    depthTextureDescriptor.viewFormats = &depthTextureFormat;
    depthTexture = device.CreateTexture(&depthTextureDescriptor);

    wgpu::TextureDescriptor msaaDesc{};
    msaaDesc.size = { surfaceConfig.width, surfaceConfig.height, 1 };
    msaaDesc.sampleCount = 4;
    msaaDesc.format = surfaceFormat;
    msaaDesc.usage = wgpu::TextureUsage::RenderAttachment;
    msaaTexture = device.CreateTexture(&msaaDesc);
    msaaTextureView = msaaTexture.CreateView();

    depthTextureView = depthTexture.CreateView(&depthTextureViewDescriptor);

    pipelineDesc.depthStencil = &depthStencilState;

    pipelineDesc.multisample.count = 4;
    pipelineDesc.multisample.mask = ~0u;
    pipelineDesc.multisample.alphaToCoverageEnabled = false;

    renderPipeline = device.CreateRenderPipeline(&pipelineDesc);
}

void Renderer::initSampler() {
    wgpu::SamplerDescriptor samplerDescriptor{};
    samplerDescriptor.addressModeU = wgpu::AddressMode::Repeat;
    samplerDescriptor.addressModeV = wgpu::AddressMode::Repeat;
    samplerDescriptor.addressModeW = wgpu::AddressMode::ClampToEdge;
    samplerDescriptor.magFilter = wgpu::FilterMode::Linear;
    samplerDescriptor.minFilter = wgpu::FilterMode::Linear;
    samplerDescriptor.mipmapFilter = wgpu::MipmapFilterMode::Linear;
    samplerDescriptor.lodMinClamp = 0.0f;
    samplerDescriptor.lodMaxClamp = 8.0f;
    samplerDescriptor.compare = wgpu::CompareFunction::Undefined;
    samplerDescriptor.maxAnisotropy = 16;
    sampler = device.CreateSampler(&samplerDescriptor);
}

} // namespace C2Lux
