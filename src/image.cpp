#include "image.h"
#include "error_handling.h"
#include <fstream>
#include <algorithm>
#include <cmath>
#include <limits>

void Image::save_ppm(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        ErrorHandling::Logger::error("Failed to open file for writing: " + filename);
        return;
    }
    
    file << "P3\n" << width << " " << height << "\n255\n";
    
    for (int j = height - 1; j >= 0; --j) {
        for (int i = 0; i < width; ++i) {
            const Color pixel = get_pixel(i, j);
            const Color mapped = Image::tone_map(pixel);
            
            const int r = Image::clamp_to_byte(mapped.x);
            const int g = Image::clamp_to_byte(mapped.y);
            const int b = Image::clamp_to_byte(mapped.z);
            
            file << r << " " << g << " " << b << "\n";
            
            if (file.fail()) {
                ErrorHandling::Logger::error("Failed to write pixel data to file: " + filename);
                return;
            }
        }
    }
    
    if (file.fail()) {
        ErrorHandling::Logger::error("Failed to write complete image to file: " + filename);
    }
}

Color Image::tone_map(const Color& hdr_color) {
    // Improved Reinhard tone mapping with exposure control
    constexpr float exposure = 1.0f;
    const Color exposed = hdr_color * exposure;
    
    Color result;
    result.x = exposed.x / (1.0f + exposed.x);
    result.y = exposed.y / (1.0f + exposed.y);
    result.z = exposed.z / (1.0f + exposed.z);
    
    // Gamma correction (sRGB)
    constexpr float inv_gamma = 1.0f / 2.2f;
    result.x = std::pow(std::max(0.0f, result.x), inv_gamma);
    result.y = std::pow(std::max(0.0f, result.y), inv_gamma);
    result.z = std::pow(std::max(0.0f, result.z), inv_gamma);
    
    return result;
}

int Image::clamp_to_byte(float value) {
    return static_cast<int>(std::clamp(value * 256.0f, 0.0f, 255.0f));
}
