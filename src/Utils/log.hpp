#pragma once

#include <print>
#include <string_view>
#include <utility>

/**
 * @brief Simple logger
 * 
 */
/*
Usage:
    Logger::Log(Logger::LogLevel::Level, "string {}", Args...)
*/
class Logger {
public:
    enum class LogLevel {
        // Info during development
        DEBUG,
        // General information
        INFO,
        // Recoverable errors
        WARN,
        // Unrecoverable errors
        ERROR,
    };

    template <typename... Args>
    static void Log(LogLevel level, std::format_string<Args...> fmt, Args&&... args) {
        auto [color, name]{GetLevelInfo(level)};

        std::print("{}[{}]\x1b[0m ", color, name);
        std::println(fmt, std::forward<Args>(args)...);
    }

private:
    static std::pair<std::string_view, std::string_view> GetLevelInfo(LogLevel level) {
        switch (level) 
        {
        case Logger::LogLevel::DEBUG: return {"\x1b[90m", "DEBUG"};    
        case Logger::LogLevel::INFO:  return {"\x1b[32m", "INFO"};    
        case Logger::LogLevel::WARN:  return {"\x1b[33m", "WARN"};    
        case Logger::LogLevel::ERROR: return {"\x1b[31m", "ERROR"};    
        }

        return {"\x1b[0m", "UNKNOWN"};
    }
};

#define LOGD(format, ...) Logger::Log(Logger::LogLevel::DEBUG, format __VA_OPT__(,) __VA_ARGS__)
#define LOGI(format, ...) Logger::Log(Logger::LogLevel::INFO,  format __VA_OPT__(,) __VA_ARGS__)
#define LOGW(format, ...) Logger::Log(Logger::LogLevel::WARN,  format __VA_OPT__(,) __VA_ARGS__)
#define LOGE(format, ...) Logger::Log(Logger::LogLevel::ERROR, format __VA_OPT__(,) __VA_ARGS__)
