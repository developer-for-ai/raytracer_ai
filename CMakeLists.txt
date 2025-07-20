cmake_minimum_required(VERSION 3.16)
project(RayTracerGPU VERSION 1.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Platform detection
if(WIN32)
    set(PLATFORM_NAME "Windows")
elseif(APPLE)
    set(PLATFORM_NAME "macOS")
elseif(UNIX)
    set(PLATFORM_NAME "Linux")
endif()

message(STATUS "Building for ${PLATFORM_NAME}")

# Compiler optimizations - Cross-platform
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra")
    # Use native optimizations only on native builds
    if(NOT CMAKE_CROSSCOMPILING)
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=native")
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS_RELEASE "/O2 /DNDEBUG")
    set(CMAKE_CXX_FLAGS_DEBUG "/DEBUG /Od")
endif()

# Find required packages
find_package(Threads REQUIRED)
find_package(OpenGL REQUIRED)

# Platform-specific OpenGL and windowing libraries
if(WIN32)
    # Windows - Use vcpkg or manual installation
    find_package(glfw3 REQUIRED)
    find_package(GLEW REQUIRED)
    set(GRAPHICS_LIBS glfw GLEW::GLEW)
elseif(APPLE)
    # macOS - Use Homebrew or manual installation
    find_package(glfw3 REQUIRED)
    find_package(GLEW REQUIRED)
    # macOS specific frameworks
    find_library(COCOA_LIBRARY Cocoa REQUIRED)
    find_library(IOKIT_LIBRARY IOKit REQUIRED)
    find_library(COREVIDEO_LIBRARY CoreVideo REQUIRED)
    set(GRAPHICS_LIBS glfw GLEW::GLEW ${COCOA_LIBRARY} ${IOKIT_LIBRARY} ${COREVIDEO_LIBRARY})
else()
    # Linux - Use pkg-config
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GLFW REQUIRED glfw3)
    find_package(GLEW REQUIRED)
    set(GRAPHICS_LIBS ${GLFW_LIBRARIES} GLEW::GLEW)
endif()

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

# Link libraries - Cross-platform
target_link_libraries(RayTracerGPU 
    Threads::Threads
    OpenGL::GL
    ${GRAPHICS_LIBS}
)

# Include directories - Cross-platform
target_include_directories(RayTracerGPU PRIVATE 
    include
)

# Platform-specific include directories
if(UNIX AND NOT APPLE)
    # Linux - Add GLFW include dirs from pkg-config
    target_include_directories(RayTracerGPU PRIVATE ${GLFW_INCLUDE_DIRS})
endif()

# Windows-specific settings
if(WIN32)
    # Set Windows subsystem for GUI applications
    set_target_properties(RayTracerGPU PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
    # Copy DLLs to output directory in Windows
    if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        add_custom_command(TARGET RayTracerGPU POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "Windows build completed"
        )
    endif()
endif()

# macOS-specific settings
if(APPLE)
    # Set macOS deployment target
    set_target_properties(RayTracerGPU PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/Info.plist.in"
    )
endif()

# Set output directory
set_target_properties(RayTracerGPU PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
