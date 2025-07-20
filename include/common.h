#pragma once
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Mathematical and physical constants
namespace Constants {
    constexpr float PI = static_cast<float>(M_PI);
    constexpr float TWO_PI = 2.0f * PI;
    constexpr float HALF_PI = 0.5f * PI;
    constexpr float INV_PI = 1.0f / PI;
    constexpr float EPSILON = 1e-6f;
    constexpr float SMALL_EPSILON = 1e-8f;
    constexpr float RAY_T_MIN = 0.001f;
    constexpr float RAY_T_MAX = 1e30f;
}

// Utility functions
namespace Math {
    constexpr float deg_to_rad(float degrees) noexcept { return degrees * Constants::PI / 180.0f; }
    constexpr float rad_to_deg(float radians) noexcept { return radians * 180.0f / Constants::PI; }
    
    inline float clamp(float value, float min_val, float max_val) noexcept {
        return std::max(min_val, std::min(max_val, value));
    }
}

// Fast vector math structures
struct Vec3 {
    float x, y, z;
    
    // Constructors
    constexpr Vec3() noexcept : x(0), y(0), z(0) {}
    constexpr Vec3(float x, float y, float z) noexcept : x(x), y(y), z(z) {}
    constexpr explicit Vec3(float scalar) noexcept : x(scalar), y(scalar), z(scalar) {}
    
    // Basic arithmetic operators
    constexpr Vec3 operator+(const Vec3& v) const noexcept { return Vec3(x + v.x, y + v.y, z + v.z); }
    constexpr Vec3 operator-(const Vec3& v) const noexcept { return Vec3(x - v.x, y - v.y, z - v.z); }
    constexpr Vec3 operator*(float t) const noexcept { return Vec3(x * t, y * t, z * t); }
    constexpr Vec3 operator*(const Vec3& v) const noexcept { return Vec3(x * v.x, y * v.y, z * v.z); }
    constexpr Vec3 operator/(float t) const noexcept { 
        return std::abs(t) < 1e-8f ? Vec3(0, 0, 0) : Vec3(x / t, y / t, z / t); 
    }
    constexpr Vec3 operator-() const noexcept { return Vec3(-x, -y, -z); }
    
    // Compound assignment operators
    Vec3& operator+=(const Vec3& v) noexcept { x += v.x; y += v.y; z += v.z; return *this; }
    Vec3& operator-=(const Vec3& v) noexcept { x -= v.x; y -= v.y; z -= v.z; return *this; }
    Vec3& operator*=(float t) noexcept { x *= t; y *= t; z *= t; return *this; }
    Vec3& operator/=(float t) noexcept { 
        if (std::abs(t) >= 1e-8f) { x /= t; y /= t; z /= t; }
        else { x = y = z = 0; }
        return *this; 
    }
    
    // Vector operations
    constexpr float dot(const Vec3& v) const noexcept { return x * v.x + y * v.y + z * v.z; }
    constexpr Vec3 cross(const Vec3& v) const noexcept { 
        return Vec3(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x); 
    }
    
    float length() const noexcept { return std::sqrt(x * x + y * y + z * z); }
    constexpr float length_squared() const noexcept { return x * x + y * y + z * z; }
    
    Vec3 normalize() const noexcept {
        const float len = length();
        return len > 1e-8f ? *this / len : Vec3(0, 0, 0);
    }
    
    Vec3 reflect(const Vec3& normal) const noexcept {
        return *this - normal * (2.0f * this->dot(normal));
    }
    
    Vec3 refract(const Vec3& normal, float eta) const noexcept {
        const float cos_i = -this->dot(normal);
        const float sin_t2 = eta * eta * (1.0f - cos_i * cos_i);
        if (sin_t2 >= 1.0f) return Vec3(0, 0, 0); // Total internal reflection
        return *this * eta + normal * (eta * cos_i - std::sqrt(1.0f - sin_t2));
    }
    
    // Static factory methods
    static constexpr Vec3 zero() noexcept { return Vec3(0, 0, 0); }
    static constexpr Vec3 one() noexcept { return Vec3(1, 1, 1); }
    static constexpr Vec3 unit_x() noexcept { return Vec3(1, 0, 0); }
    static constexpr Vec3 unit_y() noexcept { return Vec3(0, 1, 0); }
    static constexpr Vec3 unit_z() noexcept { return Vec3(0, 0, 1); }
    
    // Comparison operators
    constexpr bool operator==(const Vec3& v) const noexcept { 
        return std::abs(x - v.x) < 1e-6f && std::abs(y - v.y) < 1e-6f && std::abs(z - v.z) < 1e-6f; 
    }
    constexpr bool operator!=(const Vec3& v) const noexcept { return !(*this == v); }
};

// Non-member operators
constexpr Vec3 operator*(float t, const Vec3& v) noexcept { return v * t; }

// Additional Math functions that depend on Vec3
namespace Math {
    inline Vec3 clamp(const Vec3& v, float min_val, float max_val) noexcept {
        return Vec3(clamp(v.x, min_val, max_val), 
                   clamp(v.y, min_val, max_val), 
                   clamp(v.z, min_val, max_val));
    }
}

using Color = Vec3;

struct Ray {
    Vec3 origin, direction;
    float t_min, t_max;
    
    Ray() noexcept : t_min(Constants::RAY_T_MIN), t_max(Constants::RAY_T_MAX) {}
    Ray(const Vec3& o, const Vec3& d) noexcept 
        : origin(o), direction(d), t_min(Constants::RAY_T_MIN), t_max(Constants::RAY_T_MAX) {}
    Ray(const Vec3& o, const Vec3& d, float tmin, float tmax) noexcept 
        : origin(o), direction(d), t_min(tmin), t_max(tmax) {}
    
    Vec3 at(float t) const noexcept { return origin + direction * t; }
    
    // Create a ray with normalized direction
    static Ray create_normalized(const Vec3& origin, const Vec3& direction) noexcept {
        return Ray(origin, direction.normalize());
    }
};

struct HitRecord {
    Vec3 point;
    Vec3 normal;
    float t;
    bool front_face;
    int material_id;
    
    void set_face_normal(const Ray& ray, const Vec3& outward_normal) {
        front_face = ray.direction.dot(outward_normal) < 0;
        normal = front_face ? outward_normal : outward_normal * -1.0f;
    }
};

// Forward declarations
class Material;
class Geometry;
class Scene;
class Camera;
class BVH;
class Parser;
class Image;
