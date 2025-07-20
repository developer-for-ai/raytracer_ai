# 📐 Intersection Algorithms Guide

Detailed guide to ray-geometry intersection algorithms used in the raytracer, including mathematical foundations and GPU implementations.

## Overview

The raytracer implements efficient intersection algorithms for different geometric primitives. All algorithms are optimized for GPU execution with SIMD-friendly operations and minimal branching.

### Supported Geometry Types

1. **Ray-Sphere Intersection** - Analytical quadratic solution
2. **Ray-Triangle Intersection** - Möller-Trumbore algorithm  
3. **Ray-Plane Intersection** - Analytical plane equation solution

## Mathematical Foundations

### Ray Representation

A ray is defined parametrically as:
```
Ray(t) = Origin + t * Direction
```

Where:
- `Origin` - Ray starting point (Vec3)
- `Direction` - Ray direction vector (normalized Vec3)
- `t` - Parameter (distance along ray, t ≥ 0)

```glsl
struct Ray {
    vec3 origin;
    vec3 direction;
};
```

### Hit Record

Intersection results are stored in a hit record:

```glsl
struct HitRecord {
    vec3 point;        // Intersection point
    vec3 normal;       // Surface normal at intersection
    float t;           // Distance parameter along ray
    bool front_face;   // True if ray hits front face
    int material_id;   // Material index
};
```

## Ray-Sphere Intersection

### Mathematical Derivation

For a sphere with center `C` and radius `r`, the implicit equation is:
```
|P - C|² = r²
```

Substituting the ray equation:
```
|Origin + t * Direction - C|² = r²
```

Expanding into quadratic form:
```
at² + 2bt + c = 0
```

Where:
- `a = Direction · Direction`
- `b = (Origin - C) · Direction`  
- `c = |Origin - C|² - r²`

### GPU Implementation

```glsl
bool hit_sphere(GPUSphere sphere, Ray ray, float t_min, float t_max, out HitRecord rec) {
    vec3 oc = ray.origin - sphere.center;
    
    // Quadratic coefficients
    float a = dot(ray.direction, ray.direction);
    float half_b = dot(oc, ray.direction);
    float c = dot(oc, oc) - sphere.radius * sphere.radius;
    
    // Discriminant test
    float discriminant = half_b * half_b - a * c;
    if (discriminant < 0.0) return false;
    
    float sqrtd = sqrt(discriminant);
    
    // Find nearest valid root
    float root = (-half_b - sqrtd) / a;
    if (root < t_min || root > t_max) {
        root = (-half_b + sqrtd) / a;
        if (root < t_min || root > t_max) return false;
    }
    
    // Fill hit record
    rec.t = root;
    rec.point = ray.origin + root * ray.direction;
    vec3 outward_normal = (rec.point - sphere.center) / sphere.radius;
    
    // Determine front face
    rec.front_face = dot(ray.direction, outward_normal) < 0.0;
    rec.normal = rec.front_face ? outward_normal : -outward_normal;
    rec.material_id = sphere.material_id;
    
    return true;
}
```

### Optimizations

**Half-B Optimization:**
```glsl
// Use half_b instead of full b to avoid factor of 2
float half_b = dot(oc, ray.direction);
float discriminant = half_b * half_b - a * c;
float root = (-half_b - sqrtd) / a;
```

**Early Discriminant Exit:**
```glsl
if (discriminant < 0.0) return false;  // No intersection
```

**Radius Squared Precomputation:**
```glsl
// Store r² instead of r to avoid multiplication
float radius_squared = sphere.radius * sphere.radius;
```

## Ray-Triangle Intersection

### Möller-Trumbore Algorithm

The Möller-Trumbore algorithm computes ray-triangle intersection directly without computing the plane equation. It's more efficient and numerically stable than other methods.

### Mathematical Foundation

For triangle with vertices `v0`, `v1`, `v2`:
- Edge vectors: `e1 = v1 - v0`, `e2 = v2 - v0`
- Ray equation: `O + tD = (1-u-v)v0 + uv1 + vv2`

Solving the system yields barycentric coordinates `(u,v)` and distance `t`.

### GPU Implementation

```glsl
bool hit_triangle(GPUTriangle tri, Ray ray, float t_min, float t_max, out HitRecord rec) {
    vec3 edge1 = tri.v1 - tri.v0;
    vec3 edge2 = tri.v2 - tri.v0;
    
    vec3 h = cross(ray.direction, edge2);
    float a = dot(edge1, h);
    
    // Ray parallel to triangle
    if (abs(a) < 1e-8) return false;
    
    float f = 1.0 / a;
    vec3 s = ray.origin - tri.v0;
    float u = f * dot(s, h);
    
    // Check barycentric coordinate u
    if (u < 0.0 || u > 1.0) return false;
    
    vec3 q = cross(s, edge1);
    float v = f * dot(ray.direction, q);
    
    // Check barycentric coordinate v
    if (v < 0.0 || u + v > 1.0) return false;
    
    // Compute t
    float t = f * dot(edge2, q);
    
    // Check ray parameter bounds
    if (t < t_min || t > t_max) return false;
    
    // Valid intersection found
    rec.t = t;
    rec.point = ray.origin + t * ray.direction;
    
    // Compute normal
    vec3 normal = normalize(cross(edge1, edge2));
    rec.front_face = dot(ray.direction, normal) < 0.0;
    rec.normal = rec.front_face ? normal : -normal;
    rec.material_id = tri.material_id;
    
    return true;
}
```

### Optimizations

**Single Division:**
```glsl
float f = 1.0 / a;  // Single division, multiply by f elsewhere
```

**Early Exit Conditions:**
```glsl
// Exit as soon as any condition fails
if (abs(a) < 1e-8) return false;
if (u < 0.0 || u > 1.0) return false;
if (v < 0.0 || u + v > 1.0) return false;
```

**Epsilon Tuning:**
```glsl
const float EPSILON = 1e-8;  // Adjust based on scene scale
```

## Ray-Plane Intersection

### Mathematical Foundation

For plane with point `P` and normal `N`:
```
(Ray(t) - P) · N = 0
```

Expanding:
```
(O + tD - P) · N = 0
t = (P - O) · N / (D · N)
```

### GPU Implementation

```glsl
bool hit_plane(GPUPlane plane, Ray ray, float t_min, float t_max, out HitRecord rec) {
    float denom = dot(ray.direction, plane.normal);
    
    // Ray parallel to plane
    if (abs(denom) < 1e-6) return false;
    
    vec3 p_to_origin = plane.point - ray.origin;
    float t = dot(p_to_origin, plane.normal) / denom;
    
    // Check ray parameter bounds
    if (t < t_min || t > t_max) return false;
    
    // Fill hit record
    rec.t = t;
    rec.point = ray.origin + t * ray.direction;
    rec.front_face = denom < 0.0;
    rec.normal = rec.front_face ? plane.normal : -plane.normal;
    rec.material_id = plane.material_id;
    
    return true;
}
```

### Infinite Plane Handling

```glsl
// For infinite planes, always return true if intersection exists
if (t >= t_min && t <= t_max) {
    // ... fill hit record
    return true;
}
```

## Scene Intersection

### Multiple Geometry Testing

```glsl
bool intersect_scene(Ray ray, out HitRecord hit) {
    hit.t = 1e30;  // Initialize to large distance
    bool hit_anything = false;
    
    // Test all spheres
    for (int i = 0; i < num_spheres; i++) {
        HitRecord temp_hit;
        if (hit_sphere(spheres[i], ray, 0.001, hit.t, temp_hit)) {
            hit = temp_hit;
            hit_anything = true;
        }
    }
    
    // Test all triangles
    for (int i = 0; i < num_triangles; i++) {
        HitRecord temp_hit;
        if (hit_triangle(triangles[i], ray, 0.001, hit.t, temp_hit)) {
            hit = temp_hit;
            hit_anything = true;
        }
    }
    
    // Test all planes
    for (int i = 0; i < num_planes; i++) {
        HitRecord temp_hit;
        if (hit_plane(planes[i], ray, 0.001, hit.t, temp_hit)) {
            hit = temp_hit;
            hit_anything = true;
        }
    }
    
    return hit_anything;
}
```

### Self-Intersection Prevention

```glsl
const float SHADOW_BIAS = 0.001;

// Offset ray origin to prevent self-intersection
Ray shadow_ray;
shadow_ray.origin = hit.point + SHADOW_BIAS * hit.normal;
shadow_ray.direction = light_direction;
```

## Numerical Stability

### Floating-Point Precision Issues

**Problem:** Intersection tests can fail due to floating-point precision.

**Solutions:**

**Adaptive Epsilon:**
```glsl
float get_epsilon(vec3 point) {
    return max(1e-8, length(point) * 1e-6);
}
```

**Robust Triangle Test:**
```glsl
// Use higher precision for small triangles
float triangle_area = length(cross(edge1, edge2)) * 0.5;
float epsilon = triangle_area < 1e-6 ? 1e-10 : 1e-8;
```

**Safe Division:**
```glsl
float safe_divide(float a, float b) {
    return abs(b) > 1e-10 ? a / b : 0.0;
}
```

### Ray Parameter Bounds

```glsl
const float T_MIN = 0.001;    // Avoid self-intersection
const float T_MAX = 1000.0;   // Scene bounds
```

**Dynamic T_MIN:**
```glsl
float get_t_min(vec3 ray_origin) {
    return max(1e-6, length(ray_origin) * 1e-8);
}
```

## Performance Optimization

### Intersection Performance Ranking

1. **Sphere** - Fastest (simple quadratic)
2. **Plane** - Fast (single division)
3. **Triangle** - Medium (more complex but well-optimized)

### SIMD Optimization

**Vectorized Operations:**
```glsl
// Use vec3 operations instead of component-wise
vec3 oc = ray.origin - sphere.center;      // Vectorized subtraction
float c = dot(oc, oc) - radius_squared;    // Single dot product
```

**Minimize Branching:**
```glsl
// Use mathematical operations instead of conditionals
float hit_mask = float(discriminant >= 0.0);
return bool(hit_mask * float(t >= t_min && t <= t_max));
```

### Memory Access Patterns

**Structure-of-Arrays (SoA):**
```glsl
// Instead of Array-of-Structures
layout(std430, binding = 0) buffer SpherePositions { vec3 positions[]; };
layout(std430, binding = 1) buffer SphereRadii { float radii[]; };
layout(std430, binding = 2) buffer SphereMaterials { int materials[]; };
```

**Cache-Friendly Access:**
```glsl
// Process geometry in groups for better cache usage
for (int group = 0; group < num_groups; group++) {
    int start = group * GROUP_SIZE;
    int end = min(start + GROUP_SIZE, num_objects);
    
    for (int i = start; i < end; i++) {
        // Process objects[i]
    }
}
```

## Advanced Intersection Techniques

### Backface Culling

```glsl
bool hit_triangle_culled(GPUTriangle tri, Ray ray, float t_min, float t_max, out HitRecord rec) {
    vec3 edge1 = tri.v1 - tri.v0;
    vec3 edge2 = tri.v2 - tri.v0;
    
    vec3 normal = cross(edge1, edge2);
    
    // Backface culling - skip if ray and normal point same direction
    if (dot(ray.direction, normal) > 0.0) return false;
    
    // Continue with normal intersection test...
}
```

### Watertight Triangle Intersection

```glsl
// Watertight implementation to avoid gaps between triangles
bool hit_triangle_watertight(GPUTriangle tri, Ray ray, float t_min, float t_max, out HitRecord rec) {
    // Use double precision for critical calculations
    dvec3 edge1 = dvec3(tri.v1 - tri.v0);
    dvec3 edge2 = dvec3(tri.v2 - tri.v0);
    
    // ... rest of intersection with double precision
}
```

### Intersection Caching

```glsl
// Cache recent intersections for coherent rays
struct IntersectionCache {
    int last_hit_object;
    float last_t;
    vec3 last_normal;
};

bool intersect_scene_cached(Ray ray, inout IntersectionCache cache, out HitRecord hit) {
    // Try cached object first
    if (cache.last_hit_object >= 0) {
        if (hit_object(cache.last_hit_object, ray, 0.001, 1000.0, hit)) {
            return true;
        }
    }
    
    // Fall back to full scene intersection
    return intersect_scene(ray, hit);
}
```

## Debugging Intersection Issues

### Visualization Techniques

**Ray Visualization:**
```glsl
// Output ray direction as color for debugging
if (debug_mode) {
    imageStore(output_texture, pixel_coords, vec4(ray.direction * 0.5 + 0.5, 1.0));
    return;
}
```

**Hit Normal Visualization:**
```glsl
// Visualize surface normals
if (intersect_scene(ray, hit)) {
    vec3 normal_color = hit.normal * 0.5 + 0.5;
    imageStore(output_texture, pixel_coords, vec4(normal_color, 1.0));
}
```

**Distance Visualization:**
```glsl
// Visualize hit distances
if (intersect_scene(ray, hit)) {
    float normalized_t = hit.t / 20.0;  // Normalize to scene scale
    imageStore(output_texture, pixel_coords, vec4(vec3(normalized_t), 1.0));
}
```

### Common Issues and Solutions

**Self-Intersection:**
- **Symptom**: Shadow acne, incorrect shadows
- **Solution**: Increase shadow bias (`t_min`)

**Missing Intersections:**
- **Symptom**: Objects disappearing at certain angles
- **Solution**: Check epsilon values, numerical precision

**Incorrect Normals:**
- **Symptom**: Wrong lighting, inside-out appearance
- **Solution**: Verify normal calculation, front-face determination

**Poor Performance:**
- **Symptom**: Low FPS with many triangles
- **Solution**: Implement BVH, reduce triangle count, optimize loops
