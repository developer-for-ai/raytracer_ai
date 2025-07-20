#pragma once
#include "common.h"

enum class MaterialType {
    LAMBERTIAN,
    METAL,
    DIELECTRIC,
    EMISSIVE
};

class Material {
public:
    MaterialType type;
    Color albedo;
    float roughness;
    float ior; // Index of refraction
    Color emission;
    
    Material(MaterialType t, const Color& a, float r = 0.0f, float i = 1.0f, const Color& e = Color(0, 0, 0))
        : type(t), albedo(a), roughness(r), ior(i), emission(e) {}
    
    bool scatter(const Ray& ray_in, const HitRecord& hit, Color& attenuation, Ray& scattered, std::mt19937& rng) const;
    
private:
    Vec3 random_in_unit_sphere(std::mt19937& rng) const;
    Vec3 random_unit_vector(std::mt19937& rng) const;
    Vec3 random_in_hemisphere(const Vec3& normal, std::mt19937& rng) const;
    float reflectance(float cosine, float ref_idx) const;
};
