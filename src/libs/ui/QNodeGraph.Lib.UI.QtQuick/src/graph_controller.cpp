#include <QNodeGraph/Lib/UI/QtQuick/graph_controller.h>

namespace QNodeGraph::UI {

GraphController::GraphController(QObject* parent)
    : QObject(parent), m_document("QNodeGraphModern") {}

int GraphController::nodeCount() const noexcept {
    return static_cast<int>(m_document.nodeCount());
}

void GraphController::addDemoNode() {
    m_document.addNode();
    emit nodeCountChanged();
}

void GraphController::clearGraph() {
    m_document.clear();
    emit nodeCountChanged();
}

} // namespace QNodeGraph::UI

