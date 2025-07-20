# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2025-07-20

### Added
- **Dual-mode GPU raytracer**: Interactive windowed mode OR headless file output mode
- **Real-time ray tracing**: WASD + mouse camera controls at 30-60 FPS
- **Headless GPU rendering**: Fast file output without display overhead using `-o` flag
- **Multi-threaded CPU raytracer**: High-quality offline rendering with progress tracking
- **BVH acceleration structure**: Fast ray-triangle intersection for complex scenes
- **Physically-based materials**:
  - Lambertian (diffuse) materials with configurable albedo
  - Metal materials with adjustable roughness
  - Dielectric materials with refractive index
- **Advanced lighting system**:
  - Point lights with position and intensity
  - Spot lights with direction, angle, and falloff
  - Area/plane lights for soft shadows
  - Ambient lighting for global illumination
- **Scene file parser**: Custom scene format supporting materials, lights, and objects
- **Cross-platform build system**: CMake with automatic dependency detection
- **Performance tools**: Benchmarking script and FPS monitoring
- **Comprehensive documentation**: README, implementation guide, and examples

### Technical Features
- OpenGL 4.3 compute shaders for GPU ray tracing
- GLFW for windowing and input handling  
- GLEW for OpenGL extension loading
- Multi-format image output (PPM, PNG)
- Real-time performance statistics
- Frame capture functionality
- Error handling and debugging support

### Performance
- **CPU Mode**: 100-1000 samples per pixel, high quality
- **GPU Interactive**: 1-16 samples per pixel, real-time
- **GPU Headless**: Fast rendering without display overhead

### Usage Examples
```bash
# CPU high-quality rendering
./build/bin/RayTracer examples/showcase.scene -o render.ppm -s 200 -w 1920 -h 1080

# GPU interactive mode
./build/bin/RayTracerGPU examples/realtime.scene -w 1200 -h 800

# GPU headless rendering
./build/bin/RayTracerGPU examples/cornell_box.scene -o output.ppm -s 16 -w 800 -h 600
```

## [Unreleased]

### Planned Features
- Denoising algorithms for cleaner images at lower sample counts
- Volumetric rendering for atmospheric effects
- HDR environment mapping
- Live material editing during interactive sessions
- Scene hot-reloading for rapid development
- Multi-GPU support for faster rendering
- OptiX backend for NVIDIA RTX acceleration
- Web-based interface for cloud rendering

### Known Issues
- PNG output falls back to PPM format (libpng integration needed)
- Limited to triangle and sphere primitives
- No texture mapping support yet
- Single-threaded GPU raytracer (could use multiple compute dispatches)

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for development guidelines and how to contribute to this project.
