cmake_minimum_required(VERSION 3.22 FATAL_ERROR)
project(webgpu-backend-dawn VERSION 1.0.0)

message(STATUS "Using Dawn backend for WebGPU")

add_library(webgpu INTERFACE)

if (EMSCRIPTEN)

	# This is used to advertise the flavor of WebGPU that this zip provides
	target_compile_definitions(webgpu INTERFACE WEBGPU_BACKEND_EMSCRIPTEN)

	# Emscripten 4.0.10+ provides Dawn's current C and C++ headers through this port.
	target_compile_options(webgpu INTERFACE --use-port=emdawnwebgpu)
	target_link_options(webgpu INTERFACE --use-port=emdawnwebgpu)

else (EMSCRIPTEN)

	include(cmake/FetchDawn.cmake)

	target_link_libraries(webgpu INTERFACE webgpu_dawn)

	# This is used to advertise the flavor of WebGPU that this zip provides
	target_compile_definitions(webgpu INTERFACE WEBGPU_BACKEND_DAWN)

endif (EMSCRIPTEN)

# Does nothing, as this dawn-based distribution of WebGPU is statically linked
function(target_copy_webgpu_binaries Target)
endfunction()
