cmake_minimum_required(VERSION 3.16)
project(optix_photon_benchmark LANGUAGES CXX CUDA)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_STANDARD 17)

# Find OptiX
find_package(CUDAToolkit REQUIRED)

if(NOT DEFINED OptiX_INSTALL_DIR)
    set(OptiX_INSTALL_DIR $ENV{OptiX_INSTALL_DIR})
endif()

if(NOT OptiX_INSTALL_DIR)
    message(FATAL_ERROR "OptiX_INSTALL_DIR not set. Please set it via -DOptiX_INSTALL_DIR=<path> or environment variable")
endif()

message(STATUS "Using OptiX from: ${OptiX_INSTALL_DIR}")

# Include directories
include_directories(
    ${OptiX_INSTALL_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CUDAToolkit_INCLUDE_DIRS}
)

# Compile PTX from CUDA source
set(CUDA_NVRTC_FLAGS "-I${OptiX_INSTALL_DIR}/include" "-I${CMAKE_CURRENT_SOURCE_DIR}")

# Find nvcc
find_program(NVCC_EXECUTABLE nvcc HINTS ${CUDAToolkit_BIN_DIR})

# Custom command to compile PTX
add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/photon_kernels.optixir
    COMMAND ${NVCC_EXECUTABLE}
        -optix-ir
        -I${OptiX_INSTALL_DIR}/include
        -I${CMAKE_CURRENT_SOURCE_DIR}
        -O3
        --use_fast_math
        -o ${CMAKE_BINARY_DIR}/photon_kernels.optixir
        ${CMAKE_CURRENT_SOURCE_DIR}/photon_kernels.cu
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/photon_kernels.cu
    COMMENT "Compiling OptiX IR for photon kernels"
)

add_custom_target(photon_ptx ALL DEPENDS ${CMAKE_BINARY_DIR}/photon_kernels.optixir)

# Main executable
add_executable(photon_benchmark
    photon_benchmark.cpp
)

add_dependencies(photon_benchmark photon_ptx)

target_link_libraries(photon_benchmark
    CUDA::cuda_driver
    CUDA::cudart
)

target_compile_definitions(photon_benchmark PRIVATE
    PTX_DIR="${CMAKE_BINARY_DIR}"
)
