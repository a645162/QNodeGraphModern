#include <QGuiApplication>
#include <QQmlApplicationEngine>

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("QNodeGraph Basic Demo"));
    QQmlApplicationEngine engine;
    engine.loadFromModule("QNodeGraph.BasicDemo", "Main");
    if (engine.rootObjects().isEmpty()) {
        return 1;
    }

    return app.exec();
}
