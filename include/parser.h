#pragma once
#include "scene.h"
#include <string>
#include <vector>
#include <sstream>

class Parser {
private:
    // Helper structure to track face parsing statistics
    struct FaceStatistics {
        int triangle_faces = 0;
        int quad_faces = 0;
        int polygon_faces = 0;
        int total_triangles_created = 0;
        
        void update_face_type(size_t face_size);
        void log_statistics(size_t vertex_count) const;
    };

public:
    static bool parse_scene_file(const std::string& filename, Scene& scene);
    
private:
    static bool parse_obj_file(const std::string& filename, Scene& scene);
    static bool parse_obj_file_with_material(const std::string& filename, Scene& scene, int material_id, bool setup_camera = false);
    static bool parse_scene_description_file(const std::string& filename, Scene& scene);
    static Vec3 parse_vec3(const std::string& line);
    static Color parse_color(const std::string& line);
    
    // Helper functions for OBJ parsing
    static std::string strip_comments(const std::string& line);
    static std::vector<int> parse_face_indices(std::istringstream& iss);
    static void triangulate_face(const std::vector<int>& face_indices, 
                                const std::vector<Vec3>& vertices,
                                Scene& scene, 
                                int material_id, 
                                FaceStatistics& stats);
    static void update_bounds(const Vec3& vertex, Vec3& min_bounds, Vec3& max_bounds);
    static void setup_camera_from_bounds(Scene& scene, const Vec3& min_bounds, const Vec3& max_bounds);
};
