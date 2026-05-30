cmake_minimum_required(VERSION 3.16)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
project(RayTracer)

set(CMAKE_CXX_STANDARD 17)

add_executable(raytracer src/main.cpp)
target_include_directories(raytracer PRIVATE src/)