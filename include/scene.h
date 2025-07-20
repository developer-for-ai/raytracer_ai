#pragma once
#include "common.h"
#include "geometry.h"
#include "material.h"
#include "light.h"
#include "camera.h"
#include "bvh.h"
#include <vector>
#include <memory>

class Scene {
public:
    std::vector<std::shared_ptr<Geometry>> objects;
    std::vector<std::shared_ptr<Material>> materials;
    std::vector<std::shared_ptr<Light>> lights;  // Added lights collection
    std::unique_ptr<BVH> bvh;
    Camera camera;
    Color background_color;
    Color ambient_light;  // Added ambient lighting
    
    Scene() : camera(Vec3(0, 0, 0), Vec3(0, 0, -1), Vec3(0, 1, 0), 45.0f, 16.0f/9.0f),
              background_color(Color(0.5f, 0.7f, 1.0f)), ambient_light(Color(0.1f, 0.1f, 0.1f)) {}
    
    void add_object(std::shared_ptr<Geometry> obj) {
        objects.push_back(obj);
    }
    
    void add_material(std::shared_ptr<Material> mat) {
        materials.push_back(mat);
    }
    
    void add_light(std::shared_ptr<Light> light) {
        lights.push_back(light);
    }
    
    void build_acceleration_structure() {
        if (!objects.empty()) {
            bvh = std::make_unique<BVH>(objects);
        }
    }
    
    bool hit(const Ray& ray, float t_min, float t_max, HitRecord& rec) const {
        if (bvh) {
            return bvh->hit(ray, t_min, t_max, rec);
        }
        
        // Fallback: brute force
        HitRecord temp_rec;
        bool hit_anything = false;
        float closest_so_far = t_max;
        
        for (const auto& obj : objects) {
            if (obj->hit(ray, t_min, closest_so_far, temp_rec)) {
                hit_anything = true;
                closest_so_far = temp_rec.t;
                rec = temp_rec;
            }
        }
        
        return hit_anything;
    }
    
    std::shared_ptr<Material> get_material(int id) const {
        if (id >= 0 && id < static_cast<int>(materials.size())) {
            return materials[id];
        }
        return nullptr;
    }
};
