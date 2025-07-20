# Scene Files Reference

This document describes the curated scene files included with the raytracer.

## Available Scene Files

### Basic Demos
- **`cornell_box.scene`** - Classic Cornell Box reference scene with colored walls
- **`tetrahedron_view.scene`** - Simple OBJ loading demo using tetrahedron.obj

### Material & Lighting Showcases
- **`materials_showcase.scene`** - Comprehensive demonstration of all material types (lambertian, metal, dielectric, emissive, glossy, subsurface)
- **`lighting_demo.scene`** - Demonstration of various lighting types (point, spot, area, ambient)
- **`showcase.scene`** - Complete feature showcase with complex materials, lighting, and BVH performance

### Complex Scenes
- **`temple_final_complete.scene`** - Indian temple scene with OBJ geometry, multiple materials, and advanced lighting. Uses `load_obj` command for triangle mesh architecture.

## OBJ Files
- **`tetrahedron.obj`** - Simple 4-triangle tetrahedron for testing
- **`indian_temple.obj`** - Complex temple architecture with 294 triangles
- **`triangle_grid.obj`** - Performance test grid (12 triangles)
- **`large_triangle_grid.obj`** - Performance test grid (108 triangles)  
- **`massive_triangle_grid.obj`** - Performance test grid (432 triangles)

## Usage Examples

```bash
# Basic material showcase
./build/bin/RayTracerGPU examples/materials_showcase.scene

# Complex temple scene with triangles
./build/bin/RayTracerGPU examples/temple_final_complete.scene

# Simple OBJ loading
./build/bin/RayTracerGPU examples/tetrahedron_view.scene

# Interactive exploration (temple)
./build/bin/RayTracerGPU examples/indian_temple.obj

# Performance testing
./build/bin/RayTracerGPU examples/massive_triangle_grid.obj
```

## Scene File Format

Scene files support the following commands:
- `camera pos_x pos_y pos_z target_x target_y target_z up_x up_y up_z fov aspect`
- `background r g b`
- `ambient r g b`
- `material name type color_r color_g color_b [extra_params]`
- `sphere center_x center_y center_z radius material_name`
- `plane point_x point_y point_z normal_x normal_y normal_z material_name`
- `point_light pos_x pos_y pos_z intensity_r intensity_g intensity_b [radius]`
- `load_obj filename.obj material_name`

Material types: `lambertian`, `metal`, `dielectric`, `emissive`, `glossy`, `subsurface`
