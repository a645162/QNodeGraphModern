#include "graph_controller_helpers.h"
#include <QNodeGraph/Lib/UI/QtQuick/graph_controller.h>

#include <QFileInfo>

#include <QHash>
#include <QMetaType>
#include <QString>
#include <QVariantMap>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <limits>
#include <queue>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>

namespace QNodeGraph::UI {

using namespace detail;

GraphController::GraphController(QObject* parent)
    : QAbstractListModel(parent), m_document("QNodeGraphModern"),
      m_registry(QNodeGraph::Graph::NodeRegistry::withBuiltins()) {
    bindDocumentObserver();
}

void GraphController::bindDocumentObserver() {
    m_document.setChangeObserver([this](const Core::GraphChange& change) {
        emit graphChanged(static_cast<qulonglong>(change.nodeId),
                          static_cast<int>(change.kind));
    });
}

void GraphController::setDocument(Core::GraphDocument document) {
    const auto oldCount = nodeCount();
    cancelConnection();
    beginResetModel();
    m_document = std::move(document);
    bindDocumentObserver();
    m_nodeOrder = m_document.nodeIds();
    m_nodePreviews.clear();
    m_commandStack.clear();
    endResetModel();
    setSelection({});
    if (nodeCount() != oldCount) {
        emit nodeCountChanged();
    }
    notifyTopologyChanged();
    emit historyChanged();
    emit propertiesChanged();
}

void GraphController::setRegistry(QNodeGraph::Graph::NodeRegistry registry) {
    m_registry = std::move(registry);
    // 改注册表后图内已有节点的图标/强调色/默认尺寸可能全部变化，需整表刷新。
    refreshRegistryDependentRoles();
    emit registryChanged();
}

const QNodeGraph::Graph::NodeRegistry&
GraphController::registry() const noexcept {
    return m_registry;
}

void GraphController::refreshRegistryDependentRoles() {
    if (!m_nodeOrder.empty()) {
        emit dataChanged(index(0, 0), index(nodeCount() - 1, 0),
                         {NodeIconRole, NodeIconSourceRole, NodeAccentRole,
                          NodeWidthRole, NodeHeightRole, NodeIsBackdropRole,
                          NodeIsGroupRole, NodeContentUrlRole,
                          NodeOnNodePropertiesRole});
    }
    notifyTopologyChanged();
    emit propertiesChanged();
}

int GraphController::nodeCount() const noexcept {
    return static_cast<int>(m_nodeOrder.size());
}

bool GraphController::connectionPending() const noexcept {
    return m_pendingOutputRow >= 0;
}

int GraphController::pendingOutputRow() const noexcept {
    return m_pendingOutputRow;
}

QVariantMap GraphController::connectionPreview() const {
    if (!connectionPending() || m_pendingOutputRow >= nodeCount()) {
        return {};
    }
    const auto nodeId =
        m_nodeOrder[static_cast<std::size_t>(m_pendingOutputRow)];
    const auto* node = m_document.node(nodeId);
    if (node == nullptr) {
        return {};
    }
    QVariantMap value;
    value.insert(QStringLiteral("outputRow"), m_pendingOutputRow);
    value.insert(QStringLiteral("outputX"), node->position.x);
    value.insert(QStringLiteral("outputY"), outputPortPoint(*node).y);
    value.insert(QStringLiteral("inputX"), m_previewPoint.x);
    value.insert(QStringLiteral("inputY"), m_previewPoint.y);
    value.insert(QStringLiteral("outputWidth"), nodeWidth(*node));
    return value;
}

int GraphController::selectedRow() const noexcept { return m_selectedRow; }

QVariantList GraphController::selectedRows() const {
    QVariantList rows;
    rows.reserve(static_cast<qsizetype>(m_selectedRows.size()));
    for (const auto row : m_selectedRows) {
        rows.push_back(row);
    }
    return rows;
}

bool GraphController::canUndo() const noexcept {
    return m_commandStack.canUndo();
}

bool GraphController::canRedo() const noexcept {
    return m_commandStack.canRedo();
}

QVariantList GraphController::selectedProperties() const {
    QVariantList values;
    if (m_selectedRow < 0 || m_selectedRow >= nodeCount()) {
        return values;
    }
    const auto* node =
        m_document.node(m_nodeOrder[static_cast<std::size_t>(m_selectedRow)]);
    if (node == nullptr) {
        return values;
    }
    for (const auto& [name, value] : node->properties) {
        if (name == "enabled") {
            continue;
        }
        if (name.size() >= 8 &&
            name.compare(name.size() - 8, 8, ".display") == 0) {
            continue;
        }
        const auto display = nodeStringProperty(
            *node, (name + ".display").c_str(), QStringLiteral("panel"));
        if (display == QStringLiteral("node") ||
            display == QStringLiteral("hidden")) {
            continue;
        }
        QVariantMap entry{
            {QStringLiteral("name"), QString::fromStdString(name)},
            {QStringLiteral("value"), propertyVariant(value)},
            {QStringLiteral("valueType"), propertyType(value)}};
        appendPropertyEditorMetadata(entry, m_registry.find(node->type), name);
        values.push_back(std::move(entry));
    }
    return values;
}

QString GraphController::lastError() const { return m_lastError; }

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
        value.insert(QStringLiteral("outputY"), outputPortPoint(*outputNode).y);
        value.insert(QStringLiteral("inputX"), inputNode->position.x);
        value.insert(QStringLiteral("inputY"), inputPortPoint(*inputNode).y);
        value.insert(QStringLiteral("outputWidth"), nodeWidth(*outputNode));
        value.insert(QStringLiteral("selected"),
                     outputRow == m_selectedConnectionOutputRow &&
                         inputRow == m_selectedConnectionInputRow);
        values.push_back(value);
    }
    return values;
}

int GraphController::selectedConnectionOutputRow() const noexcept {
    return m_selectedConnectionOutputRow;
}

int GraphController::selectedConnectionInputRow() const noexcept {
    return m_selectedConnectionInputRow;
}

int GraphController::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(m_nodeOrder.size());
}

QVariant GraphController::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 ||
        index.row() >= static_cast<int>(m_nodeOrder.size())) {
        return {};
    }
    const auto* node =
        m_document.node(m_nodeOrder[static_cast<std::size_t>(index.row())]);
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
            if (port != nullptr &&
                port->direction == Core::PortDirection::Input) {
                ++count;
            }
        }
        return count;
    }
    case OutputPortCountRole: {
        int count = 0;
        for (const auto portId : node->ports) {
            const auto* port = m_document.port(portId);
            if (port != nullptr &&
                port->direction == Core::PortDirection::Output) {
                ++count;
            }
        }
        return count;
    }
    case NodeIconRole: {
        const auto* descriptor = m_registry.find(node->type);
        return descriptor != nullptr && !descriptor->iconGlyph.empty()
                   ? QString::fromStdString(descriptor->iconGlyph)
                   : fallbackIconGlyph();
    }
    case NodeIconSourceRole: {
        const auto* descriptor = m_registry.find(node->type);
        return descriptor != nullptr && !descriptor->iconSource.empty()
                   ? QString::fromStdString(descriptor->iconSource)
                   : fallbackIconSource();
    }
    case NodeAccentRole: {
        const auto* descriptor = m_registry.find(node->type);
        return descriptor != nullptr && !descriptor->accent.empty()
                   ? QString::fromStdString(descriptor->accent)
                   : fallbackAccent();
    }
    case NodeEnabledRole: {
        const auto* enabled = m_document.property(node->id, "enabled");
        return enabled != nullptr && std::holds_alternative<bool>(*enabled)
                   ? QVariant(std::get<bool>(*enabled))
                   : QVariant(true);
    }
    case NodeLabelRole: {
        const auto* label = m_document.property(node->id, "label");
        if (label != nullptr && std::holds_alternative<std::string>(*label)) {
            return QString::fromStdString(std::get<std::string>(*label));
        }
        return QString::fromStdString(node->name);
    }
    case NodeWidthRole:
        return nodeNumberProperty(
            *node, "width",
            m_registry.find(node->type) != nullptr
                ? m_registry.find(node->type)->defaultWidth
                : kDemoNodeWidth);
    case NodeHeightRole:
        return nodeNumberProperty(
            *node, "height",
            m_registry.find(node->type) != nullptr
                ? m_registry.find(node->type)->defaultHeight
                : kDemoNodeHeight);
    case NodeColorRole:
        return nodeStringProperty(*node, "color", QStringLiteral("#2a333d"));
    case NodeIsBackdropRole: {
        const auto* descriptor = m_registry.find(node->type);
        return descriptor != nullptr &&
               descriptor->visualRole ==
                   QNodeGraph::Graph::NodeVisualRole::Backdrop;
    }
    case NodeIsGroupRole: {
        const auto* descriptor = m_registry.find(node->type);
        return descriptor != nullptr &&
               descriptor->visualRole ==
                   QNodeGraph::Graph::NodeVisualRole::Group;
    }
    case NodeGroupIdRole: {
        const auto iterator = node->properties.find("groupId");
        if (iterator != node->properties.end() &&
            std::holds_alternative<std::int64_t>(iterator->second)) {
            const auto groupId = static_cast<Core::NodeId>(
                std::get<std::int64_t>(iterator->second));
            return rowFor(groupId);
        }
        return -1;
    }
    case NodeSelectedRole:
        return std::find(m_selectedRows.cbegin(), m_selectedRows.cend(),
                         index.row()) != m_selectedRows.cend();
    case NodeInputPortsRole:
        return nodePorts(*node, m_document, Core::PortDirection::Input);
    case NodeOutputPortsRole:
        return nodePorts(*node, m_document, Core::PortDirection::Output);
    case NodeFixedRole: {
        const auto iterator = node->properties.find("fixed");
        return iterator != node->properties.end() &&
                       std::holds_alternative<bool>(iterator->second)
                   ? QVariant(std::get<bool>(iterator->second))
                   : QVariant(false);
    }
    case NodeContentUrlRole: {
        if (const auto* descriptor = m_registry.find(node->type);
            descriptor != nullptr && !descriptor->qmlContentUrl.empty()) {
            return QString::fromStdString(descriptor->qmlContentUrl);
        }
        return QString{};
    }
    case NodeOnNodePropertiesRole: {
        QVariantList values;
        const auto* descriptor = m_registry.find(node->type);
        for (const auto& [name, value] : node->properties) {
            if (name == "enabled") {
                continue;
            }
            if (name.size() >= 8 &&
                name.compare(name.size() - 8, 8, ".display") == 0) {
                continue;
            }
            const auto display = nodeStringProperty(
                *node, (name + ".display").c_str(), QStringLiteral("panel"));
            if (display != QStringLiteral("node") &&
                display != QStringLiteral("both")) {
                continue;
            }
            // 带上 valueType 与控件元数据，QML 才能按描述符选择控件（滑块 /
            // 勾选框 / 输入框），而不是把非 bool 一律当文本处理。
            QVariantMap entry{
                {QStringLiteral("name"), QString::fromStdString(name)},
                {QStringLiteral("value"), propertyVariant(value)},
                {QStringLiteral("valueType"), propertyType(value)}};
            appendPropertyEditorMetadata(entry, descriptor, name);
            values.push_back(std::move(entry));
        }
        return values;
    }
    case NodeOutputUsedRole:
        return m_outputUsed.contains(node->id);
    case NodeEffectiveRole:
        // 输出无人使用 → 该节点对最终结果无贡献，UI 降级显示。
        return m_outputUsed.contains(node->id);
    case NodeInputConnectedRole:
        return !m_outputUsed.contains(node->id) ||
               nodePortConnected(node->id, Core::PortDirection::Input);
    case NodeOutputConnectedRole:
        return !m_outputUsed.contains(node->id) ||
               nodePortConnected(node->id, Core::PortDirection::Output);
    case NodePreviewSourceRole:
    case NodePreviewWidthRole:
    case NodePreviewHeightRole:
    case NodePreviewChannelsRole: {
        const auto preview = m_nodePreviews.find(node->id);
        if (preview == m_nodePreviews.end()) {
            return role == NodePreviewSourceRole ? QVariant(QString{})
                                                 : QVariant(0);
        }
        if (role == NodePreviewSourceRole) {
            return preview->second.source;
        }
        if (role == NodePreviewWidthRole) {
            return preview->second.width;
        }
        if (role == NodePreviewHeightRole) {
            return preview->second.height;
        }
        return preview->second.channels;
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
        {NodeIconRole, "nodeIcon"},
        {NodeIconSourceRole, "nodeIconSource"},
        {NodeAccentRole, "nodeAccent"},
        {NodeEnabledRole, "nodeEnabled"},
        {NodeLabelRole, "nodeLabel"},
        {NodeWidthRole, "nodeWidth"},
        {NodeHeightRole, "nodeHeight"},
        {NodeColorRole, "nodeColor"},
        {NodeIsBackdropRole, "nodeIsBackdrop"},
        {NodeIsGroupRole, "nodeIsGroup"},
        {NodeGroupIdRole, "nodeGroupId"},
        {NodeSelectedRole, "nodeSelected"},
        {NodePreviewSourceRole, "nodePreviewSource"},
        {NodePreviewWidthRole, "nodePreviewWidth"},
        {NodePreviewHeightRole, "nodePreviewHeight"},
        {NodePreviewChannelsRole, "nodePreviewChannels"},
        {NodeInputPortsRole, "nodeInputPorts"},
        {NodeOutputPortsRole, "nodeOutputPorts"},
        {NodeFixedRole, "nodeFixed"},
        {NodeContentUrlRole, "nodeContentUrl"},
        {NodeOnNodePropertiesRole, "nodeOnNodeProperties"},
        {NodeOutputUsedRole, "nodeOutputUsed"},
        {NodeEffectiveRole, "nodeEffective"},
        {NodeInputConnectedRole, "nodeInputConnected"},
        {NodeOutputConnectedRole, "nodeOutputConnected"},
    };
}

int GraphController::sliceConnections(double startX, double startY, double endX,
                                      double endY) {
    const Core::Point sliceStart{startX, startY};
    const Core::Point sliceEnd{endX, endY};
    std::vector<std::pair<Core::PortId, Core::PortId>> matches;
    for (const auto& connection : m_document.connections()) {
        const auto* outputPort = m_document.port(connection.outputPort);
        const auto* inputPort = m_document.port(connection.inputPort);
        if (outputPort == nullptr || inputPort == nullptr) {
            continue;
        }
        const auto* outputNode = m_document.node(outputPort->nodeId);
        const auto* inputNode = m_document.node(inputPort->nodeId);
        if (outputNode == nullptr || inputNode == nullptr) {
            continue;
        }
        const Core::Point curveStart = outputPortPoint(*outputNode);
        const Core::Point curveEnd = inputPortPoint(*inputNode);
        const auto distance =
            std::max(40.0, std::abs(curveEnd.x - curveStart.x) * 0.5);
        const Core::Point controlStart{curveStart.x + distance, curveStart.y};
        const Core::Point controlEnd{curveEnd.x - distance, curveEnd.y};
        if (cubicIntersects(curveStart, controlStart, controlEnd, curveEnd,
                            sliceStart, sliceEnd)) {
            matches.emplace_back(connection.outputPort, connection.inputPort);
        }
    }

    int removed = 0;
    for (const auto [outputPort, inputPort] : matches) {
        const auto result = m_commandStack.execute(
            std::make_unique<Core::DisconnectPortsCommand>(outputPort,
                                                           inputPort),
            m_document);
        if (result) {
            ++removed;
        }
    }
    if (removed > 0) {
        notifyTopologyChanged();
        emit historyChanged();
    }
    return removed;
}

QVariantMap GraphController::portAt(double worldX, double worldY) const {
    const auto hitRadiusSquared = kPortHitRadius * kPortHitRadius;
    for (std::size_t row = 0; row < m_nodeOrder.size(); ++row) {
        const auto* node = m_document.node(m_nodeOrder[row]);
        if (node == nullptr) {
            continue;
        }
        const auto centerY = inputPortPoint(*node).y;
        const auto matches = [worldX, worldY, centerY,
                              hitRadiusSquared](double x) {
            const auto dx = worldX - x;
            const auto dy = worldY - centerY;
            return dx * dx + dy * dy <= hitRadiusSquared;
        };
        if (portFor(node->id, Core::PortDirection::Input) != 0 &&
            matches(node->position.x)) {
            return {{QStringLiteral("row"), static_cast<int>(row)},
                    {QStringLiteral("direction"), QStringLiteral("input")},
                    {QStringLiteral("x"), node->position.x},
                    {QStringLiteral("y"), centerY}};
        }
        if (portFor(node->id, Core::PortDirection::Output) != 0 &&
            matches(outputPortPoint(*node).x)) {
            return {{QStringLiteral("row"), static_cast<int>(row)},
                    {QStringLiteral("direction"), QStringLiteral("output")},
                    {QStringLiteral("x"), outputPortPoint(*node).x},
                    {QStringLiteral("y"), centerY}};
        }
    }
    return {};
}

QVariantMap GraphController::connectionAt(double worldX, double worldY) const {
    constexpr int kSamples = 32;
    constexpr double kHitDistance = 10.0;
    const Core::Point point{worldX, worldY};
    double closest = std::numeric_limits<double>::max();
    QVariantMap result;
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
        const auto start = outputPortPoint(*outputNode);
        const auto end = inputPortPoint(*inputNode);
        const auto distance = std::max(40.0, std::abs(end.x - start.x) * 0.5);
        const Core::Point controlStart{start.x + distance, start.y};
        const Core::Point controlEnd{end.x - distance, end.y};
        Core::Point previous = start;
        for (int sample = 1; sample <= kSamples; ++sample) {
            const double t = static_cast<double>(sample) / kSamples;
            const double inverse = 1.0 - t;
            const Core::Point current{
                inverse * inverse * inverse * start.x +
                    3.0 * inverse * inverse * t * controlStart.x +
                    3.0 * inverse * t * t * controlEnd.x + t * t * t * end.x,
                inverse * inverse * inverse * start.y +
                    3.0 * inverse * inverse * t * controlStart.y +
                    3.0 * inverse * t * t * controlEnd.y + t * t * t * end.y};
            const auto segmentX = current.x - previous.x;
            const auto segmentY = current.y - previous.y;
            const auto lengthSquared =
                segmentX * segmentX + segmentY * segmentY;
            const auto projection =
                lengthSquared > 0.0
                    ? std::clamp(((point.x - previous.x) * segmentX +
                                  (point.y - previous.y) * segmentY) /
                                     lengthSquared,
                                 0.0, 1.0)
                    : 0.0;
            const auto nearestX = previous.x + projection * segmentX;
            const auto nearestY = previous.y + projection * segmentY;
            const auto dx = point.x - nearestX;
            const auto dy = point.y - nearestY;
            const auto currentDistance = std::sqrt(dx * dx + dy * dy);
            if (currentDistance < closest) {
                closest = currentDistance;
                result = {{QStringLiteral("outputRow"), outputRow},
                          {QStringLiteral("inputRow"), inputRow}};
            }
            previous = current;
        }
    }
    return closest <= kHitDistance ? result : QVariantMap{};
}

bool GraphController::selectConnectionAt(double worldX, double worldY) {
    const auto hit = connectionAt(worldX, worldY);
    const auto outputRow = hit.value(QStringLiteral("outputRow"), -1).toInt();
    const auto inputRow = hit.value(QStringLiteral("inputRow"), -1).toInt();
    if (outputRow < 0 || inputRow < 0) {
        if (m_selectedConnectionOutputRow >= 0 ||
            m_selectedConnectionInputRow >= 0) {
            m_selectedConnectionOutputRow = -1;
            m_selectedConnectionInputRow = -1;
            emit connectionsChanged();
            emit selectionChanged();
        }
        return false;
    }
    if (m_selectedConnectionOutputRow == outputRow &&
        m_selectedConnectionInputRow == inputRow) {
        return true;
    }
    m_selectedConnectionOutputRow = outputRow;
    m_selectedConnectionInputRow = inputRow;
    emit connectionsChanged();
    emit selectionChanged();
    return true;
}

bool GraphController::removeSelectedConnection() {
    if (m_selectedConnectionOutputRow < 0 || m_selectedConnectionInputRow < 0) {
        return false;
    }
    const auto removed = removeConnectionBetween(m_selectedConnectionOutputRow,
                                                 m_selectedConnectionInputRow);
    if (removed) {
        m_selectedConnectionOutputRow = -1;
        m_selectedConnectionInputRow = -1;
        emit selectionChanged();
    }
    return removed;
}

bool GraphController::beginConnection(int outputRow) {
    if (outputRow < 0 || outputRow >= nodeCount()) {
        return false;
    }
    const auto nodeId = m_nodeOrder[static_cast<std::size_t>(outputRow)];
    if (portFor(nodeId, Core::PortDirection::Output) == 0) {
        return false;
    }
    const auto* node = m_document.node(nodeId);
    if (node == nullptr) {
        return false;
    }
    m_previewPoint = outputPortPoint(*node);
    m_pendingOutputRow = outputRow;
    emit connectionPendingChanged();
    emit connectionPreviewChanged();
    return true;
}

void GraphController::updateConnectionPreview(double worldX, double worldY) {
    if (!connectionPending()) {
        return;
    }
    m_previewPoint = {worldX, worldY};
    emit connectionPreviewChanged();
}

bool GraphController::completeConnectionAt(double worldX, double worldY) {
    if (!connectionPending()) {
        return false;
    }
    const auto target = portAt(worldX, worldY);
    if (target.isEmpty() ||
        target.value(QStringLiteral("direction")).toString() !=
            QStringLiteral("input")) {
        return false;
    }
    const auto inputRow = target.value(QStringLiteral("row")).toInt();
    if (!connectRows(m_pendingOutputRow, inputRow)) {
        return false;
    }
    cancelConnection();
    return true;
}

bool GraphController::removeConnectionBetween(int outputRow, int inputRow) {
    if (outputRow < 0 || inputRow < 0 || outputRow >= nodeCount() ||
        inputRow >= nodeCount()) {
        return false;
    }
    const auto outputPort =
        portFor(m_nodeOrder[static_cast<std::size_t>(outputRow)],
                Core::PortDirection::Output);
    const auto inputPort =
        portFor(m_nodeOrder[static_cast<std::size_t>(inputRow)],
                Core::PortDirection::Input);
    if (outputPort == 0 || inputPort == 0) {
        return false;
    }
    if (const auto result = m_commandStack.execute(
            std::make_unique<Core::DisconnectPortsCommand>(outputPort,
                                                           inputPort),
            m_document);
        !result) {
        return false;
    }
    notifyTopologyChanged();
    emit historyChanged();
    return true;
}

bool GraphController::isNodeEffective(Core::NodeId nodeId) const {
    return m_outputUsed.contains(nodeId);
}

void GraphController::cancelConnection() {
    if (!connectionPending()) {
        return;
    }
    m_pendingOutputRow = -1;
    emit connectionPendingChanged();
    emit connectionPreviewChanged();
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

void GraphController::syncModelOrder() {
    const auto current = m_document.nodeIds();
    if (current == m_nodeOrder) {
        return;
    }
    beginResetModel();
    m_nodeOrder = current;
    endResetModel();
    std::vector<int> validRows;
    for (const auto row : m_selectedRows) {
        if (row >= 0 && row < nodeCount()) {
            validRows.push_back(row);
        }
    }
    setSelection(std::move(validRows));
}

void GraphController::setSelection(std::vector<int> rows) {
    std::sort(rows.begin(), rows.end());
    rows.erase(std::unique(rows.begin(), rows.end()), rows.end());
    rows.erase(std::remove_if(
                   rows.begin(), rows.end(),
                   [this](int row) { return row < 0 || row >= nodeCount(); }),
               rows.end());
    const auto newPrimary = rows.empty() ? -1 : rows.back();
    if (rows == m_selectedRows && newPrimary == m_selectedRow) {
        return;
    }
    m_selectedRows = std::move(rows);
    m_selectedRow = newPrimary;
    if (nodeCount() > 0) {
        emit dataChanged(index(0, 0), index(nodeCount() - 1, 0),
                         {NodeSelectedRole});
    }
    emit selectedRowChanged();
    emit selectionChanged();
    emit propertiesChanged();
}

void GraphController::recomputeOutputUsage() {
    // 汇点 = 没有输出端口的节点（图像预览 / 保存 / 视口输出）。它们的输入
    // 被消费，因此其上游「输出被使用」。沿连接反向做一次可达性遍历即可。
    m_outputUsed.clear();
    std::queue<Core::NodeId> pending;
    for (const auto nodeId : m_document.nodeIds()) {
        const auto* node = m_document.node(nodeId);
        if (node == nullptr) {
            continue;
        }
        if (portFor(nodeId, Core::PortDirection::Output) == 0) {
            if (m_outputUsed.insert(nodeId).second) {
                pending.push(nodeId);
            }
        }
    }
    // 反向遍历：输入端口所在节点 ← 上游输出端口所在节点。
    while (!pending.empty()) {
        const auto nodeId = pending.front();
        pending.pop();
        const auto* node = m_document.node(nodeId);
        if (node == nullptr) {
            continue;
        }
        for (const auto portId : node->ports) {
            const auto* port = m_document.port(portId);
            if (port == nullptr ||
                port->direction != Core::PortDirection::Input) {
                continue;
            }
            for (const auto& connection : m_document.connections()) {
                if (connection.inputPort != portId) {
                    continue;
                }
                const auto* upstreamPort =
                    m_document.port(connection.outputPort);
                if (upstreamPort == nullptr) {
                    continue;
                }
                if (m_outputUsed.insert(upstreamPort->nodeId).second) {
                    pending.push(upstreamPort->nodeId);
                }
            }
        }
    }
}

void GraphController::notifyTopologyChanged() {
    recomputeOutputUsage();
    emit connectionsChanged();
    if (!m_nodeOrder.empty()) {
        emit dataChanged(index(0, 0), index(nodeCount() - 1, 0),
                         {NodeOutputUsedRole, NodeEffectiveRole,
                          NodeInputConnectedRole, NodeOutputConnectedRole});
    }
}

bool GraphController::nodePortConnected(
    const Core::NodeId nodeId, const Core::PortDirection direction) const {
    const auto* node = m_document.node(nodeId);
    if (node == nullptr) {
        return true;
    }
    for (const auto portId : node->ports) {
        const auto* port = m_document.port(portId);
        if (port == nullptr || port->direction != direction) {
            continue;
        }
        // A descriptor may explicitly opt into optional inputs (for example a
        // two-source compositor that also supports a single image). Such ports
        // remain valid when disconnected and must not produce a false alarm.
        if (direction == Core::PortDirection::Input) {
            const auto* optional =
                m_document.property(nodeId, "optionalInputs");
            if (optional != nullptr &&
                std::holds_alternative<bool>(*optional) &&
                std::get<bool>(*optional)) {
                continue;
            }
        }
        const auto connected = std::any_of(
            m_document.connections().cbegin(), m_document.connections().cend(),
            [portId](const Core::Connection& connection) {
                return connection.inputPort == portId ||
                       connection.outputPort == portId;
            });
        if (!connected) {
            return false;
        }
    }
    return true;
}

void GraphController::setError(QString message) {
    if (m_lastError == message) {
        return;
    }
    m_lastError = std::move(message);
    emit errorChanged();
}

int GraphController::rowFor(Core::NodeId nodeId) const {
    for (std::size_t row = 0; row < m_nodeOrder.size(); ++row) {
        if (m_nodeOrder[row] == nodeId) {
            return static_cast<int>(row);
        }
    }
    return -1;
}

bool GraphController::connectRows(int outputRow, int inputRow) {
    if (outputRow < 0 || inputRow < 0 || outputRow >= nodeCount() ||
        inputRow >= nodeCount()) {
        return false;
    }
    const auto outputNode = m_nodeOrder[static_cast<std::size_t>(outputRow)];
    const auto inputNode = m_nodeOrder[static_cast<std::size_t>(inputRow)];
    const auto outputPort = portFor(outputNode, Core::PortDirection::Output);
    // For multi-input nodes, use the first free input port so repeated drags to
    // the same node populate imageA, then imageB (the common compositor flow).
    auto inputPort = Core::PortId{0};
    const auto* inputValue = m_document.node(inputNode);
    if (inputValue != nullptr) {
        for (const auto candidate : inputValue->ports) {
            const auto* port = m_document.port(candidate);
            if (port == nullptr ||
                port->direction != Core::PortDirection::Input) {
                continue;
            }
            const auto connected =
                std::any_of(m_document.connections().cbegin(),
                            m_document.connections().cend(),
                            [candidate](const Core::Connection& connection) {
                                return connection.inputPort == candidate;
                            });
            if (!connected) {
                inputPort = candidate;
                break;
            }
            if (inputPort == 0) {
                inputPort = candidate;
            }
        }
    }
    if (outputPort == 0 || inputPort == 0) {
        setError(
            QStringLiteral("Both an output and an input port are required."));
        return false;
    }
    const auto* input = m_document.port(inputPort);
    if (input != nullptr && !input->acceptsMultipleConnections) {
        const auto existing = std::find_if(
            m_document.connections().begin(), m_document.connections().end(),
            [inputPort](const Core::Connection& connection) {
                return connection.inputPort == inputPort;
            });
        if (existing != m_document.connections().end()) {
            const auto previousOutput = existing->outputPort;
            int previousRow = -1;
            if (const auto* previousPort = m_document.port(previousOutput);
                previousPort != nullptr) {
                previousRow = rowFor(previousPort->nodeId);
            }
            if (const auto result = m_commandStack.execute(
                    std::make_unique<Core::DisconnectPortsCommand>(
                        previousOutput, inputPort),
                    m_document);
                !result) {
                setError(QString::fromStdString(result.error().message));
                return false;
            }
            emit nodeDisconnected(previousRow, inputRow);
        }
    }
    const auto result = m_commandStack.execute(
        std::make_unique<Core::ConnectPortsCommand>(outputPort, inputPort),
        m_document);
    if (!result) {
        setError(QString::fromStdString(result.error().message));
        return false;
    }
    setError({});
    emit nodeConnected(outputRow, inputRow);
    notifyTopologyChanged();
    emit historyChanged();
    return true;
}

bool GraphController::isFixedNode(Core::NodeId nodeId) const {
    const auto* value = m_document.property(nodeId, "fixed");
    return value != nullptr && std::holds_alternative<bool>(*value) &&
           std::get<bool>(*value);
}

} // namespace QNodeGraph::UI
