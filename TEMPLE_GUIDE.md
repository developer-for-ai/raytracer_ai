# 🏛️ Temple Scene Guide

This guide covers the temple scenes and 3D models included with the raytracer, showcasing advanced architectural geometry and interactive exploration.

## Temple Architecture

The temple scenes demonstrate authentic Indian temple architecture with modern ray tracing techniques.

### Architectural Elements

#### **Garbhagriha (Sanctum)**
- The innermost chamber where the main deity resides
- Accessible through the front doorway
- Interior walls with proper material assignments
- Sacred space with mystical lighting

#### **Mandapa (Assembly Hall)**
- Open hall for gathering and ceremonies
- Walkable interior space with proper floor geometry
- Multiple material zones (stone, marble, decorative)
- Natural and artificial lighting

#### **Shikhara (Spire)**
- Towering spire representing the cosmic mountain
- Complex triangulated geometry
- Gradient materials from base to peak
- Dramatic lighting showcase

#### **Gopuram (Entrance Tower)**
- Ornate entrance structure
- Multiple levels with detailed geometry
- Rich material variety (stone, gold, decorative elements)
- Prominent lighting features

## Available Temple Files

### 3D Models (.obj)
- **`temple.obj`** - Complete temple with interior access and proper doorways
- **`temple_simple.obj`** - Simplified version for performance testing (24 triangles)

### Scene Files (.scene)
- **`temple_scene.scene`** - Full temple scene with OBJ geometry, multiple materials, and advanced lighting
- **`temple_sacred.scene`** - Atmospheric temple scene with mystical lighting focused on sacred elements

## Interactive Exploration

### How to Explore the Temple

1. **Start the Interactive Viewer:**
   ```bash
   ./build/bin/RayTracerGPU examples/temple.obj
   ```

2. **Navigation Controls:**
   - **WASD** - Move forward/left/backward/right
   - **Mouse** - Look around (click to capture mouse)
   - **Space/Shift** - Move up/down
   - **R** - Reset camera to starting position

3. **Exploration Tips:**
   - **Enter through the front doorway** - Look for the opening in the front wall
   - **Walk into the interior** - The temple has a proper walkable floor
   - **Explore different rooms** - Move around to see various architectural details
   - **Try different lighting angles** - Use mouse look to see how materials respond to light

### Temple Scene Variations

#### **temple_scene.scene** - Complete Architectural Showcase
```bash
./build/bin/RayTracerGPU examples/temple_scene.scene
```
Features:
- Full OBJ temple geometry with decorative sphere arrangements
- Multiple material types (temple_stone, marble, gold, decorative)
- Advanced lighting setup with warm interior and cool exterior lighting
- Emissive materials for glowing architectural elements

#### **temple_sacred.scene** - Mystical Atmosphere
```bash
./build/bin/RayTracerGPU examples/temple_sacred.scene
```
Features:
- Atmospheric lighting focused on spiritual ambiance
- Enhanced material properties for mystical effects
- Specialized lighting for sacred chamber illumination
- Soft shadows and ambient lighting for meditative atmosphere

## Technical Implementation

### Geometry Features
- **Quad Triangulation:** All quad faces automatically converted to triangles
- **Interior Access:** Proper doorway geometry allows walking inside
- **Face Winding:** Consistent face normals for proper lighting
- **Material Zones:** Different materials for walls, floors, decorative elements

### Performance Characteristics
- **294 triangles** in the complete temple.obj
- **15-22 FPS** real-time performance on modern GPUs
- **Optimized for triangle-heavy scenes** with efficient GPU memory usage
- **BVH acceleration** for fast ray-triangle intersection

### Materials Used
```
temple_stone   - Main structural material (gray stone)
marble         - Interior decorative surfaces (white)
gold           - Ornamental elements and deity fixtures
decorative     - Colorful accent materials
emissive       - Light-emitting architectural elements
```

## High-Quality Rendering

### Offline Rendering Commands
```bash
# High-quality temple render
./build/bin/RayTracerGPU examples/temple_scene.scene -o temple_hq.ppm -w 1920 -h 1080 -s 32

# Sacred atmosphere render
./build/bin/RayTracerGPU examples/temple_sacred.scene -o temple_sacred.ppm -w 1600 -h 1200 -s 16

# Direct OBJ rendering with auto-camera
./build/bin/RayTracerGPU examples/temple.obj -o temple_direct.ppm -w 1920 -h 1080 -s 24
```

## Cultural Context

### Indian Temple Architecture
The temple design incorporates authentic elements of traditional Indian temple architecture:

- **Sacred Geometry:** Proportions based on ancient architectural principles
- **Directional Significance:** Proper orientation for spiritual practices
- **Material Symbolism:** Stone for permanence, marble for purity, gold for divine energy
- **Interior-Exterior Flow:** Transition from worldly exterior to sacred interior space

### Lighting Symbolism
- **Warm Interior Light:** Represents divine presence and spiritual illumination
- **Cool Exterior Light:** Represents the material world and earthly concerns
- **Gradient Transitions:** Symbolic journey from material to spiritual realms

## Development and Customization

### Adding Elements to Temple Scenes
The temple scenes support the full range of raytracer features:

```
# Add decorative spheres
sphere -1.5 1 1.5  0.3  gold

# Add atmospheric lighting
point_light 0 3 0  1.2 1.0 0.8  0.1
```

### Creating Your Own Temple Scene
1. Load the temple OBJ: `load_obj temple.obj temple_stone`
2. Add decorative elements (spheres)
3. Set up appropriate materials and lighting
4. Position camera for optimal viewing angle

The temple scenes demonstrate the full capabilities of the raytracer while providing an immersive cultural and architectural experience.
