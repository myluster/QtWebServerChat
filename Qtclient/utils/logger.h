#ifndef LOGGER_H
#define LOGGER_H

#include <QDateTime>
#include <QString>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QMutex>
#include <memory>

// 日志级别枚举
enum class LogLevel {
    TRACE = 0,
    DEBUG = 1,
    INFO = 2,
    WARN = 3,
    ERROR = 4,
    CRITICAL = 5
};

class Logger {
public:
    static Logger& getInstance();
    
    // 删除拷贝构造函数和赋值操作符
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    // 日志级别设置方法
    void setLevel(LogLevel level);
    
    // 日志方法
    void trace(const QString& message);
    void debug(const QString& message);
    void info(const QString& message);
    void warn(const QString& message);
    void error(const QString& message);
    void critical(const QString& message);
    
    // 模板方法，支持格式化字符串
    template<typename... Args>
    void trace(const QString& format, Args&&... args) {
        log(LogLevel::TRACE, format.arg(std::forward<Args>(args)...));
    }
    
    template<typename... Args>
    void debug(const QString& format, Args&&... args) {
        log(LogLevel::DEBUG, format.arg(std::forward<Args>(args)...));
    }
    
    template<typename... Args>
    void info(const QString& format, Args&&... args) {
        log(LogLevel::INFO, format.arg(std::forward<Args>(args)...));
    }
    
    template<typename... Args>
    void warn(const QString& format, Args&&... args) {
        log(LogLevel::WARN, format.arg(std::forward<Args>(args)...));
    }
    
    template<typename... Args>
    void error(const QString& format, Args&&... args) {
        log(LogLevel::ERROR, format.arg(std::forward<Args>(args)...));
    }
    
    template<typename... Args>
    void critical(const QString& format, Args&&... args) {
        log(LogLevel::CRITICAL, format.arg(std::forward<Args>(args)...));
    }

private:
    Logger();
    ~Logger() = default;
    
    void log(LogLevel level, const QString& message);
    QString levelToString(LogLevel level) const;
    QString getCurrentTimestamp() const;
    void writeToFile(const QString& logEntry);
    
    LogLevel currentLevel;
    std::unique_ptr<QFile> logFile;
    QMutex logMutex;
};

// 方便使用的宏定义
#define LOG_TRACE(...) Logger::getInstance().trace(__VA_ARGS__)
#define LOG_DEBUG(...) Logger::getInstance().debug(__VA_ARGS__)
#define LOG_INFO(...) Logger::getInstance().info(__VA_ARGS__)
#define LOG_WARN(...) Logger::getInstance().warn(__VA_ARGS__)
#define LOG_ERROR(...) Logger::getInstance().error(__VA_ARGS__)
#define LOG_CRITICAL(...) Logger::getInstance().critical(__VA_ARGS__)

#endif // LOGGER_H