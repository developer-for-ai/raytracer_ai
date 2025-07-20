#include "raytracer.h"
#include "image.h"
#include <iostream>
#include <chrono>
#include <string>

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " <scene_file> [options]\n";
    std::cout << "Options:\n";
    std::cout << "  -w, --width <int>        Image width (default: 800)\n";
    std::cout << "  -h, --height <int>       Image height (default: 600)\n";
    std::cout << "  -s, --samples <int>      Samples per pixel (default: 100)\n";
    std::cout << "  -d, --depth <int>        Maximum ray depth (default: 50)\n";
    std::cout << "  -t, --threads <int>      Number of threads (default: auto)\n";
    std::cout << "  -o, --output <filename>  Output filename (default: output.ppm)\n";
    std::cout << "  --help                   Show this help message\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    // Check for help first
    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "--help") {
            print_usage(argv[0]);
            return 0;
        }
    }
    
    std::string scene_file = argv[1];
    std::string output_file = "output.ppm";
    int width = 800;
    int height = 600;
    int samples = 100;
    int depth = 50;
    int threads = 0; // Auto-detect
    
    // Parse command line arguments
    for (int i = 2; i < argc; i++) {
        std::string arg = argv[i];
        
        if ((arg == "-w" || arg == "--width") && i + 1 < argc) {
            width = std::stoi(argv[++i]);
        } else if ((arg == "-h" || arg == "--height") && i + 1 < argc) {
            height = std::stoi(argv[++i]);
        } else if ((arg == "-s" || arg == "--samples") && i + 1 < argc) {
            samples = std::stoi(argv[++i]);
        } else if ((arg == "-d" || arg == "--depth") && i + 1 < argc) {
            depth = std::stoi(argv[++i]);
        } else if ((arg == "-t" || arg == "--threads") && i + 1 < argc) {
            threads = std::stoi(argv[++i]);
        } else if ((arg == "-o" || arg == "--output") && i + 1 < argc) {
            output_file = argv[++i];
        } else if (arg == "--help") {
            print_usage(argv[0]);
            return 0;
        } else {
            std::cerr << "Unknown option: " << arg << std::endl;
            print_usage(argv[0]);
            return 1;
        }
    }
    
    try {
        // Create raytracer and load scene
        RayTracer raytracer(depth, samples, threads);
        std::cout << "Loading scene: " << scene_file << std::endl;
        raytracer.load_scene(scene_file);
        
        // Create output image
        Image image(width, height);
        
        // Start timing
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // Render the scene
        raytracer.render(image);
        
        // End timing
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time);
        
        // Save the image
        std::cout << "Saving image: " << output_file << std::endl;
        if (output_file.substr(output_file.find_last_of('.') + 1) == "ppm") {
            image.save_ppm(output_file);
        } else {
            image.save_png(output_file);
        }
        
        std::cout << "Rendering completed in " << duration.count() << " seconds" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
