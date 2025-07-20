#include "camera.h"
#include <random>

void Camera::update_camera() {
    float theta = fov * M_PI / 180.0f;
    float half_height = std::tan(theta / 2.0f);
    float half_width = aspect_ratio * half_height;
    
    w = (position - target).normalize();
    u = up.cross(w).normalize();
    v = w.cross(u);
    
    lens_radius = aperture / 2.0f;
    
    lower_left_corner = position - u * (half_width * focus_distance)
                       - v * (half_height * focus_distance)
                       - w * focus_distance;
    horizontal = u * (2.0f * half_width * focus_distance);
    vertical = v * (2.0f * half_height * focus_distance);
}

Ray Camera::get_ray(float s, float t, std::mt19937& rng) const {
    Vec3 rd = random_in_unit_disk(rng) * lens_radius;
    Vec3 offset = u * rd.x + v * rd.y;
    
    Vec3 ray_origin = position + offset;
    Vec3 ray_direction = lower_left_corner + horizontal * s + vertical * t - ray_origin;
    
    return Ray(ray_origin, ray_direction);
}

Vec3 Camera::random_in_unit_disk(std::mt19937& rng) const {
    std::uniform_real_distribution<float> dis(-1.0f, 1.0f);
    Vec3 p;
    do {
        p = Vec3(dis(rng), dis(rng), 0);
    } while (p.dot(p) >= 1.0f);
    return p;
}
