#include <QGuiApplication>
#include <QSurfaceFormat>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QThreadPool>

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

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    const QUrl url(QStringLiteral("qrc:/qml/main.qml"));

    engine.load(url);

    return app.exec();
}
