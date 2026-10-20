# This file is part of the "Learn WebGPU for C++" book.
#   https://eliemichel.github.io/LearnWebGPU
# 
# MIT License
# Copyright (c) 2022-2024 Elie Michel and the wgpu-native authors
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

# Keep the version fixed: updating Dawn also requires checking the native API.
set(C2LUX_DAWN_TAG "v20261016.190333" CACHE STRING "Dawn release tag")

if(TARGET webgpu_dawn)
    return()
endif()

include(FetchContent)

# Upstream now fetches its own dependencies; no local Python patch is needed.
set(DAWN_FETCH_DEPENDENCIES ON CACHE BOOL "" FORCE)
set(DAWN_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
set(DAWN_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(DAWN_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
set(DAWN_BUILD_PROTOBUF OFF CACHE BOOL "" FORCE)
set(DAWN_ENABLE_D3D11 OFF CACHE BOOL "" FORCE)
set(DAWN_ENABLE_DESKTOP_GL OFF CACHE BOOL "" FORCE)
set(DAWN_ENABLE_OPENGLES OFF CACHE BOOL "" FORCE)
set(DAWN_ENABLE_NULL OFF CACHE BOOL "" FORCE)
# GLFW creates desktop HWND surfaces; WinUI/UWP integration is unnecessary.
set(DAWN_USE_WINDOWS_UI OFF CACHE BOOL "" FORCE)
set(TINT_BUILD_CMD_TOOLS OFF CACHE BOOL "" FORCE)
set(TINT_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(TINT_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
set(TINT_BUILD_SPV_READER OFF CACHE BOOL "" FORCE)

if(WIN32)
    # Preserve the viewer's Vulkan default; D3D12 can be enabled explicitly.
    set(DAWN_ENABLE_D3D12 OFF CACHE BOOL "Build Dawn's D3D12 backend")
    # Load the installed GPU driver rather than expecting bundled DLLs.
    set(DAWN_FORCE_SYSTEM_COMPONENT_LOAD ON CACHE BOOL "Load system GPU libraries")
endif()

# Metal on macOS, Vulkan on Linux/Windows. Platform backends remain configurable.
FetchContent_Declare(dawn
    GIT_REPOSITORY https://github.com/google/dawn.git
    GIT_TAG ${C2LUX_DAWN_TAG}
    GIT_SHALLOW TRUE
    # Dawn's own dependency fetcher selects the submodules it needs.
    GIT_SUBMODULES ""
)
FetchContent_MakeAvailable(dawn)
# Keep unrelated Dawn utilities out of the default application build.
set_property(DIRECTORY "${dawn_SOURCE_DIR}" PROPERTY EXCLUDE_FROM_ALL TRUE)
