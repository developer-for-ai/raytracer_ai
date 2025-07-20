cmake_minimum_required(VERSION 3.16)
project(RayTracerGPU VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler optimizations
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -DNDEBUG")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra")
endif()

# Find required packages
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)

# Find GLFW
pkg_check_modules(GLFW REQUIRED glfw3)

# Include directories
include_directories(include)

# Source files
set(SOURCES
    src/main_interactive.cpp
    src/camera.cpp
    src/geometry.cpp
    src/bvh.cpp
    src/parser.cpp
    src/image.cpp
    src/gpu_raytracer.cpp
    src/window.cpp
    src/input.cpp
    src/scene.cpp
    src/shader.cpp
    src/error_handling.cpp
    src/profiler.cpp
)

# Create executable
add_executable(RayTracerGPU ${SOURCES})

# Link libraries
target_link_libraries(RayTracerGPU 
    Threads::Threads
    OpenGL::GL
    GLEW::GLEW
    ${GLFW_LIBRARIES}
)

# Include directories
target_include_directories(RayTracerGPU PRIVATE 
    include
    ${GLFW_INCLUDE_DIRS}
)

# Set output directory
set_target_properties(RayTracerGPU PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
