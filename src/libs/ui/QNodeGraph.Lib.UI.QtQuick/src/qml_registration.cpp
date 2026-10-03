#include <QNodeGraph/Lib/UI/QtQuick/graph_controller.h>
#include <QNodeGraph/Lib/UI/QtQuick/node_palette_model.h>
#include <QNodeGraph/Lib/UI/QtQuick/qml_registration.h>

#include <QtQml/qqml.h>

// Qt 6.12 names the generated qmlcache resource after the URI while the
// static plugin uses the target name. Keep both symbols available to static
// library consumers.
void qInitResources_qmlcache_QNodeGraph_Lib_UI();
void qInitResources_qmlcache_QNodeGraph_Lib_UI_QtQuick() {
    qInitResources_qmlcache_QNodeGraph_Lib_UI();
}

namespace QNodeGraph::UI {

void registerQNodeGraphQmlTypes() {
    qmlRegisterType<GraphController>("QNodeGraph.UI", 1, 0,
                                     "GraphController");
    qmlRegisterType<NodePaletteModel>("QNodeGraph.UI", 1, 0,
                                      "NodePaletteModel");
}

} // namespace QNodeGraph::UI
