cmake_minimum_required(VERSION 3.18)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
project(RayTracer LANGUAGES CXX CUDA)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CUDA_STANDARD 20)

find_package(SFML 3 COMPONENTS Graphics Window System REQUIRED)
find_package(CUDAToolkit REQUIRED)

add_executable(raytracer
    src/main.cpp
    src/gpu_renderer.cu     
)

set_target_properties(raytracer PROPERTIES CUDA_SEPARABLE_COMPILATION ON)
set_source_files_properties(src/main.cpp      PROPERTIES LANGUAGE CXX)
set_source_files_properties(src/render_cuda.cu PROPERTIES LANGUAGE CUDA)

target_include_directories(raytracer PRIVATE src/)
target_link_libraries(raytracer PRIVATE
    SFML::Graphics SFML::Window SFML::System
    CUDA::cudart
    CUDA::curand    
)