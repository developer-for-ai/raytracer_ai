#!/bin/bash

# Build script for Interactive GPU Ray Tracer - Linux
echo "🚀 Building Interactive GPU Ray Tracer for Linux..."
echo "=================================================="

# Detect distribution
if [ -f /etc/os-release ]; then
    . /etc/os-release
    DISTRO=$ID
else
    DISTRO="unknown"
fi

echo "Detected distribution: $DISTRO"

# Check for required dependencies
echo "Checking dependencies..."
if ! pkg-config --exists glfw3; then
    echo "❌ GLFW3 not found. Installing dependencies..."
    
    case $DISTRO in
        "ubuntu"|"debian")
            sudo apt update
            sudo apt install -y libglfw3-dev libglew-dev libgl1-mesa-dev cmake build-essential
            ;;
        "fedora"|"centos"|"rhel")
            sudo dnf install -y glfw-devel glew-devel mesa-libGL-devel cmake gcc-c++
            ;;
        "arch"|"manjaro")
            sudo pacman -S --needed glfw-x11 glew mesa cmake gcc
            ;;
        "opensuse"|"sled"|"sles")
            sudo zypper install libglfw3-devel glew-devel Mesa-libGL-devel cmake gcc-c++
            ;;
        *)
            echo "⚠️  Unknown distribution. Please install manually:"
            echo "   - GLFW3 development libraries"
            echo "   - GLEW development libraries" 
            echo "   - OpenGL development libraries"
            echo "   - CMake and C++ compiler"
            ;;
    esac
else
    echo "✅ Graphics dependencies found"
fi

# Create build directory
echo "🔧 Setting up build environment..."
mkdir -p build
cd build

# Configure with CMake
echo "⚙️  Configuring with CMake..."
cmake -DCMAKE_BUILD_TYPE=Release ..

# Build with all available cores
echo "🔨 Compiling GPU ray tracer..."
make -j$(nproc)

# Check if GPU build was successful
if [ -f "./bin/RayTracerGPU" ]; then
    echo "✅ Build successful!"
    echo "   🎮 GPU Raytracer: ./bin/RayTracerGPU"
    echo ""
    
    # Test GPU version (headless mode for build verification)
    echo "🧪 Testing GPU raytracer..."
    cd ..
    ./build/bin/RayTracerGPU examples/cornell_box.scene -o quick_test_gpu.ppm -w 200 -h 150 -s 4
    
    if [ -f "quick_test_gpu.ppm" ]; then
        echo "✅ GPU test successful!"
        
        echo ""
        echo "🎯 Usage Examples:"
        echo "================="
        echo ""
        echo "🎮 GPU Interactive Ray Tracer:"
        echo "  ./build/bin/RayTracerGPU examples/cornell_box.scene"
        echo "  ./build/bin/RayTracerGPU examples/materials_showcase.scene"
        echo "  ./build/bin/RayTracerGPU examples/lighting_demo.scene"
        echo ""
        echo "🎮 GPU File Output (Headless):"
        echo "  ./build/bin/RayTracerGPU examples/showcase.scene -o gpu_render.ppm -w 1920 -h 1080 -s 16"
        echo "  ./build/bin/RayTracerGPU examples/materials_showcase.scene -o materials.ppm -w 800 -h 600 -s 8"
        echo "  ./build/bin/RayTracerGPU examples/lighting_demo.scene -o lighting.ppm -w 1200 -h 900 -s 12"
        echo ""
        echo "🎮 Interactive Controls:"
        echo "  WASD - Move camera"
        echo "  Click - Capture/release mouse for looking"  
        echo "  Arrow Keys - Look around (alternative to mouse)"
        echo "  Space/Shift - Move up/down"
        echo "  R - Reset camera position"
        echo "  F1 - Toggle detailed stats"
        echo "  ESC - Exit"
        echo ""
        echo "⚡ Performance:"
        echo "  Real-time: 1-16 samples, 30-60 FPS interactive"
        echo "  File output: 4-64 samples for high-quality renders"
        echo ""
        echo "🌟 Interactive GPU ray tracing - the future is here!"
        
    else
        echo "❌ GPU test failed!"
        exit 1
    fi
else
    echo "❌ Build failed - GPU raytracer not found!"
    exit 1
fi
