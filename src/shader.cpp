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
    vec3 v0, v1, v2;
    int material_id;
    float padding[3];
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
    // Rejection sampling
    vec3 p;
    int attempts = 0;
    do {
        p = 2.0 * random_vec3() - 1.0;
        attempts++;
    } while (dot(p, p) >= 1.0 && attempts < 10);
    
    if (attempts >= 10) {
        p = normalize(random_vec3() - 0.5);
    }
    return p;
}

vec3 random_cosine_direction() {
    float r1 = random();
    float r2 = random();
    float z = sqrt(1.0 - r2);
    
    float phi = 2.0 * 3.14159265359 * r1;
    float x = cos(phi) * sqrt(r2);
    float y = sin(phi) * sqrt(r2);
    
    return vec3(x, y, z);
}

vec3 random_unit_vector() {
    return normalize(random_in_unit_sphere());
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
    // Möller-Trumbore ray-triangle intersection algorithm
    vec3 edge1 = tri.v1 - tri.v0;
    vec3 edge2 = tri.v2 - tri.v0;
    vec3 h = cross(ray.direction, edge2);
    float a = dot(edge1, h);
    
    // Ray is parallel to triangle
    if (abs(a) < 1e-8) return false;
    
    float f = 1.0 / a;
    vec3 s = ray.origin - tri.v0;
    float u = f * dot(s, h);
    
    if (u < 0.0 || u > 1.0) return false;
    
    vec3 q = cross(s, edge1);
    float v = f * dot(ray.direction, q);
    
    if (v < 0.0 || u + v > 1.0) return false;
    
    float t = f * dot(edge2, q);
    
    if (t < t_min || t > t_max) return false;
    
    rec.t = t;
    rec.point = ray.origin + t * ray.direction;
    vec3 normal = normalize(cross(edge1, edge2));
    rec.front_face = dot(ray.direction, normal) < 0.0;
    rec.normal = rec.front_face ? normal : -normal;
    rec.material_id = tri.material_id;
    
    return true;
}

bool hit_world(Ray ray, float t_min, float t_max, out HitRecord rec) {
    HitRecord temp_rec;
    bool hit_anything = false;
    float closest_so_far = t_max;
    
    for (int i = 0; i < spheres.length(); i++) {
        if (hit_sphere(spheres[i], ray, t_min, closest_so_far, temp_rec)) {
            hit_anything = true;
            closest_so_far = temp_rec.t;
            rec = temp_rec;
        }
    }
    
    for (int i = 0; i < triangles.length(); i++) {
        if (hit_triangle(triangles[i], ray, t_min, closest_so_far, temp_rec)) {
            hit_anything = true;
            closest_so_far = temp_rec.t;
            rec = temp_rec;
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
    
    for (int i = 0; i < lights.length(); i++) {
        Light light = lights[i];
        vec3 light_contribution = vec3(0.0);
        
        if (light.type == 0) {
            // Point light
            vec3 light_dir = light.position - point;
            float distance = length(light_dir);
            light_dir = normalize(light_dir);
            
            // Distance attenuation (inverse square)
            float attenuation = 1.0 / (1.0 + 0.1 * distance + 0.01 * distance * distance);
            
            // Check shadow with soft shadows
            bool shadowed = false;
            if (light.radius > 0.0) {
                // Soft shadows - sample multiple points on light sphere
                int shadow_samples = 4;
                int shadow_hits = 0;
                for (int s = 0; s < shadow_samples; s++) {
                    vec3 offset = random_in_unit_sphere() * light.radius;
                    vec3 sample_pos = light.position + offset;
                    if (in_shadow(point, sample_pos, distance + light.radius)) {
                        shadow_hits++;
                    }
                }
                float shadow_factor = 1.0 - float(shadow_hits) / float(shadow_samples);
                light_contribution = light.intensity * attenuation * max(0.0, dot(normal, light_dir)) * shadow_factor;
            } else {
                // Hard shadows
                if (!in_shadow(point, light.position, distance)) {
                    light_contribution = light.intensity * attenuation * max(0.0, dot(normal, light_dir));
                }
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
                
                // Shadow testing
                if (!in_shadow(point, light.position, distance)) {
                    light_contribution = light.intensity * attenuation * spot_intensity * max(0.0, dot(normal, light_dir));
                }
            }
        } else if (light.type == 2) {
            // Area light - sample multiple points on the rectangular area
            int area_samples = max(1, light.samples);
            vec3 area_contribution = vec3(0.0);
            
            for (int s = 0; s < area_samples; s++) {
                // Random point on rectangle
                vec2 rand_uv = random_vec2() - 0.5; // [-0.5, 0.5]
                vec3 sample_pos = light.position + 
                                 rand_uv.x * light.width * light.u_axis + 
                                 rand_uv.y * light.height * light.v_axis;
                
                vec3 light_dir = sample_pos - point;
                float distance = length(light_dir);
                light_dir = normalize(light_dir);
                
                // Distance attenuation
                float attenuation = 1.0 / (1.0 + 0.1 * distance + 0.01 * distance * distance);
                
                // Shadow testing
                if (!in_shadow(point, sample_pos, distance)) {
                    area_contribution += light.intensity * attenuation * max(0.0, dot(normal, light_dir));
                }
            }
            light_contribution = area_contribution / float(area_samples);
        }
        
        total_light += light_contribution;
    }
    
    return total_light;
}

// Improved material scattering with importance sampling
vec3 ray_color(Ray ray, int depth) {
    vec3 color = vec3(1.0);
    vec3 attenuation = vec3(1.0);
    
    for (int bounce = 0; bounce < depth; bounce++) {
        HitRecord rec;
        if (hit_world(ray, 0.001, 1000000.0, rec)) {
            Material mat = materials[rec.material_id];
            
            // Emissive material - return accumulated emission
            if (mat.type == 3) {
                return color * attenuation * mat.emission;
            }
            
            vec3 target;
            vec3 new_attenuation = mat.albedo;
            
            if (mat.type == 0) {
                // Lambertian with proper lighting calculation
                vec3 lighting = calculate_lighting(rec.point, rec.normal, mat);
                
                // For path tracing, we still need to scatter the ray
                vec3 w = rec.normal;
                vec3 u = normalize(cross(abs(w.x) > 0.1 ? vec3(0,1,0) : vec3(1,0,0), w));
                vec3 v = cross(w, u);
                
                vec3 cosine_dir = random_cosine_direction();
                vec3 scatter_direction = cosine_dir.x * u + cosine_dir.y * v + cosine_dir.z * w;
                
                target = rec.point + scatter_direction;
                // Apply lighting to the diffuse material
                new_attenuation = mat.albedo * lighting;
            } else if (mat.type == 1) {
                // Metal with better reflection handling and lighting
                vec3 reflected = reflect(normalize(ray.direction), rec.normal);
                vec3 fuzzed = reflected + mat.roughness * random_in_unit_sphere();
                target = rec.point + fuzzed;
                
                // Check if reflection is valid
                if (dot(fuzzed, rec.normal) <= 0.0) {
                    return vec3(0.0);
                }
                
                // Apply lighting to metal surfaces too
                vec3 lighting = calculate_lighting(rec.point, rec.normal, mat);
                new_attenuation = mat.albedo * lighting;
            } else if (mat.type == 2) {
                // Improved dielectric with Schlick approximation
                float cos_theta = min(dot(-normalize(ray.direction), rec.normal), 1.0);
                float sin_theta = sqrt(1.0 - cos_theta * cos_theta);
                
                float etai_over_etat = rec.front_face ? (1.0 / mat.ior) : mat.ior;
                bool cannot_refract = etai_over_etat * sin_theta > 1.0;
                
                // Schlick's approximation for reflectance
                float r0 = (1.0 - etai_over_etat) / (1.0 + etai_over_etat);
                r0 = r0 * r0;
                float reflectance = r0 + (1.0 - r0) * pow((1.0 - cos_theta), 5.0);
                
                vec3 direction;
                if (cannot_refract || reflectance > random()) {
                    direction = reflect(normalize(ray.direction), rec.normal);
                } else {
                    direction = refract(normalize(ray.direction), rec.normal, etai_over_etat);
                }
                
                target = rec.point + direction;
                // Apply subtle lighting even to glass (for colored glass effects)
                vec3 lighting = calculate_lighting(rec.point, rec.normal, mat);
                new_attenuation = mix(vec3(1.0), mat.albedo * lighting, 0.1);
            } else if (mat.type == 4) {
                // Glossy material - blend between diffuse and specular reflection
                vec3 reflected = reflect(normalize(ray.direction), rec.normal);
                vec3 diffuse_dir = random_cosine_direction();
                
                // Create orthonormal basis around normal
                vec3 w = rec.normal;
                vec3 u = normalize(cross(abs(w.x) > 0.1 ? vec3(0,1,0) : vec3(1,0,0), w));
                vec3 v = cross(w, u);
                vec3 diffuse_target = diffuse_dir.x * u + diffuse_dir.y * v + diffuse_dir.z * w;
                
                // Blend between specular and diffuse based on roughness
                vec3 specular_target = reflected + mat.roughness * random_in_unit_sphere();
                target = rec.point + mix(specular_target, diffuse_target, mat.roughness);
                
                // Mix specular reflection and diffuse color
                vec3 lighting = calculate_lighting(rec.point, rec.normal, mat);
                new_attenuation = mix(vec3(mat.specular), mat.albedo, mat.roughness) * lighting;
            } else if (mat.type == 5) {
                // Subsurface scattering approximation
                vec3 w = rec.normal;
                vec3 u = normalize(cross(abs(w.x) > 0.1 ? vec3(0,1,0) : vec3(1,0,0), w));
                vec3 v = cross(w, u);
                
                // Add some randomness for subsurface effect
                vec3 scatter_dir = random_cosine_direction();
                // Bend direction slightly inward for subsurface effect
                scatter_dir = normalize(mix(scatter_dir, -rec.normal, mat.subsurface * 0.3));
                
                target = rec.point + scatter_dir.x * u + scatter_dir.y * v + scatter_dir.z * w;
                
                // Subsurface materials absorb and scatter light
                vec3 lighting = calculate_lighting(rec.point, rec.normal, mat);
                new_attenuation = mat.albedo * (1.0 + mat.subsurface) * lighting;
            }
            
            // Update ray for next iteration
            ray = Ray(rec.point, normalize(target - rec.point));
            attenuation *= new_attenuation;
            
            // Russian roulette for energy conservation at higher bounces
            if (bounce > 3) {
                float max_component = max(max(attenuation.r, attenuation.g), attenuation.b);
                if (random() > max_component) {
                    break;
                }
                attenuation /= max_component;
            }
        } else {
            // Background hit - return accumulated color with background
            vec3 unit_direction = normalize(ray.direction);
            float t = 0.5 * (unit_direction.y + 1.0);
            vec3 background = (1.0 - t) * vec3(1.0, 1.0, 1.0) + t * vec3(0.5, 0.7, 1.0);
            return color * attenuation * background;
        }
    }
    
    // Ray didn't terminate, return black (energy absorbed)
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
    
    // Use stratified sampling for better coverage
    for (int s = 0; s < samples_per_pixel; s++) {
        vec2 sample_offset = stratified_sample(s, samples_per_pixel);
        
        float u = (float(pixel_coords.x) + sample_offset.x) / float(image_size.x);
        float v = (float(pixel_coords.y) + sample_offset.y) / float(image_size.y);
        
        vec3 ray_origin = camera.position;
        vec3 ray_direction = camera.lower_left_corner + u * camera.horizontal + v * camera.vertical - camera.position;
        
        Ray ray = Ray(ray_origin, normalize(ray_direction));
        pixel_color += ray_color(ray, max_depth);
    }
    
    pixel_color /= float(samples_per_pixel);
    
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
