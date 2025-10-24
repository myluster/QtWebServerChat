#include <QGuiApplication>
#include <QSurfaceFormat>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QThreadPool>
#include "backend/networkmanager.h"
#include "utils/qmllogger.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    // 启用高DPI缩放支持
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    // 设置线程池大小以提高并发性能
    QThreadPool::globalInstance()->setMaxThreadCount(QThread::idealThreadCount());

    QSurfaceFormat format;

    // 将这个 format 应用为应用的默认格式
    QSurfaceFormat::setDefaultFormat(format);

    QQuickStyle::setStyle("Fusion");

    QQuickWindow::setDefaultAlphaBuffer(true);


    QQmlApplicationEngine engine;

    // 创建一个 NetworkManager 实例，并将其父对象设置为 app，这样它将在 app 退出时被自动清理
    NetworkManager *networkManager = new NetworkManager(&app);

    // 将此实例注册为 QML 单例
    // QML 中将通过 "NetworkManager" 这个名字全局访问
    qmlRegisterSingletonInstance("Network", 1, 0, "NetworkManager", networkManager);

    // 创建QML日志实例并注册为QML单例
    QMLLogger *qmlLogger = new QMLLogger(&app);
    qmlRegisterSingletonInstance("Logger", 1, 0, "Logger", qmlLogger);

    const QUrl url(QStringLiteral("qrc:/qml/main.qml"));

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.load(url);

    return app.exec();
}