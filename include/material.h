#pragma once
#include "common.h"

enum class MaterialType {
    LAMBERTIAN,     // Diffuse surfaces
    METAL,          // Reflective with controllable roughness
    DIELECTRIC,     // Glass/transparent with IOR  
    EMISSIVE,       // Light sources
    GLOSSY,         // Glossy/specular reflection with roughness
    SUBSURFACE      // Subsurface scattering approximation
};

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
