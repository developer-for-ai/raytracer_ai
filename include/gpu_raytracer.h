#pragma once
#include "common.h"
#include "scene.h"
#include <memory>

// Forward declarations to avoid including OpenGL headers in header
typedef unsigned int GLuint;

struct GPUMaterial {
    Vec3 albedo;
    float roughness;
    Vec3 emission; 
    float ior;
    int type; // 0=lambertian, 1=metal, 2=dielectric, 3=emissive, 4=glossy, 5=subsurface
    float metallic;
    float specular;
    float subsurface;
};

struct GPUSphere {
    Vec3 center;
    float radius;
    int material_id;
    float padding[3];
};

struct GPUCamera {
    Vec3 position;
    float padding1;
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
    GLuint camera_buffer;
    GLuint light_buffer;
    
    int window_width, window_height;
    int num_materials, num_spheres, num_lights;
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
