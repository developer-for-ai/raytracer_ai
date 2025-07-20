# 💻 GPU Architecture Guide

The raytracer uses OpenGL compute shaders for massively parallel ray processing. This guide covers the complete GPU architecture and implementation details.

## Overview

The GPU raytracer leverages OpenGL 4.3 compute shaders to process millions of rays in parallel, achieving real-time performance for complex scenes.

### Key Components
- **Compute Shader Pipeline** - GLSL compute shaders for ray processing
- **GPU Buffer Management** - Efficient scene data storage
- **Ray Processing** - Parallel ray-scene intersection
- **Temporal Accumulation** - Progressive quality improvement

## Compute Shader Architecture

### Shader Dispatch Model

```cpp
// Dispatch compute shader with 8x8 work groups
int groups_x = (width + 7) / 8;
int groups_y = (height + 7) / 8;
glDispatchCompute(groups_x, groups_y, 1);
```

**Work Group Structure:**
```glsl
#version 430
layout(local_size_x = 8, local_size_y = 8) in;

void main() {
    ivec2 pixel_coords = ivec2(gl_GlobalInvocationID.xy);
    
    // Each invocation processes one pixel
    vec3 color = trace_ray_for_pixel(pixel_coords);
    
    // Write result to output texture
    imageStore(output_texture, pixel_coords, vec4(color, 1.0));
}
```

### GPU Memory Layout

#### Scene Data Buffers

**Spheres Buffer:**
```glsl
struct GPUSphere {
    vec3 center;
    float radius;
    int material_id;
    float _padding[3];  // GPU alignment
};

layout(std430, binding = 0) restrict readonly buffer SphereBuffer {
    GPUSphere spheres[];
};
```

**Triangles Buffer:**
```glsl
struct GPUTriangle {
    vec3 v0, v1, v2;    // Triangle vertices
    int material_id;
    float _padding[3];   // GPU alignment
};

layout(std430, binding = 1) restrict readonly buffer TriangleBuffer {
    GPUTriangle triangles[];
};
```

**Materials Buffer:**
```glsl
struct GPUMaterial {
    vec3 color;
    int type;           // MaterialType enum
    float param1;       // roughness, ior, etc.
    float param2;       // additional parameter
    float param3;       // additional parameter
    float _padding;     // GPU alignment
};

layout(std430, binding = 2) restrict readonly buffer MaterialBuffer {
    GPUMaterial materials[];
};
```

**Lights Buffer:**
```glsl
struct GPULight {
    vec3 position;
    int type;           // Light type
    vec3 direction;     // For spot lights
    float inner_angle;  // Spot light inner angle
    vec3 intensity;
    float outer_angle;  // Spot light outer angle
    vec3 u_axis;        // Area light U axis
    float width;        // Area light width
    vec3 normal;        // Area light normal
    float height;       // Area light height
    float radius;       // Soft shadow radius
    int samples;        // Area light samples
    float _padding[2];
};

layout(std430, binding = 3) restrict readonly buffer LightBuffer {
    GPULight lights[];
};
```

### Buffer Management

#### Buffer Creation and Upload

```cpp
class GPURayTracer {
private:
    GLuint sphere_buffer;
    GLuint triangle_buffer;
    GLuint material_buffer;
    GLuint light_buffer;
    
public:
    void upload_scene_data(const Scene& scene) {
        upload_spheres(scene.get_spheres());
        upload_triangles(scene.get_triangles());
        upload_materials(scene.get_materials());
        upload_lights(scene.get_lights());
    }
    
private:
    void upload_spheres(const std::vector<std::shared_ptr<Sphere>>& spheres) {
        std::vector<GPUSphere> gpu_spheres;
        
        for (const auto& sphere : spheres) {
            GPUSphere gpu_sphere;
            gpu_sphere.center = sphere->center;
            gpu_sphere.radius = sphere->radius;
            gpu_sphere.material_id = sphere->material_id;
            gpu_spheres.push_back(gpu_sphere);
        }
        
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, sphere_buffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, 
                    gpu_spheres.size() * sizeof(GPUSphere),
                    gpu_spheres.data(), GL_STATIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, sphere_buffer);
    }
};
```

## Ray Tracing Pipeline

### Main Ray Tracing Loop

```glsl
vec3 trace_ray_for_pixel(ivec2 pixel_coords) {
    // Generate camera ray
    Ray ray = generate_camera_ray(pixel_coords);
    
    vec3 accumulated_color = vec3(0.0);
    vec3 ray_attenuation = vec3(1.0);
    
    // Bounce ray through scene
    for (int depth = 0; depth < max_ray_depth; depth++) {
        HitRecord hit;
        if (!intersect_scene(ray, hit)) {
            // Ray missed - add background color
            accumulated_color += ray_attenuation * background_color;
            break;
        }
        
        // Calculate lighting at intersection
        vec3 lighting = calculate_lighting(hit);
        accumulated_color += ray_attenuation * lighting * materials[hit.material_id].emission;
        
        // Scatter ray for next bounce
        if (!scatter_ray(ray, hit, ray_attenuation)) {
            break;  // Ray absorbed
        }
    }
    
    return accumulated_color;
}
```

### Scene Intersection

```glsl
bool intersect_scene(Ray ray, out HitRecord hit) {
    hit.t = 1e30;  // Initialize to large distance
    bool hit_anything = false;
    
    // Test sphere intersections
    for (int i = 0; i < num_spheres; i++) {
        HitRecord temp_hit;
        if (hit_sphere(spheres[i], ray, 0.001, hit.t, temp_hit)) {
            hit = temp_hit;
            hit_anything = true;
        }
    }
    
    // Test triangle intersections
    for (int i = 0; i < num_triangles; i++) {
        HitRecord temp_hit;
        if (hit_triangle(triangles[i], ray, 0.001, hit.t, temp_hit)) {
            hit = temp_hit;
            hit_anything = true;
        }
    }
    
    return hit_anything;
}
```

### Ray-Sphere Intersection

```glsl
bool hit_sphere(GPUSphere sphere, Ray ray, float t_min, float t_max, out HitRecord rec) {
    vec3 oc = ray.origin - sphere.center;
    float a = dot(ray.direction, ray.direction);
    float half_b = dot(oc, ray.direction);
    float c = dot(oc, oc) - sphere.radius * sphere.radius;
    
    float discriminant = half_b * half_b - a * c;
    if (discriminant < 0.0) return false;
    
    float sqrtd = sqrt(discriminant);
    float root = (-half_b - sqrtd) / a;
    
    if (root < t_min || root > t_max) {
        root = (-half_b + sqrtd) / a;
        if (root < t_min || root > t_max) return false;
    }
    
    rec.t = root;
    rec.point = ray.origin + root * ray.direction;
    vec3 outward_normal = (rec.point - sphere.center) / sphere.radius;
    rec.front_face = dot(ray.direction, outward_normal) < 0.0;
    rec.normal = rec.front_face ? outward_normal : -outward_normal;
    rec.material_id = sphere.material_id;
    
    return true;
}
```

### Ray-Triangle Intersection (Möller-Trumbore)

```glsl
bool hit_triangle(GPUTriangle tri, Ray ray, float t_min, float t_max, out HitRecord rec) {
    vec3 edge1 = tri.v1 - tri.v0;
    vec3 edge2 = tri.v2 - tri.v0;
    vec3 h = cross(ray.direction, edge2);
    float a = dot(edge1, h);
    
    if (abs(a) < 1e-8) return false;  // Ray parallel to triangle
    
    float f = 1.0 / a;
    vec3 s = ray.origin - tri.v0;
    float u = f * dot(s, h);
    
    if (u < 0.0 || u > 1.0) return false;
    
    vec3 q = cross(s, edge1);
    float v = f * dot(ray.direction, q);
    
    if (v < 0.0 || u + v > 1.0) return false;
    
    float t = f * dot(edge2, q);
    
    if (t < t_min || t > t_max) return false;
    
    // Valid intersection
    rec.t = t;
    rec.point = ray.origin + t * ray.direction;
    vec3 normal = normalize(cross(edge1, edge2));
    rec.front_face = dot(ray.direction, normal) < 0.0;
    rec.normal = rec.front_face ? normal : -normal;
    rec.material_id = tri.material_id;
    
    return true;
}
```

## Material Scattering

### Material Processing

```glsl
bool scatter_ray(inout Ray ray, HitRecord hit, inout vec3 attenuation) {
    GPUMaterial mat = materials[hit.material_id];
    
    switch (mat.type) {
        case MATERIAL_LAMBERTIAN:
            return scatter_lambertian(ray, hit, mat, attenuation);
        case MATERIAL_METAL:
            return scatter_metal(ray, hit, mat, attenuation);
        case MATERIAL_DIELECTRIC:
            return scatter_dielectric(ray, hit, mat, attenuation);
        case MATERIAL_EMISSIVE:
            return scatter_emissive(ray, hit, mat, attenuation);
        default:
            return false;
    }
}
```

### Lambertian Scattering

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

### Metal Scattering

```glsl
bool scatter_metal(inout Ray ray, HitRecord hit, GPUMaterial mat, inout vec3 attenuation) {
    vec3 reflected = reflect(normalize(ray.direction), hit.normal);
    
    // Add roughness
    vec3 scattered = reflected + mat.param1 * random_in_unit_sphere();
    
    ray.origin = hit.point;
    ray.direction = normalize(scattered);
    attenuation *= mat.color;
    
    return dot(scattered, hit.normal) > 0.0;
}
```

## Lighting System

### Light Processing

```glsl
vec3 calculate_lighting(HitRecord hit) {
    vec3 total_lighting = vec3(0.0);
    
    for (int i = 0; i < num_lights; i++) {
        vec3 light_contribution = process_light(lights[i], hit);
        total_lighting += light_contribution;
    }
    
    // Add ambient lighting
    total_lighting += ambient_light * materials[hit.material_id].color;
    
    return total_lighting;
}
```

### Point Light Processing

```glsl
vec3 process_point_light(GPULight light, HitRecord hit) {
    vec3 light_direction = light.position - hit.point;
    float distance = length(light_direction);
    light_direction = normalize(light_direction);
    
    // Shadow test
    Ray shadow_ray;
    shadow_ray.origin = hit.point + 0.001 * hit.normal;
    shadow_ray.direction = light_direction;
    
    HitRecord shadow_hit;
    if (intersect_scene(shadow_ray, shadow_hit) && shadow_hit.t < distance) {
        return vec3(0.0);  // In shadow
    }
    
    // Calculate lighting
    float attenuation = 1.0 / (distance * distance);
    float lambert = max(0.0, dot(hit.normal, light_direction));
    
    return light.intensity * lambert * attenuation * materials[hit.material_id].color;
}
```

## Temporal Accumulation

### Progressive Quality Improvement

```glsl
// Accumulation texture for temporal sampling
layout(binding = 4, rgba32f) restrict coherent uniform image2D accumulation_texture;

uniform int sample_count;
uniform bool reset_accumulation;

void main() {
    ivec2 pixel_coords = ivec2(gl_GlobalInvocationID.xy);
    
    // Trace ray for current sample
    vec3 current_color = trace_ray_for_pixel(pixel_coords);
    
    if (reset_accumulation) {
        // Reset accumulation buffer
        imageStore(accumulation_texture, pixel_coords, vec4(current_color, 1.0));
        imageStore(output_texture, pixel_coords, vec4(current_color, 1.0));
    } else {
        // Accumulate with previous samples
        vec4 accumulated = imageLoad(accumulation_texture, pixel_coords);
        accumulated.rgb = (accumulated.rgb * float(sample_count - 1) + current_color) / float(sample_count);
        
        imageStore(accumulation_texture, pixel_coords, accumulated);
        imageStore(output_texture, pixel_coords, vec4(accumulated.rgb, 1.0));
    }
}
```

## Performance Optimization

### Memory Access Patterns

1. **Coalesced Reads**: Adjacent threads access adjacent memory
2. **Cache Optimization**: Minimize random memory access
3. **Buffer Alignment**: Use proper GPU memory alignment (16 bytes)

### Compute Shader Optimization

```glsl
// Use shared memory for frequently accessed data
shared GPUMaterial shared_materials[MAX_MATERIALS];

// Load materials into shared memory once per work group
if (gl_LocalInvocationIndex < num_materials) {
    shared_materials[gl_LocalInvocationIndex] = materials[gl_LocalInvocationIndex];
}
barrier();

// Now use shared_materials instead of materials buffer
```

### Early Ray Termination

```glsl
// Terminate rays that contribute little to final color
if (length(ray_attenuation) < 0.001) {
    break;  // Ray contribution negligible
}
```

## Debugging GPU Code

### Debug Output

```glsl
// Debug information buffer
layout(std430, binding = 5) restrict writeonly buffer DebugBuffer {
    vec4 debug_data[];
};

void debug_output(vec3 data, int pixel_index) {
    debug_data[pixel_index] = vec4(data, 1.0);
}
```

### Performance Profiling

```cpp
// GPU timing
GLuint query;
glGenQueries(1, &query);
glBeginQuery(GL_TIME_ELAPSED, query);

// Dispatch compute shader
glDispatchCompute(groups_x, groups_y, 1);

glEndQuery(GL_TIME_ELAPSED);

GLuint64 elapsed_time;
glGetQueryObjectui64v(query, GL_QUERY_RESULT, &elapsed_time);
Logger::info("GPU rendering time: {} ms", elapsed_time / 1000000.0);
```

## Error Handling

### OpenGL Error Checking

```cpp
void check_gl_error(const std::string& operation) {
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        Logger::error("OpenGL error in {}: {}", operation, error);
        throw std::runtime_error("OpenGL error");
    }
}
```

### Shader Compilation Validation

```cpp
bool compile_shader(GLuint shader, const std::string& source) {
    const char* source_cstr = source.c_str();
    glShaderSource(shader, 1, &source_cstr, nullptr);
    glCompileShader(shader);
    
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    
    if (!success) {
        char info_log[512];
        glGetShaderInfoLog(shader, 512, nullptr, info_log);
        Logger::error("Shader compilation failed: {}", info_log);
        return false;
    }
    
    return true;
}
```
