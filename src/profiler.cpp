#include "profiler.h"
#include "error_handling.h"
#include <iostream>
#include <iomanip>
#include <limits>

PerformanceProfiler PerformanceProfiler::instance_;

void PerformanceProfiler::start_timer(const std::string& name) {
    // For manual timing (not used with RAII approach)
}

void PerformanceProfiler::end_timer(const std::string& name) {
    // For manual timing (not used with RAII approach)
}

void PerformanceProfiler::print_results() const {
    if (profiles_.empty()) {
        ErrorHandling::Logger::info("No profiling data available");
        return;
    }
    
    ErrorHandling::Logger::info("=== Performance Profiling Results ===");
    ErrorHandling::Logger::info("Name                     | Calls  | Total(ms) | Avg(ms)  | Min(ms)  | Max(ms)");
    ErrorHandling::Logger::info("-------------------------|--------|-----------|----------|----------|----------");
    
    for (const auto& [name, data] : profiles_) {
        double avg_time = data.total_time / data.call_count;
        
        std::ostringstream oss;
        oss << std::left << std::setw(24) << name << " | "
            << std::right << std::setw(6) << data.call_count << " | "
            << std::right << std::setw(9) << std::fixed << std::setprecision(2) << data.total_time << " | "
            << std::right << std::setw(8) << std::fixed << std::setprecision(2) << avg_time << " | "
            << std::right << std::setw(8) << std::fixed << std::setprecision(2) << data.min_time << " | "
            << std::right << std::setw(8) << std::fixed << std::setprecision(2) << data.max_time;
        
        ErrorHandling::Logger::info(oss.str());
    }
    
    ErrorHandling::Logger::info("=====================================");
}

void PerformanceProfiler::reset() {
    profiles_.clear();
}
