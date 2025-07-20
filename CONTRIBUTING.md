# Contributing to Interactive GPU Raytracer

Thank you for your interest in contributing to this project! 

## Development Setup

1. **Prerequisites:**
   ```bash
   # Ubuntu/Debian
   sudo apt install cmake build-essential libglfw3-dev libglew-dev libpng-dev
   
   # macOS
   brew install cmake glfw glew libpng
   ```

2. **Build:**
   ```bash
   git clone <repository-url>
   cd raytracer_ai
   ./build.sh
   ```

## Code Style

- Use consistent C++ style with 4-space indentation
- Follow RAII principles for resource management
- Add comments for complex algorithms
- Include error handling for OpenGL operations

## Testing

Before submitting:
```bash
./build.sh          # Builds and tests both CPU and GPU versions
./benchmark.sh      # Performance testing
```

## Commit Guidelines

- Use descriptive commit messages
- Group related changes into single commits
- Test thoroughly before committing
- Update documentation for new features

## Feature Ideas

- [ ] Denoising algorithms
- [ ] Volumetric rendering
- [ ] HDR environment mapping
- [ ] Live material editing
- [ ] Scene hot-reloading
- [ ] Multi-GPU support
- [ ] OptiX backend
- [ ] Web interface

## Bug Reports

Please include:
- System specifications (OS, GPU, drivers)
- Steps to reproduce
- Expected vs actual behavior
- Console output and logs

## Performance Optimization

When optimizing:
- Profile first, optimize second  
- Maintain readable code
- Document performance characteristics
- Test on multiple hardware configurations
