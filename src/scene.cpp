#include "scene.h"
#include <algorithm>

// Scene implementation is mostly header-only for performance
// This file exists for any future scene-specific implementations

Color Scene::calculate_lighting(const Vec3& point, const Vec3& normal, const Vec3& view_dir, 
                               const Material& material) const {
    Color final_color = ambient_light;  // Start with ambient lighting
    
    // Iterate through all lights in the scene
    for (const auto& light : lights) {
        if (!light->enabled) continue;
        
        // Sample the light
        LightSample light_sample = light->sample(point, normal);
        
        // Check for shadow (simple version - can be enhanced with proper shadow rays)
        bool in_shadow = false;
        Ray shadow_ray(point + normal * 0.001f, light_sample.direction);  // Offset to avoid self-intersection
        HitRecord shadow_hit;
        if (hit(shadow_ray, 0.001f, light_sample.distance - 0.001f, shadow_hit)) {
            in_shadow = true;
        }
        
        if (!in_shadow) {
            // Calculate diffuse contribution (Lambertian)
            float cos_theta = std::max(0.0f, normal.dot(light_sample.direction));
            Color diffuse = material.albedo * light_sample.intensity * cos_theta;
            
            // Calculate specular contribution (Blinn-Phong)
            if (material.type == MaterialType::METAL) {
                Vec3 half_vector = (view_dir + light_sample.direction).normalize();
                float cos_alpha = std::max(0.0f, normal.dot(half_vector));
                float shininess = (1.0f - material.roughness) * 128.0f;  // Convert roughness to shininess
                float specular_strength = std::pow(cos_alpha, shininess);
                Color specular = light_sample.intensity * specular_strength * material.albedo;
                diffuse = diffuse + specular;
            }
            
            final_color = final_color + diffuse;
        }
    }
    
    return final_color;
}
