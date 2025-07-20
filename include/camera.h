#pragma once
#include "common.h"

class Camera {
public:
    Vec3 position;
    Vec3 target;
    Vec3 up;
    float fov;
    float aspect_ratio;
    float aperture;
    float focus_distance;
    
    // Computed values
    Vec3 u, v, w;
    Vec3 lower_left_corner;
    Vec3 horizontal;
    Vec3 vertical;
    float lens_radius;
    
    Camera(Vec3 pos, Vec3 tar, Vec3 vup, float vertical_fov, float aspect, float aperture_size = 0.0f, float focus_dist = 1.0f)
        : position(pos), target(tar), up(vup), fov(vertical_fov), aspect_ratio(aspect), aperture(aperture_size), focus_distance(focus_dist) {
        update_camera();
    }
    
    void update_camera();
};
