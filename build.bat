@echo off
REM Build script for Interactive GPU Ray Tracer - Windows
echo 🚀 Building Interactive GPU Ray Tracer for Windows...
echo ====================================================

REM Check if vcpkg is available
where vcpkg >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo ❌ vcpkg not found in PATH. Please install vcpkg and add it to PATH.
    echo    Visit: https://github.com/Microsoft/vcpkg
    echo    Required packages: glfw3, glew, opengl
    echo.
    echo    Install with: vcpkg install glfw3 glew opengl --triplet x64-windows
    pause
    exit /b 1
)

REM Check for required packages
echo Checking vcpkg packages...
vcpkg list | findstr "glfw3" >nul
if %ERRORLEVEL% NEQ 0 (
    echo ❌ glfw3 not found. Installing required packages...
    vcpkg install glfw3 glew opengl --triplet x64-windows
) else (
    echo ✅ Graphics dependencies found
)

REM Create build directory
echo 🔧 Setting up build environment...
if not exist "build" mkdir build
cd build

REM Configure with CMake
echo 📦 Configuring project with CMake...
cmake .. -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows

if %ERRORLEVEL% NEQ 0 (
    echo ❌ CMake configuration failed
    pause
    exit /b 1
)

REM Build project
echo 🔨 Building project...
cmake --build . --config Release

if %ERRORLEVEL% NEQ 0 (
    echo ❌ Build failed
    pause
    exit /b 1
)

echo ✅ Build completed successfully!
echo 📁 Executable: build\bin\Release\RayTracerGPU.exe
echo.
echo 🎮 To run: build\bin\Release\RayTracerGPU.exe examples\showcase.scene
pause
