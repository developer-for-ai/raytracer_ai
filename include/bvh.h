#pragma once
#include "common.h"
#include "geometry.h"
#include <memory>
#include <vector>

struct BVHNode {
    Vec3 min_bounds, max_bounds;
    std::shared_ptr<BVHNode> left, right;
    std::shared_ptr<Geometry> geometry;
    
    BVHNode() = default;
    BVHNode(std::shared_ptr<Geometry> geom) : geometry(geom) {
        if (geom) {
            min_bounds = geom->get_min_bounds();
            max_bounds = geom->get_max_bounds();
        }
    }
};

class BVH {
private:
    std::shared_ptr<BVHNode> root;
    
    std::shared_ptr<BVHNode> build_bvh(std::vector<std::shared_ptr<Geometry>>& objects, int start, int end);
    bool hit_bvh(const std::shared_ptr<BVHNode>& node, const Ray& ray, float t_min, float t_max, HitRecord& rec) const;
    bool hit_box(const Vec3& min_bounds, const Vec3& max_bounds, const Ray& ray) const;
    
public:
    BVH() = default;
    BVH(std::vector<std::shared_ptr<Geometry>>& objects);
    
    bool hit(const Ray& ray, float t_min, float t_max, HitRecord& rec) const;
};
