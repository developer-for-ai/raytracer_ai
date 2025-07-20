#pragma once
#include <string>

// Forward declaration
typedef unsigned int GLuint;

class Shader {
public:
    static std::string load_file(const std::string& filepath);
    static bool compile_shader(const std::string& source, unsigned int type, unsigned int& shader_id);
    static bool link_program(unsigned int vertex_shader, unsigned int fragment_shader, unsigned int& program_id);
    static bool create_compute_shader(const std::string& source, unsigned int& program_id);
    static void check_compile_errors(unsigned int shader, const std::string& type);
    
    // Built-in shaders as strings to avoid file dependencies
    static const char* get_raytracing_compute_shader();
    static const char* get_display_vertex_shader();
    static const char* get_display_fragment_shader();
};
