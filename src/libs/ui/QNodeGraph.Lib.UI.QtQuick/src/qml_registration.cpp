#include <QNodeGraph/Lib/UI/QtQuick/graph_controller.h>
#include <QNodeGraph/Lib/UI/QtQuick/qml_registration.h>

#include <QtQml/qqml.h>

namespace QNodeGraph::UI {

void registerQNodeGraphQmlTypes() {
    qmlRegisterType<GraphController>("QNodeGraph.UI", 1, 0,
                                     "GraphController");
}

} // namespace QNodeGraph::UI

