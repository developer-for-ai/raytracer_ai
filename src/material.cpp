#include "material.h"
#include <random>

Vec3 Material::random_in_unit_sphere(std::mt19937& rng) const {
    std::uniform_real_distribution<float> dis(-1.0f, 1.0f);
    Vec3 p;
    do {
        p = Vec3(dis(rng), dis(rng), dis(rng));
    } while (p.length_squared() >= 1.0f);
    return p;
}

Vec3 Material::random_unit_vector(std::mt19937& rng) const {
    return random_in_unit_sphere(rng).normalize();
}

Vec3 Material::random_in_hemisphere(const Vec3& normal, std::mt19937& rng) const {
    Vec3 in_unit_sphere = random_in_unit_sphere(rng);
    if (in_unit_sphere.dot(normal) > 0.0f) {
        return in_unit_sphere;
    } else {
        return in_unit_sphere * -1.0f;
    }
}

float Material::reflectance(float cosine, float ref_idx) const {
    // Schlick's approximation
    float r0 = (1.0f - ref_idx) / (1.0f + ref_idx);
    r0 = r0 * r0;
    return r0 + (1.0f - r0) * std::pow((1.0f - cosine), 5);
}

bool Material::scatter(const Ray& ray_in, const HitRecord& hit, Color& attenuation, Ray& scattered, std::mt19937& rng) const {
    switch (type) {
        case MaterialType::LAMBERTIAN: {
            Vec3 scatter_direction = hit.normal + random_unit_vector(rng);
            
            // Catch degenerate scatter direction
            if (scatter_direction.length_squared() < 1e-8f) {
                scatter_direction = hit.normal;
            }
            
            scattered = Ray(hit.point, scatter_direction);
            attenuation = albedo;
            return true;
        }
        
        case MaterialType::METAL: {
            Vec3 reflected = ray_in.direction.normalize().reflect(hit.normal);
            scattered = Ray(hit.point, reflected + random_in_unit_sphere(rng) * roughness);
            attenuation = albedo;
            return scattered.direction.dot(hit.normal) > 0;
        }
        
        case MaterialType::DIELECTRIC: {
            attenuation = Color(1.0f, 1.0f, 1.0f);
            float refraction_ratio = hit.front_face ? (1.0f / ior) : ior;
            
            Vec3 unit_direction = ray_in.direction.normalize();
            float cos_theta = std::min((unit_direction * -1.0f).dot(hit.normal), 1.0f);
            float sin_theta = std::sqrt(1.0f - cos_theta * cos_theta);
            
            bool cannot_refract = refraction_ratio * sin_theta > 1.0f;
            Vec3 direction;
            
            std::uniform_real_distribution<float> dis(0.0f, 1.0f);
            if (cannot_refract || reflectance(cos_theta, refraction_ratio) > dis(rng)) {
                direction = unit_direction.reflect(hit.normal);
            } else {
                direction = unit_direction.refract(hit.normal, refraction_ratio);
            }
            
            scattered = Ray(hit.point, direction);
            return true;
        }
        
        case MaterialType::EMISSIVE: {
            // Emissive materials don't scatter light
            return false;
        }
        
        default:
            return false;
    }
}
