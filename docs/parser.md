# 🔍 Parser System Guide

The raytracer uses a robust parsing system to load scene files and OBJ models. This guide covers the complete parsing architecture and how to extend it.

## Architecture Overview

The parser system consists of several components:

- **Scene Parser** (`src/parser.cpp`) - Main scene file parsing
- **OBJ Parser** (within scene parser) - 3D model loading  
- **Material Parser** - Material definition processing
- **Light Parser** - Lighting system configuration

## Scene File Parsing

### Core Parser Functions

```cpp
// Main entry point
bool parse_scene_file(const std::string& filename, Scene& scene);

// Helper functions
std::string strip_comments(const std::string& line);
std::vector<std::string> split_line(const std::string& line);
bool parse_vec3(const std::vector<std::string>& tokens, int start, Vec3& vec);
bool parse_color(const std::vector<std::string>& tokens, int start, Vec3& color);
```

### Scene Commands

The parser recognizes these scene file commands:

#### Camera Configuration
```
camera pos_x pos_y pos_z target_x target_y target_z up_x up_y up_z fov aspect
```
- **Position**: Camera location in world space
- **Target**: Point camera looks at
- **Up Vector**: Camera orientation (usually 0 1 0)
- **FOV**: Field of view in degrees
- **Aspect**: Width/height ratio

#### Environment Settings
```
background r g b        # Background color (0-1 range)
ambient r g b          # Global ambient lighting
```

#### Material Definitions
```
material name type color_r color_g color_b [extra_params]
```

**Material Types:**
- `lambertian r g b` - Diffuse materials
- `metal r g b roughness` - Reflective materials (roughness 0-1)
- `dielectric r g b ior` - Glass/transparent (IOR = index of refraction)
- `emissive r g b intensity` - Light-emitting materials
- `glossy r g b roughness` - Specular materials
- `subsurface r g b scattering` - Subsurface scattering

#### Geometry Objects
```
sphere center_x center_y center_z radius material_name
plane point_x point_y point_z normal_x normal_y normal_z material_name
```

#### Lighting
```
point_light pos_x pos_y pos_z intensity_r intensity_g intensity_b [radius]
spot_light pos_x pos_y pos_z dir_x dir_y dir_z intensity_r intensity_g intensity_b inner_angle outer_angle [radius]
area_light pos_x pos_y pos_z normal_x normal_y normal_z u_axis_x u_axis_y u_axis_z intensity_r intensity_g intensity_b width height [samples]
```

#### OBJ Loading
```
load_obj filename.obj material_name
```

### Comment Handling

The parser supports both inline and full-line comments:

```cpp
std::string strip_comments(const std::string& line) {
    size_t comment_pos = line.find('#');
    if (comment_pos != std::string::npos) {
        return line.substr(0, comment_pos);
    }
    return line;
}
```

**Examples:**
```
sphere 0 0 0 1.0 red_mat    # This is a red sphere
# This is a full line comment
material gold metal 1.0 0.8 0.2 0.1  # Gold material with low roughness
```

## OBJ File Parsing

### OBJ Parser Implementation

```cpp
bool parse_obj_file(const std::string& filename, Scene& scene, int material_id);
```

The OBJ parser handles:
- **Vertices** (`v x y z`)
- **Faces** (`f v1 v2 v3` or `f v1 v2 v3 v4`)
- **Comments** (`# comment text`)
- **Quad Triangulation** - Automatic conversion of quads to triangles

### Parsing Process

1. **File Loading**: Opens and validates OBJ file
2. **Vertex Parsing**: Extracts 3D vertex positions
3. **Face Parsing**: Processes face definitions with vertex indices
4. **Triangulation**: Converts quads and polygons to triangles
5. **Statistics**: Reports parsing results

### Face Processing

```cpp
struct FaceStatistics {
    int triangles = 0;
    int quads = 0; 
    int polygons = 0;
    int triangles_created = 0;
};

// Helper functions
bool parse_face_indices(const std::vector<std::string>& tokens, 
                       std::vector<int>& indices);
void triangulate_face(const std::vector<Vec3>& vertices,
                     const std::vector<int>& indices,
                     Scene& scene, int material_id,
                     FaceStatistics& stats);
```

### Triangulation Algorithm

For N-gons (N > 3), uses triangle fan tessellation:

```cpp
// Triangle fan: connect first vertex to all subsequent edges
for (size_t i = 1; i < indices.size() - 1; ++i) {
    scene.add_object(std::make_shared<Triangle>(
        vertices[indices[0] - 1],      // First vertex
        vertices[indices[i] - 1],      // Current vertex  
        vertices[indices[i + 1] - 1],  // Next vertex
        material_id
    ));
}
```

### Automatic Camera Positioning

When loading OBJ files, the parser automatically calculates optimal camera position:

```cpp
void setup_camera_from_bounds(const Vec3& min_bound, const Vec3& max_bound, 
                             Scene& scene) {
    Vec3 center = (min_bound + max_bound) * 0.5f;
    Vec3 size = max_bound - min_bound;
    float max_dim = std::max({size.x(), size.y(), size.z()});
    
    // Position camera to view entire object
    Vec3 camera_pos = center + Vec3(max_dim * 1.5f, max_dim * 1.2f, max_dim * 1.5f);
    scene.set_camera(Camera(camera_pos, center, Vec3(0, 1, 0), 45.0f, 1.5f));
}
```

## Error Handling

### Validation and Error Messages

The parser provides detailed error reporting:

```cpp
// Example error handling
if (tokens.size() < expected_size) {
    Logger::error("Invalid command '{}' at line {}: expected {} parameters, got {}", 
                  command, line_number, expected_size - 1, tokens.size() - 1);
    return false;
}

// Material validation
if (material_map.find(material_name) == material_map.end()) {
    Logger::error("Undefined material '{}' used at line {}", material_name, line_number);
    return false;
}
```

### Common Parsing Errors

1. **Missing Materials**: Using undefined material names
2. **Invalid Vertex Indices**: OBJ face references non-existent vertices
3. **Malformed Commands**: Wrong parameter counts or types
4. **File Not Found**: Missing OBJ or scene files
5. **Invalid Numbers**: Non-numeric values where numbers expected

## Extending the Parser

### Adding New Commands

To add a new scene command:

1. **Add Command Recognition**:
```cpp
if (command == "new_command") {
    return parse_new_command(tokens, scene, line_number);
}
```

2. **Implement Parser Function**:
```cpp
bool parse_new_command(const std::vector<std::string>& tokens, 
                       Scene& scene, int line_number) {
    if (tokens.size() < expected_params + 1) {
        Logger::error("Invalid new_command at line {}", line_number);
        return false;
    }
    
    // Parse parameters
    float param1 = std::stof(tokens[1]);
    // ... more parameters
    
    // Add to scene
    scene.add_new_object(param1, /* ... */);
    return true;
}
```

### Adding New Material Types

1. **Extend MaterialType enum** (in `include/material.h`)
2. **Update material parser**:
```cpp
else if (type == "new_material") {
    if (tokens.size() < 5) return false;
    
    Vec3 color;
    if (!parse_color(tokens, 2, color)) return false;
    
    float custom_param = std::stof(tokens[5]);
    material = Material(MaterialType::NEW_MATERIAL, color, custom_param);
}
```

3. **Update GPU shader** to handle new material type

## Performance Considerations

### Parsing Performance

- **File I/O**: Uses buffered reading for large files
- **String Processing**: Efficient tokenization and validation
- **Memory Management**: Minimal allocations during parsing
- **Error Handling**: Early validation to avoid expensive operations

### Optimization Tips

1. **Prevalidate Files**: Check file existence before parsing
2. **Batch Operations**: Group related parsing operations
3. **Memory Reservation**: Reserve vector capacity for known sizes
4. **Avoid String Copies**: Use string views where possible

## Testing the Parser

### Unit Tests

```cpp
// Example parser test
TEST(ParserTest, BasicSphereCommand) {
    Scene scene;
    std::vector<std::string> tokens = {"sphere", "0", "0", "0", "1.0", "test_mat"};
    
    // Setup test material
    Material test_material(MaterialType::LAMBERTIAN, Vec3(1, 0, 0));
    scene.add_material("test_mat", test_material);
    
    bool result = parse_sphere(tokens, scene, 1);
    EXPECT_TRUE(result);
    EXPECT_EQ(scene.objects.size(), 1);
}
```

### Integration Tests

```bash
# Test with real scene files
./build/bin/RayTracerGPU examples/cornell_box.scene -o test.ppm -w 100 -h 100 -s 1
```

## Best Practices

1. **Always Validate Input**: Check parameter counts and types
2. **Provide Clear Error Messages**: Include line numbers and context
3. **Support Comments**: Allow documentation in scene files
4. **Handle Edge Cases**: Empty files, malformed input, etc.
5. **Maintain Backwards Compatibility**: When extending the format
6. **Document Examples**: Provide working examples for all features
