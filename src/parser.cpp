#include "parser.h"
#include "light.h"
#include "error_handling.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <map>

// Implementation of FaceStatistics methods
void Parser::FaceStatistics::update_face_type(size_t face_size) {
    if (face_size == 3) {
        triangle_faces++;
    } else if (face_size == 4) {
        quad_faces++;
    } else {
        polygon_faces++;
    }
}

void Parser::FaceStatistics::log_statistics(size_t vertex_count) const {
    ErrorHandling::Logger::info("OBJ parsing: " + std::to_string(vertex_count) + " vertices, " +
                               std::to_string(triangle_faces + quad_faces + polygon_faces) + " faces (" +
                               std::to_string(triangle_faces) + " triangles, " +
                               std::to_string(quad_faces) + " quads, " +
                               std::to_string(polygon_faces) + " polygons) -> " +
                               std::to_string(total_triangles_created) + " triangles created");
}

bool Parser::parse_scene_file(const std::string& filename, Scene& scene) {
    // Determine file type by extension
    size_t dot_pos = filename.find_last_of('.');
    if (dot_pos == std::string::npos) {
        ErrorHandling::Logger::error("File has no extension: " + filename);
        return false;
    }
    
    std::string extension = filename.substr(dot_pos + 1);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    
    if (extension == "obj") {
        return parse_obj_file(filename, scene);
    } else if (extension == "scene" || extension == "txt") {
        return parse_scene_description_file(filename, scene);
    } else {
        ErrorHandling::Logger::error("Unsupported file format: " + extension);
        return false;
    }
}

bool Parser::parse_obj_file(const std::string& filename, Scene& scene) {
    // Create default material for standalone OBJ files
    auto default_material = std::make_shared<Material>(MaterialType::LAMBERTIAN, Color(0.8f, 0.8f, 0.8f));
    scene.add_material(default_material);
    int material_id = 0;  // Use default material
    
    // Call the main OBJ parser with camera setup enabled
    return parse_obj_file_with_material(filename, scene, material_id, true);
}

// Helper function to parse vertex indices from a face line
std::vector<int> Parser::parse_face_indices(std::istringstream& iss) {
    std::vector<int> face_indices;
    std::string vertex_data;
    
    while (iss >> vertex_data) {
        // Stop parsing if we hit a comment (shouldn't happen with strip_comments, but just in case)
        if (vertex_data[0] == '#') {
            break;
        }
        
        // Parse vertex/texture/normal format (v/vt/vn)
        size_t slash_pos = vertex_data.find('/');
        std::string vertex_str = (slash_pos != std::string::npos) ? 
                               vertex_data.substr(0, slash_pos) : vertex_data;
        
        try {
            int vertex_index = std::stoi(vertex_str) - 1; // OBJ indices are 1-based
            if (vertex_index < 0) {
                ErrorHandling::Logger::warning("Invalid negative vertex index in OBJ file, skipping face");
                face_indices.clear();
                break;
            }
            face_indices.push_back(vertex_index);
        } catch (const std::exception& e) {
            ErrorHandling::Logger::warning("Invalid vertex index '" + vertex_str + "' in OBJ file, skipping face");
            face_indices.clear();
            break;
        }
    }
    
    return face_indices;
}

// Helper function to triangulate a face and add triangles to the scene
void Parser::triangulate_face(const std::vector<int>& face_indices, 
                             const std::vector<Vec3>& vertices,
                             Scene& scene, 
                             int material_id, 
                             FaceStatistics& stats) {
    if (face_indices.size() < 3) {
        return;
    }
    
    // Track face type statistics
    stats.update_face_type(face_indices.size());
    
    // Fan triangulation for polygons with more than 3 vertices
    for (size_t i = 1; i < face_indices.size() - 1; i++) {
        if (static_cast<size_t>(face_indices[0]) < vertices.size() && 
            static_cast<size_t>(face_indices[i]) < vertices.size() && 
            static_cast<size_t>(face_indices[i + 1]) < vertices.size()) {
            scene.add_object(std::make_shared<Triangle>(
                vertices[face_indices[0]],
                vertices[face_indices[i]],
                vertices[face_indices[i + 1]],
                material_id
            ));
            stats.total_triangles_created++;
        } else {
            ErrorHandling::Logger::warning("Vertex index out of bounds in OBJ file, skipping triangle");
        }
    }
}

// Helper function to update bounding box for camera positioning
void Parser::update_bounds(const Vec3& vertex, Vec3& min_bounds, Vec3& max_bounds) {
    min_bounds.x = std::min(min_bounds.x, vertex.x);
    min_bounds.y = std::min(min_bounds.y, vertex.y);
    min_bounds.z = std::min(min_bounds.z, vertex.z);
    max_bounds.x = std::max(max_bounds.x, vertex.x);
    max_bounds.y = std::max(max_bounds.y, vertex.y);
    max_bounds.z = std::max(max_bounds.z, vertex.z);
}

// Helper function to set up camera based on model bounds
void Parser::setup_camera_from_bounds(Scene& scene, const Vec3& min_bounds, const Vec3& max_bounds) {
    Vec3 center = (min_bounds + max_bounds) * 0.5f;
    Vec3 size = max_bounds - min_bounds;
    float max_dimension = std::max({size.x, size.y, size.z});
    
    // Position camera to view the entire model
    Vec3 camera_pos = center + Vec3(0.0f, max_dimension * 0.3f, max_dimension * 1.5f);
    Vec3 camera_target = center;
    Vec3 camera_up(0.0f, 1.0f, 0.0f);
    
    scene.camera = Camera(camera_pos, camera_target, camera_up, 50.0f, 16.0f/9.0f);
}

bool Parser::parse_obj_file_with_material(const std::string& filename, Scene& scene, int material_id, bool setup_camera) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        ErrorHandling::Logger::error("Could not open file: " + filename);
        return false;
    }
    
    std::vector<Vec3> vertices;
    std::string line;
    FaceStatistics stats;
    
    // Bounds tracking for camera positioning
    Vec3 min_bounds{1000000.0f, 1000000.0f, 1000000.0f};
    Vec3 max_bounds{-1000000.0f, -1000000.0f, -1000000.0f};
    
    while (std::getline(file, line)) {
        // Strip comments from the line first
        std::string clean_line = strip_comments(line);
        std::istringstream iss(clean_line);
        std::string prefix;
        iss >> prefix;
        
        // Skip empty lines
        if (prefix.empty()) {
            continue;
        }
        
        if (prefix == "v") {
            // Vertex
            float x, y, z;
            iss >> x >> y >> z;
            Vec3 vertex(x, y, z);
            vertices.push_back(vertex);
            
            // Update bounds for camera positioning
            if (setup_camera) {
                update_bounds(vertex, min_bounds, max_bounds);
            }
        } else if (prefix == "f") {
            // Parse face indices
            std::vector<int> face_indices = parse_face_indices(iss);
            
            // Triangulate and add to scene
            triangulate_face(face_indices, vertices, scene, material_id, stats);
        }
    }
    
    // Set up camera based on model bounds if requested
    if (setup_camera && vertices.size() > 0) {
        setup_camera_from_bounds(scene, min_bounds, max_bounds);
    }
    
    // Log OBJ parsing statistics
    stats.log_statistics(vertices.size());
    
    return true;
}

bool Parser::parse_scene_description_file(const std::string& filename, Scene& scene) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        ErrorHandling::Logger::error("Could not open file: " + filename);
        return false;
    }
    
    std::string line;
    std::map<std::string, int> material_map;
    int line_number = 0;
    bool has_valid_content = false;
    
    while (std::getline(file, line)) {
        line_number++;
        
        // Strip comments from the line first
        std::string clean_line = strip_comments(line);
        std::istringstream iss(clean_line);
        std::string command;
        
        // Skip empty lines
        if (clean_line.empty() || clean_line.find_first_not_of(" \t\n\r") == std::string::npos) {
            continue;
        }
        
        iss >> command;
        
        if (command == "camera") {
            Vec3 pos, target, up;
            float fov, aspect;
            if (!(iss >> pos.x >> pos.y >> pos.z >> target.x >> target.y >> target.z >> up.x >> up.y >> up.z >> fov >> aspect)) {
                ErrorHandling::Logger::error("Invalid camera format at line  at line " + std::to_string(line_number) + ": " + line);
                return false;
            }
            
            // Validate camera parameters
            if (fov <= 0 || fov >= 180) {
                ErrorHandling::Logger::error("Camera FOV must be between 0 and 180 degrees at line  at line " + std::to_string(line_number) + ": " + line);
                return false;
            }
            if (aspect <= 0) {
                ErrorHandling::Logger::error("Camera aspect ratio must be positive at line  at line " + std::to_string(line_number) + ": " + line);
                return false;
            }
            if (up.length() < 1e-6f) {
                ErrorHandling::Logger::error("Camera up vector cannot be zero at line  at line " + std::to_string(line_number) + ": " + line);
                return false;
            }
            scene.camera = Camera(pos, target, up, fov, aspect);
            has_valid_content = true;
        } else if (command == "background") {
            if (!(iss >> scene.background_color.x >> scene.background_color.y >> scene.background_color.z)) {
                ErrorHandling::Logger::error("Invalid background format at line  at line " + std::to_string(line_number) + ": " + line);
                return false;
            }
            
            // Validate background color is non-negative
            if (scene.background_color.x < 0 || scene.background_color.y < 0 || scene.background_color.z < 0) {
                ErrorHandling::Logger::error("Background color must be non-negative at line  at line " + std::to_string(line_number) + ": " + line);
                return false;
            }
            has_valid_content = true;
        } else if (command == "material") {
            std::string name, type;
            if (!(iss >> name >> type)) {
                ErrorHandling::Logger::error("Invalid material format at line  at line " + std::to_string(line_number) + ": " + line);
                return false;
            }
            
            Color albedo;
            if (!(iss >> albedo.x >> albedo.y >> albedo.z)) {
                ErrorHandling::Logger::error("Invalid material color at line  at line " + std::to_string(line_number) + ": " + line);
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
                ErrorHandling::Logger::error("Unknown material type ' at line " + std::to_string(line_number) + ": " + line);
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
                ErrorHandling::Logger::error("Invalid sphere format at line  at line " + std::to_string(line_number) + ": " + line);
                return false;
            }
            
            // Validate sphere radius
            if (radius <= 0.0f) {
                ErrorHandling::Logger::error("Sphere radius must be positive at line  at line " + std::to_string(line_number) + ": " + line);
                return false;
            }
            
            int material_id = 0;
            if (material_map.find(material_name) != material_map.end()) {
                material_id = material_map[material_name];
            } else {
                ErrorHandling::Logger::error("Undefined material ' at line " + std::to_string(line_number) + ": " + line);
                return false;
            }
            
            scene.add_object(std::make_shared<Sphere>(center, radius, material_id));
            has_valid_content = true;
            
        } else if (command == "plane") {
            Vec3 point, normal;
            std::string material_name;
            if (!(iss >> point.x >> point.y >> point.z >> normal.x >> normal.y >> normal.z >> material_name)) {
                ErrorHandling::Logger::error("Invalid plane format at line  at line " + std::to_string(line_number) + ": " + line);
                return false;
            }
            
            // Validate normal vector is not zero
            if (normal.length() < 1e-6f) {
                ErrorHandling::Logger::error("Plane normal vector cannot be zero at line  at line " + std::to_string(line_number) + ": " + line);
                return false;
            }
            
            int material_id = 0;
            if (material_map.find(material_name) != material_map.end()) {
                material_id = material_map[material_name];
            } else {
                ErrorHandling::Logger::error("Undefined material ' at line " + std::to_string(line_number) + ": " + line);
                return false;
            }
            
            scene.add_object(std::make_shared<Plane>(point, normal, material_id));
            has_valid_content = true;
            
        } else if (command == "point_light") {
            Vec3 position;
            Color intensity;
            float radius = 0.0f;
            if (!(iss >> position.x >> position.y >> position.z >> intensity.x >> intensity.y >> intensity.z)) {
                ErrorHandling::Logger::error("Invalid point_light format at line  at line " + std::to_string(line_number) + ": " + line);
                return false;
            }
            if (iss >> radius) {
                // Soft shadows radius provided
            }
            
            // Validate light intensity is non-negative
            if (intensity.x < 0 || intensity.y < 0 || intensity.z < 0) {
                ErrorHandling::Logger::error("Light intensity must be non-negative at line  at line " + std::to_string(line_number) + ": " + line);
                return false;
            }
            
            // Validate radius is non-negative
            if (radius < 0) {
                ErrorHandling::Logger::error("Light radius must be non-negative at line  at line " + std::to_string(line_number) + ": " + line);
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
                ErrorHandling::Logger::error("Invalid spot_light format at line  at line " + std::to_string(line_number) + ": " + line);
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
                ErrorHandling::Logger::error("Invalid area_light format at line  at line " + std::to_string(line_number) + ": " + line);
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
                ErrorHandling::Logger::error("Invalid ambient format at line  at line " + std::to_string(line_number) + ": " + line);
                return false;
            }
            has_valid_content = true;
        } else if (command == "load_obj") {
            std::string obj_filename, material_name;
            if (!(iss >> obj_filename >> material_name)) {
                ErrorHandling::Logger::error("Invalid load_obj format at line " + std::to_string(line_number) + ": " + line);
                return false;
            }
            
            // Find material by name in our local material_map
            int material_id = -1;
            if (material_map.find(material_name) != material_map.end()) {
                material_id = material_map[material_name];
            } else {
                ErrorHandling::Logger::error("Material '" + material_name + "' not found for load_obj at line " + std::to_string(line_number));
                return false;
            }
            
            // Load OBJ file with specific material
            if (!Parser::parse_obj_file_with_material(obj_filename, scene, material_id)) {
                ErrorHandling::Logger::error("Failed to load OBJ file '" + obj_filename + "' at line " + std::to_string(line_number));
                return false;
            }
            
            has_valid_content = true;
        } else if (command == "cylinder") {
            Vec3 base_center, axis;
            float radius, height;
            std::string material_name;
            if (!(iss >> base_center.x >> base_center.y >> base_center.z 
                >> axis.x >> axis.y >> axis.z 
                >> radius >> height >> material_name)) {
                ErrorHandling::Logger::error("Invalid cylinder format at line " + std::to_string(line_number) + ": " + line);
                return false;
            }
            
            // Validate cylinder parameters
            if (radius <= 0.0f) {
                ErrorHandling::Logger::error("Cylinder radius must be positive at line " + std::to_string(line_number) + ": " + line);
                return false;
            }
            if (height <= 0.0f) {
                ErrorHandling::Logger::error("Cylinder height must be positive at line " + std::to_string(line_number) + ": " + line);
                return false;
            }
            if (axis.length() < 1e-6f) {
                ErrorHandling::Logger::error("Cylinder axis vector cannot be zero at line " + std::to_string(line_number) + ": " + line);
                return false;
            }
            
            int material_id = 0;
            if (material_map.find(material_name) != material_map.end()) {
                material_id = material_map[material_name];
            } else {
                ErrorHandling::Logger::error("Undefined material '" + material_name + "' at line " + std::to_string(line_number) + ": " + line);
                return false;
            }
            
            scene.add_object(std::make_shared<Cylinder>(base_center, axis, radius, height, material_id));
            has_valid_content = true;
        } else {
            // Unrecognized command - this is an error
            ErrorHandling::Logger::error("Unknown command ' at line " + std::to_string(line_number) + ": " + line);
            return false;
        }
    }
    
    // Check if we found any valid scene content
    if (!has_valid_content) {
        ErrorHandling::Logger::error("Scene file contains no valid scene elements");
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

// Helper function to strip comments from a line
std::string Parser::strip_comments(const std::string& line) {
    size_t comment_pos = line.find('#');
    if (comment_pos != std::string::npos) {
        return line.substr(0, comment_pos);
    }
    return line;
}
