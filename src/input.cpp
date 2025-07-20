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
#define GLFW_KEY_SPACE 32
#define GLFW_KEY_LEFT_SHIFT 340
#define GLFW_KEY_ESCAPE 256
#define GLFW_KEY_F1 290
#define GLFW_PRESS 1
#define GLFW_RELEASE 0
#endif

InputHandler::InputHandler(Camera* cam, Window* win) 
    : camera(cam), window(win), first_mouse(true), movement_speed(2.5f), mouse_sensitivity(0.1f) {
    std::memset(keys_pressed, false, sizeof(keys_pressed));
    last_mouse_x = 0.0;
    last_mouse_y = 0.0;
}

void InputHandler::process_keyboard(int key, int action) {
    if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS) {
            keys_pressed[key] = true;
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
    }
    
    double x_offset = x_pos - last_mouse_x;
    double y_offset = last_mouse_y - y_pos; // Reversed since y-coordinates go from bottom to top
    last_mouse_x = x_pos;
    last_mouse_y = y_pos;
    
    x_offset *= mouse_sensitivity;
    y_offset *= mouse_sensitivity;
    
    // Update camera orientation
    static float yaw = -90.0f;  // Start looking down negative Z axis
    static float pitch = 0.0f;
    
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
        camera->position.y += velocity;
        camera->target.y += velocity;
        camera_moved = true;
    }
    if (keys_pressed[GLFW_KEY_LEFT_SHIFT]) {
        camera->position.y -= velocity;
        camera->target.y -= velocity;
        camera_moved = true;
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
