#pragma once
#include "common.h"
#include "camera.h"

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
