#include "Renderer.hpp"
#include "CoreData.hpp"
#include "glm/ext.hpp"
#include "UILayer.hpp"
#include "c2m/c2m.hpp"

// std
#include <iostream>
#include <cmath>


namespace C2Lux {

Renderer::Renderer() : instance(nullptr), adapter(nullptr), device(nullptr), queue(nullptr), surface(nullptr) {}


bool Renderer::initInstance() {
    const char* toggleName = "enable_immediate_error_handling";
    wgpu::DawnTogglesDescriptor toggles{};
    toggles.enabledToggles = &toggleName;
    toggles.enabledToggleCount = 1;
    wgpu::InstanceDescriptor instanceDesc{};
    instanceDesc.nextInChain = &toggles;
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
        [this, &requestEnded](wgpu::RequestAdapterStatus status, wgpu::Adapter adapter_, const char* message) {
            if (status == wgpu::RequestAdapterStatus::Success) {
                adapter = adapter_;
            } else {
                std::cerr << "Could not get WebGPU adapter: ";
                if (message) std::cerr << message;
                std::cerr << std::endl;
            }
            requestEnded = true;
        }
    );

    while (!requestEnded) {
        instance.ProcessEvents();
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

    // TODO: Change to cpp-API
    deviceDesc.deviceLostCallbackInfo.callback = [](WGPUDevice const * device, WGPUDeviceLostReason reason, char const* message, void* /* pUserData */) {
        std::cerr << "Device lost: reason " << reason << " device: " << device;
        if (message) std::cerr << " (" << message << ")";
        std::cerr << std::endl;
    };
    wgpu::RequiredLimits requiredLimits = getRequiredLimits();
    deviceDesc.requiredLimits = &requiredLimits;
    
    bool requestEnded = false;

    adapter.RequestDevice(
        &deviceDesc,
        wgpu::CallbackMode::AllowProcessEvents,
        [this, &requestEnded](wgpu::RequestDeviceStatus status, wgpu::Device device_, char const* message) {
            if (status == wgpu::RequestDeviceStatus::Success) {
                device = device_;
            } else {
                std::cerr << "Could not get WebGPU device: ";
                if (message) std::cerr << message;
                std::cerr << std::endl;
            }
            requestEnded = true;
        }
    );

    while (!requestEnded) {
        instance.ProcessEvents();
    }

    // TODO: Change to cpp-API
    device.SetUncapturedErrorCallback(
        [](WGPUErrorType type, char const* message, void*) {
            std::cerr << "Uncaptured device error: type" << static_cast<uint32_t>(type);
            if (message) std::cerr << " (" << message << ")";
            std::cerr << std::endl;
        },
        nullptr
    );

    return device != nullptr;
}

bool Renderer::initQueue() {
    queue = device.GetQueue();
    
    queue.OnSubmittedWorkDone(
        wgpu::CallbackMode::AllowProcessEvents,
        [](wgpu::QueueWorkDoneStatus status) {
            std::cout << "Queued work finished with status: " << static_cast<uint32_t>(status) << std::endl;
        }
    );

    return queue != nullptr;
}

void Renderer::initBindGroups() {
    std::vector<wgpu::BindGroupLayoutEntry> bindingLayoutEntries(1);
    
    // Uniforms
    wgpu::BindGroupLayoutEntry& uniformBindingLayout = bindingLayoutEntries[0];
    setDefault(uniformBindingLayout);
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

void Renderer::setupSurfaceConfig(int width, int height) {
    surfaceConfig.nextInChain = nullptr;
    surfaceConfig.device = device;
    surfaceConfig.usage = wgpu::TextureUsage::RenderAttachment;
    surfaceConfig.presentMode = wgpu::PresentMode::Fifo;
    surfaceConfig.viewFormatCount = 0;
    surfaceConfig.viewFormats = nullptr;
    
    wgpu::SurfaceCapabilities capabilities;
    surface.GetCapabilities(adapter, &capabilities);
    if (capabilities.formatCount > 0) {
        surfaceConfig.format = capabilities.formats[0];
    } else {
        surfaceConfig.format = wgpu::TextureFormat::BGRA8Unorm;
    }
    if (capabilities.alphaModeCount > 0) {
        surfaceConfig.alphaMode = capabilities.alphaModes[0];
    } else {
        surfaceConfig.alphaMode = wgpu::CompositeAlphaMode::Auto;
    }
    surfaceFormat = surfaceConfig.format;
    surfaceConfig.width = width;
    surfaceConfig.height = height;
    surface.Configure(&surfaceConfig);
}

wgpu::RequiredLimits Renderer::getRequiredLimits() const {
    wgpu::SupportedLimits supportedLimits;
    supportedLimits.nextInChain = nullptr;
    adapter.GetLimits(&supportedLimits);
    
    wgpu::RequiredLimits requiredLimits = {};
    setDefault(requiredLimits.limits);

    requiredLimits.limits.maxVertexAttributes = 4;
    requiredLimits.limits.maxVertexBuffers = 3;
    requiredLimits.limits.maxBufferSize = 150000;
    requiredLimits.limits.maxBindGroups = 2;
    requiredLimits.limits.maxUniformBuffersPerShaderStage = 2;
    requiredLimits.limits.maxUniformBufferBindingSize = 2048;
    requiredLimits.limits.maxVertexBufferArrayStride = 20;
    requiredLimits.limits.minUniformBufferOffsetAlignment = supportedLimits.limits.minUniformBufferOffsetAlignment;
    requiredLimits.limits.minStorageBufferOffsetAlignment = supportedLimits.limits.minStorageBufferOffsetAlignment;
    requiredLimits.limits.maxInterStageShaderComponents = 16;
    requiredLimits.limits.maxTextureDimension1D = supportedLimits.limits.maxTextureDimension1D;
    requiredLimits.limits.maxTextureDimension2D = supportedLimits.limits.maxTextureDimension2D;
    requiredLimits.limits.maxTextureArrayLayers = 1;
    requiredLimits.limits.maxSampledTexturesPerShaderStage = 1;
    requiredLimits.limits.maxSamplersPerShaderStage = 1;

    // Allow textures up to 2K
    requiredLimits.limits.maxTextureDimension1D = 2048;
    requiredLimits.limits.maxTextureDimension2D = 2048;

    return requiredLimits;
}

void Renderer::setDefault(wgpu::Limits& limits) const {
    limits.maxTextureDimension1D = WGPU_LIMIT_U32_UNDEFINED;
	limits.maxTextureDimension2D = WGPU_LIMIT_U32_UNDEFINED;
	limits.maxTextureDimension3D = WGPU_LIMIT_U32_UNDEFINED;
	limits.maxTextureArrayLayers = WGPU_LIMIT_U32_UNDEFINED;
	limits.maxBindGroups = WGPU_LIMIT_U32_UNDEFINED;
	limits.maxBindGroupsPlusVertexBuffers = WGPU_LIMIT_U32_UNDEFINED;
	limits.maxBindingsPerBindGroup = WGPU_LIMIT_U32_UNDEFINED;
	limits.maxDynamicUniformBuffersPerPipelineLayout = WGPU_LIMIT_U32_UNDEFINED;
	limits.maxDynamicStorageBuffersPerPipelineLayout = WGPU_LIMIT_U32_UNDEFINED;
	limits.maxSampledTexturesPerShaderStage = WGPU_LIMIT_U32_UNDEFINED;
	limits.maxSamplersPerShaderStage = WGPU_LIMIT_U32_UNDEFINED;
	limits.maxStorageBuffersPerShaderStage = WGPU_LIMIT_U32_UNDEFINED;
	limits.maxStorageTexturesPerShaderStage = WGPU_LIMIT_U32_UNDEFINED;
	limits.maxUniformBuffersPerShaderStage = WGPU_LIMIT_U32_UNDEFINED;
	limits.maxUniformBufferBindingSize = WGPU_LIMIT_U64_UNDEFINED;
	limits.maxStorageBufferBindingSize = WGPU_LIMIT_U64_UNDEFINED;
	limits.minUniformBufferOffsetAlignment = WGPU_LIMIT_U32_UNDEFINED;
	limits.minStorageBufferOffsetAlignment = WGPU_LIMIT_U32_UNDEFINED;
	limits.maxVertexBuffers = WGPU_LIMIT_U32_UNDEFINED;
	limits.maxBufferSize = WGPU_LIMIT_U64_UNDEFINED;
	limits.maxVertexAttributes = WGPU_LIMIT_U32_UNDEFINED;
	limits.maxVertexBufferArrayStride = WGPU_LIMIT_U32_UNDEFINED;
	limits.maxInterStageShaderComponents = WGPU_LIMIT_U32_UNDEFINED;
	limits.maxInterStageShaderVariables = WGPU_LIMIT_U32_UNDEFINED;
	limits.maxColorAttachments = WGPU_LIMIT_U32_UNDEFINED;
	limits.maxColorAttachmentBytesPerSample = WGPU_LIMIT_U32_UNDEFINED;
	limits.maxComputeWorkgroupStorageSize = WGPU_LIMIT_U32_UNDEFINED;
	limits.maxComputeInvocationsPerWorkgroup = WGPU_LIMIT_U32_UNDEFINED;
	limits.maxComputeWorkgroupSizeX = WGPU_LIMIT_U32_UNDEFINED;
	limits.maxComputeWorkgroupSizeY = WGPU_LIMIT_U32_UNDEFINED;
	limits.maxComputeWorkgroupSizeZ = WGPU_LIMIT_U32_UNDEFINED;
	limits.maxComputeWorkgroupsPerDimension = WGPU_LIMIT_U32_UNDEFINED;
}

void Renderer::setDefault(wgpu::BindGroupLayoutEntry &bindingLayout) {
    bindingLayout.buffer.nextInChain = nullptr;
    bindingLayout.buffer.type = wgpu::BufferBindingType::Undefined;
    bindingLayout.buffer.hasDynamicOffset = false;

    bindingLayout.sampler.nextInChain = nullptr;
    bindingLayout.sampler.type = wgpu::SamplerBindingType::Undefined;

    bindingLayout.storageTexture.nextInChain = nullptr;
    bindingLayout.storageTexture.access = wgpu::StorageTextureAccess::Undefined;
    bindingLayout.storageTexture.format = wgpu::TextureFormat::Undefined;
    bindingLayout.storageTexture.viewDimension = wgpu::TextureViewDimension::Undefined;

    bindingLayout.texture.nextInChain = nullptr;
    bindingLayout.texture.multisampled = false;
    bindingLayout.texture.sampleType = wgpu::TextureSampleType::Undefined;
    bindingLayout.texture.viewDimension = wgpu::TextureViewDimension::Undefined;
}

void Renderer::draw(UILayer& ui, const Camera& camera) {
    
    device.Tick();

    Uniforms uniforms;
    uniforms.time = glfwGetTime();
    uniforms.modelMatrix = glm::mat4(1.0f);
    uniforms.viewMatrix = camera.getViewMatrix();
    camera.debug();
    float aspect = static_cast<float>(surfaceConfig.width) / static_cast<float>(surfaceConfig.height);
    uniforms.projectionMatrix = camera.getProjectionMatrix(aspect);
    uniforms.cameraPosition = camera.getPosition();
    uniforms.color = { ui.modelColor.x, ui.modelColor.y, ui.modelColor.z, ui.modelColor.w };


    LightData lightData;
    lightData.direction = { ui.lightDirection[0], ui.lightDirection[1], ui.lightDirection[2] };
    lightData.ambient = ui.ambientOcclusion;
    lightData.intensity = ui.intensity;
    lightData.shininess = ui.shininess;
    lightData.specularStrength = ui.specularStrength;

    queue.WriteBuffer(lightBuffer, 0, &lightData, sizeof(LightData));
    queue.WriteBuffer(uniformBuffer, 0, &uniforms, sizeof(Uniforms));

    wgpu::TextureView targetView = getNextSurfaceViewData();
    if (!targetView) {
        std::cout << "Can't get texture!" << std::endl;
        return;
    }

    wgpu::CommandEncoderDescriptor encoderDesc = {};
    encoderDesc.nextInChain = nullptr;
    encoderDesc.label = "Command encoder";
    wgpu::CommandEncoder encoder = device.CreateCommandEncoder(&encoderDesc);

    wgpu::RenderPassDescriptor renderPassDesc = {};
    renderPassDesc.nextInChain = nullptr;
    renderPassDesc.depthStencilAttachment = nullptr;
    renderPassDesc.timestampWrites = nullptr;

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
    pass1.SetVertexBuffer(0, vertexBuffer);
    pass1.SetIndexBuffer(indexBuffer, wgpu::IndexFormat::Uint32, 0, indexCount * sizeof(uint32_t));
    pass1.SetBindGroup(0, bindGroups[0], 0, nullptr);
    pass1.SetBindGroup(1, bindGroups[1], 0, nullptr);
    pass1.DrawIndexed(indexCount, 1, 0, 0, 0);
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
    device.Tick();
    wgpu::TextureView targetView = getNextSurfaceViewData();
    if (!targetView) {
        std::cout << "Can't get texture!" << std::endl;
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
    wgpu::SurfaceTexture surfaceTexture;
    surface.GetCurrentTexture(&surfaceTexture);

    if (surfaceTexture.status != wgpu::SurfaceGetCurrentTextureStatus::Success) {
        return nullptr;
    }

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
    if (vertexBuffer) vertexBuffer.Destroy();
    if (indexBuffer) indexBuffer.Destroy();

    wgpu::BufferDescriptor bufferDesc{};
    bufferDesc.nextInChain = nullptr;
    bufferDesc.size = model.getVertexData().size() * sizeof(VertexAttributes);
    bufferDesc.label = "Vertex buffer";
    bufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex;
    bufferDesc.mappedAtCreation = false;
    vertexBuffer = device.CreateBuffer(&bufferDesc);
    queue.WriteBuffer(vertexBuffer, 0, model.getVertexData().data(), bufferDesc.size);

    wgpu::BufferDescriptor indexBufferDesc{};
    indexBufferDesc.nextInChain = nullptr;
    indexBufferDesc.size = model.getIndexData().size() * sizeof(uint32_t);
    indexBufferDesc.label = "Index buffer";
    indexBufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Index;
    indexBufferDesc.mappedAtCreation = false;
    indexBuffer = device.CreateBuffer(&indexBufferDesc);
    queue.WriteBuffer(indexBuffer, 0, model.getIndexData().data(), indexBufferDesc.size);

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

    // NOTE: Request compile log from driver(Deprecated)
    shader.GetCompilationInfo([](WGPUCompilationInfoRequestStatus status, WGPUCompilationInfo const* compilationInfo, void* userdata) {
        if (compilationInfo->messageCount > 0) {
            std::cout << "--- WGSL Compilation Info ---" << std::endl;
            for (uint32_t i = 0; i < compilationInfo->messageCount; ++i) {
                const auto& msg = compilationInfo->messages[i];
                std::cout << "Line " << msg.lineNum << ":" << msg.linePos 
                          << " - " << msg.message << std::endl;
            }
            std::cout << "-----------------------------" << std::endl;
        }
    }, nullptr);

    // WARN: Add error if shaderModule == nullptr

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
    setDefault(depthStencilState);
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

void Renderer::setDefault(wgpu::DepthStencilState& depthStencilState) {
    depthStencilState.format = wgpu::TextureFormat::Undefined;
    depthStencilState.depthWriteEnabled = false;
    depthStencilState.depthCompare = wgpu::CompareFunction::Always;
    depthStencilState.stencilReadMask = 0xFFFFFFFF;
    depthStencilState.stencilWriteMask = 0xFFFFFFFF;
    depthStencilState.depthBias = 0;
    depthStencilState.depthBiasSlopeScale = 0;
    depthStencilState.depthBiasClamp = 0;
    setDefault(depthStencilState.stencilFront);
    setDefault(depthStencilState.stencilBack);
}

void Renderer::setDefault(wgpu::StencilFaceState& stencilFaceState) {    
    stencilFaceState.compare = wgpu::CompareFunction::Always;
    stencilFaceState.depthFailOp = wgpu::StencilOperation::Keep;
    stencilFaceState.failOp = wgpu::StencilOperation::Keep;
    stencilFaceState.passOp = wgpu::StencilOperation::Keep;
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
