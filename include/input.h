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
    bool mouse_initialized;  // Track if we've initialized from camera
    float yaw, pitch;        // Camera orientation angles
    Camera* camera;
    Window* window;  // Reference to window for checking mouse capture state
    
    float movement_speed;
    float vertical_speed_multiplier; // Multiplier for vertical movement (Space/Shift)
    float mouse_sensitivity;
    
    // Initial camera state for reset
    Vec3 initial_position;
    Vec3 initial_target;
    Vec3 initial_up;
    
    void initialize_from_camera();  // Initialize angles from current camera
    
public:
    InputHandler(Camera* cam, Window* win = nullptr);
    
    void set_window(Window* win) { window = win; }
    void process_keyboard(int key, int action);
    void process_mouse(double x_pos, double y_pos);
    void update(float delta_time);
    
    void set_movement_speed(float speed) { movement_speed = speed; }
    void set_mouse_sensitivity(float sensitivity) { mouse_sensitivity = sensitivity; }
    
    // Reset camera to initial state
    void reset_camera();
    
    bool is_key_pressed(int key) const;
};
