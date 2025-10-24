#include "logger.h"
#include <QDateTime>
#include <QDir>
#include <QStandardPaths>
#include <QCoreApplication>
#include <iostream>

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

Logger::Logger() : currentLevel(LogLevel::DEBUG) {
    // 创建日志目录
    QString logDirPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (logDirPath.isEmpty()) {
        logDirPath = QDir::currentPath() + "/logs";
    } else {
        logDirPath += "/logs";
    }
    
    QDir logDir(logDirPath);
    if (!logDir.exists()) {
        logDir.mkpath(".");
    }
    
    // 创建日志文件
    QString logFilePath = logDirPath + "/client.log";
    logFile = std::make_unique<QFile>(logFilePath);
    
    if (!logFile->open(QIODevice::WriteOnly | QIODevice::Append)) {
        std::cerr << "Failed to open log file: " << logFilePath.toStdString() << std::endl;
    }
}

void Logger::setLevel(LogLevel level) {
    currentLevel = level;
}

void Logger::trace(const QString& message) {
    log(LogLevel::TRACE, message);
}

void Logger::debug(const QString& message) {
    log(LogLevel::DEBUG, message);
}

void Logger::info(const QString& message) {
    log(LogLevel::INFO, message);
}

void Logger::warn(const QString& message) {
    log(LogLevel::WARN, message);
}

void Logger::error(const QString& message) {
    log(LogLevel::ERROR, message);
}

void Logger::critical(const QString& message) {
    log(LogLevel::CRITICAL, message);
}

void Logger::log(LogLevel level, const QString& message) {
    // 检查日志级别
    if (level < currentLevel) {
        return;
    }
    
    QMutexLocker locker(&logMutex);
    
    QString timestamp = getCurrentTimestamp();
    QString levelStr = levelToString(level);
    QString logEntry = QString("[%1] [%2] %3\n").arg(timestamp, levelStr, message);
    
    // 输出到控制台
    std::cout << logEntry.toStdString();
    
    // 写入文件
    writeToFile(logEntry);
}

QString Logger::levelToString(LogLevel level) const {
    switch (level) {
        case LogLevel::TRACE:    return "TRACE";
        case LogLevel::DEBUG:    return "DEBUG";
        case LogLevel::INFO:     return "INFO";
        case LogLevel::WARN:     return "WARN";
        case LogLevel::ERROR:    return "ERROR";
        case LogLevel::CRITICAL: return "CRITICAL";
        default:                 return "UNKNOWN";
    }
}

QString Logger::getCurrentTimestamp() const {
    return QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
}

void Logger::writeToFile(const QString& logEntry) {
    if (logFile && logFile->isOpen()) {
        QTextStream stream(logFile.get());
        stream << logEntry;
        logFile->flush();
    }
}