#include "gpu_raytracer.h"
#include "shader.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>

GPURayTracer::GPURayTracer(int width, int height) 
    : window_width(width), window_height(height), num_materials(0), num_spheres(0),
      compute_shader(0), shader_program(0), output_texture(0), accumulation_texture(0),
      material_buffer(0), sphere_buffer(0), camera_buffer(0),
      frame_count(0), reset_accumulation(true) {
}

GPURayTracer::~GPURayTracer() {
    if (output_texture) glDeleteTextures(1, &output_texture);
    if (accumulation_texture) glDeleteTextures(1, &accumulation_texture);
    if (material_buffer) glDeleteBuffers(1, &material_buffer);
    if (sphere_buffer) glDeleteBuffers(1, &sphere_buffer);
    if (camera_buffer) glDeleteBuffers(1, &camera_buffer);
    if (shader_program) glDeleteProgram(shader_program);
}

bool GPURayTracer::initialize() {
    // Initialize GLEW if not already done
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return false;
    }
    
    // Check for compute shader support
    if (!GLEW_ARB_compute_shader) {
        std::cerr << "Compute shaders not supported" << std::endl;
        return false;
    }
    
    // Create compute shader program
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
    glGenBuffers(1, &camera_buffer);
    
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
        GLchar info_log[512];
        glGetShaderInfoLog(compute_shader, 512, nullptr, info_log);
        std::cerr << "Compute shader compilation failed: " << info_log << std::endl;
        return false;
    }
    
    // Create program
    shader_program = glCreateProgram();
    glAttachShader(shader_program, compute_shader);
    glLinkProgram(shader_program);
    
    // Check linking
    glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
    if (!success) {
        GLchar info_log[512];
        glGetProgramInfoLog(shader_program, 512, nullptr, info_log);
        std::cerr << "Compute program linking failed: " << info_log << std::endl;
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
        
        switch (mat->type) {
            case MaterialType::LAMBERTIAN: gpu_mat.type = 0; break;
            case MaterialType::METAL: gpu_mat.type = 1; break;
            case MaterialType::DIELECTRIC: gpu_mat.type = 2; break;
            case MaterialType::EMISSIVE: gpu_mat.type = 3; break;
        }
        
        gpu_materials.push_back(gpu_mat);
    }
    
    // Convert spheres to GPU format
    std::vector<GPUSphere> gpu_spheres;
    for (const auto& obj : scene.objects) {
        // Only handle spheres for now
        if (auto sphere = std::dynamic_pointer_cast<Sphere>(obj)) {
            GPUSphere gpu_sphere;
            gpu_sphere.center = sphere->center;
            gpu_sphere.radius = sphere->radius;
            gpu_sphere.material_id = sphere->material_id;
            gpu_spheres.push_back(gpu_sphere);
        }
    }
    
    num_materials = gpu_materials.size();
    num_spheres = gpu_spheres.size();
    
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
