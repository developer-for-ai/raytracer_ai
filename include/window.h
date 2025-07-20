#pragma once
#include "common.h"
#include "scene.h"
#include "gpu_raytracer.h"
#include <string>
#include <functional>
#include <memory>

// Forward declarations
struct GLFWwindow;
typedef unsigned int GLuint;

class Window {
private:
    GLFWwindow* window;
    int width, height;
    std::string title;
    
    std::unique_ptr<GPURayTracer> gpu_raytracer;
    
    // Mouse capture state
    bool mouse_captured = false;
    bool show_detailed_stats = false; // Toggle for detailed performance stats
    
    // Callbacks
    std::function<void(int, int, int, int)> key_callback;
    std::function<void(double, double)> mouse_callback;
    std::function<void(int, int)> resize_callback;
    
    // OpenGL resources for displaying the rendered texture
    GLuint display_shader_program;
    GLuint quad_vao, quad_vbo;
    
    void setup_display_rendering();
    bool compile_display_shaders();
    
public:
    Window(int w, int h, const std::string& title);
    ~Window();
    
    bool initialize();
    void run();
    void close();
    
    bool should_close() const;
    void swap_buffers();
    void poll_events();
    
    // Getters
    int get_width() const { return width; }
    int get_height() const { return height; }
    GLFWwindow* get_handle() const { return window; }
    bool is_mouse_captured() const { return mouse_captured; }
    
    // Mouse control
    void toggle_mouse_capture();
    void update_fps_display(float fps, float frame_time);
    void toggle_detailed_stats() { show_detailed_stats = !show_detailed_stats; }
    void capture_frame(const std::string& filename);  // Capture current frame to file
    
    // Callbacks
    void set_key_callback(std::function<void(int, int, int, int)> callback);
    void set_mouse_callback(std::function<void(double, double)> callback);
    void set_resize_callback(std::function<void(int, int)> callback);
    
    // Rendering
    void load_scene(const Scene& scene);
    void render_frame(const Camera& camera, int samples = 1, int max_depth = 10);
};
