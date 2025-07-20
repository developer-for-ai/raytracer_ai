cmake_minimum_required(VERSION 3.16)
project(RayTracer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler-specific optimizations
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -DNDEBUG")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra")
endif()

# Find required packages
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)

# Find OpenGL
find_package(OpenGL REQUIRED)

# Find GLFW
pkg_check_modules(GLFW REQUIRED glfw3)

# Find GLEW
find_package(GLEW REQUIRED)

# Include directories
include_directories(include)

# Source files for offline raytracer
set(OFFLINE_SOURCES
    src/main.cpp
    src/raytracer.cpp
    src/scene.cpp
    src/camera.cpp
    src/material.cpp
    src/geometry.cpp
    src/bvh.cpp
    src/parser.cpp
    src/image.cpp
    src/light.cpp
)

# Source files for interactive raytracer
set(INTERACTIVE_SOURCES
    src/main_interactive.cpp
    src/scene.cpp
    src/camera.cpp
    src/material.cpp
    src/geometry.cpp
    src/bvh.cpp
    src/parser.cpp
    src/light.cpp
    src/image.cpp
    src/image.cpp
    src/gpu_raytracer.cpp
    src/window.cpp
    src/input.cpp
    src/shader.cpp
)

# Create offline raytracer executable
add_executable(RayTracer ${OFFLINE_SOURCES})

# Create interactive raytracer executable
add_executable(RayTracerGPU ${INTERACTIVE_SOURCES})

# Link libraries for offline raytracer
target_link_libraries(RayTracer 
    Threads::Threads
)

# Link libraries for interactive raytracer
target_link_libraries(RayTracerGPU 
    Threads::Threads
    OpenGL::GL
    GLEW::GLEW
    ${GLFW_LIBRARIES}
)

# Include directories
target_include_directories(RayTracer PRIVATE 
    include
)

target_include_directories(RayTracerGPU PRIVATE 
    include
    ${GLFW_INCLUDE_DIRS}
)

# Set output directory
set_target_properties(RayTracer RayTracerGPU PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
