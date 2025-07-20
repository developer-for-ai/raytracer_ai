#include "parser.h"
#include "light.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>

Scene Parser::parse_scene_file(const std::string& filename) {
    Scene scene;
    
    // Determine file type by extension
    std::string extension = filename.substr(filename.find_last_of('.') + 1);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    
    if (extension == "obj") {
        parse_obj_file(filename, scene);
    } else if (extension == "scene" || extension == "txt") {
        parse_scene_description_file(filename, scene);
    } else {
        std::cerr << "Unsupported file format: " << extension << std::endl;
        // Create a default scene
        auto mat = std::make_shared<Material>(MaterialType::LAMBERTIAN, Color(0.7f, 0.3f, 0.3f));
        scene.add_material(mat);
        scene.add_object(std::make_shared<Sphere>(Vec3(0, 0, -1), 0.5f, 0));
    }
    
    return scene;
}

void Parser::parse_obj_file(const std::string& filename, Scene& scene) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Could not open file: " << filename << std::endl;
        return;
    }
    
    std::vector<Vec3> vertices;
    std::string line;
    
    // Default material
    auto default_material = std::make_shared<Material>(MaterialType::LAMBERTIAN, Color(0.8f, 0.8f, 0.8f));
    scene.add_material(default_material);
    
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;
        
        if (prefix == "v") {
            // Vertex
            float x, y, z;
            iss >> x >> y >> z;
            vertices.push_back(Vec3(x, y, z));
        } else if (prefix == "f") {
            // Face - simple triangulation (assumes triangular faces)
            std::vector<int> face_indices;
            std::string vertex_data;
            
            while (iss >> vertex_data) {
                // Parse vertex/texture/normal format (v/vt/vn)
                size_t slash_pos = vertex_data.find('/');
                int vertex_index = std::stoi(vertex_data.substr(0, slash_pos)) - 1; // OBJ indices are 1-based
                face_indices.push_back(vertex_index);
            }
            
            // Create triangles (fan triangulation for polygons with more than 3 vertices)
            for (size_t i = 1; i < face_indices.size() - 1; i++) {
                if (face_indices[0] < vertices.size() && 
                    face_indices[i] < vertices.size() && 
                    face_indices[i + 1] < vertices.size()) {
                    scene.add_object(std::make_shared<Triangle>(
                        vertices[face_indices[0]],
                        vertices[face_indices[i]],
                        vertices[face_indices[i + 1]],
                        0
                    ));
                }
            }
        }
    }
}

void Parser::parse_scene_description_file(const std::string& filename, Scene& scene) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Could not open file: " << filename << std::endl;
        return;
    }
    
    std::string line;
    std::map<std::string, int> material_map;
    
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string command;
        iss >> command;
        
        if (command == "camera") {
            Vec3 pos, target, up;
            float fov, aspect;
            iss >> pos.x >> pos.y >> pos.z >> target.x >> target.y >> target.z >> up.x >> up.y >> up.z >> fov >> aspect;
            scene.camera = Camera(pos, target, up, fov, aspect);
        } else if (command == "background") {
            iss >> scene.background_color.x >> scene.background_color.y >> scene.background_color.z;
        } else if (command == "material") {
            std::string name, type;
            iss >> name >> type;
            
            Color albedo;
            iss >> albedo.x >> albedo.y >> albedo.z;
            
            MaterialType mat_type = MaterialType::LAMBERTIAN;
            float roughness = 0.0f;
            float ior = 1.0f;
            
            if (type == "metal") {
                mat_type = MaterialType::METAL;
                iss >> roughness;
            } else if (type == "dielectric") {
                mat_type = MaterialType::DIELECTRIC;
                iss >> ior;
            } else if (type == "emissive") {
                mat_type = MaterialType::EMISSIVE;
            }
            
            auto material = std::make_shared<Material>(mat_type, albedo, roughness, ior);
            material_map[name] = scene.materials.size();
            scene.add_material(material);
            
        } else if (command == "sphere") {
            Vec3 center;
            float radius;
            std::string material_name;
            iss >> center.x >> center.y >> center.z >> radius >> material_name;
            
            int material_id = 0;
            if (material_map.find(material_name) != material_map.end()) {
                material_id = material_map[material_name];
            }
            
            scene.add_object(std::make_shared<Sphere>(center, radius, material_id));
            
        } else if (command == "plane") {
            Vec3 point, normal;
            std::string material_name;
            iss >> point.x >> point.y >> point.z >> normal.x >> normal.y >> normal.z >> material_name;
            
            int material_id = 0;
            if (material_map.find(material_name) != material_map.end()) {
                material_id = material_map[material_name];
            }
            
            scene.add_object(std::make_shared<Plane>(point, normal, material_id));
            
        } else if (command == "point_light") {
            Vec3 position;
            Color intensity;
            float radius = 0.0f;
            iss >> position.x >> position.y >> position.z >> intensity.x >> intensity.y >> intensity.z;
            if (iss >> radius) {
                // Soft shadows radius provided
            }
            
            auto light = std::make_shared<PointLight>(position, intensity, radius);
            scene.add_light(light);
            
        } else if (command == "spot_light") {
            Vec3 position, direction;
            Color intensity;
            float inner_angle, outer_angle, radius = 0.0f;
            iss >> position.x >> position.y >> position.z 
                >> direction.x >> direction.y >> direction.z
                >> intensity.x >> intensity.y >> intensity.z
                >> inner_angle >> outer_angle;
            if (iss >> radius) {
                // Soft shadows radius provided
            }
            
            auto light = std::make_shared<SpotLight>(position, direction, intensity, inner_angle, outer_angle, radius);
            scene.add_light(light);
            
        } else if (command == "area_light") {
            Vec3 position, normal, u_axis;
            Color intensity;
            float width, height;
            int samples = 4;
            iss >> position.x >> position.y >> position.z
                >> normal.x >> normal.y >> normal.z
                >> u_axis.x >> u_axis.y >> u_axis.z
                >> intensity.x >> intensity.y >> intensity.z
                >> width >> height;
            if (iss >> samples) {
                // Sample count provided
            }
            
            auto light = std::make_shared<AreaPlaneLight>(position, normal, u_axis, intensity, width, height, samples);
            scene.add_light(light);
            
        } else if (command == "ambient") {
            iss >> scene.ambient_light.x >> scene.ambient_light.y >> scene.ambient_light.z;
        }
    }
}

Vec3 Parser::parse_vec3(const std::string& line) {
    std::istringstream iss(line);
    Vec3 result;
    iss >> result.x >> result.y >> result.z;
    return result;
}

Color Parser::parse_color(const std::string& line) {
    return parse_vec3(line); // Color is just an alias for Vec3
}
