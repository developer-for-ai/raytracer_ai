# 💡 Lighting System Guide

Comprehensive guide to the advanced lighting system with multiple light types, soft shadows, and realistic illumination.

## Overview

The lighting system implements physically-based lighting with support for multiple light types, soft shadows, and realistic attenuation. All lighting calculations are performed in the GPU compute shaders for maximum performance.

### Light Types Supported

1. **Point Lights** - Omnidirectional light sources
2. **Spot Lights** - Directional cone-shaped lights  
3. **Area Lights** - Rectangular plane lights for soft illumination
4. **Ambient Light** - Global ambient illumination

## Light Type Details

### Point Lights

**Physics:** Omnidirectional light with distance-based attenuation following inverse square law.

**Scene File Syntax:**
```
point_light pos_x pos_y pos_z intensity_r intensity_g intensity_b [radius]
```

**Parameters:**
- `pos_x pos_y pos_z` - Light position in world space
- `intensity_r intensity_g intensity_b` - Light color and brightness
- `radius` - (Optional) Soft shadow radius for area-like effects (default: 0.0)

**Examples:**
```
point_light 0 5 0  2.0 2.0 2.0        # Bright white light above origin
point_light -3 2 4  1.0 0.8 0.6       # Warm tungsten light
point_light 5 8 -2  0.8 0.9 1.2 0.1   # Cool light with soft shadows
```

**Implementation:**
```glsl
vec3 calculate_point_light(GPULight light, HitRecord hit, vec3 view_dir) {
    vec3 light_dir = light.position - hit.point;
    float distance = length(light_dir);
    light_dir = normalize(light_dir);
    
    // Distance attenuation (inverse square law)
    float attenuation = 1.0 / (distance * distance);
    
    // Shadow testing
    if (is_in_shadow(hit.point, light_dir, distance, light.radius)) {
        return vec3(0.0);
    }
    
    // Lambertian shading
    float lambert = max(0.0, dot(hit.normal, light_dir));
    
    return light.intensity * lambert * attenuation;
}
```

### Spot Lights

**Physics:** Directional cone-shaped light with inner and outer angle falloff.

**Scene File Syntax:**
```
spot_light pos_x pos_y pos_z dir_x dir_y dir_z intensity_r intensity_g intensity_b inner_angle outer_angle [radius]
```

**Parameters:**
- `pos_x pos_y pos_z` - Light position
- `dir_x dir_y dir_z` - Light direction (normalized automatically)
- `intensity_r intensity_g intensity_b` - Light color and brightness
- `inner_angle` - Inner cone angle in degrees (full intensity)
- `outer_angle` - Outer cone angle in degrees (falloff to zero)
- `radius` - (Optional) Soft shadow radius

**Examples:**
```
spot_light 0 5 0  0 -1 0  2.0 2.0 2.0 15 30       # Downward spotlight
spot_light -5 3 5  1 -1 -1  1.5 1.0 0.8 20 45     # Angled spotlight
spot_light 2 4 -1  -1 -2 1  1.0 1.2 0.6 10 25 0.05  # Narrow spot with soft shadows
```

**Implementation:**
```glsl
vec3 calculate_spot_light(GPULight light, HitRecord hit, vec3 view_dir) {
    vec3 light_dir = light.position - hit.point;
    float distance = length(light_dir);
    light_dir = normalize(light_dir);
    
    // Check if point is within light cone
    vec3 spot_dir = normalize(light.direction);
    float theta = degrees(acos(dot(-light_dir, spot_dir)));
    
    if (theta > light.outer_angle) {
        return vec3(0.0);  // Outside cone
    }
    
    // Distance attenuation
    float attenuation = 1.0 / (distance * distance);
    
    // Cone falloff
    float cone_falloff = 1.0;
    if (theta > light.inner_angle) {
        cone_falloff = (light.outer_angle - theta) / (light.outer_angle - light.inner_angle);
    }
    
    // Shadow testing
    if (is_in_shadow(hit.point, light_dir, distance, light.radius)) {
        return vec3(0.0);
    }
    
    // Lambertian shading
    float lambert = max(0.0, dot(hit.normal, light_dir));
    
    return light.intensity * lambert * attenuation * cone_falloff;
}
```

### Area Lights

**Physics:** Rectangular plane lights that provide soft, realistic illumination through area sampling.

**Scene File Syntax:**
```
area_light pos_x pos_y pos_z normal_x normal_y normal_z u_axis_x u_axis_y u_axis_z intensity_r intensity_g intensity_b width height [samples]
```

**Parameters:**
- `pos_x pos_y pos_z` - Light center position
- `normal_x normal_y normal_z` - Light surface normal (direction of emission)
- `u_axis_x u_axis_y u_axis_z` - Light U-axis (defines orientation)
- `intensity_r intensity_g intensity_b` - Light color and brightness
- `width height` - Light dimensions in world units
- `samples` - (Optional) Number of shadow samples (default: 4, max: 16)

**Examples:**
```
area_light 0 4 0  0 -1 0  1 0 0  0.8 0.8 1.0 2 1        # Horizontal ceiling light
area_light -3 2 0  1 0 0  0 1 0  1.2 0.8 0.6 1 2        # Vertical wall light
area_light 0 6 -4  0 -0.5 1  1 0 0  1.5 1.5 1.5 4 2 16  # Large angled light with high quality
```

**Implementation:**
```glsl
vec3 calculate_area_light(GPULight light, HitRecord hit, vec3 view_dir) {
    vec3 total_lighting = vec3(0.0);
    
    // Sample multiple points on light surface
    int samples = max(1, light.samples);
    
    for (int i = 0; i < samples; i++) {
        // Generate random point on light surface
        float u = random_float() - 0.5;  // -0.5 to 0.5
        float v = random_float() - 0.5;  // -0.5 to 0.5
        
        vec3 light_point = light.position + 
                          u * light.width * normalize(light.u_axis) +
                          v * light.height * cross(normalize(light.normal), normalize(light.u_axis));
        
        vec3 light_dir = light_point - hit.point;
        float distance = length(light_dir);
        light_dir = normalize(light_dir);
        
        // Check if light faces the hit point
        if (dot(-light_dir, normalize(light.normal)) <= 0.0) {
            continue;  // Back side of light
        }
        
        // Shadow testing
        if (is_in_shadow(hit.point, light_dir, distance, 0.0)) {
            continue;
        }
        
        // Distance attenuation
        float attenuation = 1.0 / (distance * distance);
        
        // Lambertian shading
        float lambert = max(0.0, dot(hit.normal, light_dir));
        
        total_lighting += light.intensity * lambert * attenuation;
    }
    
    // Average the samples
    return total_lighting / float(samples);
}
```

### Ambient Lighting

**Physics:** Global uniform illumination simulating indirect lighting bounces.

**Scene File Syntax:**
```
ambient r g b
```

**Parameters:**
- `r g b` - Ambient light color and intensity (typically low values: 0.01-0.2)

**Examples:**
```
ambient 0.1 0.1 0.15    # Cool ambient light
ambient 0.05 0.05 0.05  # Minimal ambient
ambient 0.2 0.15 0.1    # Warm ambient light
```

**Implementation:**
```glsl
vec3 calculate_ambient_light(vec3 ambient_color, GPUMaterial material) {
    return ambient_color * material.color;
}
```

## Soft Shadows

### Shadow Calculation

All light types support soft shadows through radius-based area sampling:

```glsl
bool is_in_shadow(vec3 point, vec3 light_dir, float light_distance, float light_radius) {
    if (light_radius <= 0.0) {
        // Hard shadows - single ray test
        return test_shadow_ray(point, light_dir, light_distance);
    } else {
        // Soft shadows - multiple ray test
        int shadow_samples = 4;  // Configurable
        int hits = 0;
        
        for (int i = 0; i < shadow_samples; i++) {
            // Generate random direction within light radius
            vec3 random_offset = random_in_unit_sphere() * light_radius;
            vec3 sample_dir = normalize(light_dir * light_distance + random_offset) ;
            
            if (test_shadow_ray(point, sample_dir, length(light_dir * light_distance + random_offset))) {
                hits++;
            }
        }
        
        // Return true if most samples are blocked
        return hits > shadow_samples / 2;
    }
}

bool test_shadow_ray(vec3 origin, vec3 direction, float max_distance) {
    Ray shadow_ray;
    shadow_ray.origin = origin + 0.001 * direction;  // Bias to prevent self-intersection
    shadow_ray.direction = direction;
    
    HitRecord hit;
    return intersect_scene(shadow_ray, hit) && hit.t < max_distance;
}
```

### Shadow Quality Settings

```glsl
// Shadow quality levels
#define SHADOW_SAMPLES_LOW 1      // Hard shadows
#define SHADOW_SAMPLES_MEDIUM 4   // Basic soft shadows
#define SHADOW_SAMPLES_HIGH 16    // High quality soft shadows

// Dynamic quality based on distance
int get_shadow_samples(float distance) {
    if (distance < 5.0) return SHADOW_SAMPLES_HIGH;
    if (distance < 20.0) return SHADOW_SAMPLES_MEDIUM;
    return SHADOW_SAMPLES_LOW;
}
```

## Light Attenuation

### Distance Attenuation Models

**Physically Accurate (Inverse Square):**
```glsl
float attenuation = 1.0 / (distance * distance);
```

**Linear Attenuation:**
```glsl
float attenuation = max(0.0, 1.0 - distance / max_range);
```

**Quadratic Attenuation with Linear Component:**
```glsl
float attenuation = 1.0 / (1.0 + 0.1 * distance + 0.01 * distance * distance);
```

## Multiple Light Handling

### Light Accumulation

```glsl
vec3 calculate_total_lighting(HitRecord hit, vec3 view_dir) {
    vec3 total_lighting = vec3(0.0);
    
    // Add ambient lighting
    total_lighting += ambient_light * materials[hit.material_id].color;
    
    // Process each light
    for (int i = 0; i < num_lights; i++) {
        vec3 light_contribution = vec3(0.0);
        
        switch (lights[i].type) {
            case LIGHT_POINT:
                light_contribution = calculate_point_light(lights[i], hit, view_dir);
                break;
            case LIGHT_SPOT:
                light_contribution = calculate_spot_light(lights[i], hit, view_dir);
                break;
            case LIGHT_AREA:
                light_contribution = calculate_area_light(lights[i], hit, view_dir);
                break;
        }
        
        total_lighting += light_contribution;
    }
    
    return total_lighting;
}
```

### Light Culling

```glsl
// Cull lights that don't contribute significantly
bool should_process_light(GPULight light, HitRecord hit) {
    float distance = length(light.position - hit.point);
    float max_contribution = length(light.intensity) / (distance * distance);
    
    return max_contribution > 0.01;  // Minimum contribution threshold
}
```

## Performance Optimization

### Light Performance Ranking

1. **Ambient** - Fastest (no shadow testing)
2. **Point (hard shadows)** - Fast (single shadow ray)
3. **Spot (hard shadows)** - Medium (cone test + shadow ray)
4. **Point (soft shadows)** - Medium (multiple shadow rays)
5. **Spot (soft shadows)** - Slow (cone test + multiple shadow rays)
6. **Area** - Slowest (multiple samples + multiple shadow rays)

### Optimization Techniques

**Light Distance Culling:**
```glsl
float light_distance = length(light.position - hit.point);
if (light_distance > light.max_range) continue;
```

**Early Shadow Exit:**
```glsl
// For soft shadows, exit early if enough samples are blocked
int blocked_samples = 0;
for (int i = 0; i < total_samples; i++) {
    if (test_shadow_ray(...)) {
        blocked_samples++;
        if (blocked_samples > total_samples / 2) {
            return true;  // Definitely in shadow
        }
    }
}
```

**Adaptive Shadow Sampling:**
```glsl
int shadow_samples = get_shadow_samples_for_light(light, hit_distance);
```

## Lighting Examples

### Indoor Scene Lighting

```
# Warm interior lighting
ambient 0.05 0.04 0.03

# Main ceiling light
area_light 0 3 0  0 -1 0  1 0 0  1.2 1.0 0.8  2 1 8

# Table lamp
point_light 2 1 1  0.8 0.6 0.4 0.1

# Window light (cool daylight)
area_light -5 2 0  1 0 0  0 1 0  2.5 2.5 3.0  1 3 6
```

### Outdoor Scene Lighting

```
# Sun (distant directional-like)
spot_light 100 100 50  -1 -1 -0.5  3.0 2.8 2.5  5 10

# Sky ambient
ambient 0.3 0.4 0.6

# Ground bounce (subtle upward light)
area_light 0 -0.1 0  0 1 0  1 0 0  0.2 0.25 0.35  20 20 4
```

### Dramatic Scene Lighting

```
# Single key light
spot_light -3 4 2  1 -2 -1  2.0 1.8 1.5  25 45 0.05

# Rim light
spot_light 5 3 -3  -1 -1 1  0.8 0.9 1.2  30 60

# Minimal ambient for contrast
ambient 0.02 0.02 0.03
```

### Colorful Party Lighting

```
# Multiple colored lights
point_light -2 3 1  2.0 0.5 0.5 0.2    # Red
point_light 0 3 -2  0.5 2.0 0.5 0.2    # Green  
point_light 3 3 0   0.5 0.5 2.0 0.2    # Blue
point_light 1 4 2   2.0 0.5 2.0 0.15   # Magenta

# Dynamic ambient
ambient 0.1 0.1 0.15
```

## Troubleshooting Lighting Issues

### Scene Too Dark
- **Cause**: Insufficient light intensity or too much distance attenuation
- **Solution**: Increase light intensities, add more lights, or increase ambient

### Harsh Shadows
- **Cause**: Hard shadows (radius = 0.0)
- **Solution**: Add radius parameter to lights (0.05-0.2 typical)

### Poor Performance with Many Lights
- **Cause**: Too many lights or high-quality area lights
- **Solution**: Reduce light count, lower area light samples, use light culling

### Unrealistic Lighting
- **Cause**: Wrong light types or intensities
- **Solution**: Use area lights for realistic sources, point lights for bulbs, spot lights for focused illumination

### Flickering Shadows
- **Cause**: Insufficient shadow samples or floating-point precision
- **Solution**: Increase shadow samples, add shadow bias, use stable random numbers
