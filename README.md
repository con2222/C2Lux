[Читать на русском](README.ru.md) | [Read in English](README.md)

# C2Lux

**C2Lux** is a lightweight, cross-platform 3D graphics sandbox and interactive shader laboratory built from scratch using **C++17** and **WebGPU (Google Dawn)**. 

The project is designed as an educational playground to experiment with real-time lighting algorithms, custom shader architectures, and modern low-level graphics techniques.

> **Project Status: Early Development (v0.0.1-alpha)** > This project is currently an active work-in-progress. The architecture is evolving rapidly, and new features are being added frequently.

---

## Current Features

- **3D Model Loading:** Real-time parsing and loading of Wavefront `.obj` meshes with integrated support for vertex attributes (positions, normals, baked diffuse colors, and UV coordinates).
- **Custom Blinn-Phong Shading:** A pure WGSL implementation of the classical Blinn-Phong reflection model, combining ambient, diffuse, and specular components.
- **Live Parameter Tuning:** Fully interactive control panel powered by **Dear ImGui** to tweak light direction, ambient occlusion factor, light intensity, material shininess, and specular strength in real time.

---

## Roadmap & Future Plans

The ultimate goal of **C2Lux** is to become a highly flexible environment for graphics testing and rendering experiments. The upcoming feature pipeline includes:

### 1. Broadening Lighting Models
- Moving beyond Blinn-Phong to implement **Physically Based Rendering (PBR)** using the Cook-Torrance microfacet specular BRDF.
- Adding multiple light source support (Point lights, Spotlights) with attenuation models.

### 2. Shader Switching & Extensibility Standard
- **Live Shader Hot-Reloading:** Ability to switch between different graphics pipelines and WGSL files dynamically during runtime to visualize differences instantly.
- **C2Lux Shader Template Standard:** Creating a strict structure/specification for user-written shaders. By adhering to a template that maps to the engine's internal layouts, users will be able to write their own custom shaders and load them straight into the application.

### 3. Shadows & Environment Simulation
- **Shadow Mapping:** Implementing depth-pass rendering to achieve realistic real-time shadows.
- **Obstacle Spawning:** Adding the capability to place simple procedural shapes (walls, blocks, primitives) within the scene to serve as light-blocking geometry.

### 4. Texturing & Materials
- Transitioning from vertex-color baking to full **Texture Mapping** (Albedo, Normal, and Roughness/Metallic maps) parsed from `.mtl` files.

### 5. Custom Mathematics Library
- **Replacing GLM:** Developing a proprietary, lightweight 3D mathematics library from scratch to replace the GLM dependency.

### 6. UI/UX Modernization
- **Custom ImGui Styling:** Replacing the default dark theme with a refined, modern color palette, custom padding, and smooth window rounding.
- **Layout Organization:** Grouping parameters into collapsing headers and tabs to maintain a clean workspace as features expand.

### 7. Expanded Asset Support
- **More 3D Formats:** Adding support for modern 3D model formats like `.gltf` and `.fbx` alongside `.obj`.
- **Image Textures:** Implementing texture loading for common image formats (JPEG, PNG) to support rich material rendering and mapping.

---

## Tech Stack

- **Core:** C++17
- **Graphics API:** WebGPU via Google's Dawn framework
- **Windowing & Input:** GLFW
- **GUI:** Dear ImGui
- **Math Library:** GLM (OpenGL Mathematics)
- **Asset Loading:** tinyobjloader & stb_image

---

## Building the Project

### Prerequisites
- A compiler supporting C++17
- CMake 3.20 or higher
- A compatible GPU supporting Vulkan, DirectX 12, or Metal

### Compilation
```bash
git clone [https://github.com/con2222/C2Lux.git](https://github.com/con2222/C2Lux.git)
cd C2Lux
mkdir build && cd build
cmake ..
cmake --build .
```