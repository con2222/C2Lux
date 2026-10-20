cmake_minimum_required(VERSION 3.22 FATAL_ERROR)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(PROJECT_NAME 3D-Viewer)
set(APP_NAME app)

include(FetchContent)

option(DEV_MODE "Set up development helper settings" ON)

project(${PROJECT_NAME}
        VERSION 0.1
        LANGUAGES C CXX
)

option(USE_WAYLAND "Build with Wayland support" ON)
option(USE_X11 "Build with X11 support" ON)

if (WIN32)
    message(STATUS "Configuring for Windows (D3D12/Vulkan)")
elseif (APPLE)
    message(STATUS "Configuring for macOS (Metal)")
elseif (EMSCRIPTEN)
    message(STATUS "Configuring for Web (emdawnwebgpu)")
elseif (UNIX)
    message(STATUS "Configuring for Linux (X11 + Wayland)")
    set(DAWN_USE_WAYLAND ${USE_WAYLAND} CACHE BOOL "" FORCE)
    set(DAWN_USE_X11 ${USE_X11} CACHE BOOL "" FORCE)
endif()

set(GLFW_BUILD_WAYLAND ${USE_WAYLAND} CACHE BOOL "Enable Wayland for GLFW" FORCE)
set(GLFW_BUILD_X11 ${USE_X11} CACHE BOOL "Enable X11 for GLFW" FORCE)

FetchContent_Declare(glfw GIT_REPOSITORY https://github.com/glfw/glfw.git GIT_TAG 3.4)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

FetchContent_Declare(nfd GIT_REPOSITORY https://github.com/btzy/nativefiledialog-extended.git GIT_TAG v1.3.0)
set(NFD_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(nfd)

add_subdirectory(vendor/webgpu)
add_subdirectory(vendor/glfw3webgpu)
add_subdirectory(vendor/glm)
add_subdirectory(vendor/imgui)

set(SOURCES 
    src/main.cpp
    src/App.cpp
    src/Window.cpp
    src/Renderer.cpp
    src/ResourceManager.cpp
    src/Mesh.cpp
)

add_executable(${APP_NAME} ${SOURCES} "src/UILayer.cpp" "src/Camera.cpp")

if (DEV_MODE)
    # In dev mode, we load resources from the source tree, so that when we
    # dynamically edit resources (like shaders), these are correctly
    # versionned.
    target_compile_definitions(${APP_NAME} PRIVATE
        RESOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/resources"   
        SHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/shaders"
        FONT_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fonts"
    )
else ()
    # In release mode, we just load resources relatively to wherever the
    # executable is launched from, so that the binary is portable
    target_compile_definitions(${APP_NAME} PRIVATE
        RESOURCE_DIR="./resources"
        SHADER_DIR="./shaders"
        FONT_DIR="./fonts"
    )
endif()

foreach(asset_dir IN ITEMS fonts shaders resources)
    add_custom_command(TARGET ${APP_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/${asset_dir}"
            "$<TARGET_FILE_DIR:${APP_NAME}>/${asset_dir}"
        VERBATIM
    )
endforeach()

target_include_directories(${APP_NAME} PRIVATE 
    include/
    vendor/
)

# WebGPU uses a [0, 1] clip-space depth range.
target_compile_definitions(${APP_NAME} PRIVATE GLFW_INCLUDE_NONE GLM_FORCE_DEPTH_ZERO_TO_ONE)
if(MSVC)
    target_compile_options(${APP_NAME} PRIVATE /utf-8)
endif()

target_link_libraries(${APP_NAME} PRIVATE
    glfw
    webgpu
    glfw3webgpu
    glm
    imgui
    nfd
)

set_target_properties(${APP_NAME} PROPERTIES
    COMPILE_WARNING_AS_ERROR ON
)

include(${CMAKE_CURRENT_SOURCE_DIR}/utils.cmake)
target_copy_webgpu_binaries(${APP_NAME})
