# 📄 Scene File Format Specification

Complete reference for the raytracer's scene file format, including syntax, commands, and examples.

## File Format Overview

Scene files use a human-readable text format with one command per line. Comments are supported using `#` character.

### Basic Structure

```
# Scene file example
camera 0 0 5  0 0 0  0 1 0  45.0 1.5
background 0.5 0.7 1.0

material red_plastic lambertian 0.8 0.2 0.2
sphere 0 0 0  1.0  red_plastic

point_light 5 5 5  1.0 1.0 1.0
```

## Command Reference

### Camera Configuration

#### `camera` - Define Camera Parameters
```
camera pos_x pos_y pos_z target_x target_y target_z up_x up_y up_z fov aspect
```

**Parameters:**
- `pos_x pos_y pos_z` - Camera position in world space
- `target_x target_y target_z` - Point the camera looks at
- `up_x up_y up_z` - Camera up vector (usually `0 1 0`)
- `fov` - Field of view in degrees (typical: 20-90)
- `aspect` - Aspect ratio width/height (typical: 1.0-2.0)

**Example:**
```
camera 3.0 2.0 3.0  0.0 0.0 0.0  0.0 1.0 0.0  45.0 1.5
```

### Environment Settings

#### `background` - Set Background Color
```
background r g b
```

**Parameters:**
- `r g b` - RGB color values (0.0 to 1.0 range)

**Examples:**
```
background 0.5 0.7 1.0        # Sky blue
background 0.0 0.0 0.0        # Black
background 1.0 1.0 1.0        # White
```

#### `ambient` - Global Ambient Lighting
```
ambient r g b
```

**Parameters:**
- `r g b` - Ambient light color and intensity (0.0 to 1.0)

**Example:**
```
ambient 0.1 0.1 0.15          # Subtle blue ambient
```

### Material Definitions

#### `material` - Define Material Properties
```
material name type color_r color_g color_b [extra_params]
```

**Material Types:**

**Lambertian (Diffuse):**
```
material mat_name lambertian r g b
```

**Metal (Reflective):**
```
material mat_name metal r g b roughness
```
- `roughness` - Surface roughness (0.0 = perfect mirror, 1.0 = very rough)

**Dielectric (Glass/Transparent):**
```
material mat_name dielectric r g b ior
```
- `ior` - Index of refraction (glass ≈ 1.5, diamond ≈ 2.4)

**Emissive (Light-Emitting):**
```
material mat_name emissive r g b intensity
```
- `intensity` - Light emission intensity (1.0+ for bright lights)

**Glossy (Specular):**
```
material mat_name glossy r g b roughness
```
- `roughness` - Specular roughness (0.0 = mirror-like, 1.0 = diffuse-like)

**Subsurface (Scattering):**
```
material mat_name subsurface r g b scattering
```
- `scattering` - Subsurface scattering amount (0.0 to 1.0)

**Examples:**
```
material red_plastic lambertian 0.8 0.2 0.2
material polished_metal metal 0.9 0.9 0.9 0.1
material glass dielectric 0.9 0.9 0.9 1.5
material light_source emissive 1.0 1.0 0.8 5.0
material glossy_paint glossy 0.6 0.3 0.1 0.2
material marble subsurface 0.9 0.9 0.8 0.3
```

### Geometry Objects

#### `sphere` - Create Sphere
```
sphere center_x center_y center_z radius material_name
```

**Parameters:**
- `center_x center_y center_z` - Sphere center position
- `radius` - Sphere radius (must be positive)
- `material_name` - Previously defined material

**Examples:**
```
sphere 0 0 0  1.0  red_plastic
sphere -2 1 -1  0.5  glass
sphere 2 0.5 1  0.8  polished_metal
```

#### `plane` - Create Infinite Plane
```
plane point_x point_y point_z normal_x normal_y normal_z material_name
```

**Parameters:**
- `point_x point_y point_z` - Any point on the plane
- `normal_x normal_y normal_z` - Plane normal vector (will be normalized)
- `material_name` - Previously defined material

**Examples:**
```
plane 0 -1 0  0 1 0  ground           # Horizontal ground plane
plane 0 0 -5  0 0 1  back_wall        # Vertical back wall
plane -3 0 0  1 0 0  left_wall        # Vertical left wall
```

### 3D Model Loading

#### `load_obj` - Load OBJ File
```
load_obj filename.obj material_name
```

**Parameters:**
- `filename.obj` - Path to OBJ file (relative to scene file)
- `material_name` - Material to apply to all triangles

**Examples:**
```
load_obj temple.obj temple_stone
load_obj models/bunny.obj white_marble
load_obj assets/teapot.obj shiny_metal
```

**Notes:**
- OBJ files are automatically triangulated
- Camera is automatically positioned to view the entire model
- Supports comments in OBJ files
- Handles both triangular and quad faces

### Lighting System

#### `point_light` - Point Light Source
```
point_light pos_x pos_y pos_z intensity_r intensity_g intensity_b [radius]
```

**Parameters:**
- `pos_x pos_y pos_z` - Light position
- `intensity_r intensity_g intensity_b` - Light color and brightness
- `radius` - (Optional) Soft shadow radius for area-like effects

**Examples:**
```
point_light 5 8 5  2.0 2.0 1.8        # Bright white light
point_light -2 3 4  1.0 0.5 0.2 0.1   # Warm light with soft shadows
point_light 0 10 0  0.5 0.5 1.5       # Blue-tinted light
```

#### `spot_light` - Spot Light Source
```
spot_light pos_x pos_y pos_z dir_x dir_y dir_z intensity_r intensity_g intensity_b inner_angle outer_angle [radius]
```

**Parameters:**
- `pos_x pos_y pos_z` - Light position
- `dir_x dir_y dir_z` - Light direction (will be normalized)
- `intensity_r intensity_g intensity_b` - Light color and brightness
- `inner_angle` - Inner cone angle in degrees (full intensity)
- `outer_angle` - Outer cone angle in degrees (falloff to zero)
- `radius` - (Optional) Soft shadow radius

**Examples:**
```
spot_light 0 5 0  0 -1 0  2.0 2.0 2.0 15 30     # Downward spotlight
spot_light -5 3 5  1 -0.5 -1  1.5 1.0 0.8 20 45 0.05  # Angled spotlight with soft shadows
```

#### `area_light` - Area Light Source
```
area_light pos_x pos_y pos_z normal_x normal_y normal_z u_axis_x u_axis_y u_axis_z intensity_r intensity_g intensity_b width height [samples]
```

**Parameters:**
- `pos_x pos_y pos_z` - Light center position
- `normal_x normal_y normal_z` - Light surface normal
- `u_axis_x u_axis_y u_axis_z` - Light U-axis (defines orientation)
- `intensity_r intensity_g intensity_b` - Light color and brightness
- `width height` - Light dimensions
- `samples` - (Optional) Number of shadow samples for quality

**Examples:**
```
area_light 0 4 0  0 -1 0  1 0 0  0.8 0.8 1.0 2 1     # Horizontal rectangular light
area_light -2 2 0  1 0 0  0 1 0  1.2 0.8 0.6 1 2 16  # Vertical light with high quality shadows
```

## Comments and Documentation

### Comment Syntax

```
# Full line comment - everything after # is ignored
sphere 0 0 0 1.0 red_mat    # Inline comment
```

### Documentation Best Practices

```
# =================================
# Cornell Box Scene
# Classic computer graphics test scene
# =================================

# Camera setup - positioned to view entire scene
camera 278 278 -800  278 278 0  0 1 0  40.0 1.0

# Environment
background 0.0 0.0 0.0    # Black background
ambient 0.0 0.0 0.0       # No ambient light

# Materials
material white lambertian 0.73 0.73 0.73
material red lambertian 0.65 0.05 0.05
material green lambertian 0.12 0.45 0.15
material light emissive 15.0 15.0 15.0 1.0

# Scene geometry
plane 0 0 0    0 1 0  white      # Floor
plane 0 555 0  0 -1 0 white      # Ceiling
plane 0 0 0    0 0 1  white      # Back wall
plane 0 0 555  0 0 -1 white      # Front wall (camera side)
plane 555 0 0  -1 0 0 green      # Left wall (green)
plane -555 0 0 1 0 0  red        # Right wall (red)

# Lighting
area_light 278 554 279.5  0 -1 0  1 0 0  15 15 15  105 105 16
```

## Scene Organization

### File Structure Recommendations

```
scenes/
├── basic/
│   ├── cornell_box.scene
│   ├── three_spheres.scene
│   └── materials_demo.scene
├── complex/
│   ├── temple_interior.scene
│   ├── architectural.scene
│   └── lighting_showcase.scene
└── models/
    ├── temple.obj
    ├── bunny.obj
    └── teapot.obj
```

### Modular Scene Design

**Base Scene:**
```
# base_scene.scene
camera 0 2 5  0 0 0  0 1 0  45.0 1.5
background 0.5 0.7 1.0
ambient 0.1 0.1 0.15

# Standard materials
material white lambertian 0.8 0.8 0.8
material red lambertian 0.8 0.2 0.2
material metal metal 0.9 0.9 0.9 0.1

# Ground plane
plane 0 -1 0  0 1 0  white
```

**Extended Scene:**
```
# Include common elements by copying base patterns
# Add specific objects for this scene
sphere 0 0 0  1.0  red
sphere -2 0 1  0.8  metal
point_light 5 5 5  2.0 2.0 2.0
```

## Validation and Error Handling

### Common Errors

1. **Undefined Materials:**
   ```
   sphere 0 0 0 1.0 nonexistent_mat  # Error: material not defined
   ```

2. **Invalid Parameters:**
   ```
   camera 0 0 0  # Error: missing parameters
   sphere 0 0 0 -1.0 mat  # Error: negative radius
   ```

3. **File Not Found:**
   ```
   load_obj missing_file.obj mat  # Error: file doesn't exist
   ```

### Validation Checklist

- ✅ All materials defined before use
- ✅ Positive values for radius, intensity, etc.
- ✅ Normalized direction vectors for lights
- ✅ Valid file paths for OBJ files
- ✅ Proper parameter counts for all commands
- ✅ Comments don't break command parsing

## Performance Considerations

### Scene Complexity Guidelines

**Real-time Performance (30+ FPS):**
- Spheres: Unlimited (very fast)
- Triangles: < 500 for real-time
- Lights: < 10 for good performance
- Materials: Unlimited

**High-Quality Offline (16+ samples):**
- Triangles: < 10,000 
- Lights: < 50
- Complex material mixing acceptable

### Optimization Tips

1. **Use Spheres for Simple Geometry**: Much faster than triangulated equivalents
2. **Limit Triangle Count**: Biggest performance impact
3. **Reduce Light Count**: Each light requires shadow testing
4. **Optimize Camera Position**: Avoid extreme close-ups that waste rays
5. **Use Background Colors**: Instead of large geometry for environments

## Complete Example Scenes

### Basic Three Spheres
```
# Simple three sphere scene
camera 0 2 8  0 0 0  0 1 0  45.0 1.5
background 0.5 0.7 1.0
ambient 0.1 0.1 0.1

# Materials
material ground lambertian 0.5 0.5 0.5
material center lambertian 0.1 0.2 0.5
material left dielectric 1.0 1.0 1.0 1.5
material right metal 0.8 0.6 0.2 0.0

# Geometry
plane 0 -1 0  0 1 0  ground
sphere 0 0 0     1.0   center
sphere -2 0 0    1.0   left
sphere 2 0 0     1.0   right

# Lighting
point_light 5 5 5  2.0 2.0 2.0
```

### Material Showcase
```
# Comprehensive material demonstration
camera 0 3 12  0 1 0  0 1 0  35.0 1.5
background 0.2 0.2 0.3
ambient 0.05 0.05 0.1

# Material definitions
material ground lambertian 0.8 0.8 0.8
material lambertian_red lambertian 0.8 0.3 0.3
material metal_gold metal 0.8 0.6 0.2 0.1
material metal_rough metal 0.7 0.7 0.7 0.5
material glass dielectric 1.0 1.0 1.0 1.5
material light_source emissive 4.0 4.0 3.0 2.0
material glossy_blue glossy 0.2 0.3 0.8 0.2
material subsurface_white subsurface 0.9 0.9 0.9 0.4

# Ground
plane 0 0 0  0 1 0  ground

# Sphere arrangement
sphere -6 1 0   1.0  lambertian_red
sphere -3 1 0   1.0  metal_gold
sphere 0 1 0    1.0  metal_rough
sphere 3 1 0    1.0  glass
sphere 6 1 0    1.0  glossy_blue
sphere 0 4 -3   0.8  light_source
sphere -1.5 1 3 1.0  subsurface_white

# Lighting setup
point_light 0 8 0   1.5 1.5 1.5
point_light 8 5 8   1.0 0.8 0.6
spot_light -8 6 4  1 -1 -1  1.2 1.0 0.8  20 40
area_light 0 6 -6  0 -1 0.5  1 0 0  0.8 0.8 1.0  4 2
```
