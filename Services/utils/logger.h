#ifndef LOGGER_H
#define LOGGER_H

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <memory>
#include <string>

// 添加日志级别的枚举定义
enum class Level {
    TRACE = 0,
    DEBUG = 1,
    INFO = 2,
    WARN = 3,
    ERROR = 4,
    CRITICAL = 5,
    OFF = 6
};

class Logger {
public:
    static Logger& getInstance();
    
    // 删除拷贝构造函数和赋值操作符
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    // 日志级别设置方法
    void setLevel(Level level);
    
    // 日志格式设置方法
    void setPattern(const std::string& pattern);
    
    // 日志方法
    template<typename... Args>
    void trace(const std::string& fmt, Args&&... args) {
        logger_->trace(fmt, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    void debug(const std::string& fmt, Args&&... args) {
        logger_->debug(fmt, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    void info(const std::string& fmt, Args&&... args) {
        logger_->info(fmt, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    void warn(const std::string& fmt, Args&&... args) {
        logger_->warn(fmt, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    void error(const std::string& fmt, Args&&... args) {
        logger_->error(fmt, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    void critical(const std::string& fmt, Args&&... args) {
        logger_->critical(fmt, std::forward<Args>(args)...);
    }

private:
    Logger();
    ~Logger() = default;
    
    std::shared_ptr<spdlog::logger> logger_;
};

// 方便使用的宏定义
#define LOG_TRACE(...) Logger::getInstance().trace(__VA_ARGS__)
#define LOG_DEBUG(...) Logger::getInstance().debug(__VA_ARGS__)
#define LOG_INFO(...) Logger::getInstance().info(__VA_ARGS__)
#define LOG_WARN(...) Logger::getInstance().warn(__VA_ARGS__)
#define LOG_ERROR(...) Logger::getInstance().error(__VA_ARGS__)
#define LOG_CRITICAL(...) Logger::getInstance().critical(__VA_ARGS__)

#endif // LOGGER_H