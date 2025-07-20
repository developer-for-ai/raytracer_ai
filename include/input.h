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
    Camera* camera;
    Window* window;  // Reference to window for checking mouse capture state
    
    float movement_speed;
    float mouse_sensitivity;
    
public:
    InputHandler(Camera* cam, Window* win = nullptr);
    
    void set_window(Window* win) { window = win; }
    void process_keyboard(int key, int action);
    void process_mouse(double x_pos, double y_pos);
    void update(float delta_time);
    
    void set_movement_speed(float speed) { movement_speed = speed; }
    void set_mouse_sensitivity(float sensitivity) { mouse_sensitivity = sensitivity; }
    
    bool is_key_pressed(int key) const;
};
