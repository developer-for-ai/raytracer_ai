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

layout(std430, binding = 1) buffer MaterialBuffer {
    Material materials[];
};

layout(std430, binding = 2) buffer SphereBuffer {
    Sphere spheres[];
};

layout(std430, binding = 3) buffer CameraBuffer {
    GPUCamera camera;
};

uniform int max_depth;
uniform int samples_per_pixel;
uniform float time;

uint rng_state = 0u;

uint wang_hash(uint seed) {
    seed = (seed ^ 61u) ^ (seed >> 16u);
    seed *= 9u;
    seed = seed ^ (seed >> 4u);
    seed *= 0x27d4eb2du;
    seed = seed ^ (seed >> 15u);
    return seed;
}

float random() {
    rng_state = wang_hash(rng_state);
    return float(rng_state) / float(0xFFFFFFFFu);
}

vec3 random_vec3() {
    return vec3(random(), random(), random());
}

vec3 random_in_unit_sphere() {
    vec3 p;
    do {
        p = 2.0 * random_vec3() - 1.0;
    } while (dot(p, p) >= 1.0);
    return p;
}

vec3 random_unit_vector() {
    return normalize(random_in_unit_sphere());
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

vec3 ray_color(Ray ray, int depth) {
    vec3 color = vec3(1.0);
    vec3 attenuation = vec3(1.0);
    
    for (int bounce = 0; bounce < depth; bounce++) {
        HitRecord rec;
        if (hit_world(ray, 0.001, 1000000.0, rec)) {
            Material mat = materials[rec.material_id];
            
            // Emissive material
            if (mat.type == 3) {
                return color * mat.emission;
            }
            
            vec3 target;
            vec3 new_attenuation = mat.albedo;
            
            if (mat.type == 0) {
                // Lambertian
                target = rec.point + rec.normal + random_unit_vector();
            } else if (mat.type == 1) {
                // Metal
                vec3 reflected = reflect(normalize(ray.direction), rec.normal);
                target = rec.point + reflected + mat.roughness * random_in_unit_sphere();
                if (dot(target - rec.point, rec.normal) <= 0.0) {
                    return vec3(0.0);
                }
            } else if (mat.type == 2) {
                // Dielectric (simplified)
                vec3 refracted = refract(normalize(ray.direction), rec.normal, 1.0 / mat.ior);
                target = rec.point + refracted;
                new_attenuation = vec3(1.0);
            }
            
            // Update ray for next iteration
            ray = Ray(rec.point, normalize(target - rec.point));
            attenuation *= new_attenuation;
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
    
    // Initialize RNG
    rng_state = wang_hash(pixel_coords.y * image_size.x + pixel_coords.x + uint(time * 1000000.0));
    
    vec3 color = vec3(0.0);
    
    for (int s = 0; s < samples_per_pixel; s++) {
        float u = (float(pixel_coords.x) + random()) / float(image_size.x);
        float v = (float(pixel_coords.y) + random()) / float(image_size.y);
        
        vec3 ray_origin = camera.position;
        vec3 ray_direction = camera.lower_left_corner + u * camera.horizontal + v * camera.vertical - camera.position;
        
        Ray ray = Ray(ray_origin, normalize(ray_direction));
        color += ray_color(ray, max_depth);
    }
    
    color /= float(samples_per_pixel);
    
    // Gamma correction
    color = sqrt(color);
    
    imageStore(output_image, pixel_coords, vec4(color, 1.0));
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
