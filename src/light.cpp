#include "light.h"
#include <cmath>
#include <random>

static std::mt19937 rng(std::random_device{}());
static std::uniform_real_distribution<float> dis(0.0f, 1.0f);

// Helper function for random number generation
float random_float() {
    return dis(rng);
}

Vec3 random_in_unit_sphere() {
    Vec3 p;
    do {
        p = Vec3(random_float(), random_float(), random_float()) * 2.0f - Vec3(1, 1, 1);
    } while (p.length_squared() >= 1.0f);
    return p;
}

// PointLight implementation
LightSample PointLight::sample(const Vec3& surface_point, const Vec3& surface_normal) const {
    LightSample sample;
    
    // For soft shadows, offset the light position slightly
    Vec3 light_pos = position;
    if (radius > 0.0f) {
        Vec3 offset = random_in_unit_sphere() * radius;
        light_pos = light_pos + offset;
    }
    
    Vec3 light_dir = light_pos - surface_point;
    float distance = light_dir.length();
    
    sample.position = light_pos;
    sample.direction = light_dir.normalize();
    sample.distance = distance;
    sample.pdf = 1.0f;  // Point lights have delta distribution
    
    // Apply inverse square law attenuation
    float attenuation = 1.0f / (distance * distance);
    sample.intensity = intensity * attenuation;
    
    return sample;
}

Color PointLight::get_intensity(const Vec3& point) const {
    float distance = (point - position).length();
    float attenuation = 1.0f / (distance * distance);
    return intensity * attenuation;
}

bool PointLight::is_visible_from(const Vec3& point, const Vec3& light_point) const {
    // Simple visibility test - can be enhanced with shadow ray testing
    return true;
}

// SpotLight implementation  
LightSample SpotLight::sample(const Vec3& surface_point, const Vec3& surface_normal) const {
    LightSample sample;
    
    // For soft shadows, offset the light position slightly
    Vec3 light_pos = position;
    if (radius > 0.0f) {
        Vec3 offset = random_in_unit_sphere() * radius;
        light_pos = light_pos + offset;
    }
    
    Vec3 light_dir = light_pos - surface_point;
    float distance = light_dir.length();
    
    sample.position = light_pos;
    sample.direction = light_dir.normalize();
    sample.distance = distance;
    sample.pdf = 1.0f;
    
    // Apply distance and cone attenuation
    float distance_attenuation = 1.0f / (distance * distance);
    float cone_attenuation = get_cone_attenuation(surface_point);
    
    sample.intensity = intensity * distance_attenuation * cone_attenuation;
    
    return sample;
}

Color SpotLight::get_intensity(const Vec3& point) const {
    float distance = (point - position).length();
    float distance_attenuation = 1.0f / (distance * distance);
    float cone_attenuation = get_cone_attenuation(point);
    
    return intensity * distance_attenuation * cone_attenuation;
}

float SpotLight::get_cone_attenuation(const Vec3& point) const {
    Vec3 light_to_point = (point - position).normalize();
    float cos_angle = direction.dot(light_to_point);
    
    float cos_inner = std::cos(inner_angle * M_PI / 180.0f);
    float cos_outer = std::cos(outer_angle * M_PI / 180.0f);
    
    if (cos_angle > cos_inner) {
        return 1.0f;  // Full intensity
    } else if (cos_angle > cos_outer) {
        // Linear falloff between inner and outer cone
        return (cos_angle - cos_outer) / (cos_inner - cos_outer);
    } else {
        return 0.0f;  // Outside cone
    }
}

bool SpotLight::is_visible_from(const Vec3& point, const Vec3& light_point) const {
    return get_cone_attenuation(point) > 0.0f;
}

// AreaPlaneLight implementation
LightSample AreaPlaneLight::sample(const Vec3& surface_point, const Vec3& surface_normal) const {
    LightSample sample;
    
    // Sample a random point on the light plane
    Vec3 light_pos = get_random_point_on_plane();
    
    Vec3 light_dir = light_pos - surface_point;
    float distance = light_dir.length();
    
    sample.position = light_pos;
    sample.direction = light_dir.normalize();
    sample.distance = distance;
    
    // Calculate PDF based on area and solid angle
    float area = width * height;
    float cos_theta = std::abs(normal.dot(sample.direction));
    sample.pdf = (distance * distance) / (area * cos_theta);
    
    // Area light intensity is constant across the surface
    sample.intensity = intensity;
    
    return sample;
}

Color AreaPlaneLight::get_intensity(const Vec3& point) const {
    // For area lights, intensity depends on the solid angle subtended
    // This is a simplified version - full implementation would integrate over the area
    return intensity;
}

Vec3 AreaPlaneLight::get_random_point_on_plane() const {
    float u = (random_float() - 0.5f) * width;
    float v = (random_float() - 0.5f) * height;
    
    return position + u_axis * u + v_axis * v;
}

bool AreaPlaneLight::is_visible_from(const Vec3& point, const Vec3& light_point) const {
    // Check if the point is on the correct side of the plane
    Vec3 to_point = (point - light_point).normalize();
    return normal.dot(to_point) > 0.0f;
}
