#include "image.h"
#include <fstream>
#include <algorithm>
#include <cmath>

void Image::save_ppm(const std::string& filename) const {
    std::ofstream file(filename);
    file << "P3\n" << width << " " << height << "\n255\n";
    
    for (int j = height - 1; j >= 0; --j) {
        for (int i = 0; i < width; ++i) {
            Color pixel = get_pixel(i, j);
            Color mapped = tone_map(pixel);
            
            int r = clamp_to_byte(mapped.x);
            int g = clamp_to_byte(mapped.y);
            int b = clamp_to_byte(mapped.z);
            
            file << r << " " << g << " " << b << "\n";
        }
    }
}

Color Image::tone_map(const Color& hdr_color) {
    // Simple Reinhard tone mapping
    Color result;
    result.x = hdr_color.x / (1.0f + hdr_color.x);
    result.y = hdr_color.y / (1.0f + hdr_color.y);
    result.z = hdr_color.z / (1.0f + hdr_color.z);
    
    // Gamma correction
    result.x = std::sqrt(result.x);
    result.y = std::sqrt(result.y);
    result.z = std::sqrt(result.z);
    
    return result;
}

int Image::clamp_to_byte(float value) {
    return static_cast<int>(std::clamp(value * 256.0f, 0.0f, 255.0f));
}
