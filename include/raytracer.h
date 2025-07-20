#pragma once
#include "scene.h"
#include "image.h"
#include "parser.h"
#include <thread>
#include <atomic>

class RayTracer {
private:
    Scene scene;
    int max_depth;
    int samples_per_pixel;
    int num_threads;
    
    Color ray_color(const Ray& ray, int depth, std::mt19937& rng) const;
    void render_tile(Image& image, int start_x, int start_y, int end_x, int end_y, 
                    std::atomic<int>& progress) const;
    
public:
    RayTracer(int depth = 50, int samples = 100, int threads = 0) 
        : max_depth(depth), samples_per_pixel(samples) {
        num_threads = threads > 0 ? threads : std::thread::hardware_concurrency();
    }
    
    void load_scene(const std::string& filename) {
        scene = Parser::parse_scene_file(filename);
        scene.build_acceleration_structure();
    }
    
    void render(Image& image) const;
    
    // Getters/setters
    void set_max_depth(int depth) { max_depth = depth; }
    void set_samples_per_pixel(int samples) { samples_per_pixel = samples; }
    void set_num_threads(int threads) { num_threads = threads; }
    
    const Scene& get_scene() const { return scene; }
};
