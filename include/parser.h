#pragma once
#include "scene.h"
#include <string>

class Parser {
public:
    static Scene parse_scene_file(const std::string& filename);
    
private:
    static void parse_obj_file(const std::string& filename, Scene& scene);
    static void parse_scene_description_file(const std::string& filename, Scene& scene);
    static Vec3 parse_vec3(const std::string& line);
    static Color parse_color(const std::string& line);
};
