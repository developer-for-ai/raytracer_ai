# 🎨 Material System Guide

Comprehensive guide to the raytracer's physically-based material system, including implementation details and usage examples.

## Overview

The material system implements physically-based rendering (PBR) principles with 6 distinct material types, each simulating different physical properties of light interaction.

### Material Types

1. **Lambertian** - Perfect diffuse reflection
2. **Metal** - Reflective surfaces with controllable roughness
3. **Dielectric** - Transparent materials with refraction
4. **Emissive** - Light-emitting surfaces
5. **Glossy** - Specular reflection with adjustable properties
6. **Subsurface** - Materials with subsurface scattering

## Material Implementation

### Material Structure

```cpp
enum class MaterialType {
    LAMBERTIAN = 0,
    METAL = 1,
    DIELECTRIC = 2,
    EMISSIVE = 3,
    GLOSSY = 4,
    SUBSURFACE = 5
};

struct Material {
    Vec3 color;           // Base color (albedo)
    MaterialType type;    // Material type
    float param1;         // Type-specific parameter 1
    float param2;         // Type-specific parameter 2
    float param3;         // Type-specific parameter 3
};
```

### GPU Material Structure

```glsl
struct GPUMaterial {
    vec3 color;
    int type;
    float param1;
    float param2;
    float param3;
    float _padding;
};
```

## Material Types in Detail

### 1. Lambertian Materials

**Physics:** Perfect diffuse reflection following Lambert's cosine law.

**Properties:**
- Uniform scattering in all directions
- Color determined by surface albedo
- No specular reflection

**Parameters:**
- `color` - Surface albedo (0.0 to 1.0 for each RGB component)

**Scene File Syntax:**
```
material name lambertian r g b
```

**Examples:**
```
material red_paint lambertian 0.8 0.2 0.2
material white_paper lambertian 0.9 0.9 0.9
material dark_wood lambertian 0.3 0.2 0.1
```

**Implementation:**
```glsl
bool scatter_lambertian(inout Ray ray, HitRecord hit, GPUMaterial mat, inout vec3 attenuation) {
    vec3 scatter_direction = hit.normal + random_unit_vector();
    
    // Catch degenerate scatter direction
    if (near_zero(scatter_direction)) {
        scatter_direction = hit.normal;
    }
    
    ray.origin = hit.point;
    ray.direction = normalize(scatter_direction);
    attenuation *= mat.color;
    
    return true;
}
```

### 2. Metal Materials

**Physics:** Specular reflection with microsurface roughness modeling.

**Properties:**
- Mirror-like reflection modified by surface roughness
- Roughness controls reflection sharpness
- Conductive materials (no transmission)

**Parameters:**
- `color` - Metal tint/reflectance
- `param1` - Roughness (0.0 = perfect mirror, 1.0 = very rough)

**Scene File Syntax:**
```
material name metal r g b roughness
```

**Examples:**
```
material polished_silver metal 0.95 0.95 0.95 0.05
material brushed_aluminum metal 0.8 0.85 0.9 0.3
material old_copper metal 0.7 0.4 0.3 0.6
material gold metal 1.0 0.8 0.3 0.1
```

**Implementation:**
```glsl
bool scatter_metal(inout Ray ray, HitRecord hit, GPUMaterial mat, inout vec3 attenuation) {
    vec3 reflected = reflect(normalize(ray.direction), hit.normal);
    vec3 scattered = reflected + mat.param1 * random_in_unit_sphere();
    
    ray.origin = hit.point;
    ray.direction = normalize(scattered);
    attenuation *= mat.color;
    
    return dot(scattered, hit.normal) > 0.0;
}
```

### 3. Dielectric Materials

**Physics:** Transparent materials with refraction and reflection (Fresnel equations).

**Properties:**
- Refraction based on index of refraction (IOR)
- Fresnel reflection varies with viewing angle
- Total internal reflection at critical angles

**Parameters:**
- `color` - Transmission tint (usually white or slightly colored)
- `param1` - Index of refraction (IOR)

**Scene File Syntax:**
```
material name dielectric r g b ior
```

**Common IOR Values:**
- Air: 1.0
- Water: 1.33
- Glass: 1.5
- Diamond: 2.4
- Sapphire: 1.77

**Examples:**
```
material clear_glass dielectric 0.95 0.95 0.95 1.5
material water dielectric 0.9 0.95 1.0 1.33
material diamond dielectric 1.0 1.0 1.0 2.4
material colored_glass dielectric 0.8 0.9 0.7 1.52
```

**Implementation:**
```glsl
bool scatter_dielectric(inout Ray ray, HitRecord hit, GPUMaterial mat, inout vec3 attenuation) {
    attenuation *= mat.color;
    
    float refraction_ratio = hit.front_face ? (1.0 / mat.param1) : mat.param1;
    
    vec3 unit_direction = normalize(ray.direction);
    float cos_theta = min(dot(-unit_direction, hit.normal), 1.0);
    float sin_theta = sqrt(1.0 - cos_theta * cos_theta);
    
    bool cannot_refract = refraction_ratio * sin_theta > 1.0;
    
    vec3 direction;
    if (cannot_refract || reflectance(cos_theta, refraction_ratio) > random_float()) {
        direction = reflect(unit_direction, hit.normal);
    } else {
        direction = refract(unit_direction, hit.normal, refraction_ratio);
    }
    
    ray.origin = hit.point;
    ray.direction = direction;
    
    return true;
}

float reflectance(float cosine, float ref_idx) {
    // Schlick's approximation for reflectance
    float r0 = (1.0 - ref_idx) / (1.0 + ref_idx);
    r0 = r0 * r0;
    return r0 + (1.0 - r0) * pow((1.0 - cosine), 5.0);
}
```

### 4. Emissive Materials

**Physics:** Light-emitting surfaces that contribute to scene illumination.

**Properties:**
- Emit light according to specified color and intensity
- Do not reflect incoming light (purely emissive)
- Contribute to global illumination

**Parameters:**
- `color` - Emission color
- `param1` - Emission intensity multiplier

**Scene File Syntax:**
```
material name emissive r g b intensity
```

**Examples:**
```
material ceiling_light emissive 1.0 1.0 0.9 5.0
material neon_red emissive 1.0 0.1 0.1 3.0
material led_blue emissive 0.2 0.4 1.0 8.0
material firefly emissive 0.8 1.0 0.4 2.0
```

**Implementation:**
```glsl
bool scatter_emissive(inout Ray ray, HitRecord hit, GPUMaterial mat, inout vec3 attenuation) {
    // Emissive materials absorb all incoming light
    return false;
}

vec3 emit_light(GPUMaterial mat) {
    return mat.color * mat.param1;
}
```

### 5. Glossy Materials

**Physics:** Combination of diffuse and specular reflection with controllable balance.

**Properties:**
- Smooth specular highlights
- Controllable roughness for highlight sharpness
- Maintains some diffuse component

**Parameters:**
- `color` - Base material color
- `param1` - Roughness (0.0 = mirror-like, 1.0 = diffuse-like)

**Scene File Syntax:**
```
material name glossy r g b roughness
```

**Examples:**
```
material car_paint glossy 0.8 0.2 0.2 0.1
material ceramic glossy 0.9 0.9 0.9 0.15
material plastic glossy 0.3 0.6 0.9 0.25
material varnished_wood glossy 0.6 0.4 0.2 0.2
```

**Implementation:**
```glsl
bool scatter_glossy(inout Ray ray, HitRecord hit, GPUMaterial mat, inout vec3 attenuation) {
    // Mix between specular and diffuse based on roughness
    float specular_chance = 0.8 * (1.0 - mat.param1);
    
    if (random_float() < specular_chance) {
        // Specular reflection
        vec3 reflected = reflect(normalize(ray.direction), hit.normal);
        vec3 scattered = reflected + mat.param1 * random_in_unit_sphere();
        
        ray.origin = hit.point;
        ray.direction = normalize(scattered);
        attenuation *= mat.color / specular_chance;
        
        return dot(scattered, hit.normal) > 0.0;
    } else {
        // Diffuse reflection
        vec3 scatter_direction = hit.normal + random_unit_vector();
        
        ray.origin = hit.point;
        ray.direction = normalize(scatter_direction);
        attenuation *= mat.color / (1.0 - specular_chance);
        
        return true;
    }
}
```

### 6. Subsurface Materials

**Physics:** Materials with subsurface scattering for translucent effects.

**Properties:**
- Light penetrates surface and scatters internally
- Creates soft, translucent appearance
- Common in organic materials (skin, wax, marble)

**Parameters:**
- `color` - Base scattering color
- `param1` - Scattering amount (0.0 = surface only, 1.0 = deep penetration)

**Scene File Syntax:**
```
material name subsurface r g b scattering
```

**Examples:**
```
material skin subsurface 0.9 0.7 0.6 0.3
material marble subsurface 0.95 0.95 0.9 0.4
material candle_wax subsurface 0.9 0.8 0.7 0.5
material jade subsurface 0.3 0.8 0.4 0.6
```

**Implementation:**
```glsl
bool scatter_subsurface(inout Ray ray, HitRecord hit, GPUMaterial mat, inout vec3 attenuation) {
    // Probabilistic surface vs. subsurface scattering
    float surface_chance = 1.0 - mat.param1;
    
    if (random_float() < surface_chance) {
        // Surface scattering (like lambertian)
        vec3 scatter_direction = hit.normal + random_unit_vector();
        ray.origin = hit.point;
        ray.direction = normalize(scatter_direction);
    } else {
        // Subsurface scattering (enter material)
        vec3 scatter_direction = -hit.normal + random_unit_vector();
        ray.origin = hit.point - 0.001 * hit.normal;  // Enter surface
        ray.direction = normalize(scatter_direction);
    }
    
    attenuation *= mat.color;
    return true;
}
```

## Material Lighting Interaction

### Lighting Calculation

Materials interact with lights based on their properties:

```glsl
vec3 calculate_material_response(GPUMaterial mat, HitRecord hit, vec3 light_direction, vec3 light_color) {
    switch (mat.type) {
        case MATERIAL_LAMBERTIAN:
            return mat.color * max(0.0, dot(hit.normal, light_direction)) * light_color;
            
        case MATERIAL_METAL:
            vec3 reflected = reflect(-light_direction, hit.normal);
            vec3 view_dir = -normalize(ray.direction);
            float spec = pow(max(0.0, dot(reflected, view_dir)), 1.0 / (mat.param1 + 0.01));
            return mat.color * spec * light_color;
            
        case MATERIAL_GLOSSY:
            float lambert = max(0.0, dot(hit.normal, light_direction));
            vec3 reflected = reflect(-light_direction, hit.normal);
            vec3 view_dir = -normalize(ray.direction);
            float spec = pow(max(0.0, dot(reflected, view_dir)), 1.0 / (mat.param1 + 0.01));
            return mat.color * (lambert + spec * (1.0 - mat.param1)) * light_color;
            
        case MATERIAL_EMISSIVE:
            return mat.color * mat.param1;  // Self-emission only
            
        default:
            return vec3(0.0);
    }
}
```

## Advanced Material Techniques

### Material Blending

```cpp
// CPU-side material interpolation
Material blend_materials(const Material& mat1, const Material& mat2, float factor) {
    Material result;
    result.color = lerp(mat1.color, mat2.color, factor);
    result.type = (factor < 0.5f) ? mat1.type : mat2.type;
    result.param1 = lerp(mat1.param1, mat2.param1, factor);
    return result;
}
```

### Procedural Material Properties

```glsl
// Shader-based procedural variation
float noise_factor = noise(hit.point * 10.0);
mat.param1 = mat.param1 * (0.8 + 0.4 * noise_factor);  // Vary roughness
mat.color = mat.color * (0.9 + 0.2 * noise_factor);    // Vary color
```

### Material Validation

```cpp
bool validate_material(const Material& mat) {
    // Check color range
    if (mat.color.x() < 0.0f || mat.color.x() > 1.0f ||
        mat.color.y() < 0.0f || mat.color.y() > 1.0f ||
        mat.color.z() < 0.0f || mat.color.z() > 1.0f) {
        return false;
    }
    
    // Type-specific validation
    switch (mat.type) {
        case MaterialType::METAL:
        case MaterialType::GLOSSY:
            return mat.param1 >= 0.0f && mat.param1 <= 1.0f;  // Roughness
            
        case MaterialType::DIELECTRIC:
            return mat.param1 >= 1.0f && mat.param1 <= 3.0f;  // IOR
            
        case MaterialType::EMISSIVE:
            return mat.param1 >= 0.0f;  // Intensity
            
        case MaterialType::SUBSURFACE:
            return mat.param1 >= 0.0f && mat.param1 <= 1.0f;  // Scattering
            
        default:
            return true;
    }
}
```

## Performance Considerations

### Material Performance Ranking

1. **Lambertian** - Fastest (simple cosine scattering)
2. **Metal** - Fast (single reflection)
3. **Glossy** - Medium (probabilistic branching)
4. **Emissive** - Fast (no scattering)
5. **Dielectric** - Medium (Fresnel calculations, refraction)
6. **Subsurface** - Slow (multiple scattering events)

### Optimization Tips

1. **Reduce Subsurface Materials**: Most expensive due to multiple bounces
2. **Limit High-IOR Dielectrics**: Cause more total internal reflection
3. **Use Emissive Sparingly**: Too many light sources slow lighting
4. **Optimize Metal Roughness**: Very low roughness requires more precision
5. **Balance Glossy Parameters**: Extreme values affect convergence

## Material Examples and Use Cases

### Architectural Materials

```
# Building materials
material concrete lambertian 0.6 0.6 0.6
material brick lambertian 0.7 0.4 0.3
material marble subsurface 0.9 0.9 0.85 0.2
material steel metal 0.8 0.8 0.8 0.1
material glass dielectric 0.9 0.9 0.9 1.5
```

### Natural Materials

```
# Natural world
material grass lambertian 0.3 0.6 0.2
material tree_bark lambertian 0.4 0.3 0.2
material water dielectric 0.8 0.9 1.0 1.33
material sand lambertian 0.8 0.7 0.5
material snow lambertian 0.95 0.95 0.95
```

### Artistic Materials

```
# Artistic and decorative
material gold_leaf metal 1.0 0.8 0.3 0.05
material silver_mirror metal 0.95 0.95 0.95 0.02
material colored_glass dielectric 0.2 0.8 0.3 1.6
material ceramic_glaze glossy 0.8 0.9 1.0 0.1
material fabric lambertian 0.6 0.4 0.7
```

### Lighting Materials

```
# Light sources
material led_white emissive 1.0 1.0 0.95 10.0
material neon_tube emissive 0.2 0.6 1.0 5.0
material fire emissive 1.0 0.4 0.1 8.0
material fluorescent emissive 0.9 0.95 1.0 6.0
```

## Troubleshooting Common Issues

### Dark/Black Materials
- **Cause**: Color values too low or zero
- **Solution**: Increase albedo values (typically 0.1-0.9)

### Overly Bright Emissive Materials
- **Cause**: Intensity too high
- **Solution**: Reduce intensity parameter (typically 1.0-10.0)

### Glass Appears Opaque
- **Cause**: IOR too low or color too dark
- **Solution**: Use IOR ≥ 1.1, bright colors (0.9+ for clear glass)

### Metal Looks Like Plastic
- **Cause**: Roughness too high
- **Solution**: Lower roughness (<0.3 for metallic appearance)

### Subsurface Too Transparent
- **Cause**: Scattering parameter too high
- **Solution**: Reduce scattering (0.1-0.5 for most materials)

### Materials Converge Slowly
- **Cause**: Too many complex materials (dielectric, subsurface)
- **Solution**: Use simpler materials where possible, increase sample count
