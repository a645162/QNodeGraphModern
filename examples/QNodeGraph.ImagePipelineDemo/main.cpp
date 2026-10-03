#include <QNodeGraph/Lib/UI/QtQuick/qml_registration.h>

#include <QGuiApplication>
#include <QQmlApplicationEngine>

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("QNodeGraph Image Pipeline Demo"));

    QNodeGraph::UI::registerQNodeGraphQmlTypes();

    QQmlApplicationEngine engine;
    engine.loadFromModule("QNodeGraph.ImagePipelineDemo", "Demo");
    if (engine.rootObjects().isEmpty()) {
        return 1;
    }

    return app.exec();
}
