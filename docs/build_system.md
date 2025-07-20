# 🏗️ Cross-Platform Build System Guide

Complete guide to the CMake-based cross-platform build system with automatic platform detection, dependency management, and compilation for **Windows**, **macOS**, and **Linux**.

## Overview

The GPU raytracer features a sophisticated CMake build system that automatically detects your platform, manages dependencies, and applies platform-specific optimizations for optimal performance.

### System Requirements

**Minimum Requirements:**
- CMake 3.16 or higher
- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2019+)
- OpenGL 4.3 compatible graphics drivers
- Git (for version detection)

**Cross-Platform Dependencies:**
- **GLFW 3.3+** - Cross-platform windowing and input handling
- **GLEW 2.0+** - OpenGL extension loading library
- **OpenGL 4.3+** - Compute shader support for GPU raytracing

## 🚀 Quick Start

### Automated Platform Detection

The build system automatically detects your platform and uses appropriate tools:

```bash
# Linux - Uses pkg-config and native package managers
./build.sh

# macOS - Uses Homebrew and Xcode tools  
./build_macos.sh

# Windows - Uses vcpkg and Visual Studio
build.bat
```

## Platform-Specific Build Details

### 🐧 **Linux Build System**

**Automated Build Script:**
```bash
# Automatic distribution detection and dependency installation
./build.sh
```

**Supported Distributions:**
- **Ubuntu/Debian/Mint**: `apt` package manager with mesa drivers
- **Fedora/CentOS/RHEL**: `dnf` package manager with mesa libraries
- **Arch/Manjaro**: `pacman` package manager with X11 GLFW
- **openSUSE/SLES**: `zypper` package manager with development libraries

**Distribution-Specific Commands:**
```bash
# Ubuntu/Debian
sudo apt install libglfw3-dev libglew-dev libgl1-mesa-dev cmake build-essential

# Fedora/CentOS/RHEL
sudo dnf install glfw-devel glew-devel mesa-libGL-devel cmake gcc-c++

# Arch/Manjaro  
sudo pacman -S glfw-x11 glew mesa cmake gcc

# openSUSE
sudo zypper install libglfw3-devel glew-devel Mesa-libGL-devel cmake gcc-c++
```

### 🍎 **macOS Build System**

**Automated Build Script:**
```bash
# Automatic dependency installation and build with Homebrew
./build_macos.sh
```

**System Requirements:**
- **macOS 10.14+** (Mojave or later) - OpenGL 4.1 Core Profile support
- **Xcode Command Line Tools** - C++ compiler and system headers
- **Homebrew** - Package manager for GLFW/GLEW libraries

**Dependency Installation:**
```bash
# Install Homebrew package manager
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install graphics libraries
brew install glfw glew cmake

# Install development tools
xcode-select --install
```

**macOS-Specific Features:**
- **App Bundle Creation** - Proper `.app` bundle with `Info.plist.in`
- **Retina Display Support** - High-DPI rendering optimization
- **GPU Switching** - Automatic graphics switching on MacBooks
- **Framework Integration** - Cocoa, IOKit, and CoreVideo frameworks

### 🪟 **Windows Build System**

**Automated Build Script:**
```batch
# Automatic dependency installation and build with vcpkg
build.bat
```

**System Requirements:**
- **Windows 10/11** - Modern Windows with DirectX/OpenGL support
- **Visual Studio 2019/2022** - C++ compiler with CMake tools
- **vcpkg** - C++ package manager for dependencies

**Development Environment Setup:**
```batch
# Install vcpkg (one-time setup)
git clone https://github.com/Microsoft/vcpkg.git C:\vcpkg
cd C:\vcpkg
.\bootstrap-vcpkg.bat

# Set environment variables
set VCPKG_ROOT=C:\vcpkg
set PATH=%PATH%;C:\vcpkg

# Install dependencies
vcpkg install glfw3 glew opengl --triplet x64-windows
```

**Windows-Specific Features:**
- **Visual Studio Integration** - Native MSVC compiler support
- **DLL Management** - Automatic dependency resolution
- **Windows Subsystem** - GUI application configuration
- **x64 Architecture** - 64-bit Windows targeting

## Platform-Specific Build Details

### Linux (Ubuntu/Debian)

```bash
# Install dependencies
sudo apt update
sudo apt install cmake build-essential libglfw3-dev libglew-dev

# Build
./build.sh
```

### Linux (Fedora/CentOS/RHEL)

```bash
# Install dependencies
sudo dnf install cmake gcc-c++ glfw-devel glew-devel

# Build
./build.sh
```

### Linux (Arch)

```bash
# Install dependencies
sudo pacman -S cmake gcc glfw glew

# Build  
./build.sh
```

### macOS

```bash
# Install dependencies via Homebrew
brew install cmake glfw glew

# Build
./build.sh
```

### Windows (MSYS2/MinGW)

```bash
# Install dependencies
pacman -S mingw-w64-x86_64-cmake
pacman -S mingw-w64-x86_64-glfw
pacman -S mingw-w64-x86_64-glew

# Build
./build.sh
```

### Windows (Visual Studio)

```cmd
:: Use vcpkg for dependencies
vcpkg install glfw3 glew

:: Configure and build
cmake -DCMAKE_TOOLCHAIN_FILE=[vcpkg root]/scripts/buildsystems/vcpkg.cmake ..
cmake --build . --config Release
```

## Manual Build Process

```bash
# Create build directory
mkdir -p build
cd build

# Configure with CMake
cmake ..

# Compile (parallel build with available cores)
make -j$(nproc)

# Run executable
./bin/RayTracerGPU examples/cornell_box.scene
```

## CMake Configuration

### CMakeLists.txt Structure

```cmake
cmake_minimum_required(VERSION 3.10)
project(RayTracerGPU VERSION 1.1.0)

# Set C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build type and optimization
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -Wall -Wextra")
```

### Dependency Detection

```cmake
# Find required packages
find_package(PkgConfig REQUIRED)
find_package(OpenGL REQUIRED)

# GLFW
pkg_check_modules(GLFW REQUIRED glfw3)

# GLEW
find_package(GLEW REQUIRED)
if(NOT GLEW_FOUND)
    pkg_check_modules(GLEW REQUIRED glew)
endif()
```

### Compilation Flags

```cmake
# Compiler-specific optimizations
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall -Wextra -Wpedantic
        -O3 -march=native
        -ffast-math
        -funroll-loops
    )
endif()

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        /W4 /O2 /arch:AVX2
    )
endif()
```

## Build Configurations

### Release Build (Default)

```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

**Optimizations:**
- `-O3` - Maximum optimization
- `-march=native` - CPU-specific optimizations
- `-DNDEBUG` - Disable debug assertions
- `-ffast-math` - Aggressive floating-point optimizations

### Debug Build

```bash
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)
```

**Debug Features:**
- `-O0` - No optimization for debugging
- `-g` - Full debug information
- `-Wall -Wextra` - Enhanced warnings
- Debug assertions enabled

### Profile Build

```bash
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
make -j$(nproc)
```

**Profile Features:**
- `-O2` - Good optimization with debug info
- `-g` - Debug symbols for profiling
- Suitable for performance analysis

## CMake Configuration Details

### Cross-Platform CMake Features

**Automatic Platform Detection:**
```cmake
# Platform detection and messaging
if(WIN32)
    set(PLATFORM_NAME "Windows")
elseif(APPLE)  
    set(PLATFORM_NAME "macOS")
elseif(UNIX)
    set(PLATFORM_NAME "Linux")
endif()

message(STATUS "Building for ${PLATFORM_NAME}")
```

**Compiler-Specific Optimizations:**
```cmake
# Cross-platform compiler optimizations
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
    if(NOT CMAKE_CROSSCOMPILING)
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=native")
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS_RELEASE "/O2 /DNDEBUG")
endif()
```

**Platform-Specific Library Linking:**
```cmake
# Windows libraries
if(WIN32)
    set(GRAPHICS_LIBS glfw GLEW::GLEW)
    
# macOS frameworks  
elseif(APPLE)
    find_library(COCOA_LIBRARY Cocoa REQUIRED)
    find_library(IOKIT_LIBRARY IOKit REQUIRED)
    find_library(COREVIDEO_LIBRARY CoreVideo REQUIRED)
    set(GRAPHICS_LIBS glfw GLEW::GLEW ${COCOA_LIBRARY} ${IOKIT_LIBRARY} ${COREVIDEO_LIBRARY})
    
# Linux pkg-config
else()
    pkg_check_modules(GLFW REQUIRED glfw3)
    set(GRAPHICS_LIBS ${GLFW_LIBRARIES} GLEW::GLEW)
endif()
```

## Advanced Build Options

### Custom Install Directory

```bash
cmake -DCMAKE_INSTALL_PREFIX=/usr/local ..
make install
```

### Verbose Build Output

```bash
make VERBOSE=1
# or
cmake --build . -- VERBOSE=1
```

### Parallel Compilation

```bash
# Use all available cores
make -j$(nproc)

# Specify core count
make -j8

# On Windows with MSVC
cmake --build . --parallel 8
```

### Clean Build

```bash
# Remove build directory
rm -rf build

# Or clean existing build
make clean
# or
cmake --build . --target clean
```

## Troubleshooting Build Issues

### Common Dependency Errors

**GLFW Not Found:**
```
CMake Error: Could not find a package configuration file provided by "glfw3"
```

**Solutions:**
```bash
# Ubuntu/Debian
sudo apt install libglfw3-dev

# Fedora
sudo dnf install glfw-devel

# macOS
brew install glfw

# Check pkg-config path
export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH
```

**GLEW Not Found:**
```
CMake Error: Could not find a package configuration file provided by "GLEW"
```

**Solutions:**
```bash
# Ubuntu/Debian  
sudo apt install libglew-dev

# Fedora
sudo dnf install glew-devel

# macOS
brew install glew
```

**OpenGL Version Issues:**
```
OpenGL 4.3 not supported
```

**Solutions:**
- Update graphics drivers
- Check GPU compatibility: `glxinfo | grep "OpenGL version"`
- Use software rendering (slow): `export LIBGL_ALWAYS_SOFTWARE=1`

### Compilation Errors

**C++17 Not Supported:**
```
error: 'std::optional' has not been declared
```

**Solutions:**
```bash
# Update compiler
sudo apt install gcc-9 g++-9

# Specify compiler version
export CC=gcc-9
export CXX=g++-9
cmake ..
```

**Missing Headers:**
```
fatal error: GL/glew.h: No such file or directory
```

**Solutions:**
```bash
# Install development headers
sudo apt install libgl1-mesa-dev

# Check include paths
find /usr/include -name "glew.h" 2>/dev/null
```

### Linking Errors

**Undefined References:**
```
undefined reference to `glfwCreateWindow'
```

**Solutions:**
```bash
# Check library installation
ldconfig -p | grep glfw

# Manual library specification
cmake -DGLFW_LIBRARIES=/usr/local/lib/libglfw.so ..
```

**Missing OpenGL Context:**
```
Runtime error: Failed to create OpenGL context
```

**Solutions:**
- Run on system with display: `DISPLAY=:0 ./program`
- Use Xvfb for headless: `xvfb-run -a ./program`
- Check graphics drivers: `glxinfo | grep "direct rendering"`

## Build Optimization

### Compiler Optimizations

```cmake
# CPU-specific optimizations
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -mtune=native")

# Link-time optimization
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -flto")

# Fast math (may reduce precision)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -ffast-math")
```

### Build Speed Optimization

```bash
# Use faster linker (if available)
sudo apt install lld
export CMAKE_CXX_FLAGS="-fuse-ld=lld"

# Use ccache for faster rebuilds
sudo apt install ccache
export CMAKE_CXX_COMPILER_LAUNCHER=ccache

# Enable parallel template instantiation
export CMAKE_CXX_FLAGS="-ftemplate-backtrace-limit=0"
```

### Memory Usage Optimization

```bash
# Reduce memory usage during compilation
export CMAKE_CXX_FLAGS="-fno-gnu-unique"

# Limit parallel jobs for low-memory systems
make -j2  # Instead of -j$(nproc)
```

## Custom Build Targets

### Add Test Target

```cmake
# Enable testing
enable_testing()

# Add test executable
add_executable(test_raytracer test/test_main.cpp)
target_link_libraries(test_raytracer ${RAYTRACER_LIBS})

# Register test
add_test(NAME RaytracerTests COMMAND test_raytracer)
```

### Add Install Target

```cmake
# Install executable
install(TARGETS RayTracerGPU
    RUNTIME DESTINATION bin
)

# Install examples
install(DIRECTORY examples/
    DESTINATION share/raytracer/examples
    FILES_MATCHING PATTERN "*.scene" PATTERN "*.obj"
)

# Install documentation
install(FILES README.md CHANGELOG.md
    DESTINATION share/doc/raytracer
)
```

### Add Package Target

```cmake
# CPack configuration
set(CPACK_PACKAGE_NAME "RayTracerGPU")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Interactive GPU Ray Tracer")
set(CPACK_PACKAGE_VENDOR "Your Name")

# Package formats
set(CPACK_GENERATOR "TGZ;ZIP")
if(UNIX AND NOT APPLE)
    list(APPEND CPACK_GENERATOR "DEB;RPM")
endif()

include(CPack)
```

## CI/CD Integration

### GitHub Actions

```yaml
name: Build and Test

on: [push, pull_request]

jobs:
  build:
    runs-on: ubuntu-latest
    
    steps:
    - uses: actions/checkout@v3
    
    - name: Install dependencies
      run: |
        sudo apt update
        sudo apt install cmake build-essential libglfw3-dev libglew-dev
        
    - name: Configure CMake
      run: cmake -B build -DCMAKE_BUILD_TYPE=Release
      
    - name: Build
      run: cmake --build build --parallel
      
    - name: Test
      run: |
        cd build
        xvfb-run -a ./bin/RayTracerGPU ../examples/cornell_box.scene -o test.ppm -w 100 -h 100 -s 1
```

### Docker Build

```dockerfile
FROM ubuntu:22.04

RUN apt update && apt install -y \
    cmake build-essential \
    libglfw3-dev libglew-dev \
    xvfb

COPY . /raytracer
WORKDIR /raytracer

RUN ./build.sh

ENTRYPOINT ["xvfb-run", "-a", "./build/bin/RayTracerGPU"]
```

## Performance Analysis

### Build Time Analysis

```bash
# Time the build process
time make -j$(nproc)

# Analyze build dependencies
make -j1 2>&1 | grep "Building"

# Profile compilation
ccache -s  # If using ccache
```

### Binary Analysis

```bash
# Check binary size
ls -lh bin/RayTracerGPU

# Analyze symbols
nm bin/RayTracerGPU | head -20

# Check dependencies
ldd bin/RayTracerGPU

# Profile with perf
perf record ./bin/RayTracerGPU examples/showcase.scene
perf report
```

## Maintenance and Updates

### Version Management

```bash
# Check current version
./bin/RayTracerGPU --version

# Update version in CMakeLists.txt
sed -i 's/VERSION [0-9.]\+/VERSION 1.2.0/' CMakeLists.txt
```

### Dependency Updates

```bash
# Check for outdated packages
# Ubuntu
apt list --upgradable | grep -E "(glfw|glew|cmake)"

# macOS
brew outdated | grep -E "(glfw|glew|cmake)"

# Update dependencies
sudo apt update && sudo apt upgrade
# or
brew upgrade
```

### Clean Maintenance

```bash
# Remove old build files
find . -name "*.o" -delete
find . -name "CMakeCache.txt" -delete

# Reset to clean state
git clean -xfd  # WARNING: Removes all untracked files
```
