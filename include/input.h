#pragma once
#include "common.h"
#include "camera.h"

// GLFW key constants (fallback definitions for older GLFW versions)
#ifndef GLFW_KEY_W
#define GLFW_KEY_W 87
#define GLFW_KEY_A 65  
#define GLFW_KEY_S 83
#define GLFW_KEY_D 68
#define GLFW_KEY_R 82
#define GLFW_KEY_SPACE 32
#define GLFW_KEY_LEFT_SHIFT 340
#define GLFW_KEY_ESCAPE 256
#define GLFW_KEY_F1 290
#define GLFW_KEY_UP 265
#define GLFW_KEY_DOWN 264
#define GLFW_KEY_LEFT 263
#define GLFW_KEY_RIGHT 262
#define GLFW_PRESS 1
#define GLFW_RELEASE 0
#endif

// Forward declaration
class Window;

class InputHandler {
private:
    bool keys_pressed[1024];
    double last_mouse_x, last_mouse_y;
    bool first_mouse;
    bool mouse_initialized;
    float yaw, pitch;
    Camera* camera;
    Window* window;
    
    float movement_speed;
    float mouse_sensitivity;
    
    Vec3 initial_position;
    Vec3 initial_target;
    Vec3 initial_up;
    
    void initialize_from_camera();
    
public:
    InputHandler(Camera* cam, Window* win = nullptr);
    
    void process_keyboard(int key, int action);
    void process_mouse(double x_pos, double y_pos);
    void update(float delta_time);
    void reset_camera();
};
