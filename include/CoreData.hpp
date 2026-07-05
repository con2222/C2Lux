#pragma once

#include "glm/glm.hpp"

inline constexpr float PI = 3.14159265358979323846;

struct Uniforms {
    glm::mat4 modelMatrix;
    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;
    glm::vec4 color;
    glm::vec3 cameraPosition;
    float time;
};

struct LightData {
    glm::vec3 direction;
    float ambient;
    float intensity;
    float shininess;
    float specularStrength;
    float _pad;
};

static_assert(sizeof(Uniforms) % 16 == 0);
static_assert(sizeof(LightData) % 16 == 0);