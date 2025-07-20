#pragma once
#include <string>
#include <iostream>

namespace ErrorHandling {
    enum class LogLevel {
        DEBUG,
        INFO, 
        WARNING,
        ERROR,
        CRITICAL
    };
    
    class Logger {
    private:
        static LogLevel current_level;
        
    public:
        static void set_level(LogLevel level) noexcept { current_level = level; }
        
        static void log(LogLevel level, const std::string& message) {
            if (level >= current_level) {
                std::cerr << level_to_string(level) << ": " << message << std::endl;
            }
        }
        
        static void debug(const std::string& message) { log(LogLevel::DEBUG, message); }
        static void info(const std::string& message) { log(LogLevel::INFO, message); }
        static void warning(const std::string& message) { log(LogLevel::WARNING, message); }
        static void error(const std::string& message) { log(LogLevel::ERROR, message); }
        static void critical(const std::string& message) { log(LogLevel::CRITICAL, message); }
        
    private:
        static constexpr const char* level_to_string(LogLevel level) noexcept {
            switch (level) {
                case LogLevel::DEBUG: return "DEBUG";
                case LogLevel::INFO: return "INFO";
                case LogLevel::WARNING: return "WARNING";
                case LogLevel::ERROR: return "ERROR";
                case LogLevel::CRITICAL: return "CRITICAL";
                default: return "UNKNOWN";
            }
        }
    };
    
    // Helper macros for common error patterns
    #define LOG_ERROR_AND_RETURN_FALSE(msg) do { ErrorHandling::Logger::error(msg); return false; } while(0)
    #define LOG_CRITICAL_AND_EXIT(msg) do { ErrorHandling::Logger::critical(msg); std::exit(1); } while(0)
}
