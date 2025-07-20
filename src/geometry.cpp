#include "geometry.h"
#include <algorithm>

bool Sphere::hit(const Ray& ray, float t_min, float t_max, HitRecord& rec) const {
    Vec3 oc = ray.origin - center;
    float a = ray.direction.dot(ray.direction);
    float half_b = oc.dot(ray.direction);
    float c = oc.dot(oc) - radius * radius;
    
    float discriminant = half_b * half_b - a * c;
    if (discriminant < 0) return false;
    
    float sqrtd = std::sqrt(discriminant);
    float root = (-half_b - sqrtd) / a;
    if (root < t_min || t_max < root) {
        root = (-half_b + sqrtd) / a;
        if (root < t_min || t_max < root) {
            return false;
        }
    }
    
    rec.t = root;
    rec.point = ray.at(rec.t);
    Vec3 outward_normal = (rec.point - center) / radius;
    rec.set_face_normal(ray, outward_normal);
    rec.material_id = material_id;
    
    return true;
}

bool Triangle::hit(const Ray& ray, float t_min, float t_max, HitRecord& rec) const {
    // Möller–Trumbore intersection algorithm
    const float EPSILON = 0.0000001f;
    Vec3 edge1 = v1 - v0;
    Vec3 edge2 = v2 - v0;
    Vec3 h = ray.direction.cross(edge2);
    float a = edge1.dot(h);
    
    if (a > -EPSILON && a < EPSILON) {
        return false; // Ray is parallel to triangle
    }
    
    float f = 1.0f / a;
    Vec3 s = ray.origin - v0;
    float u = f * s.dot(h);
    
    if (u < 0.0f || u > 1.0f) {
        return false;
    }
    
    Vec3 q = s.cross(edge1);
    float v = f * ray.direction.dot(q);
    
    if (v < 0.0f || u + v > 1.0f) {
        return false;
    }
    
    float t = f * edge2.dot(q);
    
    if (t > t_min && t < t_max) {
        rec.t = t;
        rec.point = ray.at(t);
        rec.set_face_normal(ray, normal);
        rec.material_id = material_id;
        return true;
    }
    
    return false;
}

Vec3 Triangle::get_min_bounds() const {
    return Vec3(
        std::min({v0.x, v1.x, v2.x}),
        std::min({v0.y, v1.y, v2.y}),
        std::min({v0.z, v1.z, v2.z})
    );
}

Vec3 Triangle::get_max_bounds() const {
    return Vec3(
        std::max({v0.x, v1.x, v2.x}),
        std::max({v0.y, v1.y, v2.y}),
        std::max({v0.z, v1.z, v2.z})
    );
}

bool Plane::hit(const Ray& ray, float t_min, float t_max, HitRecord& rec) const {
    float denom = normal.dot(ray.direction);
    if (std::abs(denom) < 1e-6f) {
        return false; // Ray is parallel to plane
    }
    
    float t = (point - ray.origin).dot(normal) / denom;
    
    if (t >= t_min && t <= t_max) {
        rec.t = t;
        rec.point = ray.at(t);
        rec.set_face_normal(ray, normal);
        rec.material_id = material_id;
        return true;
    }
    
    return false;
}
