#include "pipeline_controller.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("QNodeGraph Image Pipeline Demo"));

    QNodeGraph::UI::ImageFrameProvider provider;
    PipelineController controller(&provider);
    QQmlApplicationEngine engine;
    engine.addImageProvider(QStringLiteral("pipeline"), &provider);
    engine.rootContext()->setContextProperty(QStringLiteral("pipelineController"),
                                             &controller);
    engine.loadFromModule("QNodeGraph.ImagePipelineDemo", "Demo");
    if (engine.rootObjects().isEmpty()) {
        return 1;
    }

    return app.exec();
}
