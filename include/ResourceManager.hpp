#pragma once

#include "Mesh.hpp"
#include "webgpu/webgpu_cpp.h"

//std
#include <filesystem>


namespace C2Lux {

class ResourceManager {
public:
    struct VertexHasher {
        size_t operator()(const VertexAttributes& v) const {
            return std::hash<float>()(v.position.x) ^ (std::hash<float>()(v.position.y) << 1) ^ (std::hash<float>()(v.position.z) << 2);
        }
    };

    ResourceManager(std::filesystem::path sPath = SHADER_DIR, std::filesystem::path oPath = RESOURCE_DIR);

    Mesh loadObj(std::string_view objFileName);
    wgpu::ShaderModule loadShaderModule(std::string_view shaderName, wgpu::Device device);

private:
    std::filesystem::path shaderPath;
    std::filesystem::path objPath;
public:
    void setShaderPath(std::filesystem::path sPath) { shaderPath = sPath; }
    void setObjPath(std::filesystem::path oPath) { objPath = oPath; }

};



} // namespace C2Lux



