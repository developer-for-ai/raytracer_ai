#!/bin/bash

# Build script for Interactive GPU Ray Tracer - macOS
echo "🚀 Building Interactive GPU Ray Tracer for macOS..."
echo "==================================================="

# Check for Homebrew
if ! command -v brew &> /dev/null; then
    echo "❌ Homebrew not found. Please install Homebrew first:"
    echo "   /bin/bash -c \"\$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)\""
    exit 1
fi

# Check for required dependencies
echo "Checking dependencies..."
if ! brew list glfw &> /dev/null || ! brew list glew &> /dev/null; then
    echo "❌ Graphics libraries not found. Installing dependencies..."
    brew install glfw glew cmake
else
    echo "✅ Graphics dependencies found"
fi

# Check for Xcode Command Line Tools
if ! xcode-select -p &> /dev/null; then
    echo "❌ Xcode Command Line Tools not found. Installing..."
    xcode-select --install
    echo "Please complete Xcode Command Line Tools installation and run this script again."
    exit 1
fi

# Create build directory
echo "🔧 Setting up build environment..."
mkdir -p build
cd build

# Configure with CMake
echo "📦 Configuring project with CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release

if [ $? -ne 0 ]; then
    echo "❌ CMake configuration failed"
    exit 1
fi

# Build project
echo "🔨 Building project..."
make -j$(sysctl -n hw.ncpu)

if [ $? -ne 0 ]; then
    echo "❌ Build failed"
    exit 1
fi

echo "✅ Build completed successfully!"
echo "📁 Executable: build/bin/RayTracerGPU"
echo ""
echo "🎮 To run: ./build/bin/RayTracerGPU examples/showcase.scene"

# Check if we can run the executable
if [ -f "bin/RayTracerGPU" ]; then
    echo "🧪 Testing executable..."
    if ./bin/RayTracerGPU --help &> /dev/null; then
        echo "✅ Executable test passed"
    else
        echo "⚠️  Executable created but may need additional setup"
    fi
fi
