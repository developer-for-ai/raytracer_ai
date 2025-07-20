#include "input.h"
#include "window.h"
#include <cstring>
#include <cmath>

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
      movement_speed(300.0f), mouse_sensitivity(0.1f), yaw(-90.0f), pitch(0.0f) {
    std::memset(keys_pressed, false, sizeof(keys_pressed));
    last_mouse_x = 0.0;
    last_mouse_y = 0.0;
    
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
            if (key == GLFW_KEY_R) {
                reset_camera();
            }
        } else if (action == GLFW_RELEASE) {
            keys_pressed[key] = false;
        }
    }
}

void InputHandler::process_mouse(double x_pos, double y_pos) {
    if (!window || !window->is_mouse_captured()) {
        first_mouse = true;
        return;
    }
    
    if (first_mouse) {
        last_mouse_x = x_pos;
        last_mouse_y = y_pos;
        first_mouse = false;
        initialize_from_camera();
    }
    
    double x_offset = x_pos - last_mouse_x;
    double y_offset = last_mouse_y - y_pos;
    last_mouse_x = x_pos;
    last_mouse_y = y_pos;
    
    x_offset *= mouse_sensitivity;
    y_offset *= mouse_sensitivity;
    
    yaw += x_offset;
    pitch += y_offset;
    
    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;
    
    Vec3 front;
    front.x = std::cos(yaw * M_PI / 180.0f) * std::cos(pitch * M_PI / 180.0f);
    front.y = std::sin(pitch * M_PI / 180.0f);
    front.z = std::sin(yaw * M_PI / 180.0f) * std::cos(pitch * M_PI / 180.0f);
    
    camera->target = camera->position + front.normalize();
    camera->update_camera();
    
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
    
    Vec3 movement_direction(0, 0, 0);
    
    if (keys_pressed[GLFW_KEY_W]) {
        movement_direction = movement_direction + front;
    }
    if (keys_pressed[GLFW_KEY_S]) {
        movement_direction = movement_direction - front;
    }
    if (keys_pressed[GLFW_KEY_A]) {
        movement_direction = movement_direction - right;
    }
    if (keys_pressed[GLFW_KEY_D]) {
        movement_direction = movement_direction + right;
    }
    
    if (movement_direction.x != 0 || movement_direction.z != 0) {
        movement_direction = movement_direction.normalize();
        Vec3 movement = movement_direction * velocity;
        camera->position = camera->position + movement;
        camera->target = camera->target + movement;
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
    
    bool camera_rotated = false;
    float arrow_sensitivity = mouse_sensitivity * 10.0f;
    
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
        if (!mouse_initialized) {
            initialize_from_camera();
        }
        
        if (pitch > 89.0f) pitch = 89.0f;
        if (pitch < -89.0f) pitch = -89.0f;
        
        Vec3 front_new;
        front_new.x = std::cos(yaw * M_PI / 180.0f) * std::cos(pitch * M_PI / 180.0f);
        front_new.y = std::sin(pitch * M_PI / 180.0f);
        front_new.z = std::sin(yaw * M_PI / 180.0f) * std::cos(pitch * M_PI / 180.0f);
        
        camera->target = camera->position + front_new.normalize();
        camera->update_camera();
        
        if (window) {
            window->reset_accumulation();
        }
    }
    
    if (camera_moved) {
        camera->update_camera();
        if (window) {
            window->reset_accumulation();
        }
    }
}

void InputHandler::initialize_from_camera() {
    if (!camera || mouse_initialized) return;
    
    Vec3 direction = (camera->target - camera->position).normalize();
    yaw = std::atan2(direction.z, direction.x) * 180.0f / M_PI;
    pitch = std::asin(-direction.y) * 180.0f / M_PI;
    mouse_initialized = true;
}

void InputHandler::reset_camera() {
    if (!camera) return;
    
    camera->position = initial_position;
    camera->target = initial_target;  
    camera->up = initial_up;
    camera->update_camera();
    
    mouse_initialized = false;
    first_mouse = true;
    initialize_from_camera();
    
    if (window) {
        window->reset_accumulation();
    }
}
