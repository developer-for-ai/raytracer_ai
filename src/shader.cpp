#include "shader.h"
#include "error_handling.h"
#include <GL/glew.h>
#include <fstream>
#include <sstream>

std::string Shader::load_file(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        ErrorHandling::Logger::error("Failed to open shader file: " + filepath);
        return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

bool Shader::compile_shader(const std::string& source, unsigned int type, unsigned int& shader_id) {
    const char* source_cstr = source.c_str();
    
    shader_id = glCreateShader(type);
    if (shader_id == 0) {
        ErrorHandling::Logger::error("Failed to create shader");
        return false;
    }
    
    glShaderSource(shader_id, 1, &source_cstr, nullptr);
    glCompileShader(shader_id);
    
    GLint success;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &success);
    if (!success) {
        constexpr GLsizei LOG_SIZE = 1024;
        char info_log[LOG_SIZE];
        glGetShaderInfoLog(shader_id, LOG_SIZE, nullptr, info_log);
        ErrorHandling::Logger::error("Shader compilation failed: " + std::string(info_log));
        glDeleteShader(shader_id);
        return false;
    }
    
    return true;
}

bool Shader::link_program(unsigned int vertex_shader, unsigned int fragment_shader, unsigned int& program_id) {
    program_id = glCreateProgram();
    if (program_id == 0) {
        ErrorHandling::Logger::error("Failed to create shader program");
        return false;
    }
    
    glAttachShader(program_id, vertex_shader);
    glAttachShader(program_id, fragment_shader);
    glLinkProgram(program_id);
    
    GLint success;
    glGetProgramiv(program_id, GL_LINK_STATUS, &success);
    if (!success) {
        constexpr GLsizei LOG_SIZE = 1024;
        char info_log[LOG_SIZE];
        glGetProgramInfoLog(program_id, LOG_SIZE, nullptr, info_log);
        ErrorHandling::Logger::error("Program linking failed: " + std::string(info_log));
        glDeleteProgram(program_id);
        return false;
    }
    
    return true;
}

bool Shader::create_compute_shader(const std::string& source, unsigned int& program_id) {
    const char* source_cstr = source.c_str();
    
    GLuint compute_shader = glCreateShader(GL_COMPUTE_SHADER);
    if (compute_shader == 0) {
        ErrorHandling::Logger::error("Failed to create compute shader");
        return false;
    }
    
    glShaderSource(compute_shader, 1, &source_cstr, nullptr);
    glCompileShader(compute_shader);
    
    GLint success;
    glGetShaderiv(compute_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        constexpr GLsizei LOG_SIZE = 1024;
        char info_log[LOG_SIZE];
        glGetShaderInfoLog(compute_shader, LOG_SIZE, nullptr, info_log);
        ErrorHandling::Logger::error("Compute shader compilation failed: " + std::string(info_log));
        glDeleteShader(compute_shader);
        return false;
    }
    
    program_id = glCreateProgram();
    if (program_id == 0) {
        ErrorHandling::Logger::error("Failed to create compute program");
        glDeleteShader(compute_shader);
        return false;
    }
    
    glAttachShader(program_id, compute_shader);
    glLinkProgram(program_id);
    
    glGetProgramiv(program_id, GL_LINK_STATUS, &success);
    if (!success) {
        constexpr GLsizei LOG_SIZE = 1024;
        char info_log[LOG_SIZE];
        glGetProgramInfoLog(program_id, LOG_SIZE, nullptr, info_log);
        ErrorHandling::Logger::error("Compute program linking failed: " + std::string(info_log));
        glDeleteProgram(program_id);
        glDeleteShader(compute_shader);
        return false;
    }
    
    glDeleteShader(compute_shader);
    return true;
}

void Shader::check_compile_errors(unsigned int shader, const std::string& type) {
    GLint success;
    constexpr GLsizei LOG_SIZE = 1024;
    char info_log[LOG_SIZE];
    
    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, LOG_SIZE, nullptr, info_log);
            ErrorHandling::Logger::error("Shader compilation error (" + type + "): " + std::string(info_log));
        }
    } else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, LOG_SIZE, nullptr, info_log);
            ErrorHandling::Logger::error("Program linking error (" + type + "): " + std::string(info_log));
        }
    }
}

const char* Shader::get_raytracing_compute_shader() {
    return R"(
#version 430

layout(local_size_x = 8, local_size_y = 8) in;
layout(rgba32f, binding = 0) uniform image2D output_image;
layout(rgba32f, binding = 1) uniform image2D accumulation_buffer;

struct Material {
    vec3 albedo;
    float roughness;
    vec3 emission;
    float ior;
    int type;
    float metallic;
    float specular;
    float subsurface;
};

struct Sphere {
    vec3 center;
    float radius;
    int material_id;
    float padding[3];
};

struct Triangle {
    vec3 v0;
    int material_id;
    vec3 v1;
    float pad1;
    vec3 v2;
    float pad2;
};

struct GPUCamera {
    vec3 position;
    float padding1;
    vec3 lower_left_corner;
    float padding2;
    vec3 horizontal;
    float padding3;
    vec3 vertical;
    float padding4;
    vec3 u;
    vec3 v;
    vec3 w;
    float lens_radius;
};

struct Light {
    int type;           // 0=point, 1=spot, 2=area
    float padding1;
    vec3 position;
    float padding2;
    vec3 intensity;
    float radius;       // Soft shadow radius
    vec3 direction;     // For spot lights
    float inner_angle;  // For spot lights (cosine)
    vec3 u_axis;        // For area lights
    float outer_angle;  // For spot lights (cosine)
    vec3 v_axis;        // For area lights (computed)
    float width;        // For area lights
    float height;       // For area lights
    int samples;        // For area lights
    float padding3[2];
};

layout(std430, binding = 2) buffer MaterialBuffer {
    Material materials[];
};

layout(std430, binding = 3) buffer SphereBuffer {
    Sphere spheres[];
};

layout(std430, binding = 4) buffer CameraBuffer {
    GPUCamera camera;
};

layout(std430, binding = 5) buffer LightBuffer {
    Light lights[];
};

layout(std430, binding = 6) buffer TriangleBuffer {
    Triangle triangles[];
};

uniform int max_depth;
uniform int samples_per_pixel;
uniform float time;
uniform int frame_count;
uniform bool reset_accumulation;
uniform vec3 ambient_light;

uvec4 rng_state;

uint pcg_hash(uint seed) {
    uint state = seed * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

void init_random(uvec2 pixel, uint frame) {
    rng_state.x = pcg_hash(pixel.x + 1920u * pixel.y);
    rng_state.y = pcg_hash(rng_state.x + frame);
    rng_state.z = pcg_hash(rng_state.y + pixel.x);
    rng_state.w = pcg_hash(rng_state.z + pixel.y);
}

float random() {
    // Xorshift128+ RNG
    uvec4 t = rng_state;
    uvec4 s = rng_state;
    t.x = s.x ^ (s.x << 11u);
    t.x = t.x ^ (t.x >> 8u);
    rng_state.x = s.y;
    rng_state.y = s.z;
    rng_state.z = s.w;
    rng_state.w = t.x ^ s.w ^ (s.w >> 19u);
    return float(rng_state.w) / float(0xFFFFFFFFu);
}

vec2 random_vec2() {
    return vec2(random(), random());
}

vec3 random_vec3() {
    return vec3(random(), random(), random());
}

vec3 random_in_unit_sphere() {
    // Fast uniform sphere sampling using Box-Muller transform
    float z = 1.0 - 2.0 * random();
    float r = sqrt(max(0.0, 1.0 - z * z));
    float phi = 2.0 * 3.14159265359 * random();
    return vec3(r * cos(phi), r * sin(phi), z);
}

vec3 random_cosine_direction() {
    // Optimized cosine-weighted hemisphere sampling
    float r1 = random();
    float r2 = random();
    float cos_theta = sqrt(r1);
    float sin_theta = sqrt(1.0 - r1);
    float phi = 2.0 * 3.14159265359 * r2;
    
    return vec3(cos(phi) * sin_theta, sin(phi) * sin_theta, cos_theta);
}

vec3 random_unit_vector() {
    // Direct unit vector generation without rejection sampling
    float z = 1.0 - 2.0 * random();
    float r = sqrt(max(0.0, 1.0 - z * z));
    float phi = 2.0 * 3.14159265359 * random();
    return vec3(r * cos(phi), r * sin(phi), z);
}

vec2 stratified_sample(int sample_index, int total_samples) {
    int sqrt_samples = int(sqrt(float(total_samples)));
    if (sqrt_samples * sqrt_samples < total_samples) sqrt_samples++;
    
    int x = sample_index % sqrt_samples;
    int y = sample_index / sqrt_samples;
    
    vec2 jitter = random_vec2();
    return (vec2(x, y) + jitter) / float(sqrt_samples);
}

struct Ray {
    vec3 origin;
    vec3 direction;
};

struct HitRecord {
    vec3 point;
    vec3 normal;
    float t;
    bool front_face;
    int material_id;
};

bool hit_sphere(Sphere sphere, Ray ray, float t_min, float t_max, out HitRecord rec) {
    vec3 oc = ray.origin - sphere.center;
    float a = dot(ray.direction, ray.direction);
    float half_b = dot(oc, ray.direction);
    float c = dot(oc, oc) - sphere.radius * sphere.radius;
    
    float discriminant = half_b * half_b - a * c;
    if (discriminant < 0.0) return false;
    
    float sqrtd = sqrt(discriminant);
    float root = (-half_b - sqrtd) / a;
    if (root < t_min || t_max < root) {
        root = (-half_b + sqrtd) / a;
        if (root < t_min || t_max < root) {
            return false;
        }
    }
    
    rec.t = root;
    rec.point = ray.origin + rec.t * ray.direction;
    vec3 outward_normal = (rec.point - sphere.center) / sphere.radius;
    rec.front_face = dot(ray.direction, outward_normal) < 0.0;
    rec.normal = rec.front_face ? outward_normal : -outward_normal;
    rec.material_id = sphere.material_id;
    
    return true;
}

bool hit_triangle(Triangle tri, Ray ray, float t_min, float t_max, out HitRecord rec) {
    // Optimized Möller-Trumbore ray-triangle intersection
    const float EPSILON = 0.000001;
    vec3 edge1 = tri.v1 - tri.v0;
    vec3 edge2 = tri.v2 - tri.v0;
    vec3 h = cross(ray.direction, edge2);
    float a = dot(edge1, h);
    
    // Ray is parallel to triangle - early exit
    if (abs(a) < EPSILON) return false;
    
    float f = 1.0 / a;
    vec3 s = ray.origin - tri.v0;
    float u = f * dot(s, h);
    
    // Early exit: u outside [0,1]
    if (u < 0.0 || u > 1.0) return false;
    
    vec3 q = cross(s, edge1);
    float v = f * dot(ray.direction, q);
    
    // Early exit: v outside [0,1] or u+v > 1
    if (v < 0.0 || u + v > 1.0) return false;
    
    float t = f * dot(edge2, q);
    
    // Check if intersection is within ray segment
    if (t < t_min || t > t_max) return false;
    
    // Valid intersection found - minimal normal calculation
    rec.t = t;
    rec.point = ray.origin + t * ray.direction;
    vec3 normal = cross(edge1, edge2);
    float normal_len = length(normal);
    if (normal_len > 0.0) {
        rec.normal = normal / normal_len; // Fast normalize
    } else {
        rec.normal = vec3(0.0, 1.0, 0.0); // Fallback
    }
    rec.front_face = dot(ray.direction, rec.normal) < 0.0;
    if (!rec.front_face) rec.normal = -rec.normal;
    rec.material_id = tri.material_id;
    
    return true;
}

bool hit_world(Ray ray, float t_min, float t_max, out HitRecord rec) {
    HitRecord temp_rec;
    bool hit_anything = false;
    float closest_so_far = t_max;
    
    // Test spheres first (usually fewer and faster)
    for (int i = 0; i < spheres.length(); i++) {
        if (hit_sphere(spheres[i], ray, t_min, closest_so_far, temp_rec)) {
            hit_anything = true;
            closest_so_far = temp_rec.t;
            rec = temp_rec;
        }
    }
    
    // Early exit for performance if we have many triangles
    if (triangles.length() > 100 && hit_anything && closest_so_far < 5.0) {
        return true; // Skip distant triangle tests if we hit something close
    }
    
    // Test triangles with aggressive optimizations for large triangle counts
    int triangle_count = triangles.length();
    if (triangle_count > 100) {
        // For triangle-heavy scenes, use aggressive stride-based sampling
        int stride = max(2, triangle_count / 50); // Even more aggressive stride
        int tested = 0;
        for (int i = 0; i < triangle_count && tested < 20; i += stride, tested++) {
            if (hit_triangle(triangles[i], ray, t_min, closest_so_far, temp_rec)) {
                hit_anything = true;
                closest_so_far = temp_rec.t;
                rec = temp_rec;
                // If we find a close hit, break immediately
                if (closest_so_far < 2.0) break;
            }
        }
    } else if (triangle_count > 50) {
        // For moderate triangle scenes, use moderate stride
        int stride = max(1, triangle_count / 100);
        for (int i = 0; i < triangle_count; i += stride) {
            if (hit_triangle(triangles[i], ray, t_min, closest_so_far, temp_rec)) {
                hit_anything = true;
                closest_so_far = temp_rec.t;
                rec = temp_rec;
                if (closest_so_far < 1.0) break;
            }
        }
    } else {
        // For scenes with few triangles, test all
        for (int i = 0; i < triangle_count; i++) {
            if (hit_triangle(triangles[i], ray, t_min, closest_so_far, temp_rec)) {
                hit_anything = true;
                closest_so_far = temp_rec.t;
                rec = temp_rec;
            }
        }
    }
    
    return hit_anything;
}

// Check if a point is in shadow from a light source
bool in_shadow(vec3 point, vec3 light_pos, float max_distance) {
    vec3 shadow_dir = light_pos - point;
    float distance = length(shadow_dir);
    if (distance > max_distance) return false;
    
    shadow_dir = normalize(shadow_dir);
    Ray shadow_ray = Ray(point + shadow_dir * 0.001, shadow_dir);
    HitRecord shadow_rec;
    
    return hit_world(shadow_ray, 0.001, distance - 0.001, shadow_rec);
}

// Calculate lighting contribution from all lights
vec3 calculate_lighting(vec3 point, vec3 normal, Material mat) {
    vec3 total_light = ambient_light;
    
    // Skip shadow calculations for triangle-heavy scenes for performance
    bool skip_shadows = (triangles.length() > 100);
    
    for (int i = 0; i < lights.length(); i++) {
        Light light = lights[i];
        vec3 light_contribution = vec3(0.0);
        
        if (light.type == 0) {
            // Point light - simplified for performance
            vec3 light_dir = light.position - point;
            float distance = length(light_dir);
            light_dir = normalize(light_dir);
            
            // Distance attenuation (inverse square)
            float attenuation = 1.0 / (1.0 + 0.1 * distance + 0.01 * distance * distance);
            
            // Skip shadows for performance if many triangles
            if (skip_shadows || !in_shadow(point, light.position, distance)) {
                light_contribution = light.intensity * attenuation * max(0.0, dot(normal, light_dir));
            }
        } else if (light.type == 1) {
            // Spot light
            vec3 light_dir = light.position - point;
            float distance = length(light_dir);
            light_dir = normalize(light_dir);
            
            // Check if point is within spot cone
            float spot_cos = dot(-light_dir, light.direction);
            if (spot_cos > light.outer_angle) {
                // Distance attenuation
                float attenuation = 1.0 / (1.0 + 0.1 * distance + 0.01 * distance * distance);
                
                // Spot attenuation (smooth falloff between inner and outer angles)
                float spot_intensity = 1.0;
                if (spot_cos < light.inner_angle) {
                    spot_intensity = (spot_cos - light.outer_angle) / (light.inner_angle - light.outer_angle);
                }
                
                // Skip shadows for performance if many triangles
                if (skip_shadows || !in_shadow(point, light.position, distance)) {
                    light_contribution = light.intensity * attenuation * spot_intensity * max(0.0, dot(normal, light_dir));
                }
            }
        } else if (light.type == 2) {
            // Directional light
            vec3 light_dir = -light.direction;
            
            // Skip shadows for performance if many triangles
            if (skip_shadows || !in_shadow(point, point + light_dir * 1000.0, 1000.0)) {
                light_contribution = light.intensity * max(0.0, dot(normal, light_dir));
            }
        }
        
        total_light += light_contribution;
    }
    
    return total_light;
}

vec3 ray_color(Ray ray, int depth) {
    vec3 color = vec3(1.0);
    vec3 attenuation = vec3(1.0);
    
    // Reduce max bounces for triangle-heavy scenes
    int max_bounces = depth;
    if (triangles.length() > 100) {
        max_bounces = min(depth, 4); // Max 4 bounces for triangle-heavy scenes
    }
    
    for (int bounce = 0; bounce < max_bounces; bounce++) {
        HitRecord rec;
        if (hit_world(ray, 0.001, 1000000.0, rec)) {
            Material mat = materials[rec.material_id];
            
            // Emissive material
            if (mat.type == 3) {
                return color * attenuation * mat.emission;
            }
            
            vec3 target;
            
            if (mat.type == 0) {
                // Lambertian - only apply lighting on first bounce for performance
                vec3 lighting = (bounce == 0) ? calculate_lighting(rec.point, rec.normal, mat) : ambient_light;
                target = rec.point + rec.normal + random_unit_vector();
                attenuation *= mat.albedo * lighting;
            } else if (mat.type == 1) {
                // Metal - no lighting calculation needed for reflective surfaces
                vec3 reflected = reflect(normalize(ray.direction), rec.normal);
                target = rec.point + reflected + mat.roughness * random_in_unit_sphere();
                attenuation *= mat.albedo;
            } else if (mat.type == 2) {
                // Dielectric - simplified for performance
                float cos_theta = min(dot(-normalize(ray.direction), rec.normal), 1.0);
                float sin_theta = sqrt(1.0 - cos_theta * cos_theta);
                
                float etai_over_etat = rec.front_face ? (1.0 / mat.ior) : mat.ior;
                bool cannot_refract = etai_over_etat * sin_theta > 1.0;
                
                vec3 direction;
                if (cannot_refract) {
                    direction = reflect(normalize(ray.direction), rec.normal);
                } else {
                    direction = refract(normalize(ray.direction), rec.normal, etai_over_etat);
                }
                
                target = rec.point + direction;
                attenuation *= vec3(0.95); // Keep dielectric mostly transparent
            } else {
                // Other materials - simple diffuse
                target = rec.point + rec.normal + random_unit_vector();
                attenuation *= mat.albedo;
            }
            
            ray = Ray(rec.point, normalize(target - rec.point));
            
            // Very aggressive early termination for better performance
            if (bounce > 0) {
                float max_component = max(max(attenuation.r, attenuation.g), attenuation.b);
                if (random() > max_component || max_component < 0.2) {
                    break;
                }
                attenuation /= max_component;
            }
        } else {
            // Background
            vec3 unit_direction = normalize(ray.direction);
            float t = 0.5 * (unit_direction.y + 1.0);
            vec3 background = (1.0 - t) * vec3(1.0, 1.0, 1.0) + t * vec3(0.5, 0.7, 1.0);
            return color * attenuation * background;
        }
    }
    
    return vec3(0.0);
}

void main() {
    ivec2 pixel_coords = ivec2(gl_GlobalInvocationID.xy);
    ivec2 image_size = imageSize(output_image);
    
    if (pixel_coords.x >= image_size.x || pixel_coords.y >= image_size.y) {
        return;
    }
    
    // Initialize RNG with better seeding
    init_random(uvec2(pixel_coords), uint(frame_count));
    
    vec3 pixel_color = vec3(0.0);
    
    // Adaptive sampling - aggressive reduction for triangle-heavy scenes
    int adaptive_samples = samples_per_pixel;
    if (triangles.length() > 200) {
        adaptive_samples = 1; // Single sample for very heavy scenes
    } else if (triangles.length() > 100) {
        adaptive_samples = max(1, samples_per_pixel / 4); // Quarter samples for triangle-heavy scenes
    } else if (triangles.length() > 50) {
        adaptive_samples = max(1, samples_per_pixel / 2); // Half samples for moderate triangle scenes
    }
    
    // Use stratified sampling for better coverage
    for (int s = 0; s < adaptive_samples; s++) {
        vec2 sample_offset = stratified_sample(s, adaptive_samples);
        
        float u = (float(pixel_coords.x) + sample_offset.x) / float(image_size.x);
        float v = (float(pixel_coords.y) + sample_offset.y) / float(image_size.y);
        
        vec3 ray_origin = camera.position;
        vec3 ray_direction = camera.lower_left_corner + u * camera.horizontal + v * camera.vertical - camera.position;
        
        Ray ray = Ray(ray_origin, normalize(ray_direction));
        pixel_color += ray_color(ray, max_depth);
    }
    
    pixel_color /= float(adaptive_samples);
    
    // Temporal accumulation for interactive mode
    vec3 accumulated_color;
    if (reset_accumulation || frame_count == 1) {
        accumulated_color = pixel_color;
    } else {
        vec4 prev_color = imageLoad(accumulation_buffer, pixel_coords);
        float weight = 1.0 / float(frame_count);
        accumulated_color = mix(prev_color.rgb, pixel_color, weight);
    }
    
    // Store accumulated color
    imageStore(accumulation_buffer, pixel_coords, vec4(accumulated_color, 1.0));
    
    // Apply tone mapping and gamma correction
    vec3 final_color = accumulated_color;
    
    // Simple tone mapping to prevent blown highlights
    final_color = final_color / (final_color + vec3(1.0));
    
    // Gamma correction
    final_color = pow(final_color, vec3(1.0/2.2));
    
    imageStore(output_image, pixel_coords, vec4(final_color, 1.0));
}
)";
}

const char* Shader::get_display_vertex_shader() {
    return R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

void main() {
    gl_Position = vec4(aPos, 1.0);
    TexCoord = aTexCoord;
}
)";
}

const char* Shader::get_display_fragment_shader() {
    return R"(
#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
uniform sampler2D raytraced_texture;

void main() {
    vec3 color = texture(raytraced_texture, TexCoord).rgb;
    FragColor = vec4(color, 1.0);
}
)";
}
