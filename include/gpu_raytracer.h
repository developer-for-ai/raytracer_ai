#pragma once
#include "common.h"
#include "scene.h"
#include <memory>

// Forward declarations to avoid including OpenGL headers in header
typedef unsigned int GLuint;

// GPU-aligned structures (std140 layout compatible)
struct alignas(16) GPUMaterial {
    Vec3 albedo;
    float roughness;
    Vec3 emission; 
    float ior;
    int type; // MaterialType as int for GPU compatibility
    float metallic;
    float specular;
    float subsurface;
    
    GPUMaterial() = default;
    explicit GPUMaterial(const Material& mat) noexcept 
        : albedo(mat.albedo), roughness(mat.roughness), emission(mat.emission),
          ior(mat.ior), type(static_cast<int>(mat.type)), metallic(mat.metallic),
          specular(mat.specular), subsurface(mat.subsurface) {}
};

struct alignas(16) GPUSphere {
    Vec3 center;
    float radius;
    int material_id;
    float _padding[3]; // Explicit padding for alignment
    
    GPUSphere() = default;
    GPUSphere(const Vec3& c, float r, int mat_id) noexcept 
        : center(c), radius(r), material_id(mat_id), _padding{0, 0, 0} {}
};

struct alignas(16) GPUTriangle {
    Vec3 v0;             // First vertex
    int material_id;     // Material ID
    Vec3 v1;             // Second vertex  
    float _pad1;         // Padding
    Vec3 v2;             // Third vertex
    float _pad2;         // Padding
    
    GPUTriangle() = default;
    GPUTriangle(const Vec3& a, const Vec3& b, const Vec3& c, int mat_id) noexcept
        : v0(a), material_id(mat_id), v1(b), _pad1(0), v2(c), _pad2(0) {}
};

struct alignas(16) GPUCamera {
    Vec3 position;
    float _padding1;
    Vec3 lower_left_corner;
    float padding2;
    Vec3 horizontal;
    float padding3;
    Vec3 vertical;
    float padding4;
    Vec3 u, v, w;
    float lens_radius;
};

struct GPULight {
    int type;           // 0=point, 1=spot, 2=area
    float padding1;
    Vec3 position;
    float padding2;
    Vec3 intensity;
    float radius;       // Soft shadow radius
    Vec3 direction;     // For spot lights
    float inner_angle;  // For spot lights (cosine)
    Vec3 u_axis;        // For area lights
    float outer_angle;  // For spot lights (cosine)
    Vec3 v_axis;        // For area lights (computed)
    float width;        // For area lights
    float height;       // For area lights
    int samples;        // For area lights
    float padding3[2];
};

class GPURayTracer {
private:
    GLuint compute_shader;
    GLuint shader_program;
    GLuint output_texture;
    GLuint accumulation_texture;  // For temporal accumulation
    GLuint material_buffer;
    GLuint sphere_buffer;
    GLuint triangle_buffer;       // Triangle buffer
    GLuint camera_buffer;
    GLuint light_buffer;
    
    int window_width, window_height;
    int num_materials, num_spheres, num_triangles, num_lights;
    Vec3 ambient_light;
    int frame_count;              // For temporal accumulation
    bool reset_accumulation;      // Reset flag for camera movement
    
    bool compile_shader(const std::string& source, GLuint& shader);
    bool create_compute_program();
    void setup_buffers(const Scene& scene);
    
public:
    GPURayTracer(int width, int height);
    ~GPURayTracer();
    
    bool initialize();
    void load_scene(const Scene& scene);
    void render(const Camera& camera, int samples, int max_depth);
    void resize(int width, int height);
    
    // Reset accumulation buffer (call when camera moves)
    void reset_accumulation_buffer() { 
        reset_accumulation = true; 
        frame_count = 0; 
    }
    
    GLuint get_output_texture() const { return output_texture; }
    
    // Update camera without full scene reload
    void update_camera(const Camera& camera);
};
