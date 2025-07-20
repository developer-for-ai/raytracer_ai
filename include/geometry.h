#pragma once
#include "common.h"
#include <memory>

class Geometry {
public:
    virtual ~Geometry() = default;
    virtual bool hit(const Ray& ray, float t_min, float t_max, HitRecord& rec) const = 0;
    virtual Vec3 get_min_bounds() const = 0;
    virtual Vec3 get_max_bounds() const = 0;
    virtual Vec3 get_center() const = 0;
    
    int material_id = 0;
};

class Sphere : public Geometry {
public:
    Vec3 center;
    float radius;
    
    Sphere(const Vec3& c, float r, int mat_id) : center(c), radius(r) {
        material_id = mat_id;
    }
    
    bool hit(const Ray& ray, float t_min, float t_max, HitRecord& rec) const override;
    Vec3 get_min_bounds() const override { return center - Vec3(radius, radius, radius); }
    Vec3 get_max_bounds() const override { return center + Vec3(radius, radius, radius); }
    Vec3 get_center() const override { return center; }
};

class Triangle : public Geometry {
public:
    Vec3 v0, v1, v2;
    Vec3 normal;
    
    Triangle(const Vec3& a, const Vec3& b, const Vec3& c, int mat_id) : v0(a), v1(b), v2(c) {
        material_id = mat_id;
        Vec3 edge1 = v1 - v0;
        Vec3 edge2 = v2 - v0;
        normal = edge1.cross(edge2).normalize();
    }
    
    bool hit(const Ray& ray, float t_min, float t_max, HitRecord& rec) const override;
    Vec3 get_min_bounds() const override;
    Vec3 get_max_bounds() const override;
    Vec3 get_center() const override { return (v0 + v1 + v2) / 3.0f; }
};

class Plane : public Geometry {
public:
    Vec3 point;
    Vec3 normal;
    
    Plane(const Vec3& p, const Vec3& n, int mat_id) : point(p), normal(n.normalize()) {
        material_id = mat_id;
    }
    
    bool hit(const Ray& ray, float t_min, float t_max, HitRecord& rec) const override;
    Vec3 get_min_bounds() const override { return Vec3(-1e6, -1e6, -1e6); }
    Vec3 get_max_bounds() const override { return Vec3(1e6, 1e6, 1e6); }
    Vec3 get_center() const override { return point; }
};

class Cylinder : public Geometry {
public:
    Vec3 base_center;  // Center of the bottom base
    Vec3 axis;         // Cylinder axis (normalized direction vector)
    float radius;
    float height;
    
    Cylinder(const Vec3& base, const Vec3& ax, float r, float h, int mat_id) 
        : base_center(base), axis(ax.normalize()), radius(r), height(h) {
        material_id = mat_id;
    }
    
    bool hit(const Ray& ray, float t_min, float t_max, HitRecord& rec) const override;
    Vec3 get_min_bounds() const override;
    Vec3 get_max_bounds() const override;
    Vec3 get_center() const override { return base_center + axis * (height * 0.5f); }
};
