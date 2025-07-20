#pragma once
#include <chrono>
#include <string>
#include <unordered_map>

class PerformanceProfiler {
private:
    struct ProfileData {
        double total_time = 0.0;
        size_t call_count = 0;
        double min_time = std::numeric_limits<double>::max();
        double max_time = 0.0;
    };
    
    std::unordered_map<std::string, ProfileData> profiles_;
    static PerformanceProfiler instance_;
    
public:
    static PerformanceProfiler& get_instance() { return instance_; }
    
    void start_timer(const std::string& name);
    void end_timer(const std::string& name);
    void print_results() const;
    void reset();
    
    // RAII timer class for automatic profiling
    class ScopedTimer {
    private:
        std::string name_;
        std::chrono::high_resolution_clock::time_point start_time_;
        
    public:
        explicit ScopedTimer(const std::string& name) 
            : name_(name), start_time_(std::chrono::high_resolution_clock::now()) {}
            
        ~ScopedTimer() {
            auto end_time = std::chrono::high_resolution_clock::now();
            double duration = std::chrono::duration<double, std::milli>(end_time - start_time_).count();
            
            auto& profiler = PerformanceProfiler::get_instance();
            auto& data = profiler.profiles_[name_];
            data.total_time += duration;
            data.call_count++;
            data.min_time = std::min(data.min_time, duration);
            data.max_time = std::max(data.max_time, duration);
        }
    };
};

// Macro for easy profiling
#ifdef ENABLE_PROFILING
    #define PROFILE_SCOPE(name) PerformanceProfiler::ScopedTimer _timer(name)
    #define PROFILE_FUNCTION() PerformanceProfiler::ScopedTimer _timer(__FUNCTION__)
#else
    #define PROFILE_SCOPE(name)
    #define PROFILE_FUNCTION()
#endif
