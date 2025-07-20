# 💻 Cross-Platform Installation Guide

This guide provides detailed instructions for building the GPU raytracer on Windows, macOS, and Linux.

## 🐧 Linux Installation

### Automated Installation
```bash
# Clone the repository
git clone https://github.com/yourusername/raytracer_ai.git
cd raytracer_ai

# Run automated build script
./build.sh
```

### Supported Distributions

#### Ubuntu/Debian/Linux Mint
```bash
sudo apt update
sudo apt install -y libglfw3-dev libglew-dev libgl1-mesa-dev cmake build-essential
```

#### Fedora/CentOS/RHEL
```bash
sudo dnf install -y glfw-devel glew-devel mesa-libGL-devel cmake gcc-c++
```

#### Arch Linux/Manjaro
```bash
sudo pacman -S --needed glfw-x11 glew mesa cmake gcc
```

#### openSUSE
```bash
sudo zypper install libglfw3-devel glew-devel Mesa-libGL-devel cmake gcc-c++
```

### Manual Build
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

## 🍎 macOS Installation

### Automated Installation
```bash
# Clone the repository
git clone https://github.com/yourusername/raytracer_ai.git
cd raytracer_ai

# Run macOS build script
./build_macos.sh
```

### Prerequisites

#### Install Homebrew
```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

#### Install Xcode Command Line Tools
```bash
xcode-select --install
```

#### Install Dependencies
```bash
brew install glfw glew cmake
```

### System Requirements
- macOS 10.14 (Mojave) or later
- Xcode Command Line Tools
- Homebrew package manager

### Manual Build
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(sysctl -n hw.ncpu)
```

## 🪟 Windows Installation

### Automated Installation
```batch
# Clone the repository
git clone https://github.com/yourusername/raytracer_ai.git
cd raytracer_ai

# Run Windows build script
build.bat
```

### Prerequisites

#### Install Visual Studio
- Visual Studio 2019 or 2022 (Community Edition is fine)
- Include "C++ CMake tools" workload
- Include "Windows 10 SDK"

#### Install vcpkg
```batch
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat

# Add to PATH environment variable
set PATH=%PATH%;C:\path\to\vcpkg

# Set VCPKG_ROOT environment variable
set VCPKG_ROOT=C:\path\to\vcpkg
```

#### Install Dependencies
```batch
vcpkg install glfw3 glew opengl --triplet x64-windows
```

### Manual Build
```batch
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows
cmake --build . --config Release
```

## 🚀 Running the Application

### Linux/macOS
```bash
# Interactive mode
./build/bin/RayTracerGPU examples/cornell_box.scene

# Headless rendering
./build/bin/RayTracerGPU examples/showcase.scene -o output.ppm -w 1920 -h 1080 -s 16
```

### Windows
```batch
# Interactive mode
build\bin\Release\RayTracerGPU.exe examples\cornell_box.scene

# Headless rendering
build\bin\Release\RayTracerGPU.exe examples\showcase.scene -o output.ppm -w 1920 -h 1080 -s 16
```

## 🔧 Troubleshooting

### Linux Issues

**OpenGL Context Creation Failed:**
```bash
# Install additional mesa packages
sudo apt install mesa-utils mesa-common-dev

# Check OpenGL support
glxinfo | grep "OpenGL version"
```

**GLFW Window Creation Failed:**
```bash
# Install X11 development libraries
sudo apt install libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev
```

### macOS Issues

**Permission Denied Errors:**
```bash
# Fix Homebrew permissions
sudo chown -R $(whoami) /usr/local/Cellar
```

**OpenGL Version Too Old:**
- Ensure macOS 10.14+ (OpenGL 4.1 Core Profile required)
- Update to latest macOS if possible

### Windows Issues

**vcpkg Package Not Found:**
```batch
# Refresh vcpkg database
vcpkg update
vcpkg upgrade

# Reinstall packages
vcpkg remove glfw3 glew opengl --triplet x64-windows
vcpkg install glfw3 glew opengl --triplet x64-windows
```

**Build Configuration Issues:**
- Ensure Visual Studio has C++ CMake tools installed
- Use x64 Native Tools Command Prompt
- Verify VCPKG_ROOT environment variable is set

## 📋 System Requirements

### Minimum Requirements
- **OS**: Windows 10, macOS 10.14, or Linux with OpenGL 4.3+
- **GPU**: OpenGL 4.3 compatible graphics card
- **RAM**: 4GB system memory
- **Storage**: 100MB free space

### Recommended
- **OS**: Latest Windows 11, macOS 12+, or Ubuntu 22.04+
- **GPU**: Dedicated graphics card with 2GB+ VRAM
- **RAM**: 8GB+ system memory
- **CPU**: Multi-core processor for compilation

## 🏗️ Development Setup

For developers wanting to modify the codebase:

### IDE Recommendations
- **Windows**: Visual Studio 2022, CLion
- **macOS**: Xcode, CLion, VS Code
- **Linux**: CLion, VS Code, Qt Creator

### Debug Builds
```bash
# Linux/macOS
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)

# Windows
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build . --config Debug
```

### Code Style
- C++17 standard
- Follow existing naming conventions
- Include comprehensive error checking
- Document all public APIs
