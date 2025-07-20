#pragma once
#include "common.h"
#include <vector>
#include <string>

class Image {
private:
    std::vector<Color> pixels;
    int width, height;
    
public:
    Image(int w, int h) : width(w), height(h) {
        pixels.resize(width * height);
    }
    
    void set_pixel(int x, int y, const Color& color) {
        if (x >= 0 && x < width && y >= 0 && y < height) {
            pixels[y * width + x] = color;
        }
    }
    
    Color get_pixel(int x, int y) const {
        if (x >= 0 && x < width && y >= 0 && y < height) {
            return pixels[y * width + x];
        }
        return Color(0, 0, 0);
    }
    
    void save_ppm(const std::string& filename) const;
    void save_png(const std::string& filename) const; // Simple PNG implementation
    
    int get_width() const { return width; }
    int get_height() const { return height; }
    
    // Tone mapping and gamma correction
    static Color tone_map(const Color& hdr_color);
    static int clamp_to_byte(float value);
};
