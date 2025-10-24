#ifndef QMLLOGGER_H
#define QMLLOGGER_H

#include <QObject>
#include "logger.h"

class QMLLogger : public QObject
{
    Q_OBJECT

public:
    explicit QMLLogger(QObject *parent = nullptr);

    // QML可调用的日志方法
    Q_INVOKABLE void trace(const QString& message);
    Q_INVOKABLE void debug(const QString& message);
    Q_INVOKABLE void info(const QString& message);
    Q_INVOKABLE void warn(const QString& message);
    Q_INVOKABLE void error(const QString& message);
    Q_INVOKABLE void critical(const QString& message);

    // 设置日志级别
    Q_INVOKABLE void setLevel(int level);
};

#endif // QMLLOGGER_H