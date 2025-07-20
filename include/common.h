#pragma once
#include <vector>
#include <memory>
#include <string>
#include <cmath>
#include <algorithm>
#include <random>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Fast vector math structures
struct Vec3 {
    float x, y, z;
    
    Vec3() : x(0), y(0), z(0) {}
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}
    
    Vec3 operator+(const Vec3& v) const { return Vec3(x + v.x, y + v.y, z + v.z); }
    Vec3 operator-(const Vec3& v) const { return Vec3(x - v.x, y - v.y, z - v.z); }
    Vec3 operator*(float t) const { return Vec3(x * t, y * t, z * t); }
    Vec3 operator*(const Vec3& v) const { return Vec3(x * v.x, y * v.y, z * v.z); }
    Vec3 operator/(float t) const { 
        return std::abs(t) < 1e-10f ? Vec3(0, 0, 0) : *this * (1.0f / t); 
    }
    
    float dot(const Vec3& v) const { return x * v.x + y * v.y + z * v.z; }
    Vec3 cross(const Vec3& v) const { 
        return Vec3(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x); 
    }
    
    float length() const { return std::sqrt(x * x + y * y + z * z); }
    float length_squared() const { return x * x + y * y + z * z; }
    
    Vec3 normalize() const {
        float len = length();
        return len > 0 ? *this / len : Vec3(0, 0, 0);
    }
    
    Vec3 reflect(const Vec3& normal) const {
        return *this - normal * 2.0f * this->dot(normal);
    }
    
    Vec3 refract(const Vec3& normal, float eta) const {
        float cos_i = -this->dot(normal);
        float sin_t2 = eta * eta * (1.0f - cos_i * cos_i);
        if (sin_t2 >= 1.0f) return Vec3(0, 0, 0); // Total internal reflection
        return *this * eta + normal * (eta * cos_i - std::sqrt(1.0f - sin_t2));
    }
};

using Color = Vec3;

struct Ray {
    Vec3 origin, direction;
    float t_min, t_max;
    
    Ray() : t_min(0.001f), t_max(std::numeric_limits<float>::infinity()) {}
    Ray(const Vec3& o, const Vec3& d) : origin(o), direction(d), t_min(0.001f), t_max(std::numeric_limits<float>::infinity()) {}
    
    Vec3 at(float t) const { return origin + direction * t; }
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
