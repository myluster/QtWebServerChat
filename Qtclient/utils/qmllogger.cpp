#include "qmllogger.h"

QMLLogger::QMLLogger(QObject *parent)
    : QObject(parent)
{
}

void QMLLogger::trace(const QString& message)
{
    Logger::getInstance().trace(message);
}

void QMLLogger::debug(const QString& message)
{
    Logger::getInstance().debug(message);
}

void QMLLogger::info(const QString& message)
{
    Logger::getInstance().info(message);
}

void QMLLogger::warn(const QString& message)
{
    Logger::getInstance().warn(message);
}

void QMLLogger::error(const QString& message)
{
    Logger::getInstance().error(message);
}

void QMLLogger::critical(const QString& message)
{
    Logger::getInstance().critical(message);
}

void QMLLogger::setLevel(int level)
{
    if (level >= 0 && level <= 5) {
        Logger::getInstance().setLevel(static_cast<LogLevel>(level));
    }
}