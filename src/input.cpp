#include "input.h"
#include "window.h"
#include <cstring>
#include <cmath>

// GLFW key constants (we'll define the ones we need)
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

InputHandler::InputHandler(Camera* cam, Window* win) 
    : camera(cam), window(win), first_mouse(true), mouse_initialized(false),
      movement_speed(2.5f), vertical_speed_multiplier(8.0f), mouse_sensitivity(0.1f), yaw(-90.0f), pitch(0.0f) {
    std::memset(keys_pressed, false, sizeof(keys_pressed));
    last_mouse_x = 0.0;
    last_mouse_y = 0.0;
    
    // Store initial camera state for reset functionality
    if (camera) {
        initial_position = camera->position;
        initial_target = camera->target;
        initial_up = camera->up;
    }
}

void InputHandler::process_keyboard(int key, int action) {
    if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS) {
            keys_pressed[key] = true;
            
            // Handle special keys that need immediate action on press
            if (key == GLFW_KEY_R) {
                reset_camera();
            }
        } else if (action == GLFW_RELEASE) {
            keys_pressed[key] = false;
        }
    }
}

void InputHandler::process_mouse(double x_pos, double y_pos) {
    // Only process mouse input if mouse is captured
    if (!window || !window->is_mouse_captured()) {
        // Reset first mouse when not captured to prevent jumping when re-capturing
        first_mouse = true;
        return;
    }
    
    if (first_mouse) {
        last_mouse_x = x_pos;
        last_mouse_y = y_pos;
        first_mouse = false;
        
        // Initialize yaw and pitch from current camera orientation on first mouse capture
        initialize_from_camera();
    }
    
    double x_offset = x_pos - last_mouse_x;
    double y_offset = last_mouse_y - y_pos; // Reversed since y-coordinates go from bottom to top
    last_mouse_x = x_pos;
    last_mouse_y = y_pos;
    
    x_offset *= mouse_sensitivity;
    y_offset *= mouse_sensitivity;
    
    // Update camera orientation using member variables
    yaw += x_offset;
    pitch += y_offset;
    
    // Constrain pitch
    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;
    
    // Calculate new front vector
    Vec3 front;
    front.x = std::cos(yaw * M_PI / 180.0f) * std::cos(pitch * M_PI / 180.0f);
    front.y = std::sin(pitch * M_PI / 180.0f);
    front.z = std::sin(yaw * M_PI / 180.0f) * std::cos(pitch * M_PI / 180.0f);
    
    // Update camera target
    camera->target = camera->position + front.normalize();
    camera->update_camera();
    
    // Reset temporal accumulation when camera rotates
    if (window) {
        window->reset_accumulation();
    }
}

void InputHandler::update(float delta_time) {
    if (!camera) return;
    
    Vec3 front = (camera->target - camera->position).normalize();
    Vec3 right = front.cross(camera->up).normalize();
    
    float velocity = movement_speed * delta_time;
    bool camera_moved = false;
    
    // Movement
    if (keys_pressed[GLFW_KEY_W]) {
        camera->position = camera->position + front * velocity;
        camera->target = camera->target + front * velocity;
        camera_moved = true;
    }
    if (keys_pressed[GLFW_KEY_S]) {
        camera->position = camera->position - front * velocity;
        camera->target = camera->target - front * velocity;
        camera_moved = true;
    }
    if (keys_pressed[GLFW_KEY_A]) {
        camera->position = camera->position - right * velocity;
        camera->target = camera->target - right * velocity;
        camera_moved = true;
    }
    if (keys_pressed[GLFW_KEY_D]) {
        camera->position = camera->position + right * velocity;
        camera->target = camera->target + right * velocity;
        camera_moved = true;
    }
    if (keys_pressed[GLFW_KEY_SPACE]) {
        float vertical_velocity = velocity * vertical_speed_multiplier;
        camera->position.y += vertical_velocity;
        camera->target.y += vertical_velocity;
        camera_moved = true;
    }
    if (keys_pressed[GLFW_KEY_LEFT_SHIFT]) {
        float vertical_velocity = velocity * vertical_speed_multiplier;
        camera->position.y -= vertical_velocity;
        camera->target.y -= vertical_velocity;
        camera_moved = true;
    }
    
    // Arrow key camera look controls
    bool camera_rotated = false;
    float arrow_sensitivity = mouse_sensitivity * 10.0f; // Reduced from 15x to 10x for more precise control
    
    if (keys_pressed[GLFW_KEY_UP]) {
        pitch += arrow_sensitivity;
        camera_rotated = true;
    }
    if (keys_pressed[GLFW_KEY_DOWN]) {
        pitch -= arrow_sensitivity;
        camera_rotated = true;
    }
    if (keys_pressed[GLFW_KEY_LEFT]) {
        yaw -= arrow_sensitivity;
        camera_rotated = true;
    }
    if (keys_pressed[GLFW_KEY_RIGHT]) {
        yaw += arrow_sensitivity;
        camera_rotated = true;
    }
    
    if (camera_rotated) {
        // Ensure we have initialized angles from camera first
        if (!mouse_initialized) {
            initialize_from_camera();
        }
        
        // Constrain pitch
        if (pitch > 89.0f) pitch = 89.0f;
        if (pitch < -89.0f) pitch = -89.0f;
        
        // Calculate new front vector
        Vec3 front_new;
        front_new.x = std::cos(yaw * M_PI / 180.0f) * std::cos(pitch * M_PI / 180.0f);
        front_new.y = std::sin(pitch * M_PI / 180.0f);
        front_new.z = std::sin(yaw * M_PI / 180.0f) * std::cos(pitch * M_PI / 180.0f);
        
        // Update camera target
        camera->target = camera->position + front_new.normalize();
        camera->update_camera();
        
        // Reset temporal accumulation when camera rotates
        if (window) {
            window->reset_accumulation();
        }
    }
    
    if (camera_moved) {
        camera->update_camera();
        // Reset temporal accumulation when camera moves
        if (window) {
            window->reset_accumulation();
        }
    }
}

bool InputHandler::is_key_pressed(int key) const {
    if (key >= 0 && key < 1024) {
        return keys_pressed[key];
    }
    return false;
}

void InputHandler::initialize_from_camera() {
    if (!camera || mouse_initialized) return;
    
    // Calculate yaw and pitch from current camera orientation
    Vec3 direction = (camera->target - camera->position).normalize();
    
    // Calculate yaw (rotation around Y axis)
    yaw = std::atan2(direction.z, direction.x) * 180.0f / M_PI;
    
    // Calculate pitch (rotation around X axis) 
    pitch = std::asin(-direction.y) * 180.0f / M_PI;
    
    mouse_initialized = true;
}

void InputHandler::reset_camera() {
    if (!camera) return;
    
    // Reset to initial state
    camera->position = initial_position;
    camera->target = initial_target;  
    camera->up = initial_up;
    camera->update_camera();
    
    // Reset mouse state and reinitialize angles from camera
    mouse_initialized = false;
    first_mouse = true;
    initialize_from_camera();
    
    // Reset accumulation if we have a window
    if (window) {
        window->reset_accumulation();
    }
}
