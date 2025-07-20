#include "gpu_raytracer.h"
#include "shader.h"
#include "error_handling.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <cmath>

GPURayTracer::GPURayTracer(int width, int height) 
    : window_width(width), window_height(height), num_materials(0), num_spheres(0), num_triangles(0), num_cylinders(0), num_lights(0),
      compute_shader(0), shader_program(0), output_texture(0), accumulation_texture(0),
      material_buffer(0), sphere_buffer(0), triangle_buffer(0), cylinder_buffer(0), camera_buffer(0), light_buffer(0),
      ambient_light(0.1f, 0.1f, 0.1f), frame_count(0), reset_accumulation(true) {
}

GPURayTracer::~GPURayTracer() {
    if (output_texture) glDeleteTextures(1, &output_texture);
    if (accumulation_texture) glDeleteTextures(1, &accumulation_texture);
    if (material_buffer) glDeleteBuffers(1, &material_buffer);
    if (sphere_buffer) glDeleteBuffers(1, &sphere_buffer);
    if (triangle_buffer) glDeleteBuffers(1, &triangle_buffer);
    if (cylinder_buffer) glDeleteBuffers(1, &cylinder_buffer);
    if (camera_buffer) glDeleteBuffers(1, &camera_buffer);
    if (light_buffer) glDeleteBuffers(1, &light_buffer);
    if (shader_program) glDeleteProgram(shader_program);
}

bool GPURayTracer::initialize() {
    if (glewInit() != GLEW_OK) {
        ErrorHandling::Logger::error("Failed to initialize GLEW");
        return false;
    }
    
    if (!GLEW_ARB_compute_shader) {
        ErrorHandling::Logger::error("Compute shaders not supported");
        return false;
    }
    
    if (!create_compute_program()) {
        return false;
    }
    
    // Create output texture
    glGenTextures(1, &output_texture);
    glBindTexture(GL_TEXTURE_2D, output_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, window_width, window_height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindImageTexture(0, output_texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
    
    // Create accumulation texture for temporal denoising
    glGenTextures(1, &accumulation_texture);
    glBindTexture(GL_TEXTURE_2D, accumulation_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, window_width, window_height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindImageTexture(1, accumulation_texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
    
    // Generate buffer objects
    glGenBuffers(1, &material_buffer);
    glGenBuffers(1, &sphere_buffer);
    glGenBuffers(1, &triangle_buffer);
    glGenBuffers(1, &cylinder_buffer);
    glGenBuffers(1, &camera_buffer);
    glGenBuffers(1, &light_buffer);
    
    return true;
}

bool GPURayTracer::create_compute_program() {
    const char* compute_source = Shader::get_raytracing_compute_shader();
    
    compute_shader = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(compute_shader, 1, &compute_source, nullptr);
    glCompileShader(compute_shader);
    
    // Check compilation
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
    
    // Create program
    shader_program = glCreateProgram();
    glAttachShader(shader_program, compute_shader);
    glLinkProgram(shader_program);
    
    // Check linking
    glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
    if (!success) {
        constexpr GLsizei LOG_SIZE = 1024;
        char info_log[LOG_SIZE];
        glGetProgramInfoLog(shader_program, LOG_SIZE, nullptr, info_log);
        ErrorHandling::Logger::error("Compute program linking failed: " + std::string(info_log));
        glDeleteProgram(shader_program);
        glDeleteShader(compute_shader);
        return false;
    }
    
    glDeleteShader(compute_shader);
    return true;
}

void GPURayTracer::load_scene(const Scene& scene) {
    // Convert materials to GPU format
    std::vector<GPUMaterial> gpu_materials;
    for (const auto& mat : scene.materials) {
        GPUMaterial gpu_mat;
        gpu_mat.albedo = mat->albedo;
        gpu_mat.roughness = mat->roughness;
        gpu_mat.emission = mat->emission;
        gpu_mat.ior = mat->ior;
        gpu_mat.metallic = mat->metallic;
        gpu_mat.specular = mat->specular;
        gpu_mat.subsurface = mat->subsurface;
        
        switch (mat->type) {
            case MaterialType::LAMBERTIAN: gpu_mat.type = 0; break;
            case MaterialType::METAL: gpu_mat.type = 1; break;
            case MaterialType::DIELECTRIC: gpu_mat.type = 2; break;
            case MaterialType::EMISSIVE: gpu_mat.type = 3; break;
            case MaterialType::GLOSSY: gpu_mat.type = 4; break;
            case MaterialType::SUBSURFACE: gpu_mat.type = 5; break;
        }
        
        gpu_materials.push_back(gpu_mat);
    }
    
    // Convert spheres, triangles, and cylinders to GPU format
    std::vector<GPUSphere> gpu_spheres;
    std::vector<GPUTriangle> gpu_triangles;
    std::vector<GPUCylinder> gpu_cylinders;
    int triangle_count = 0;
    int sphere_count = 0;
    int cylinder_count = 0;
    
    for (const auto& obj : scene.objects) {
        if (auto sphere = std::dynamic_pointer_cast<Sphere>(obj)) {
            GPUSphere gpu_sphere;
            gpu_sphere.center = sphere->center;
            gpu_sphere.radius = sphere->radius;
            gpu_sphere.material_id = sphere->material_id;
            gpu_spheres.push_back(gpu_sphere);
            sphere_count++;
        } else if (auto triangle = std::dynamic_pointer_cast<Triangle>(obj)) {
            GPUTriangle gpu_triangle;
            gpu_triangle.v0 = triangle->v0;
            gpu_triangle.v1 = triangle->v1;
            gpu_triangle.v2 = triangle->v2;
            gpu_triangle.material_id = triangle->material_id;
            gpu_triangles.push_back(gpu_triangle);
            triangle_count++;
        } else if (auto cylinder = std::dynamic_pointer_cast<Cylinder>(obj)) {
            GPUCylinder gpu_cylinder;
            gpu_cylinder.base_center = cylinder->base_center;
            gpu_cylinder.axis = cylinder->axis;
            gpu_cylinder.radius = cylinder->radius;
            gpu_cylinder.height = cylinder->height;
            gpu_cylinder.material_id = cylinder->material_id;
            gpu_cylinders.push_back(gpu_cylinder);
            cylinder_count++;
        }
    }
    
    // Log loading statistics
    ErrorHandling::Logger::info("Scene loading: " + std::to_string(scene.objects.size()) + " total objects, " +
                                std::to_string(sphere_count) + " spheres, " + 
                                std::to_string(triangle_count) + " triangles, " +
                                std::to_string(cylinder_count) + " cylinders");
    
    num_materials = gpu_materials.size();
    num_spheres = gpu_spheres.size();
    num_triangles = gpu_triangles.size();
    num_cylinders = gpu_cylinders.size();
    
    // Convert lights to GPU format
    std::vector<GPULight> gpu_lights;
    for (const auto& light : scene.lights) {
        GPULight gpu_light = {};
        
        if (auto point_light = std::dynamic_pointer_cast<PointLight>(light)) {
            gpu_light.type = 0; // Point
            gpu_light.position = point_light->position;
            gpu_light.intensity = point_light->intensity;
            gpu_light.radius = point_light->radius;
        } else if (auto spot_light = std::dynamic_pointer_cast<SpotLight>(light)) {
            gpu_light.type = 1; // Spot
            gpu_light.position = spot_light->position;
            gpu_light.intensity = spot_light->intensity;
            gpu_light.radius = spot_light->radius;
            gpu_light.direction = spot_light->direction;
            gpu_light.inner_angle = cos(spot_light->inner_angle * M_PI / 180.0f); // Convert to cosine
            gpu_light.outer_angle = cos(spot_light->outer_angle * M_PI / 180.0f); // Convert to cosine
        } else if (auto area_light = std::dynamic_pointer_cast<AreaPlaneLight>(light)) {
            gpu_light.type = 2; // Area
            gpu_light.position = area_light->position;
            gpu_light.intensity = area_light->intensity;
            gpu_light.u_axis = area_light->u_axis;
            gpu_light.v_axis = area_light->v_axis;
            gpu_light.width = area_light->width;
            gpu_light.height = area_light->height;
            gpu_light.samples = area_light->samples;
        }
        
        gpu_lights.push_back(gpu_light);
    }
    
    num_lights = gpu_lights.size();
    
    // Upload materials
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, material_buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, gpu_materials.size() * sizeof(GPUMaterial), 
                 gpu_materials.data(), GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, material_buffer);
    
    // Upload spheres
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, sphere_buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, gpu_spheres.size() * sizeof(GPUSphere), 
                 gpu_spheres.data(), GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, sphere_buffer);
    
    // Upload triangles
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, triangle_buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, gpu_triangles.size() * sizeof(GPUTriangle), 
                 gpu_triangles.data(), GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, triangle_buffer);
    
    // Upload cylinders
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, cylinder_buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, gpu_cylinders.size() * sizeof(GPUCylinder), 
                 gpu_cylinders.data(), GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, cylinder_buffer);

    // Upload lights
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, light_buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, gpu_lights.size() * sizeof(GPULight), 
                 gpu_lights.data(), GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, light_buffer);
    
    // Store ambient light
    ambient_light = scene.ambient_light;
    
    // Reset accumulation when scene changes
    reset_accumulation_buffer();
}

void GPURayTracer::update_camera(const Camera& camera) {
    GPUCamera gpu_camera;
    gpu_camera.position = camera.position;
    gpu_camera.lower_left_corner = camera.lower_left_corner;
    gpu_camera.horizontal = camera.horizontal;
    gpu_camera.vertical = camera.vertical;
    gpu_camera.u = camera.u;
    gpu_camera.v = camera.v;
    gpu_camera.w = camera.w;
    gpu_camera.lens_radius = camera.lens_radius;
    
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, camera_buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GPUCamera), &gpu_camera, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, camera_buffer);
}

void GPURayTracer::render(const Camera& camera, int samples, int max_depth) {
    update_camera(camera);
    
    glUseProgram(shader_program);
    
    // Increment frame count for temporal accumulation
    frame_count++;
    
    // Set uniforms
    glUniform1i(glGetUniformLocation(shader_program, "max_depth"), max_depth);
    glUniform1i(glGetUniformLocation(shader_program, "samples_per_pixel"), samples);
    glUniform1f(glGetUniformLocation(shader_program, "time"), 
                static_cast<float>(glfwGetTime()));
    glUniform1i(glGetUniformLocation(shader_program, "frame_count"), frame_count);
    glUniform1i(glGetUniformLocation(shader_program, "reset_accumulation"), reset_accumulation ? 1 : 0);
    glUniform3f(glGetUniformLocation(shader_program, "ambient_light"), 
                ambient_light.x, ambient_light.y, ambient_light.z);
    
    // Clear reset flag after first use
    if (reset_accumulation) {
        reset_accumulation = false;
    }
    
    // Dispatch compute shader
    glDispatchCompute((window_width + 7) / 8, (window_height + 7) / 8, 1);
    
    // Wait for completion
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

void GPURayTracer::resize(int width, int height) {
    window_width = width;
    window_height = height;
    
    // Recreate output texture
    if (output_texture) {
        glDeleteTextures(1, &output_texture);
    }
    
    glGenTextures(1, &output_texture);
    glBindTexture(GL_TEXTURE_2D, output_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindImageTexture(0, output_texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
    
    // Recreate accumulation texture
    if (accumulation_texture) {
        glDeleteTextures(1, &accumulation_texture);
    }
    
    glGenTextures(1, &accumulation_texture);
    glBindTexture(GL_TEXTURE_2D, accumulation_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindImageTexture(1, accumulation_texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
    
    // Reset accumulation when resizing
    reset_accumulation_buffer();
}
