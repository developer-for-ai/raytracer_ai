#pragma once
#include <GL/glew.h>

// RAII wrappers for OpenGL resources to prevent leaks and ensure proper cleanup

class GLBuffer {
private:
    GLuint id_;
    
public:
    GLBuffer() : id_(0) {
        glGenBuffers(1, &id_);
    }
    
    ~GLBuffer() {
        if (id_ != 0) {
            glDeleteBuffers(1, &id_);
        }
    }
    
    // Non-copyable but movable
    GLBuffer(const GLBuffer&) = delete;
    GLBuffer& operator=(const GLBuffer&) = delete;
    
    GLBuffer(GLBuffer&& other) noexcept : id_(other.id_) {
        other.id_ = 0;
    }
    
    GLBuffer& operator=(GLBuffer&& other) noexcept {
        if (this != &other) {
            if (id_ != 0) {
                glDeleteBuffers(1, &id_);
            }
            id_ = other.id_;
            other.id_ = 0;
        }
        return *this;
    }
    
    GLuint get() const noexcept { return id_; }
    bool is_valid() const noexcept { return id_ != 0; }
};

class GLTexture {
private:
    GLuint id_;
    
public:
    GLTexture() : id_(0) {
        glGenTextures(1, &id_);
    }
    
    ~GLTexture() {
        if (id_ != 0) {
            glDeleteTextures(1, &id_);
        }
    }
    
    // Non-copyable but movable
    GLTexture(const GLTexture&) = delete;
    GLTexture& operator=(const GLTexture&) = delete;
    
    GLTexture(GLTexture&& other) noexcept : id_(other.id_) {
        other.id_ = 0;
    }
    
    GLTexture& operator=(GLTexture&& other) noexcept {
        if (this != &other) {
            if (id_ != 0) {
                glDeleteTextures(1, &id_);
            }
            id_ = other.id_;
            other.id_ = 0;
        }
        return *this;
    }
    
    GLuint get() const noexcept { return id_; }
    bool is_valid() const noexcept { return id_ != 0; }
};

class GLVertexArray {
private:
    GLuint id_;
    
public:
    GLVertexArray() : id_(0) {
        glGenVertexArrays(1, &id_);
    }
    
    ~GLVertexArray() {
        if (id_ != 0) {
            glDeleteVertexArrays(1, &id_);
        }
    }
    
    // Non-copyable but movable
    GLVertexArray(const GLVertexArray&) = delete;
    GLVertexArray& operator=(const GLVertexArray&) = delete;
    
    GLVertexArray(GLVertexArray&& other) noexcept : id_(other.id_) {
        other.id_ = 0;
    }
    
    GLVertexArray& operator=(GLVertexArray&& other) noexcept {
        if (this != &other) {
            if (id_ != 0) {
                glDeleteVertexArrays(1, &id_);
            }
            id_ = other.id_;
            other.id_ = 0;
        }
        return *this;
    }
    
    GLuint get() const noexcept { return id_; }
    bool is_valid() const noexcept { return id_ != 0; }
};

class GLProgram {
private:
    GLuint id_;
    
public:
    GLProgram() : id_(0) {
        id_ = glCreateProgram();
    }
    
    ~GLProgram() {
        if (id_ != 0) {
            glDeleteProgram(id_);
        }
    }
    
    // Non-copyable but movable
    GLProgram(const GLProgram&) = delete;
    GLProgram& operator=(const GLProgram&) = delete;
    
    GLProgram(GLProgram&& other) noexcept : id_(other.id_) {
        other.id_ = 0;
    }
    
    GLProgram& operator=(GLProgram&& other) noexcept {
        if (this != &other) {
            if (id_ != 0) {
                glDeleteProgram(id_);
            }
            id_ = other.id_;
            other.id_ = 0;
        }
        return *this;
    }
    
    GLuint get() const noexcept { return id_; }
    bool is_valid() const noexcept { return id_ != 0; }
};
