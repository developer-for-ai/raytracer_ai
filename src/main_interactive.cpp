#include "window.h"
#include "input.h"
#include "parser.h"
#include "gpu_raytracer.h"
#include "image.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <iomanip>
#include <chrono>

// Key constants
#ifndef GLFW_KEY_ESCAPE
#define GLFW_KEY_ESCAPE 256
#define GLFW_KEY_F1 290
#endif

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <scene_file> [options]\n";
        std::cout << "Interactive GPU Ray Tracer\n";
        std::cout << "Options:\n";
        std::cout << "  -w, --width <int>        Window width (default: 1200)\n";
        std::cout << "  -h, --height <int>       Window height (default: 800)\n";
        std::cout << "  -s, --samples <int>      Samples per frame (default: 1)\n";
        std::cout << "  -d, --depth <int>        Maximum ray depth (default: 8)\n";
        std::cout << "  -o, --output <filename>  Save rendered frame to file (headless mode)\n";
        std::cout << "Controls:\n";
        std::cout << "  WASD - Move camera\n";
        std::cout << "  Click - Capture/release mouse for looking\n";
        std::cout << "  Arrow Keys - Look around (alternative to mouse)\n";
        std::cout << "  Space/Shift - Move up/down\n";
        std::cout << "  R - Reset camera position\n";
        std::cout << "  F1 - Toggle detailed stats\n";
        std::cout << "  ESC - Exit\n";
        return 1;
    }

    std::string scene_file = argv[1];
    
    int window_width = 1200;
    int window_height = 800;
    int samples_per_frame = 4;
    int max_depth = 10;
    std::string output_filename = "";
    
    for (int i = 2; i < argc; i++) {
        std::string arg = argv[i];
        if ((arg == "-w" || arg == "--width") && i + 1 < argc) {
            window_width = std::stoi(argv[++i]);
        } else if ((arg == "-h" || arg == "--height") && i + 1 < argc) {
            window_height = std::stoi(argv[++i]);
        } else if ((arg == "-s" || arg == "--samples") && i + 1 < argc) {
            samples_per_frame = std::stoi(argv[++i]);
        } else if ((arg == "-d" || arg == "--depth") && i + 1 < argc) {
            max_depth = std::stoi(argv[++i]);
        } else if ((arg == "-o" || arg == "--output") && i + 1 < argc) {
            output_filename = argv[++i];
        }
    }
    
    try {
        std::cout << "Loading scene: " << scene_file << std::endl;
        
        // Load scene
        Scene scene;
        if (!Parser::parse_scene_file(scene_file, scene)) {
            std::cerr << "❌ ERROR: Failed to load scene file: " << scene_file << std::endl;
            std::cerr << "The program will now exit." << std::endl;
            return 1;
        }
        
        // If output filename is specified, render to file instead of interactive mode
        if (!output_filename.empty()) {
            std::cout << "Rendering to file: " << output_filename << std::endl;
            std::cout << "Resolution: " << window_width << "x" << window_height << std::endl;
            std::cout << "Samples per pixel: " << samples_per_frame << std::endl;
            std::cout << "Max ray depth: " << max_depth << std::endl;
            
            // Create headless GPU raytracer for file output
            // We still need a minimal OpenGL context, but no visible window
            if (!glfwInit()) {
                std::cerr << "Failed to initialize GLFW for headless rendering" << std::endl;
                return 1;
            }
            
            // Set up for headless rendering
            glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            
            GLFWwindow* headless_window = glfwCreateWindow(window_width, window_height, "Headless", nullptr, nullptr);
            if (!headless_window) {
                std::cerr << "Failed to create headless context" << std::endl;
                glfwTerminate();
                return 1;
            }
            
            glfwMakeContextCurrent(headless_window);
            
            // Initialize GLEW
            if (glewInit() != GLEW_OK) {
                std::cerr << "Failed to initialize GLEW for headless rendering" << std::endl;
                glfwTerminate();
                return 1;
            }
            
            // Create GPU raytracer
            GPURayTracer gpu_raytracer(window_width, window_height);
            if (!gpu_raytracer.initialize()) {
                std::cerr << "Failed to initialize GPU raytracer" << std::endl;
                glfwTerminate();
                return 1;
            }
            
            gpu_raytracer.load_scene(scene);
            
            // Render frame
            auto start_time = std::chrono::high_resolution_clock::now();
            gpu_raytracer.render(scene.camera, samples_per_frame, max_depth);
            auto end_time = std::chrono::high_resolution_clock::now();
            
            // Capture the rendered frame
            Image image(window_width, window_height);
            std::vector<unsigned char> pixels(window_width * window_height * 3);
            
            // Read pixels from framebuffer
            glReadPixels(0, 0, window_width, window_height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
            
            // Convert from OpenGL format to image format
            for (int y = 0; y < window_height; ++y) {
                for (int x = 0; x < window_width; ++x) {
                    int gl_y = window_height - 1 - y;  // Flip Y axis
                    int pixel_index = (gl_y * window_width + x) * 3;
                    
                    float r = pixels[pixel_index] / 255.0f;
                    float g = pixels[pixel_index + 1] / 255.0f;
                    float b = pixels[pixel_index + 2] / 255.0f;
                    
                    image.set_pixel(x, y, Color(r, g, b));
                }
            }
            
            // Save the image
            std::string extension = output_filename.substr(output_filename.find_last_of('.') + 1);
            std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
            
            std::string actual_filename = output_filename;
            if (extension == "ppm") {
                image.save_ppm(output_filename);
            } else if (extension == "png") {
                // PNG support not implemented, save as PPM instead
                actual_filename = output_filename.substr(0, output_filename.find_last_of('.')) + ".ppm";
                image.save_ppm(actual_filename);
            } else {
                // Default to PPM if no extension or unsupported extension
                actual_filename = output_filename + ".ppm";
                image.save_ppm(actual_filename);
            }
            
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
            std::cout << "GPU rendering completed in " << duration.count() << "ms" << std::endl;
            std::cout << "Image saved as: " << actual_filename << std::endl;
            
            glfwDestroyWindow(headless_window);
            glfwTerminate();
            return 0;
        }
        
        // Interactive mode - create visible window
        Window window(window_width, window_height, "Interactive GPU Ray Tracer");
        if (!window.initialize()) {
            std::cerr << "Failed to initialize window" << std::endl;
            return 1;
        }
        
        window.load_scene(scene);
        
        // Create input handler
        InputHandler input(&scene.camera, &window);
        
        // Set up callbacks
        window.set_key_callback([&input, &window](int key, int scancode, int action, int mods) {
            if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
                window.close();
            }
            // Toggle detailed stats with F1 key
            if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
                window.toggle_detailed_stats();
            }
            input.process_keyboard(key, action);
        });
        
        window.set_mouse_callback([&input](double x, double y) {
            input.process_mouse(x, y);
        });
        
        // Timing
        auto last_frame = std::chrono::high_resolution_clock::now();
        float frame_time = 0.0f;
        int frame_count = 0;
        
        std::cout << "\nReal-time GPU ray tracing started!" << std::endl;
        std::cout << "Resolution: " << window_width << "x" << window_height << std::endl;
        std::cout << "Samples per frame: " << samples_per_frame << std::endl;
        std::cout << "Max ray depth: " << max_depth << std::endl;
        std::cout << "\nControls:" << std::endl;
        std::cout << "  WASD - Move camera" << std::endl;
        std::cout << "  Click - Capture/release mouse for looking" << std::endl;
        std::cout << "  Arrow Keys - Look around (alternative to mouse)" << std::endl;
        std::cout << "  Space/Shift - Move up/down" << std::endl;
        std::cout << "  R - Reset camera position" << std::endl;
        std::cout << "  F1 - Toggle detailed stats" << std::endl;
        std::cout << "  ESC - Exit" << std::endl;
        std::cout << "\nClick in the window to start looking around!" << std::endl;
        
        // Main render loop
        int total_frames = 0;
        while (!window.should_close()) {
            auto current_frame = std::chrono::high_resolution_clock::now();
            float delta_time = std::chrono::duration<float>(current_frame - last_frame).count();
            last_frame = current_frame;
            
            frame_time += delta_time;
            frame_count++;
            total_frames++;
            
            // Update FPS every second  
            if (frame_time >= 1.0f) {
                float fps = frame_count / frame_time;
                float avg_frame_time = (frame_time / frame_count * 1000.0f);
                
                // Update window title with FPS
                window.update_fps_display(fps, avg_frame_time);
                
                // Print to console (less frequently)
                std::cout << "\rFPS: " << std::fixed << std::setprecision(1) << fps 
                         << " | Frame: " << avg_frame_time << "ms"
                         << " | Frames: " << total_frames << std::flush;
                         
                frame_time = 0.0f;
                frame_count = 0;
            }
            
            // Check for GLFW errors
            const char* error_description;
            int error_code = glfwGetError(&error_description);
            if (error_code != GLFW_NO_ERROR) {
                std::cerr << "\\nGLFW Error " << error_code << ": " << error_description << std::endl;
                break;
            }
            
            // Process input
            input.update(delta_time);
            
            // Render frame
            window.render_frame(scene.camera, samples_per_frame, max_depth);

            // Swap buffers and poll events
            window.swap_buffers();
            window.poll_events();
        }
        
        // Check why we exited
        const char* error_description;
        int error_code = glfwGetError(&error_description);
        if (error_code != GLFW_NO_ERROR) {
            std::cout << "\nExiting due to GLFW error " << error_code << ": " << error_description << std::endl;
        } else {
            std::cout << "\nExiting normally after " << total_frames << " frames..." << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
