# 🏛️ Indian Temple Scene Collection

This collection provides immersive Indian-style temple scenes for exploration and rendering in the GPU raytracer.

## 🎮 Interactive Temple Exploration

### Primary Recommendation: Indian Temple OBJ
```bash
# Best temple exploration experience (294 triangles, automatic camera positioning)
./build/bin/RayTracerGPU examples/indian_temple.obj
```

**Features:**
- Authentic Indian temple architecture generated procedurally
- 294 triangles with pillars, altar, interior spaces, and decorative elements
- **NEW: Automatic camera positioning** - no more "empty screen" issues!
- Optimized for real-time exploration at 15+ FPS
- Use WASD keys to walk through the temple interior
- Mouse/arrow keys to look around and explore details

### Alternative: Complete Scene with Decorations
```bash
# Temple with full decorations, lighting, and materials
./build/bin/RayTracerGPU examples/temple_final_complete.scene
```

## 🎨 Available Temple Scene

### **temple_final_complete.scene** - Complete Scene with OBJ Architecture
```bash
./build/bin/RayTracerGPU examples/temple_final_complete.scene
```
- **Purpose**: Complete temple scene with OBJ geometry and decorations  
- **Features**: Uses `load_obj` command for 294-triangle architecture, rich materials, atmospheric lighting
- **Performance**: Optimized for exploration at 15+ FPS
- **Best for**: Comprehensive temple experience with full triangle mesh geometry

## 🏗️ Temple Architecture Features

### Architectural Elements
- **Pillars**: Classical Indian temple columns with decorative capitals
- **Altar**: Multi-tiered sacred platform with ceremonial objects
- **Floor**: Large plane surface for walking throughout temple
- **Walls**: Temple boundaries and interior divisions
- **Entrance**: Grand entrance with threshold and decorative elements

### Materials Used
- **temple_stone/temple_walls**: Sandstone temple architecture (lambertian)
- **sacred_marble/altar_marble**: White marble for sacred areas (lambertian)
- **gold_accent/gold_temple**: Polished gold decorative elements (metal)
- **sacred_crystal**: Clear crystals for mystical elements (dielectric)
- **bronze_detail**: Bronze ceremonial objects (metal)
- **torch_flame/divine_glow**: Light-emitting materials (emissive)

### Lighting Design
- **Sunlight**: Primary illumination streaming through entrance
- **Torch lights**: Warm flickering light from temple torches
- **Mystical glows**: Colored lighting for sacred/mystical atmosphere
- **Altar lighting**: Focused illumination on sacred altar area
- **Ambient**: Soft overall illumination for good visibility

## 🎯 Usage Recommendations

### For Interactive Exploration
1. **Start with OBJ**: `./build/bin/RayTracerGPU examples/indian_temple.obj`
2. **Use WASD**: Walk through the temple spaces
3. **Look around**: Mouse or arrow keys to explore details
4. **Try different angles**: Position camera to see architectural details

### For High-Quality Renders
```bash
# Comprehensive temple scene
./build/bin/RayTracerGPU examples/temple_immersive.scene -o temple.ppm -w 1920 -h 1440 -s 8

# Interior focus
./build/bin/RayTracerGPU examples/temple_walkable_interior.scene -o interior.ppm -w 1600 -h 1200 -s 12

# Architecture focus  
./build/bin/RayTracerGPU examples/indian_temple.obj -o architecture.ppm -w 1600 -h 1200 -s 16
```

### For Performance Testing
- The temple scenes are excellent for testing triangle-heavy performance
- 294 triangles in the OBJ provides good triangle intersection testing
- Performance optimizations kick in automatically with triangle counts > 100

## 🔧 Technical Details

### Performance Optimizations
- **Adaptive sampling**: Reduced samples for triangle-heavy scenes
- **Early ray termination**: Aggressive cutoffs for performance
- **Stride-based triangle testing**: Test subset of triangles for distant rays
- **Shadow optimization**: Shadows disabled for triangle-heavy scenes
- **Material simplification**: Simplified materials for better performance

### New Features Added
- **load_obj command**: Load OBJ files within scene files with custom materials
- **Automatic camera positioning**: OBJ files loaded directly now automatically position camera
- **Proper bounds calculation**: Camera positioned based on model dimensions
- **Material assignment**: OBJ geometry can use any defined material from scene

### Scene Generation
- **Authentic temple architecture**: Complex 294-triangle structure with pillars and altar
- **Authentic proportions**: Based on classical Indian temple architecture
- **Modular design**: Easy to modify individual elements
- **Performance aware**: Designed to maintain 15+ FPS with 200+ triangles

## 🌟 Exploration Tips

### Navigation
- **Start at entrance**: Position camera outside looking in
- **Walk slowly**: Use gradual movements to appreciate details
- **Look up**: Don't forget to explore ceiling and upper architectural elements
- **Visit altar**: Central altar area has the most detailed elements
- **Check alcoves**: Side areas have hidden details and atmospheric lighting

### Visual Elements to Notice
- **Material variety**: Different surfaces (stone, marble, gold, crystal) 
- **Lighting effects**: Torch flames, mystical glows, sunlight streaming
- **Architectural details**: Pillar capitals, altar tiers, decorative elements
- **Atmospheric elements**: Floating orbs, incense effects, sacred pools
- **Depth and scale**: Background elements creating sense of space

The temple scenes demonstrate the full capabilities of the GPU raytracer with complex geometry, rich materials, advanced lighting, and real-time performance optimization.
