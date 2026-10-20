# Fetch a matching core and WebGPU backend instead of patching the vendored 1.89.8.
include(FetchContent)
FetchContent_Declare(imgui_upstream
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.93.0
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(imgui_upstream)

add_library(imgui STATIC
    ${imgui_upstream_SOURCE_DIR}/imgui.cpp
    ${imgui_upstream_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_upstream_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_upstream_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_upstream_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp
    ${imgui_upstream_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_upstream_SOURCE_DIR}/backends/imgui_impl_wgpu.cpp
)
target_include_directories(imgui SYSTEM PUBLIC ${imgui_upstream_SOURCE_DIR})
target_compile_definitions(imgui PRIVATE IMGUI_IMPL_WEBGPU_BACKEND_DAWN GLFW_INCLUDE_NONE)
target_link_libraries(imgui PUBLIC webgpu glfw)
