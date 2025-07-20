#include "bvh.h"
#include "common.h"
#include <algorithm>
#include <random>
#include <cmath>

BVH::BVH(std::vector<std::shared_ptr<Geometry>>& objects) {
    if (objects.empty()) {
        root = nullptr;
        return;
    }
    
    root = build_bvh(objects, 0, static_cast<int>(objects.size()));
}

std::shared_ptr<BVHNode> BVH::build_bvh(std::vector<std::shared_ptr<Geometry>>& objects, int start, int end) {
    auto node = std::make_shared<BVHNode>();
    
    // Input validation
    if (start >= end || start < 0 || end > static_cast<int>(objects.size())) {
        return nullptr;
    }
    
    // Base case: single object
    if (end - start == 1) {
        node->geometry = objects[start];
        node->min_bounds = node->geometry->get_min_bounds();
        node->max_bounds = node->geometry->get_max_bounds();
        return node;
    }
    
    // Calculate bounding box for all objects - use more efficient computation
    Vec3 min_bounds(std::numeric_limits<float>::max());
    Vec3 max_bounds(std::numeric_limits<float>::lowest());
    
    for (int i = start; i < end; ++i) {
        const Vec3 obj_min = objects[i]->get_min_bounds();
        const Vec3 obj_max = objects[i]->get_max_bounds();
        
        min_bounds.x = std::min(min_bounds.x, obj_min.x);
        min_bounds.y = std::min(min_bounds.y, obj_min.y);
        min_bounds.z = std::min(min_bounds.z, obj_min.z);
        
        max_bounds.x = std::max(max_bounds.x, obj_max.x);
        max_bounds.y = std::max(max_bounds.y, obj_max.y);
        max_bounds.z = std::max(max_bounds.z, obj_max.z);
    }
    
    node->min_bounds = min_bounds;
    node->max_bounds = max_bounds;
    
    // Find the longest axis
    Vec3 extent = max_bounds - min_bounds;
    int axis = 0;
    if (extent.y > extent.x) axis = 1;
    float max_extent = (axis == 0) ? extent.x : extent.y;
    if (extent.z > max_extent) axis = 2;
    
    // Sort objects along the chosen axis
    std::sort(objects.begin() + start, objects.begin() + end,
        [axis](const std::shared_ptr<Geometry>& a, const std::shared_ptr<Geometry>& b) {
            Vec3 center_a = a->get_center();
            Vec3 center_b = b->get_center();
            if (axis == 0) return center_a.x < center_b.x;
            if (axis == 1) return center_a.y < center_b.y;
            return center_a.z < center_b.z;
        });
    
    // Split in the middle
    int mid = start + (end - start) / 2;
    node->left = build_bvh(objects, start, mid);
    node->right = build_bvh(objects, mid, end);
    
    return node;
}

bool BVH::hit(const Ray& ray, float t_min, float t_max, HitRecord& rec) const {
    if (!root) return false;
    return hit_bvh(root, ray, t_min, t_max, rec);
}

bool BVH::hit_bvh(const std::shared_ptr<BVHNode>& node, const Ray& ray, float t_min, float t_max, HitRecord& rec) const {
    if (!hit_box(node->min_bounds, node->max_bounds, ray)) {
        return false;
    }
    
    // Leaf node
    if (node->geometry) {
        return node->geometry->hit(ray, t_min, t_max, rec);
    }
    
    // Internal node
    HitRecord left_rec, right_rec;
    bool hit_left = node->left && hit_bvh(node->left, ray, t_min, t_max, left_rec);
    bool hit_right = node->right && hit_bvh(node->right, ray, t_min, t_max, right_rec);
    
    if (hit_left && hit_right) {
        rec = (left_rec.t < right_rec.t) ? left_rec : right_rec;
        return true;
    } else if (hit_left) {
        rec = left_rec;
        return true;
    } else if (hit_right) {
        rec = right_rec;
        return true;
    }
    
    return false;
}

bool BVH::hit_box(const Vec3& min_bounds, const Vec3& max_bounds, const Ray& ray) const {
    // Avoid division by zero
    const float epsilon = 1e-8f;
    
    float inv_dir_x = std::abs(ray.direction.x) > epsilon ? (1.0f / ray.direction.x) : (ray.direction.x >= 0 ? 1e8f : -1e8f);
    float inv_dir_y = std::abs(ray.direction.y) > epsilon ? (1.0f / ray.direction.y) : (ray.direction.y >= 0 ? 1e8f : -1e8f);
    float inv_dir_z = std::abs(ray.direction.z) > epsilon ? (1.0f / ray.direction.z) : (ray.direction.z >= 0 ? 1e8f : -1e8f);
    
    float t1 = (min_bounds.x - ray.origin.x) * inv_dir_x;
    float t2 = (max_bounds.x - ray.origin.x) * inv_dir_x;
    if (inv_dir_x < 0.0f) std::swap(t1, t2);
    
    float t3 = (min_bounds.y - ray.origin.y) * inv_dir_y;
    float t4 = (max_bounds.y - ray.origin.y) * inv_dir_y;
    if (inv_dir_y < 0.0f) std::swap(t3, t4);
    
    float t5 = (min_bounds.z - ray.origin.z) * inv_dir_z;
    float t6 = (max_bounds.z - ray.origin.z) * inv_dir_z;
    if (inv_dir_z < 0.0f) std::swap(t5, t6);
    
    float tmin = std::max({t1, t3, t5});
    float tmax = std::min({t2, t4, t6});
    
    return tmax >= 0.0f && tmin <= tmax;
}
