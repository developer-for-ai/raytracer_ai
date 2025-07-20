#include "window.h"
#include "shader.h"
#include "image.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>
#include <cstdio>

Window::Window(int w, int h, const std::string& t) 
    : window(nullptr), width(w), height(h), title(t),
      display_shader_program(0), quad_vao(0), quad_vbo(0) {
}

Window::~Window() {
    if (quad_vao) glDeleteVertexArrays(1, &quad_vao);
    if (quad_vbo) glDeleteBuffers(1, &quad_vbo);
    if (display_shader_program) glDeleteProgram(display_shader_program);
    
    if (window) {
        glfwDestroyWindow(window);
    }
    glfwTerminate();
}

bool Window::initialize() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }
    
    // Set error callback for better debugging
    glfwSetErrorCallback([](int error, const char* description) {
        std::cerr << "GLFW Error " << error << ": " << description << std::endl;
    });
    
    // Set OpenGL version and profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    // Add hints for headless/virtual display compatibility
    glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
    glfwWindowHint(GLFW_FOCUSED, GLFW_TRUE);
    glfwWindowHint(GLFW_AUTO_ICONIFY, GLFW_FALSE);
    
    // Create window
    window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        const char* error_description;
        int error_code = glfwGetError(&error_description);
        if (error_code != GLFW_NO_ERROR) {
            std::cerr << "GLFW Error " << error_code << ": " << error_description << std::endl;
        }
        glfwTerminate();
        return false;
    }
    
    std::cout << "Window created successfully" << std::endl;
    
    glfwMakeContextCurrent(window);
    
    // Initialize GLEW
    GLenum glew_init = glewInit();
    if (glew_init != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW: " << glewGetErrorString(glew_init) << std::endl;
        return false;
    }
    
    // Initialize GPU raytracer
    gpu_raytracer = std::make_unique<GPURayTracer>(width, height);
    if (!gpu_raytracer->initialize()) {
        std::cerr << "Failed to initialize GPU raytracer" << std::endl;
        return false;
    }
    
    // Setup display rendering
    setup_display_rendering();
    
    // Enable vsync
    glfwSwapInterval(1);
    
    // Set up callbacks
    glfwSetWindowUserPointer(window, this);
    
    glfwSetKeyCallback(window, [](GLFWwindow* w, int key, int scancode, int action, int mods) {
        Window* window_ptr = static_cast<Window*>(glfwGetWindowUserPointer(w));
        if (window_ptr && window_ptr->key_callback) {
            window_ptr->key_callback(key, scancode, action, mods);
        }
    });
    
    glfwSetCursorPosCallback(window, [](GLFWwindow* w, double x, double y) {
        Window* window_ptr = static_cast<Window*>(glfwGetWindowUserPointer(w));
        if (window_ptr && window_ptr->mouse_callback) {
            window_ptr->mouse_callback(x, y);
        }
    });
    
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow* w, int width, int height) {
        Window* window_ptr = static_cast<Window*>(glfwGetWindowUserPointer(w));
        if (window_ptr) {
            window_ptr->width = width;
            window_ptr->height = height;
            glViewport(0, 0, width, height);
            window_ptr->gpu_raytracer->resize(width, height);
            if (window_ptr->resize_callback) {
                window_ptr->resize_callback(width, height);
            }
        }
    });
    
    // Add mouse button callback for capture toggle
    glfwSetMouseButtonCallback(window, [](GLFWwindow* w, int button, int action, int mods) {
        Window* window_ptr = static_cast<Window*>(glfwGetWindowUserPointer(w));
        if (window_ptr && button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
            window_ptr->toggle_mouse_capture();
        }
    });
    
    // Don't capture mouse by default - user can click to toggle
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    mouse_captured = false;
    
    return true;
}

void Window::setup_display_rendering() {
    // Compile display shaders
    if (!compile_display_shaders()) {
        std::cerr << "Failed to compile display shaders" << std::endl;
        return;
    }
    
    // Create fullscreen quad
    float quad_vertices[] = {
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
        
        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };
    
    glGenVertexArrays(1, &quad_vao);
    glGenBuffers(1, &quad_vbo);
    
    glBindVertexArray(quad_vao);
    glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
}

bool Window::compile_display_shaders() {
    const char* vertex_source = Shader::get_display_vertex_shader();
    const char* fragment_source = Shader::get_display_fragment_shader();
    
    // Compile vertex shader
    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_source, nullptr);
    glCompileShader(vertex_shader);
    
    GLint success;
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar info_log[512];
        glGetShaderInfoLog(vertex_shader, 512, nullptr, info_log);
        std::cerr << "Vertex shader compilation failed: " << info_log << std::endl;
        return false;
    }
    
    // Compile fragment shader
    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_source, nullptr);
    glCompileShader(fragment_shader);
    
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar info_log[512];
        glGetShaderInfoLog(fragment_shader, 512, nullptr, info_log);
        std::cerr << "Fragment shader compilation failed: " << info_log << std::endl;
        return false;
    }
    
    // Link program
    display_shader_program = glCreateProgram();
    glAttachShader(display_shader_program, vertex_shader);
    glAttachShader(display_shader_program, fragment_shader);
    glLinkProgram(display_shader_program);
    
    glGetProgramiv(display_shader_program, GL_LINK_STATUS, &success);
    if (!success) {
        GLchar info_log[512];
        glGetProgramInfoLog(display_shader_program, 512, nullptr, info_log);
        std::cerr << "Display program linking failed: " << info_log << std::endl;
        return false;
    }
    
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    
    return true;
}

void Window::load_scene(const Scene& scene) {
    if (gpu_raytracer) {
        gpu_raytracer->load_scene(scene);
    }
}

void Window::render_frame(const Camera& camera, int samples, int max_depth) {
    // Check for OpenGL errors before rendering
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "OpenGL error before rendering: " << error << std::endl;
        return;
    }
    
    // Render with GPU raytracer
    gpu_raytracer->render(camera, samples, max_depth);
    
    // Check for errors after GPU raytracer
    error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "OpenGL error after GPU raytracer: " << error << std::endl;
        return;
    }
    
    // Display the result
    glClear(GL_COLOR_BUFFER_BIT);
    
    glUseProgram(display_shader_program);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gpu_raytracer->get_output_texture());
    glUniform1i(glGetUniformLocation(display_shader_program, "raytraced_texture"), 0);
    
    glBindVertexArray(quad_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    // Check for errors after display rendering
    error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "OpenGL error after display rendering: " << error << std::endl;
    }
}

void Window::set_key_callback(std::function<void(int, int, int, int)> callback) {
    key_callback = callback;
}

void Window::set_mouse_callback(std::function<void(double, double)> callback) {
    mouse_callback = callback;
}

void Window::set_resize_callback(std::function<void(int, int)> callback) {
    resize_callback = callback;
}

bool Window::should_close() const {
    return glfwWindowShouldClose(window);
}

void Window::swap_buffers() {
    glfwSwapBuffers(window);
}

void Window::poll_events() {
    glfwPollEvents();
}

void Window::close() {
    glfwSetWindowShouldClose(window, true);
}

void Window::toggle_mouse_capture() {
    if (!window) return;
    
    mouse_captured = !mouse_captured;
    
    if (mouse_captured) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        std::cout << "\n[Mouse captured - click again to release]" << std::endl;
    } else {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        std::cout << "\n[Mouse released - click to capture]" << std::endl;
    }
}

void Window::update_fps_display(float fps, float frame_time) {
    if (!window) return;
    
    // Format FPS and frame time with proper precision
    char fps_buffer[64];
    snprintf(fps_buffer, sizeof(fps_buffer), "%.1f", fps);
    char time_buffer[64];
    snprintf(time_buffer, sizeof(time_buffer), "%.1f", frame_time);
    
    // Create title based on detail level
    std::string fps_title;
    if (show_detailed_stats) {
        // Detailed mode: show FPS, frame time, and additional info
        fps_title = title + " - FPS: " + fps_buffer + " | Frame: " + time_buffer + "ms" +
                   " | GPU RT | " + std::to_string(width) + "x" + std::to_string(height);
    } else {
        // Simple mode: just FPS and frame time
        fps_title = title + " - FPS: " + fps_buffer + " | " + time_buffer + "ms";
    }
    
    glfwSetWindowTitle(window, fps_title.c_str());
}

void Window::capture_frame(const std::string& filename) {
    if (!window) return;
    
    // Create an Image object to store the framebuffer data
    Image image(width, height);
    
    // Allocate buffer for reading pixels
    std::vector<unsigned char> pixels(width * height * 3);
    
    // Read pixels from the front buffer
    glReadBuffer(GL_FRONT);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    
    // Check for OpenGL errors
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "OpenGL error during frame capture: " << error << std::endl;
        return;
    }
    
    // Convert from OpenGL format (bottom-left origin) to image format (top-left origin)
    // and from unsigned char [0,255] to float [0,1]
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int gl_y = height - 1 - y;  // Flip Y axis
            int pixel_index = (gl_y * width + x) * 3;
            
            float r = pixels[pixel_index] / 255.0f;
            float g = pixels[pixel_index + 1] / 255.0f;
            float b = pixels[pixel_index + 2] / 255.0f;
            
            image.set_pixel(x, y, Color(r, g, b));
        }
    }
    
    // Save the image
    std::string extension = filename.substr(filename.find_last_of('.') + 1);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    
    if (extension == "ppm") {
        image.save_ppm(filename);
    } else if (extension == "png") {
        image.save_png(filename);
    } else {
        // Default to PPM if no extension or unsupported extension
        std::string ppm_filename = filename + ".ppm";
        image.save_ppm(ppm_filename);
        std::cout << "\n[Frame saved as: " << ppm_filename << "]" << std::endl;
        return;
    }
    
    std::cout << "\n[Frame saved as: " << filename << "]" << std::endl;
}
