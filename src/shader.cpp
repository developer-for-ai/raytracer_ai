#include "shader.h"
#include <GL/glew.h>
#include <iostream>
#include <fstream>
#include <sstream>

std::string Shader::load_file(const std::string& filepath) {
    std::ifstream file(filepath);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

bool Shader::compile_shader(const std::string& source, unsigned int type, unsigned int& shader_id) {
    const char* source_cstr = source.c_str();
    
    shader_id = glCreateShader(type);
    glShaderSource(shader_id, 1, &source_cstr, nullptr);
    glCompileShader(shader_id);
    
    GLint success;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar info_log[512];
        glGetShaderInfoLog(shader_id, 512, nullptr, info_log);
        std::cerr << "Shader compilation failed: " << info_log << std::endl;
        return false;
    }
    
    return true;
}

bool Shader::link_program(unsigned int vertex_shader, unsigned int fragment_shader, unsigned int& program_id) {
    program_id = glCreateProgram();
    glAttachShader(program_id, vertex_shader);
    glAttachShader(program_id, fragment_shader);
    glLinkProgram(program_id);
    
    GLint success;
    glGetProgramiv(program_id, GL_LINK_STATUS, &success);
    if (!success) {
        GLchar info_log[512];
        glGetProgramInfoLog(program_id, 512, nullptr, info_log);
        std::cerr << "Program linking failed: " << info_log << std::endl;
        return false;
    }
    
    return true;
}

bool Shader::create_compute_shader(const std::string& source, unsigned int& program_id) {
    const char* source_cstr = source.c_str();
    
    GLuint compute_shader = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(compute_shader, 1, &source_cstr, nullptr);
    glCompileShader(compute_shader);
    
    GLint success;
    glGetShaderiv(compute_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar info_log[512];
        glGetShaderInfoLog(compute_shader, 512, nullptr, info_log);
        std::cerr << "Compute shader compilation failed: " << info_log << std::endl;
        glDeleteShader(compute_shader);
        return false;
    }
    
    program_id = glCreateProgram();
    glAttachShader(program_id, compute_shader);
    glLinkProgram(program_id);
    
    glGetProgramiv(program_id, GL_LINK_STATUS, &success);
    if (!success) {
        GLchar info_log[512];
        glGetProgramInfoLog(program_id, 512, nullptr, info_log);
        std::cerr << "Compute program linking failed: " << info_log << std::endl;
        glDeleteShader(compute_shader);
        return false;
    }
    
    glDeleteShader(compute_shader);
    return true;
}

void Shader::check_compile_errors(unsigned int shader, const std::string& type) {
    GLint success;
    GLchar info_log[1024];
    
    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, nullptr, info_log);
            std::cerr << "Shader compilation error (" << type << "): " << info_log << std::endl;
        }
    } else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, nullptr, info_log);
            std::cerr << "Program linking error (" << type << "): " << info_log << std::endl;
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
    float padding[3];
};

struct Sphere {
    vec3 center;
    float radius;
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

layout(std430, binding = 2) buffer MaterialBuffer {
    Material materials[];
};

layout(std430, binding = 3) buffer SphereBuffer {
    Sphere spheres[];
};

layout(std430, binding = 4) buffer CameraBuffer {
    GPUCamera camera;
};

uniform int max_depth;
uniform int samples_per_pixel;
uniform float time;
uniform int frame_count;
uniform bool reset_accumulation;

// Improved RNG state - use multiple seeds for better distribution
uvec4 rng_state;

// Better hash function for improved random distribution
uint pcg_hash(uint seed) {
    uint state = seed * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

// Initialize RNG with better seeding
void init_random(uvec2 pixel, uint frame) {
    rng_state.x = pcg_hash(pixel.x + 1920u * pixel.y);
    rng_state.y = pcg_hash(rng_state.x + frame);
    rng_state.z = pcg_hash(rng_state.y + pixel.x);
    rng_state.w = pcg_hash(rng_state.z + pixel.y);
}

// Xorshift128+ RNG for better quality randomness
float random() {
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

// Improved sampling in unit sphere using rejection sampling with early bailout
vec3 random_in_unit_sphere() {
    vec3 p;
    int attempts = 0;
    do {
        p = 2.0 * random_vec3() - 1.0;
        attempts++;
    } while (dot(p, p) >= 1.0 && attempts < 10);
    
    // Fallback to normalize if rejection sampling fails
    if (attempts >= 10) {
        p = normalize(random_vec3() - 0.5);
    }
    return p;
}

// Cosine-weighted hemisphere sampling for better importance sampling
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

// Blue noise-like stratified sampling for better pixel coverage
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
    
    return hit_anything;
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
                // Lambertian with cosine-weighted importance sampling
                vec3 w = rec.normal;
                vec3 u = normalize(cross(abs(w.x) > 0.1 ? vec3(0,1,0) : vec3(1,0,0), w));
                vec3 v = cross(w, u);
                
                vec3 cosine_dir = random_cosine_direction();
                vec3 scatter_direction = cosine_dir.x * u + cosine_dir.y * v + cosine_dir.z * w;
                
                target = rec.point + scatter_direction;
                // Cosine-weighted sampling already accounts for Lambert's law
                new_attenuation *= mat.albedo;
            } else if (mat.type == 1) {
                // Metal with better reflection handling
                vec3 reflected = reflect(normalize(ray.direction), rec.normal);
                vec3 fuzzed = reflected + mat.roughness * random_in_unit_sphere();
                target = rec.point + fuzzed;
                
                // Check if reflection is valid
                if (dot(fuzzed, rec.normal) <= 0.0) {
                    return vec3(0.0);
                }
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
                new_attenuation = vec3(1.0); // Glass doesn't absorb light
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
