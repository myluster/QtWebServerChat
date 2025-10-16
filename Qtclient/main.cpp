#include <QGuiApplication>
#include <QSurfaceFormat>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QQuickWindow>

int main(int argc, char *argv[])
{   
    QGuiApplication app(argc, argv);



    QSurfaceFormat format;
    // 3. 设置抗锯齿的样本数量 (数值越高，线条越平滑，但对性能要求也越高)
    //    通常 4 或 8 已经能提供很好的效果
    format.setSamples(8);
    // 4. 将这个 format 应用为应用的默认格式
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
