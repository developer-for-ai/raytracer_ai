#pragma once
#include "common.h"

enum class LightType {
    POINT,
    SPOT,
    AREA_PLANE
};

class Light {
public:
    LightType type;
    Vec3 position;
    Color intensity;
    bool enabled;
    
    Light(LightType t, const Vec3& pos, const Color& color)
        : type(t), position(pos), intensity(color), enabled(true) {}
    
    virtual ~Light() = default;
};

// Point Light - Omnidirectional light source
class PointLight : public Light {
public:
    float radius;  // For soft shadows (0 = hard shadows)
    
    PointLight(const Vec3& pos, const Color& color, float rad = 0.0f)
        : Light(LightType::POINT, pos, color), radius(rad) {}
};

// Spot Light - Directional cone of light
class SpotLight : public Light {
public:
    Vec3 direction;      // Direction the light is pointing
    float inner_angle;   // Inner cone angle (full intensity)
    float outer_angle;   // Outer cone angle (falloff to zero)
    float radius;        // For soft shadows
    
    SpotLight(const Vec3& pos, const Vec3& dir, const Color& color, 
              float inner, float outer, float rad = 0.0f)
        : Light(LightType::SPOT, pos, color), direction(dir.normalize()),
          inner_angle(inner), outer_angle(outer), radius(rad) {}
};

// Area Light - Rectangular plane light source
class AreaPlaneLight : public Light {
public:
    Vec3 normal;        // Normal vector of the plane
    Vec3 u_axis;        // U axis of the plane (width)
    Vec3 v_axis;        // V axis of the plane (height)
    float width;        // Width of the light plane
    float height;       // Height of the light plane
    int samples;        // Number of samples for soft shadows
    
    AreaPlaneLight(const Vec3& pos, const Vec3& norm, const Vec3& u, 
                   const Color& color, float w, float h, int sample_count = 4)
        : Light(LightType::AREA_PLANE, pos, color), normal(norm.normalize()), 
          u_axis(u.normalize()), width(w), height(h), samples(sample_count) {
        // Calculate v_axis perpendicular to normal and u_axis
        v_axis = normal.cross(u_axis).normalize();
        u_axis = v_axis.cross(normal).normalize();  // Recompute for orthogonality
    }
};
