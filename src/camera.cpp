#include "camera.h"

void Camera::update_camera() {
    const float theta = Math::deg_to_rad(fov);
    const float half_height = std::tan(theta * 0.5f);
    const float half_width = aspect_ratio * half_height;
    
    w = (position - target).normalize();
    u = up.cross(w).normalize();
    v = w.cross(u);
    
    lens_radius = aperture * 0.5f;
    
    lower_left_corner = position - u * (half_width * focus_distance)
                       - v * (half_height * focus_distance)
                       - w * focus_distance;
    horizontal = u * (2.0f * half_width * focus_distance);
    vertical = v * (2.0f * half_height * focus_distance);
}
