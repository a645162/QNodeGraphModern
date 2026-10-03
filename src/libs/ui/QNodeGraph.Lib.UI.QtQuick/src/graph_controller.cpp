#include <QNodeGraph/Lib/UI/QtQuick/graph_controller.h>

#include <QHash>
#include <QString>

#include <string>

namespace QNodeGraph::UI {

GraphController::GraphController(QObject* parent)
    : QAbstractListModel(parent), m_document("QNodeGraphModern") {}

int GraphController::nodeCount() const noexcept {
    return static_cast<int>(m_nodeOrder.size());
}

int GraphController::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(m_nodeOrder.size());
}

QVariant GraphController::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 ||
        index.row() >= static_cast<int>(m_nodeOrder.size())) {
        return {};
    }
    const auto* node = m_document.node(m_nodeOrder[static_cast<std::size_t>(index.row())]);
    if (node == nullptr) {
        return {};
    }
    switch (role) {
    case Qt::DisplayRole:
    case NodeNameRole:
        return QString::fromStdString(node->name);
    case NodeIdRole:
        return QVariant::fromValue<qulonglong>(node->id);
    case NodeTypeRole:
        return QString::fromStdString(node->type);
    case NodeXRole:
        return node->position.x;
    case NodeYRole:
        return node->position.y;
    case InputPortCountRole: {
        int count = 0;
        for (const auto portId : node->ports) {
            const auto* port = m_document.port(portId);
            if (port != nullptr && port->direction == Core::PortDirection::Input) {
                ++count;
            }
        }
        return count;
    }
    case OutputPortCountRole: {
        int count = 0;
        for (const auto portId : node->ports) {
            const auto* port = m_document.port(portId);
            if (port != nullptr && port->direction == Core::PortDirection::Output) {
                ++count;
            }
        }
        return count;
    }
    default:
        return {};
    }
}

QHash<int, QByteArray> GraphController::roleNames() const {
    return {
        {NodeIdRole, "nodeId"},
        {NodeNameRole, "nodeName"},
        {NodeTypeRole, "nodeType"},
        {NodeXRole, "nodeX"},
        {NodeYRole, "nodeY"},
        {InputPortCountRole, "inputPortCount"},
        {OutputPortCountRole, "outputPortCount"},
    };
}

void GraphController::addDemoNode() {
    const auto row = static_cast<int>(m_nodeOrder.size());
    const auto node = m_document.addNode(
        "demo", "Demo Node " + std::to_string(row + 1));
    if (!node) {
        return;
    }
    const auto input = m_document.addPort(
        *node, "in", Core::PortDirection::Input, Core::PortDataType::Any);
    const auto output = m_document.addPort(
        *node, "out", Core::PortDirection::Output, Core::PortDataType::Any);
    if (!input || !output ||
        !m_document.setNodePosition(*node, {80.0 + (row % 3) * 250.0,
                                             90.0 + (row / 3) * 180.0})) {
        m_document.removeNode(*node);
        return;
    }
    beginInsertRows(QModelIndex(), row, row);
    m_nodeOrder.push_back(*node);
    endInsertRows();
    emit nodeCountChanged();
}

void GraphController::clearGraph() {
    if (m_nodeOrder.empty()) {
        return;
    }
    beginResetModel();
    m_nodeOrder.clear();
    m_document.clear();
    endResetModel();
    emit nodeCountChanged();
}

bool GraphController::moveNode(int row, double x, double y) {
    if (row < 0 || row >= static_cast<int>(m_nodeOrder.size())) {
        return false;
    }
    const auto nodeId = m_nodeOrder[static_cast<std::size_t>(row)];
    if (!m_document.setNodePosition(nodeId, {x, y})) {
        return false;
    }
    const auto modelIndex = index(row, 0);
    emit dataChanged(modelIndex, modelIndex, {NodeXRole, NodeYRole});
    return true;
}

} // namespace QNodeGraph::UI
