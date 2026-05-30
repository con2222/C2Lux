#include "Camera.hpp"
#include "glm/ext.hpp"

// std
#include <iostream>
#include <algorithm>


namespace C2Lux {

Camera::Camera() :
    position(0.0f, 0.0f, 5.0f),
    worldUp(0.0f, 1.0f, 0.0f),
    yaw(-90.0f),

    pitch(0.0f),
    movementSpeed(2.5f),
    mouseSensitivity(0.1f),
    fov(45.0f)
{
    update();
}

void Camera::update() {
    glm::vec3 newFront;
    newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    newFront.y = sin(glm::radians(pitch));
    newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(newFront);

    right = glm::normalize(glm::cross(front, worldUp));
    up = glm::normalize(glm::cross(right, front));
}

void Camera::rotate(float deltaX, float deltaY) {
    yaw += deltaX * mouseSensitivity;
    pitch -= deltaY * mouseSensitivity;

    pitch = std::clamp(pitch, -89.0f, 89.0f);
    
    update();
}

void Camera::zoom(float delta) {
    fov -= delta * 2.0f;
    fov = std::clamp(fov, 1.0f, 45.0f);
}

void Camera::moveForward(float deltaTime) { position += front * movementSpeed * deltaTime; }
void Camera::moveBackward(float deltaTime) { position -= front * movementSpeed * deltaTime; }
void Camera::moveLeft(float deltaTime) { position -= right * movementSpeed * deltaTime; }
void Camera::moveRight(float deltaTime) { position += right * movementSpeed * deltaTime; }

glm::mat4 Camera::getViewMatrix() const {
    return glm::lookAt(position, position + front, up);
}

glm::mat4 Camera::getProjectionMatrix(float aspect) const {
    return glm::perspective(glm::radians(fov), aspect, 0.01f, 100.0f);
}

void Camera::debug() const {
    std::cout << "\r"
        << "Pos: [" << position.x << ", " << position.y << ", " << position.z << "] | "
        << "Front: [" << front.x << ", " << front.y << ", " << front.z << "] | "
        << "Yaw: " << yaw << "  Pitch: " << pitch
        << std::string(10, ' ')
        << std::flush;
}

} // namespace C2Lux