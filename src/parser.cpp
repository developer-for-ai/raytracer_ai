#include "parser.h"
#include "light.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>

bool Parser::parse_scene_file(const std::string& filename, Scene& scene) {
    // Determine file type by extension
    size_t dot_pos = filename.find_last_of('.');
    if (dot_pos == std::string::npos) {
        std::cerr << "File has no extension: " << filename << std::endl;
        return false;
    }
    
    std::string extension = filename.substr(dot_pos + 1);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    
    if (extension == "obj") {
        return parse_obj_file(filename, scene);
    } else if (extension == "scene" || extension == "txt") {
        return parse_scene_description_file(filename, scene);
    } else {
        std::cerr << "Unsupported file format: " << extension << std::endl;
        return false;
    }
}

bool Parser::parse_obj_file(const std::string& filename, Scene& scene) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Could not open file: " << filename << std::endl;
        return false;
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
                std::string vertex_str = (slash_pos != std::string::npos) ? 
                                       vertex_data.substr(0, slash_pos) : vertex_data;
                
                try {
                    int vertex_index = std::stoi(vertex_str) - 1; // OBJ indices are 1-based
                    if (vertex_index < 0) {
                        std::cerr << "WARNING: Invalid negative vertex index in OBJ file, skipping face" << std::endl;
                        face_indices.clear();
                        break;
                    }
                    face_indices.push_back(vertex_index);
                } catch (const std::exception& e) {
                    std::cerr << "WARNING: Invalid vertex index '" << vertex_str << "' in OBJ file, skipping face" << std::endl;
                    face_indices.clear();
                    break;
                }
            }
            
            // Create triangles (fan triangulation for polygons with more than 3 vertices)
            if (face_indices.size() >= 3) {
                for (size_t i = 1; i < face_indices.size() - 1; i++) {
                    if (static_cast<size_t>(face_indices[0]) < vertices.size() && 
                        static_cast<size_t>(face_indices[i]) < vertices.size() && 
                        static_cast<size_t>(face_indices[i + 1]) < vertices.size()) {
                        scene.add_object(std::make_shared<Triangle>(
                            vertices[face_indices[0]],
                            vertices[face_indices[i]],
                            vertices[face_indices[i + 1]],
                            0
                        ));
                    } else {
                        std::cerr << "WARNING: Vertex index out of bounds in OBJ file, skipping triangle" << std::endl;
                    }
                }
            }
        }
    }
    return true;
}

bool Parser::parse_scene_description_file(const std::string& filename, Scene& scene) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Could not open file: " << filename << std::endl;
        return false;
    }
    
    std::string line;
    std::map<std::string, int> material_map;
    int line_number = 0;
    bool has_valid_content = false;
    
    while (std::getline(file, line)) {
        line_number++;
        std::istringstream iss(line);
        std::string command;
        
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#' || line.find_first_not_of(" \t\n\r") == std::string::npos) {
            continue;
        }
        
        iss >> command;
        
        if (command == "camera") {
            Vec3 pos, target, up;
            float fov, aspect;
            if (!(iss >> pos.x >> pos.y >> pos.z >> target.x >> target.y >> target.z >> up.x >> up.y >> up.z >> fov >> aspect)) {
                std::cerr << "ERROR: Invalid camera format at line " << line_number << ": " << line << std::endl;
                return false;
            }
            
            // Validate camera parameters
            if (fov <= 0 || fov >= 180) {
                std::cerr << "ERROR: Camera FOV must be between 0 and 180 degrees at line " << line_number << ": " << line << std::endl;
                return false;
            }
            if (aspect <= 0) {
                std::cerr << "ERROR: Camera aspect ratio must be positive at line " << line_number << ": " << line << std::endl;
                return false;
            }
            if (up.length() < 1e-6f) {
                std::cerr << "ERROR: Camera up vector cannot be zero at line " << line_number << ": " << line << std::endl;
                return false;
            }
            scene.camera = Camera(pos, target, up, fov, aspect);
            has_valid_content = true;
        } else if (command == "background") {
            if (!(iss >> scene.background_color.x >> scene.background_color.y >> scene.background_color.z)) {
                std::cerr << "ERROR: Invalid background format at line " << line_number << ": " << line << std::endl;
                return false;
            }
            
            // Validate background color is non-negative
            if (scene.background_color.x < 0 || scene.background_color.y < 0 || scene.background_color.z < 0) {
                std::cerr << "ERROR: Background color must be non-negative at line " << line_number << ": " << line << std::endl;
                return false;
            }
            has_valid_content = true;
        } else if (command == "material") {
            std::string name, type;
            if (!(iss >> name >> type)) {
                std::cerr << "ERROR: Invalid material format at line " << line_number << ": " << line << std::endl;
                return false;
            }
            
            Color albedo;
            if (!(iss >> albedo.x >> albedo.y >> albedo.z)) {
                std::cerr << "ERROR: Invalid material color at line " << line_number << ": " << line << std::endl;
                return false;
            }
            
            MaterialType mat_type = MaterialType::LAMBERTIAN;
            float roughness = 0.0f;
            float ior = 1.0f;
            float metallic = 0.0f;
            float specular = 0.5f;
            float subsurface = 0.0f;
            Color emission(0, 0, 0);
            
            if (type == "lambertian") {
                mat_type = MaterialType::LAMBERTIAN;
                // Lambertian materials can optionally have roughness
                if (!iss.eof()) {
                    iss >> roughness;
                }
            } else if (type == "metal") {
                mat_type = MaterialType::METAL;
                iss >> roughness;
                metallic = 1.0f;  // Metals are fully metallic
            } else if (type == "dielectric") {
                mat_type = MaterialType::DIELECTRIC;
                iss >> ior;
            } else if (type == "emissive") {
                mat_type = MaterialType::EMISSIVE;
                // For emissive materials, albedo is the emission color
                emission = albedo;
                // Set albedo to black for emissive materials (they don't reflect light)
                albedo = Color(0, 0, 0);
            } else if (type == "glossy") {
                mat_type = MaterialType::GLOSSY;
                if (!iss.eof()) {
                    iss >> roughness;
                }
                if (!iss.eof()) {
                    iss >> specular;  // Optional specular parameter
                }
            } else if (type == "subsurface") {
                mat_type = MaterialType::SUBSURFACE;
                if (!iss.eof()) {
                    iss >> subsurface;  // Subsurface scattering amount
                }
                if (!iss.eof()) {
                    iss >> roughness;   // Optional surface roughness
                }
            } else {
                std::cerr << "ERROR: Unknown material type '" << type << "' at line " << line_number << ": " << line << std::endl;
                return false;
            }
            
            auto material = std::make_shared<Material>(mat_type, albedo, roughness, ior, emission, metallic, specular, subsurface);
            material_map[name] = scene.materials.size();
            scene.add_material(material);
            has_valid_content = true;
            
        } else if (command == "sphere") {
            Vec3 center;
            float radius;
            std::string material_name;
            if (!(iss >> center.x >> center.y >> center.z >> radius >> material_name)) {
                std::cerr << "ERROR: Invalid sphere format at line " << line_number << ": " << line << std::endl;
                return false;
            }
            
            // Validate sphere radius
            if (radius <= 0.0f) {
                std::cerr << "ERROR: Sphere radius must be positive at line " << line_number << ": " << line << std::endl;
                return false;
            }
            
            int material_id = 0;
            if (material_map.find(material_name) != material_map.end()) {
                material_id = material_map[material_name];
            } else {
                std::cerr << "ERROR: Undefined material '" << material_name << "' at line " << line_number << ": " << line << std::endl;
                return false;
            }
            
            scene.add_object(std::make_shared<Sphere>(center, radius, material_id));
            has_valid_content = true;
            
        } else if (command == "plane") {
            Vec3 point, normal;
            std::string material_name;
            if (!(iss >> point.x >> point.y >> point.z >> normal.x >> normal.y >> normal.z >> material_name)) {
                std::cerr << "ERROR: Invalid plane format at line " << line_number << ": " << line << std::endl;
                return false;
            }
            
            // Validate normal vector is not zero
            if (normal.length() < 1e-6f) {
                std::cerr << "ERROR: Plane normal vector cannot be zero at line " << line_number << ": " << line << std::endl;
                return false;
            }
            
            int material_id = 0;
            if (material_map.find(material_name) != material_map.end()) {
                material_id = material_map[material_name];
            } else {
                std::cerr << "ERROR: Undefined material '" << material_name << "' at line " << line_number << ": " << line << std::endl;
                return false;
            }
            
            scene.add_object(std::make_shared<Plane>(point, normal, material_id));
            has_valid_content = true;
            
        } else if (command == "point_light") {
            Vec3 position;
            Color intensity;
            float radius = 0.0f;
            if (!(iss >> position.x >> position.y >> position.z >> intensity.x >> intensity.y >> intensity.z)) {
                std::cerr << "ERROR: Invalid point_light format at line " << line_number << ": " << line << std::endl;
                return false;
            }
            if (iss >> radius) {
                // Soft shadows radius provided
            }
            
            // Validate light intensity is non-negative
            if (intensity.x < 0 || intensity.y < 0 || intensity.z < 0) {
                std::cerr << "ERROR: Light intensity must be non-negative at line " << line_number << ": " << line << std::endl;
                return false;
            }
            
            // Validate radius is non-negative
            if (radius < 0) {
                std::cerr << "ERROR: Light radius must be non-negative at line " << line_number << ": " << line << std::endl;
                return false;
            }
            
            auto light = std::make_shared<PointLight>(position, intensity, radius);
            scene.add_light(light);
            has_valid_content = true;
            
        } else if (command == "spot_light") {
            Vec3 position, direction;
            Color intensity;
            float inner_angle, outer_angle, radius = 0.0f;
            if (!(iss >> position.x >> position.y >> position.z 
                >> direction.x >> direction.y >> direction.z
                >> intensity.x >> intensity.y >> intensity.z
                >> inner_angle >> outer_angle)) {
                std::cerr << "ERROR: Invalid spot_light format at line " << line_number << ": " << line << std::endl;
                return false;
            }
            if (iss >> radius) {
                // Soft shadows radius provided
            }
            
            auto light = std::make_shared<SpotLight>(position, direction, intensity, inner_angle, outer_angle, radius);
            scene.add_light(light);
            has_valid_content = true;
            
        } else if (command == "area_light") {
            Vec3 position, normal, u_axis;
            Color intensity;
            float width, height;
            int samples = 4;
            if (!(iss >> position.x >> position.y >> position.z
                >> normal.x >> normal.y >> normal.z
                >> u_axis.x >> u_axis.y >> u_axis.z
                >> intensity.x >> intensity.y >> intensity.z
                >> width >> height)) {
                std::cerr << "ERROR: Invalid area_light format at line " << line_number << ": " << line << std::endl;
                return false;
            }
            if (iss >> samples) {
                // Sample count provided
            }
            
            auto light = std::make_shared<AreaPlaneLight>(position, normal, u_axis, intensity, width, height, samples);
            scene.add_light(light);
            has_valid_content = true;
            
        } else if (command == "ambient") {
            if (!(iss >> scene.ambient_light.x >> scene.ambient_light.y >> scene.ambient_light.z)) {
                std::cerr << "ERROR: Invalid ambient format at line " << line_number << ": " << line << std::endl;
                return false;
            }
            has_valid_content = true;
        } else {
            // Unrecognized command - this is an error
            std::cerr << "ERROR: Unknown command '" << command << "' at line " << line_number << ": " << line << std::endl;
            return false;
        }
    }
    
    // Check if we found any valid scene content
    if (!has_valid_content) {
        std::cerr << "ERROR: Scene file contains no valid scene elements" << std::endl;
        return false;
    }
    
    return true;
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
