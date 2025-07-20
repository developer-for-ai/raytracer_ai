# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- **Advanced Noise Reduction**: Comprehensive denoising system for cleaner images
  - Stratified/jittered sampling for better pixel coverage
  - Cosine-weighted importance sampling for Lambertian materials  
  - Temporal accumulation with automatic reset on camera movement
  - Improved random number generation (Xorshift128+ with PCG hashing)
  - Russian roulette termination for energy conservation
  - Enhanced tone mapping and gamma correction

### Improved
- **Interactive Quality**: Increased default samples to 4 per frame and ray depth to 10
- **Material Rendering**: Better dielectric materials with Schlick approximation
- **GPU Performance**: Optimized shader buffer layout and memory usage
- **Real-time Experience**: Progressive quality improvement when camera is stationary

### Technical Details
- Added accumulation texture buffer for temporal denoising
- Implemented blue noise-like sampling distribution
- Enhanced material scattering with importance sampling
- Automatic accumulation buffer reset on camera movement
- Better convergence characteristics at low sample counts

### Planned Features
- Advanced denoising algorithms for cleaner images at lower sample counts
- Volumetric rendering for atmospheric effects  
- HDR environment mapping for realistic lighting
- Live material editing during interactive sessions
- Scene hot-reloading for rapid development
- Multi-GPU support for faster rendering
- OptiX backend for NVIDIA RTX acceleration
- Web-based interface for cloud rendering

## [1.1.1] - 2025-07-20

### Added
- **Comprehensive Technical Documentation**: Created new `docs/` directory with detailed guides
  - **Parser System Guide** - Complete scene file and OBJ parsing documentation
  - **GPU Architecture Guide** - OpenGL compute shader implementation details
  - **Material System Guide** - Advanced material types with examples and implementation
  - **Lighting System Guide** - Complete lighting pipeline with soft shadows and multiple light types
  - **Scene Format Specification** - Complete reference for scene file syntax and commands
  - **Build System Guide** - CMake configuration, dependencies, and cross-platform building
  - **Intersection Algorithms Guide** - Mathematical foundations and GPU implementations
- **Enhanced Documentation Structure**: Clear organization for developers and advanced users
- **Updated Main README**: Added links to technical documentation directory

### Enhanced
- **Developer Experience**: Comprehensive guides for extending and modifying the raytracer
- **Code Documentation**: Detailed implementation examples with working code snippets
- **Educational Value**: Mathematical foundations and algorithmic explanations

## [1.1.0] - 2025-07-20

### Added
- **Enhanced Comment Support**: Robust inline comment handling
  - Comments now allowed anywhere in OBJ and scene files
  - Improved parser with `strip_comments` helper function
  - Better error handling and file documentation support
- **Parser Code Refactoring**: Eliminated duplicate code with helper functions
  - New helper functions: `parse_face_indices`, `triangulate_face`, `update_bounds`, `setup_camera_from_bounds`
  - Centralized face statistics tracking with `FaceStatistics` struct
  - Reduced code duplication and improved maintainability

### Enhanced
- **OBJ Parsing**: Improved statistics logging and quad triangulation
  - Clear statistics: vertices, faces (triangles/quads/polygons), triangles created
  - Robust vertex index validation and error handling
  - Proper face winding for consistent rendering
- **Scene Loading**: Better error messages and validation
  - More descriptive error messages with line numbers
  - Improved material validation and undefined material detection
- **Temple Interior Access**: Redesigned temple OBJ with proper doorways
  - Front entrance doorway for interior exploration
  - Walkable interior space with proper floor and wall geometry
  - Improved temple architecture with base platform, main hall, and shikhara roof

### Temple Scenes
- **`temple_scene.scene`** - Indian temple scene with OBJ geometry, multiple materials, and advanced lighting  
- **`temple_sacred.scene`** - Atmospheric temple scene with mystical lighting

### Removed
- **Scene file cleanup**: Removed redundant and obsolete scene files
  - Removed duplicate temple scenes and test files
  - Removed basic scene duplicates 
  - Kept only essential, unique scene files
- **Documentation cleanup**: Removed obsolete documentation files
- **Updated all references**: Fixed all documentation to reference only existing files

### Enhanced
- **Documentation accuracy**: All examples and references now point to existing files
- **Streamlined project**: Reduced file count while maintaining all functionality
- **Temple OBJ models**: Updated temple architecture with proper interior access

## [1.0.2] - 2025-07-20

### Added
- **Arrow key camera controls**: Use arrow keys to look around as alternative to mouse
- **Enhanced vertical movement**: Space and Shift now move 2x faster for quicker vertical navigation
- Arrow key look controls work even without mouse capture for precise camera adjustments
- Updated in-app and documentation controls to reflect new input options

### Enhanced
- **Improved navigation experience**: More control options for different user preferences
- **Faster vertical movement**: Space/Shift movement speed increased with configurable multiplier

## [1.0.1] - 2025-07-20

### Fixed
- **Camera orientation bug**: Fixed scene disappearing after mouse movement due to incorrect yaw/pitch initialization
- Mouse look now properly initializes from actual camera orientation instead of hardcoded values

### Added  
- **Camera reset hotkey**: Press 'R' to reset camera to initial position and orientation
- Better camera recovery when the scene is lost during navigation

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

## Contributing

See [docs/contributing.md](docs/contributing.md) for development guidelines and how to contribute to this project.
