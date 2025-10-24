#include "logger.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <iostream>

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

Logger::Logger() {
    try {
        // 创建控制台sink（带颜色）
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::trace);
        
        // 创建每日轮转文件sink
        auto daily_file_sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(
            "logs/daily_chat.log", 2, 30);
        daily_file_sink->set_level(spdlog::level::trace);
        
        // 创建大小轮转文件sink (最大10MB，保留5个旧文件)
        auto rotating_file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            "logs/rotating_chat.log", 1024 * 1024 * 10, 5);
        rotating_file_sink->set_level(spdlog::level::trace);
        
        // 创建logger并注册sinks
        logger_ = std::make_shared<spdlog::logger>(
            "chat_logger", spdlog::sinks_init_list{
                console_sink, 
                daily_file_sink, 
                rotating_file_sink
            });
            
        // 注册logger以便全局访问
        spdlog::register_logger(logger_);
        
        // 设置日志格式
        logger_->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] %v");
        
        // 设置默认日志级别
        logger_->set_level(spdlog::level::trace);
        
        // 启用flush on debug level
        logger_->flush_on(spdlog::level::debug);
        
        // 设置默认logger
        spdlog::set_default_logger(logger_);
        
    } catch (const spdlog::spdlog_ex& ex) {
        std::cerr << "Log initialization failed: " << ex.what() << std::endl;
    }
}

void Logger::setLevel(Level level) {
    spdlog::level::level_enum spdlogLevel;
    switch (level) {
        case Level::TRACE: spdlogLevel = spdlog::level::trace; break;
        case Level::DEBUG: spdlogLevel = spdlog::level::debug; break;
        case Level::INFO: spdlogLevel = spdlog::level::info; break;
        case Level::WARN: spdlogLevel = spdlog::level::warn; break;
        case Level::ERROR: spdlogLevel = spdlog::level::err; break;
        case Level::CRITICAL: spdlogLevel = spdlog::level::critical; break;
        case Level::OFF: spdlogLevel = spdlog::level::off; break;
        default: spdlogLevel = spdlog::level::debug; break;
    }
    logger_->set_level(spdlogLevel);
}

void Logger::setPattern(const std::string& pattern) {
    logger_->set_pattern(pattern);
}