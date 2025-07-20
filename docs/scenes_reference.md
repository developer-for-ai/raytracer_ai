# Scene Files Reference

This document describes the curated scene files included with the raytracer.

## Available Scene Files

### Basic Demos
- **`cornell_box.scene`** - Classic Cornell Box reference scene with colored walls
- **`simple_temple_view.scene`** - Simple OBJ loading demo using temple_simple.obj

### Material & Lighting Showcases
- **`materials_showcase.scene`** - Comprehensive demonstration of all material types (lambertian, metal, dielectric, emissive, glossy, subsurface)
- **`lighting_demo.scene`** - Demonstration of various lighting types (point, spot, area, ambient)
- **`showcase.scene`** - Complete feature showcase with complex materials, lighting, and BVH performance

### Temple Scenes
- **`temple_scene.scene`** - Indian temple scene with OBJ geometry, multiple materials, and advanced lighting
- **`temple_sacred.scene`** - Atmospheric temple scene with mystical lighting

## OBJ Files
- **`temple.obj`** - Indian temple with interior access, proper doorways, and walkable spaces
- **`temple_simple.obj`** - Simplified temple structure (24 triangles)

## Geometry Features

### Temple Architecture
The temple scenes demonstrate advanced architectural geometry:
- **Interior Access**: Walk through the front doorway into the temple
- **Proper Doorways**: Realistic entrance openings in wall geometry  
- **Multi-Level Structure**: Base platform, main hall, and shikhara (spire)
- **Quad Triangulation**: Automatic conversion of quad faces to triangles

## Usage Examples

```bash
# Temple with atmospheric lighting
./build/bin/RayTracerGPU examples/temple_sacred.scene

# Complex temple scene with full materials
./build/bin/RayTracerGPU examples/temple_scene.scene

# Simple OBJ loading with automatic camera
./build/bin/RayTracerGPU examples/temple.obj

# Interactive exploration (walkable temple)
./build/bin/RayTracerGPU examples/temple.obj

# Material showcase
./build/bin/RayTracerGPU examples/materials_showcase.scene
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
- `spot_light pos_x pos_y pos_z dir_x dir_y dir_z intensity_r intensity_g intensity_b inner_angle outer_angle [radius]`
- `area_light pos_x pos_y pos_z normal_x normal_y normal_z u_axis_x u_axis_y u_axis_z intensity_r intensity_g intensity_b width height [samples]`
- `load_obj filename.obj material_name`

Material types: `lambertian`, `metal`, `dielectric`, `emissive`, `glossy`, `subsurface`

### Comments
Both scene files and OBJ files support inline comments:
```
sphere 0 0 0 1.0 red_mat    # This is a red sphere
# This is a full line comment  
plane 0 -1 0  0 1 0  ground_mat  # Ground plane
```
