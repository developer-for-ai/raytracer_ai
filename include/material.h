#pragma once
#include "common.h"

enum class MaterialType : int {
    LAMBERTIAN = 0,     // Diffuse surfaces
    METAL = 1,          // Reflective with controllable roughness
    DIELECTRIC = 2,     // Glass/transparent with IOR  
    EMISSIVE = 3,       // Light sources
    GLOSSY = 4,         // Glossy/specular reflection with roughness
    SUBSURFACE = 5      // Subsurface scattering approximation
};

// Material type utilities
namespace MaterialUtils {
    constexpr const char* type_to_string(MaterialType type) noexcept {
        switch (type) {
            case MaterialType::LAMBERTIAN: return "lambertian";
            case MaterialType::METAL: return "metal";
            case MaterialType::DIELECTRIC: return "dielectric";
            case MaterialType::EMISSIVE: return "emissive";
            case MaterialType::GLOSSY: return "glossy";
            case MaterialType::SUBSURFACE: return "subsurface";
            default: return "unknown";
        }
    }
    
    constexpr bool is_valid_type(MaterialType type) noexcept {
        return type >= MaterialType::LAMBERTIAN && type <= MaterialType::SUBSURFACE;
    }
}

class Material {
public:
    MaterialType type;
    Color albedo;           // Base color/reflectance
    float roughness;        // Surface roughness (0=mirror, 1=rough)
    float ior;              // Index of refraction for dielectrics
    Color emission;         // Emissive color for light sources
    float metallic;         // Metallic factor (0=dielectric, 1=metallic)
    float specular;         // Specular reflection amount
    float subsurface;       // Subsurface scattering amount
    
    Material(MaterialType t, const Color& a, float r = 0.0f, float i = 1.0f, const Color& e = Color(0, 0, 0),
             float m = 0.0f, float s = 0.5f, float ss = 0.0f)
        : type(t), albedo(a), roughness(r), ior(i), emission(e), metallic(m), specular(s), subsurface(ss) {}
};
