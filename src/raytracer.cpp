#include "raytracer.h"
#include <iostream>
#include <iomanip>
#include <random>
#include <vector>
#include <thread>
#include <mutex>

Color RayTracer::ray_color(const Ray& ray, int depth, std::mt19937& rng) const {
    if (depth <= 0) {
        return Color(0, 0, 0);
    }
    
    HitRecord rec;
    if (scene.hit(ray, 0.001f, std::numeric_limits<float>::infinity(), rec)) {
        auto material = scene.get_material(rec.material_id);
        
        if (material) {
            // Handle emissive materials
            if (material->type == MaterialType::EMISSIVE) {
                return material->emission;
            }
            
            // For non-emissive materials, check if we want to use direct lighting or path tracing
            if (!scene.lights.empty() && depth == max_depth) {
                // Direct lighting for primary rays
                Vec3 view_dir = (ray.origin - rec.point).normalize();
                Color direct_light = scene.calculate_lighting(rec.point, rec.normal, view_dir, *material);
                
                // Still do some reflection for metals and glass
                if (material->type == MaterialType::METAL || material->type == MaterialType::DIELECTRIC) {
                    Color attenuation;
                    Ray scattered;
                    if (material->scatter(ray, rec, attenuation, scattered, rng)) {
                        Color reflected = ray_color(scattered, depth - 1, rng);
                        return direct_light * 0.3f + attenuation * reflected * 0.7f;
                    }
                }
                
                return direct_light;
            } else {
                // Path tracing for secondary rays
                Color attenuation;
                Ray scattered;
                if (material->scatter(ray, rec, attenuation, scattered, rng)) {
                    return attenuation * ray_color(scattered, depth - 1, rng);
                }
                return Color(0, 0, 0);
            }
        }
    }
    
    // Background gradient (sky)
    Vec3 unit_direction = ray.direction.normalize();
    float t = 0.5f * (unit_direction.y + 1.0f);
    return scene.background_color * (1.0f - t) + Color(1.0f, 1.0f, 1.0f) * t;
}

void RayTracer::render_tile(Image& image, int start_x, int start_y, int end_x, int end_y, 
                           std::atomic<int>& progress) const {
    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_real_distribution<float> dis(0.0f, 1.0f);
    
    for (int j = start_y; j < end_y; ++j) {
        for (int i = start_x; i < end_x; ++i) {
            Color pixel_color(0, 0, 0);
            
            for (int s = 0; s < samples_per_pixel; ++s) {
                float u = (float(i) + dis(rng)) / float(image.get_width());
                float v = (float(j) + dis(rng)) / float(image.get_height());
                
                Ray ray = scene.camera.get_ray(u, v, rng);
                pixel_color = pixel_color + ray_color(ray, max_depth, rng);
            }
            
            pixel_color = pixel_color / float(samples_per_pixel);
            image.set_pixel(i, image.get_height() - 1 - j, pixel_color);
        }
        
        // Update progress
        progress.fetch_add(end_x - start_x);
        
        // Print progress occasionally
        if (j % 10 == 0) {
            int total_pixels = image.get_width() * image.get_height();
            float percent = (float(progress.load()) / float(total_pixels)) * 100.0f;
            std::cout << "\rProgress: " << std::fixed << std::setprecision(1) << percent << "%" << std::flush;
        }
    }
}

void RayTracer::render(Image& image) const {
    std::cout << "Rendering " << image.get_width() << "x" << image.get_height() 
              << " image with " << samples_per_pixel << " samples per pixel using " 
              << num_threads << " threads..." << std::endl;
    
    std::atomic<int> progress{0};
    std::vector<std::thread> threads;
    
    int tile_height = image.get_height() / num_threads;
    
    for (int t = 0; t < num_threads; ++t) {
        int start_y = t * tile_height;
        int end_y = (t == num_threads - 1) ? image.get_height() : (t + 1) * tile_height;
        
        threads.emplace_back([this, &image, start_y, end_y, &progress]() {
            render_tile(image, 0, start_y, image.get_width(), end_y, progress);
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    std::cout << "\nRendering complete!" << std::endl;
}
