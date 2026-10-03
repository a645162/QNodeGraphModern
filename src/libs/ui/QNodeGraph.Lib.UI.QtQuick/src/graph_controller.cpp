#include <QNodeGraph/Lib/UI/QtQuick/graph_controller.h>

#include <QHash>
#include <QString>
#include <QVariantMap>

#include <string>

namespace QNodeGraph::UI {

namespace {

constexpr double kDemoNodeWidth = 180.0;
constexpr double kDemoNodeHeight = 108.0;

} // namespace

GraphController::GraphController(QObject* parent)
    : QAbstractListModel(parent), m_document("QNodeGraphModern") {}

int GraphController::nodeCount() const noexcept {
    return static_cast<int>(m_nodeOrder.size());
}

QVariantList GraphController::connections() const {
    QVariantList values;
    for (const auto& connection : m_document.connections()) {
        const auto* outputPort = m_document.port(connection.outputPort);
        const auto* inputPort = m_document.port(connection.inputPort);
        if (outputPort == nullptr || inputPort == nullptr) {
            continue;
        }
        const auto* outputNode = m_document.node(outputPort->nodeId);
        const auto* inputNode = m_document.node(inputPort->nodeId);
        const auto outputRow = rowFor(outputPort->nodeId);
        const auto inputRow = rowFor(inputPort->nodeId);
        if (outputNode == nullptr || inputNode == nullptr || outputRow < 0 ||
            inputRow < 0) {
            continue;
        }
        QVariantMap value;
        value.insert(QStringLiteral("outputRow"), outputRow);
        value.insert(QStringLiteral("inputRow"), inputRow);
        value.insert(QStringLiteral("outputX"), outputNode->position.x);
        value.insert(QStringLiteral("outputY"),
                     outputNode->position.y + kDemoNodeHeight / 2.0);
        value.insert(QStringLiteral("inputX"), inputNode->position.x);
        value.insert(QStringLiteral("inputY"),
                     inputNode->position.y + kDemoNodeHeight / 2.0);
        value.insert(QStringLiteral("outputWidth"), kDemoNodeWidth);
        values.push_back(value);
    }
    return values;
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
    if (row > 0) {
        const auto previousNode = m_nodeOrder[static_cast<std::size_t>(row - 1)];
        const auto previousOutput = portFor(previousNode, Core::PortDirection::Output);
        if (previousOutput != 0) {
            m_document.connect(previousOutput, *input);
        }
    }
    emit nodeCountChanged();
    emit connectionsChanged();
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
    emit connectionsChanged();
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
    emit connectionsChanged();
    return true;
}

Core::PortId GraphController::portFor(Core::NodeId nodeId,
                                      Core::PortDirection direction) const {
    const auto* value = m_document.node(nodeId);
    if (value == nullptr) {
        return 0;
    }
    for (const auto portId : value->ports) {
        const auto* port = m_document.port(portId);
        if (port != nullptr && port->direction == direction) {
            return portId;
        }
    }
    return 0;
}

int GraphController::rowFor(Core::NodeId nodeId) const {
    for (std::size_t row = 0; row < m_nodeOrder.size(); ++row) {
        if (m_nodeOrder[row] == nodeId) {
            return static_cast<int>(row);
        }
    }
    return -1;
}

} // namespace QNodeGraph::UI
