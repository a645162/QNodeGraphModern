#include <QNodeGraph/Lib/UI/QtQuick/graph_controller.h>

#include <QFileInfo>

#include <QHash>
#include <QMetaType>
#include <QString>
#include <QVariantMap>

#include <algorithm>
#include <cmath>
#include <queue>
#include <string>
#include <cstdint>
#include <memory>
#include <type_traits>
#include <utility>
#include <variant>

namespace QNodeGraph::UI {

namespace {

constexpr double kDemoNodeWidth = 180.0;
constexpr double kDemoNodeHeight = 108.0;
constexpr double kPortHitRadius = 12.0;

QVariant propertyVariant(const Core::PropertyValue& value) {
    return std::visit(
        [](const auto& typed) -> QVariant {
            using Value = std::decay_t<decltype(typed)>;
            if constexpr (std::is_same_v<Value, std::int64_t>) {
                return QVariant::fromValue<qlonglong>(
                    static_cast<qlonglong>(typed));
            } else if constexpr (std::is_same_v<Value, std::string>) {
                return QString::fromStdString(typed);
            } else {
                return QVariant::fromValue(typed);
            }
        },
        value);
}

QString propertyType(const Core::PropertyValue& value) {
    return std::visit(
        [](const auto& typed) {
            using Value = std::decay_t<decltype(typed)>;
            if constexpr (std::is_same_v<Value, bool>) {
                return QStringLiteral("bool");
            } else if constexpr (std::is_same_v<Value, double>) {
                return QStringLiteral("double");
            } else if constexpr (std::is_same_v<Value, std::int64_t>) {
                return QStringLiteral("int64");
            } else {
                return QStringLiteral("string");
            }
        },
        value);
}

QString nodeIcon(const std::string& type) {
    if (type == "load_image") {
        return QStringLiteral("IMG");
    }
    if (type == "grayscale") {
        return QStringLiteral("FX");
    }
    if (type == "blur") {
        return QStringLiteral("BLR");
    }
    if (type == "edge_detect") {
        return QStringLiteral("EDG");
    }
    if (type == "image_preview") {
        return QStringLiteral("VIEW");
    }
    if (type == "save_image") {
        return QStringLiteral("SAVE");
    }
    if (type == "group") {
        return QStringLiteral("GRP");
    }
    if (type == "backdrop") {
        return QStringLiteral("BG");
    }
    return QStringLiteral("NODE");
}

QString nodeIconSource(const std::string& type) {
    const auto iconType = type == "load_image" || type == "grayscale" ||
                                  type == "blur" || type == "edge_detect" ||
                                  type == "image_preview" || type == "save_image" ||
                                  type == "group" || type == "backdrop"
                              ? type
                              : std::string{"node"};
    return QStringLiteral("qrc:/qt/qml/QNodeGraph/UI/qml/icons/%1.svg")
        .arg(QString::fromStdString(iconType));
}

QString nodeAccent(const std::string& type) {
    if (type == "load_image") {
        return QStringLiteral("#4f8fc6");
    }
    if (type == "grayscale") {
        return QStringLiteral("#6f7bd8");
    }
    if (type == "blur") {
        return QStringLiteral("#9568c7");
    }
    if (type == "edge_detect") {
        return QStringLiteral("#b26d5c");
    }
    if (type == "image_preview") {
        return QStringLiteral("#4aaf85");
    }
    if (type == "save_image") {
        return QStringLiteral("#71859a");
    }
    if (type == "group") {
        return QStringLiteral("#4a718c");
    }
    if (type == "backdrop") {
        return QStringLiteral("#557081");
    }
    return QStringLiteral("#426b91");
}

double nodeNumberProperty(const Core::Node& node, const char* name,
                          double fallback) {
    const auto iterator = node.properties.find(name);
    if (iterator == node.properties.end()) {
        return fallback;
    }
    if (std::holds_alternative<double>(iterator->second)) {
        return std::get<double>(iterator->second);
    }
    if (std::holds_alternative<std::int64_t>(iterator->second)) {
        return static_cast<double>(std::get<std::int64_t>(iterator->second));
    }
    return fallback;
}

QString nodeStringProperty(const Core::Node& node, const char* name,
                           QString fallback) {
    const auto iterator = node.properties.find(name);
    if (iterator != node.properties.end() &&
        std::holds_alternative<std::string>(iterator->second)) {
        return QString::fromStdString(std::get<std::string>(iterator->second));
    }
    return fallback;
}

double cross(const Core::Point& first, const Core::Point& second,
             const Core::Point& third) {
    return (second.x - first.x) * (third.y - first.y) -
           (second.y - first.y) * (third.x - first.x);
}

bool onSegment(const Core::Point& first, const Core::Point& second,
               const Core::Point& point) {
    constexpr double epsilon = 1e-6;
    return point.x >= std::min(first.x, second.x) - epsilon &&
           point.x <= std::max(first.x, second.x) + epsilon &&
           point.y >= std::min(first.y, second.y) - epsilon &&
           point.y <= std::max(first.y, second.y) + epsilon;
}

bool segmentsIntersect(const Core::Point& first, const Core::Point& second,
                       const Core::Point& third, const Core::Point& fourth) {
    constexpr double epsilon = 1e-6;
    const auto firstCross = cross(first, second, third);
    const auto secondCross = cross(first, second, fourth);
    const auto thirdCross = cross(third, fourth, first);
    const auto fourthCross = cross(third, fourth, second);
    if (((firstCross > epsilon && secondCross < -epsilon) ||
         (firstCross < -epsilon && secondCross > epsilon)) &&
        ((thirdCross > epsilon && fourthCross < -epsilon) ||
         (thirdCross < -epsilon && fourthCross > epsilon))) {
        return true;
    }
    return (std::abs(firstCross) <= epsilon && onSegment(first, second, third)) ||
           (std::abs(secondCross) <= epsilon && onSegment(first, second, fourth)) ||
           (std::abs(thirdCross) <= epsilon && onSegment(third, fourth, first)) ||
           (std::abs(fourthCross) <= epsilon && onSegment(third, fourth, second));
}

bool cubicIntersects(const Core::Point& start, const Core::Point& controlStart,
                     const Core::Point& controlEnd, const Core::Point& end,
                     const Core::Point& sliceStart,
                     const Core::Point& sliceEnd) {
    constexpr int samples = 24;
    auto previous = start;
    for (int index = 1; index <= samples; ++index) {
        const auto t = static_cast<double>(index) / samples;
        const auto inverse = 1.0 - t;
        const auto point = Core::Point{
            inverse * inverse * inverse * start.x +
                3.0 * inverse * inverse * t * controlStart.x +
                3.0 * inverse * t * t * controlEnd.x + t * t * t * end.x,
            inverse * inverse * inverse * start.y +
                3.0 * inverse * inverse * t * controlStart.y +
                3.0 * inverse * t * t * controlEnd.y + t * t * t * end.y};
        if (segmentsIntersect(previous, point, sliceStart, sliceEnd)) {
            return true;
        }
        previous = point;
    }
    return false;
}

} // namespace

GraphController::GraphController(QObject* parent)
    : QAbstractListModel(parent),
      m_document("QNodeGraphModern"),
      m_registry(QNodeGraph::Graph::NodeRegistry::withBuiltins()) {}

int GraphController::nodeCount() const noexcept {
    return static_cast<int>(m_nodeOrder.size());
}

bool GraphController::connectionPending() const noexcept {
    return m_pendingOutputRow >= 0;
}

QVariantMap GraphController::connectionPreview() const {
    if (!connectionPending() || m_pendingOutputRow >= nodeCount()) {
        return {};
    }
    const auto nodeId = m_nodeOrder[static_cast<std::size_t>(m_pendingOutputRow)];
    const auto* node = m_document.node(nodeId);
    if (node == nullptr) {
        return {};
    }
    QVariantMap value;
    value.insert(QStringLiteral("outputRow"), m_pendingOutputRow);
    value.insert(QStringLiteral("outputX"), node->position.x);
    value.insert(QStringLiteral("outputY"),
                 node->position.y + kDemoNodeHeight / 2.0);
    value.insert(QStringLiteral("inputX"), m_previewPoint.x);
    value.insert(QStringLiteral("inputY"), m_previewPoint.y);
    value.insert(QStringLiteral("outputWidth"), kDemoNodeWidth);
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
    const auto* node = m_document.node(
        m_nodeOrder[static_cast<std::size_t>(m_selectedRow)]);
    if (node == nullptr) {
        return values;
    }
    for (const auto& [name, value] : node->properties) {
        values.push_back(QVariantMap{
            {QStringLiteral("name"), QString::fromStdString(name)},
            {QStringLiteral("value"), propertyVariant(value)},
            {QStringLiteral("valueType"), propertyType(value)}});
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
    case NodeIconRole:
        return nodeIcon(node->type);
    case NodeIconSourceRole:
        return nodeIconSource(node->type);
    case NodeAccentRole:
        return nodeAccent(node->type);
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
        return nodeNumberProperty(*node, "width", kDemoNodeWidth);
    case NodeHeightRole:
        return nodeNumberProperty(*node, "height", kDemoNodeHeight);
    case NodeColorRole:
        return nodeStringProperty(*node, "color", QStringLiteral("#2a333d"));
    case NodeIsBackdropRole:
        return node->type == "backdrop";
    case NodeIsGroupRole:
        return node->type == "group";
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
    };
}

void GraphController::addDemoNode(bool connectToPrevious) {
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
    m_document.setProperty(*node, "enabled", true);
    m_document.setProperty(*node, "label",
                           "Demo Node " + std::to_string(row + 1));
    beginInsertRows(QModelIndex(), row, row);
    m_nodeOrder.push_back(*node);
    endInsertRows();
    if (connectToPrevious && row > 0) {
        const auto previousNode = m_nodeOrder[static_cast<std::size_t>(row - 1)];
        const auto previousOutput = portFor(previousNode, Core::PortDirection::Output);
        if (previousOutput != 0) {
            m_document.connect(previousOutput, *input);
        }
    }
    emit nodeCountChanged();
    emit connectionsChanged();
}

bool GraphController::addNodeType(QString typeId, bool connectToPrevious) {
    const auto* descriptor = m_registry.find(typeId.toStdString());
    if (descriptor == nullptr) {
        return false;
    }
    const auto row = static_cast<int>(m_nodeOrder.size());
    const auto name = descriptor->displayName + " " + std::to_string(row + 1);
    const auto node = m_registry.createNode(
        m_document, descriptor->typeId, name,
        {80.0 + (row % 3) * 250.0, 90.0 + (row / 3) * 180.0});
    if (!node) {
        return false;
    }
    m_document.setProperty(*node, "enabled", true);
    m_document.setProperty(*node, "label", name);
    if (typeId == QStringLiteral("backdrop")) {
        m_document.setProperty(*node, "width", std::int64_t{560});
        m_document.setProperty(*node, "height", std::int64_t{300});
        m_document.setProperty(*node, "color", std::string{"#304554"});
    } else if (typeId == QStringLiteral("group")) {
        m_document.setProperty(*node, "width", std::int64_t{240});
        m_document.setProperty(*node, "height", std::int64_t{140});
        m_document.setProperty(*node, "color", std::string{"#384b5b"});
    }
    const auto input = portFor(*node, Core::PortDirection::Input);
    if (connectToPrevious && row > 0 && input != 0) {
        const auto previousNode = m_nodeOrder[static_cast<std::size_t>(row - 1)];
        const auto previousOutput = portFor(previousNode, Core::PortDirection::Output);
        if (previousOutput != 0) {
            static_cast<void>(m_document.connect(previousOutput, input));
        }
    }
    beginInsertRows(QModelIndex(), row, row);
    m_nodeOrder.push_back(*node);
    endInsertRows();
    emit nodeCountChanged();
    emit connectionsChanged();
    return true;
}

bool GraphController::deleteNode(int row) {
    if (row < 0 || row >= nodeCount()) {
        return false;
    }
    const auto nodeId = m_nodeOrder[static_cast<std::size_t>(row)];
    const auto result = m_commandStack.execute(
        std::make_unique<Core::DeleteNodeCommand>(nodeId), m_document);
    if (!result) {
        return false;
    }

    cancelConnection();
    const auto oldCount = nodeCount();
    beginResetModel();
    m_nodeOrder = m_document.nodeIds();
    endResetModel();
    setSelection({});
    if (nodeCount() != oldCount) {
        emit nodeCountChanged();
    }
    emit connectionsChanged();
    emit historyChanged();
    return true;
}

bool GraphController::saveGraph(QString filePath) {
    filePath = filePath.trimmed();
    if (filePath.isEmpty()) {
        setError(QStringLiteral("A graph file path is required."));
        return false;
    }
    const auto result = Graph::GraphJson::save(m_document, filePath);
    if (!result) {
        setError(QString::fromStdString(result.error().message));
        return false;
    }
    setError({});
    return true;
}

bool GraphController::loadGraph(QString filePath) {
    filePath = filePath.trimmed();
    if (filePath.isEmpty()) {
        setError(QStringLiteral("A graph file path is required."));
        return false;
    }
    const auto result = Graph::GraphJson::load(filePath);
    if (!result) {
        setError(QString::fromStdString(result.error().message));
        return false;
    }
    const auto assets = Graph::GraphJson::validateExternalAssets(
        *result, QFileInfo(filePath).absolutePath());
    if (!assets) {
        setError(QString::fromStdString(assets.error().message));
        return false;
    }

    const auto oldCount = nodeCount();
    cancelConnection();
    beginResetModel();
    m_document = std::move(*result);
    m_nodeOrder = m_document.nodeIds();
    m_nodePreviews.clear();
    m_commandStack.clear();
    endResetModel();
    setSelection({});
    if (nodeCount() != oldCount) {
        emit nodeCountChanged();
    }
    emit connectionsChanged();
    emit historyChanged();
    setError({});
    return true;
}

QVariantMap GraphController::graphBounds() const {
    if (m_nodeOrder.empty()) {
        return {};
    }
    auto first = m_document.node(m_nodeOrder.front());
    if (first == nullptr) {
        return {};
    }
    auto left = first->position.x;
    auto top = first->position.y;
    auto right = left + nodeNumberProperty(*first, "width", kDemoNodeWidth);
    auto bottom = top + nodeNumberProperty(*first, "height", kDemoNodeHeight);
    for (std::size_t row = 1; row < m_nodeOrder.size(); ++row) {
        const auto* node = m_document.node(m_nodeOrder[row]);
        if (node == nullptr) {
            continue;
        }
        left = std::min(left, node->position.x);
        top = std::min(top, node->position.y);
        right = std::max(right, node->position.x +
                                   nodeNumberProperty(*node, "width",
                                                      kDemoNodeWidth));
        bottom = std::max(bottom, node->position.y +
                                    nodeNumberProperty(*node, "height",
                                                       kDemoNodeHeight));
    }
    return {{QStringLiteral("x"), left},
            {QStringLiteral("y"), top},
            {QStringLiteral("width"), std::max(1.0, right - left)},
            {QStringLiteral("height"), std::max(1.0, bottom - top)}};
}

bool GraphController::setNodePreview(int row, QString source, int width,
                                     int height, int channels) {
    if (row < 0 || row >= nodeCount() || width < 0 || height < 0 ||
        channels < 0) {
        return false;
    }
    const auto nodeId = m_nodeOrder[static_cast<std::size_t>(row)];
    m_nodePreviews.insert_or_assign(
        nodeId, NodePreview{std::move(source), width, height, channels});
    const auto modelIndex = index(row, 0);
    emit dataChanged(modelIndex, modelIndex,
                     {NodePreviewSourceRole, NodePreviewWidthRole,
                      NodePreviewHeightRole, NodePreviewChannelsRole});
    return true;
}

bool GraphController::clearNodePreview(int row) {
    if (row < 0 || row >= nodeCount()) {
        return false;
    }
    const auto nodeId = m_nodeOrder[static_cast<std::size_t>(row)];
    if (m_nodePreviews.erase(nodeId) == 0) {
        return false;
    }
    const auto modelIndex = index(row, 0);
    emit dataChanged(modelIndex, modelIndex,
                     {NodePreviewSourceRole, NodePreviewWidthRole,
                      NodePreviewHeightRole, NodePreviewChannelsRole});
    return true;
}

void GraphController::clearGraph() {
    setSelection({});
    if (m_nodeOrder.empty()) {
        return;
    }
    cancelConnection();
    beginResetModel();
    m_nodeOrder.clear();
    m_document.clear();
    m_nodePreviews.clear();
    m_commandStack.clear();
    endResetModel();
    emit nodeCountChanged();
    emit connectionsChanged();
    emit historyChanged();
}

bool GraphController::moveNode(int row, double x, double y) {
    if (row < 0 || row >= static_cast<int>(m_nodeOrder.size())) {
        return false;
    }
    const auto nodeId = m_nodeOrder[static_cast<std::size_t>(row)];
    const auto result = m_commandStack.execute(
        std::make_unique<Core::MoveNodeCommand>(nodeId, Core::Point{x, y}),
        m_document);
    if (!result) {
        return false;
    }
    const auto modelIndex = index(row, 0);
    emit dataChanged(modelIndex, modelIndex, {NodeXRole, NodeYRole});
    emit connectionsChanged();
    if (connectionPending()) {
        emit connectionPreviewChanged();
    }
    emit historyChanged();
    return true;
}

bool GraphController::moveSelectedNodes(int anchorRow, double x, double y) {
    if (anchorRow < 0 || anchorRow >= nodeCount() ||
        std::find(m_selectedRows.cbegin(), m_selectedRows.cend(), anchorRow) ==
            m_selectedRows.cend()) {
        return moveNode(anchorRow, x, y);
    }
    const auto* anchor = m_document.node(
        m_nodeOrder[static_cast<std::size_t>(anchorRow)]);
    if (anchor == nullptr) {
        return false;
    }
    const auto dx = x - anchor->position.x;
    const auto dy = y - anchor->position.y;
    std::vector<std::pair<Core::NodeId, Core::Point>> targets;
    targets.reserve(m_selectedRows.size());
    for (const auto row : m_selectedRows) {
        const auto* node = m_document.node(
            m_nodeOrder[static_cast<std::size_t>(row)]);
        if (node == nullptr) {
            return false;
        }
        targets.emplace_back(node->id,
                             Core::Point{node->position.x + dx,
                                         node->position.y + dy});
    }
    const auto result = m_commandStack.execute(
        std::make_unique<Core::MoveNodesCommand>(std::move(targets)),
        m_document);
    if (!result) {
        return false;
    }
    for (const auto row : m_selectedRows) {
        const auto modelIndex = index(row, 0);
        emit dataChanged(modelIndex, modelIndex, {NodeXRole, NodeYRole});
    }
    emit connectionsChanged();
    emit historyChanged();
    return true;
}

bool GraphController::autoLayout() {
    const auto count = nodeCount();
    if (count == 0) {
        return false;
    }

    std::vector<std::vector<int>> successors(static_cast<std::size_t>(count));
    std::vector<int> indegree(static_cast<std::size_t>(count), 0);
    for (const auto& connection : m_document.connections()) {
        const auto* output = m_document.port(connection.outputPort);
        const auto* input = m_document.port(connection.inputPort);
        if (output == nullptr || input == nullptr) {
            continue;
        }
        const auto outputRow = rowFor(output->nodeId);
        const auto inputRow = rowFor(input->nodeId);
        if (outputRow < 0 || inputRow < 0 || outputRow == inputRow) {
            continue;
        }
        successors[static_cast<std::size_t>(outputRow)].push_back(inputRow);
        ++indegree[static_cast<std::size_t>(inputRow)];
    }

    std::vector<int> columns(static_cast<std::size_t>(count), 0);
    std::queue<int> ready;
    for (int row = 0; row < count; ++row) {
        if (indegree[static_cast<std::size_t>(row)] == 0) {
            ready.push(row);
        }
    }
    int visited = 0;
    while (!ready.empty()) {
        const auto row = ready.front();
        ready.pop();
        ++visited;
        for (const auto successor : successors[static_cast<std::size_t>(row)]) {
            auto& successorColumn = columns[static_cast<std::size_t>(successor)];
            successorColumn =
                std::max(successorColumn, columns[static_cast<std::size_t>(row)] + 1);
            if (--indegree[static_cast<std::size_t>(successor)] == 0) {
                ready.push(successor);
            }
        }
    }
    if (visited != count) {
        // Keep cyclic/disconnected leftovers visible in the first column.
        for (int row = 0; row < count; ++row) {
            if (indegree[static_cast<std::size_t>(row)] > 0) {
                columns[static_cast<std::size_t>(row)] = 0;
            }
        }
    }

    const auto maxColumn = *std::max_element(columns.begin(), columns.end());
    std::vector<int> rowsPerColumn(static_cast<std::size_t>(maxColumn + 1), 0);
    bool changed = false;
    for (int row = 0; row < count; ++row) {
        const auto column = columns[static_cast<std::size_t>(row)];
        const auto line = rowsPerColumn[static_cast<std::size_t>(column)]++;
        const Core::Point position{80.0 + column * 260.0,
                                   90.0 + line * 180.0};
        const auto* node = m_document.node(
            m_nodeOrder[static_cast<std::size_t>(row)]);
        if (node == nullptr ||
            (node->position.x == position.x && node->position.y == position.y)) {
            continue;
        }
        const auto result = m_commandStack.execute(
            std::make_unique<Core::MoveNodeCommand>(node->id, position),
            m_document);
        if (!result) {
            return false;
        }
        changed = true;
    }
    if (!changed) {
        return false;
    }
    emit dataChanged(index(0, 0), index(count - 1, 0),
                     {NodeXRole, NodeYRole});
    emit connectionsChanged();
    if (connectionPending()) {
        emit connectionPreviewChanged();
    }
    emit historyChanged();
    return true;
}

int GraphController::sliceConnections(double startX, double startY,
                                      double endX, double endY) {
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
        const Core::Point curveStart{
            outputNode->position.x + kDemoNodeWidth,
            outputNode->position.y + kDemoNodeHeight / 2.0};
        const Core::Point curveEnd{inputNode->position.x,
                                   inputNode->position.y + kDemoNodeHeight / 2.0};
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
        emit connectionsChanged();
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
        const auto centerY = node->position.y + kDemoNodeHeight / 2.0;
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
            matches(node->position.x + kDemoNodeWidth)) {
            return {{QStringLiteral("row"), static_cast<int>(row)},
                    {QStringLiteral("direction"), QStringLiteral("output")},
                    {QStringLiteral("x"), node->position.x + kDemoNodeWidth},
                    {QStringLiteral("y"), centerY}};
        }
    }
    return {};
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
    m_previewPoint = {node->position.x + kDemoNodeWidth,
                      node->position.y + kDemoNodeHeight / 2.0};
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

void GraphController::cancelConnection() {
    if (!connectionPending()) {
        return;
    }
    m_pendingOutputRow = -1;
    emit connectionPendingChanged();
    emit connectionPreviewChanged();
}

bool GraphController::selectNode(int row) {
    if (row < -1 || row >= nodeCount()) {
        return false;
    }
    if (row == -1) {
        setSelection({});
    } else {
        setSelection({row});
    }
    return true;
}

bool GraphController::toggleNodeSelection(int row) {
    if (row < 0 || row >= nodeCount()) {
        return false;
    }
    auto rows = m_selectedRows;
    const auto found = std::find(rows.begin(), rows.end(), row);
    if (found == rows.end()) {
        rows.push_back(row);
    } else {
        rows.erase(found);
    }
    setSelection(std::move(rows));
    return true;
}

bool GraphController::selectNodesInRect(double startX, double startY,
                                        double endX, double endY,
                                        bool additive) {
    const auto left = std::min(startX, endX);
    const auto top = std::min(startY, endY);
    const auto right = std::max(startX, endX);
    const auto bottom = std::max(startY, endY);
    std::vector<int> rows = additive ? m_selectedRows : std::vector<int>{};
    for (int row = 0; row < nodeCount(); ++row) {
        const auto* node = m_document.node(
            m_nodeOrder[static_cast<std::size_t>(row)]);
        if (node == nullptr) {
            continue;
        }
        const auto width = data(index(row, 0), NodeWidthRole).toDouble();
        const auto height = data(index(row, 0), NodeHeightRole).toDouble();
        const auto intersects = node->position.x <= right &&
                                node->position.x + width >= left &&
                                node->position.y <= bottom &&
                                node->position.y + height >= top;
        if (intersects && std::find(rows.cbegin(), rows.cend(), row) ==
                              rows.cend()) {
            rows.push_back(row);
        }
    }
    setSelection(std::move(rows));
    return true;
}

bool GraphController::setNodeProperty(int row, QString name, QVariant value) {
    if (row < 0 || row >= nodeCount() || name.isEmpty()) {
        return false;
    }
    const auto nodeId = m_nodeOrder[static_cast<std::size_t>(row)];
    const auto* current = m_document.property(nodeId, name.toStdString());
    if (current == nullptr) {
        return false;
    }

    Core::PropertyValue converted;
    if (std::holds_alternative<bool>(*current)) {
        if (value.metaType() == QMetaType::fromType<bool>()) {
            converted = value.toBool();
        } else {
            const auto text = value.toString().trimmed().toLower();
            if (text == QStringLiteral("true") || text == QStringLiteral("1")) {
                converted = true;
            } else if (text == QStringLiteral("false") ||
                       text == QStringLiteral("0")) {
                converted = false;
            } else {
                return false;
            }
        }
    } else if (std::holds_alternative<double>(*current)) {
        bool ok = false;
        const auto number = value.toDouble(&ok);
        if (!ok) {
            return false;
        }
        converted = number;
    } else if (std::holds_alternative<std::int64_t>(*current)) {
        bool ok = false;
        const auto number = value.toLongLong(&ok);
        if (!ok) {
            return false;
        }
        converted = static_cast<std::int64_t>(number);
    } else {
        converted = value.toString().toStdString();
    }

    const auto result = m_commandStack.execute(
        std::make_unique<Core::SetPropertyCommand>(
            nodeId, name.toStdString(), std::move(converted)),
        m_document);
    if (!result) {
        return false;
    }
    const auto modelIndex = index(row, 0);
    if (name == QStringLiteral("enabled")) {
        emit dataChanged(modelIndex, modelIndex, {NodeEnabledRole});
    } else if (name == QStringLiteral("label")) {
        emit dataChanged(modelIndex, modelIndex, {NodeLabelRole, NodeNameRole});
    }
    emit propertiesChanged();
    emit historyChanged();
    return true;
}

bool GraphController::assignNodeToGroup(int nodeRow, int groupRow) {
    if (nodeRow < 0 || groupRow < 0 || nodeRow >= nodeCount() ||
        groupRow >= nodeCount() || nodeRow == groupRow) {
        return false;
    }
    const auto group = m_document.node(
        m_nodeOrder[static_cast<std::size_t>(groupRow)]);
    const auto node = m_document.node(
        m_nodeOrder[static_cast<std::size_t>(nodeRow)]);
    if (group == nullptr || node == nullptr || group->type != "group") {
        return false;
    }
    const auto result = m_commandStack.execute(
        std::make_unique<Core::SetPropertyCommand>(
            node->id, "groupId", static_cast<std::int64_t>(group->id)),
        m_document);
    if (!result) {
        return false;
    }
    const auto modelIndex = index(nodeRow, 0);
    emit dataChanged(modelIndex, modelIndex, {NodeGroupIdRole});
    emit propertiesChanged();
    emit historyChanged();
    return true;
}

bool GraphController::clearNodeGroup(int nodeRow) {
    if (nodeRow < 0 || nodeRow >= nodeCount()) {
        return false;
    }
    const auto node = m_document.node(
        m_nodeOrder[static_cast<std::size_t>(nodeRow)]);
    if (node == nullptr || node->properties.find("groupId") == node->properties.end()) {
        return false;
    }
    const auto result = m_commandStack.execute(
        std::make_unique<Core::SetPropertyCommand>(
            node->id, "groupId", static_cast<std::int64_t>(-1)),
        m_document);
    if (!result) {
        return false;
    }
    const auto modelIndex = index(nodeRow, 0);
    emit dataChanged(modelIndex, modelIndex, {NodeGroupIdRole});
    emit propertiesChanged();
    emit historyChanged();
    return true;
}

bool GraphController::undo() {
    const auto result = m_commandStack.undo(m_document);
    if (!result) {
        return false;
    }
    const auto previousCount = nodeCount();
    syncModelOrder();
    if (nodeCount() != previousCount) {
        emit nodeCountChanged();
    }
    if (rowCount() > 0) {
        emit dataChanged(index(0, 0), index(rowCount() - 1, 0),
                         {NodeXRole, NodeYRole, NodeIconRole, NodeAccentRole,
                          NodeEnabledRole, NodeLabelRole, NodeWidthRole,
                          NodeHeightRole, NodeColorRole, NodeIsBackdropRole,
                          NodeIsGroupRole, NodeGroupIdRole});
    }
    emit connectionsChanged();
    emit propertiesChanged();
    emit historyChanged();
    return true;
}

bool GraphController::redo() {
    const auto result = m_commandStack.redo(m_document);
    if (!result) {
        return false;
    }
    const auto previousCount = nodeCount();
    syncModelOrder();
    if (nodeCount() != previousCount) {
        emit nodeCountChanged();
    }
    if (rowCount() > 0) {
        emit dataChanged(index(0, 0), index(rowCount() - 1, 0),
                         {NodeXRole, NodeYRole, NodeIconRole, NodeAccentRole,
                          NodeEnabledRole, NodeLabelRole, NodeWidthRole,
                          NodeHeightRole, NodeColorRole, NodeIsBackdropRole,
                          NodeIsGroupRole, NodeGroupIdRole});
    }
    emit connectionsChanged();
    emit propertiesChanged();
    emit historyChanged();
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
    rows.erase(std::remove_if(rows.begin(), rows.end(), [this](int row) {
                   return row < 0 || row >= nodeCount();
               }),
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
    const auto inputPort = portFor(inputNode, Core::PortDirection::Input);
    if (outputPort == 0 || inputPort == 0) {
        return false;
    }
    const auto result = m_commandStack.execute(
        std::make_unique<Core::ConnectPortsCommand>(outputPort, inputPort),
        m_document);
    if (!result) {
        return false;
    }
    emit connectionsChanged();
    emit historyChanged();
    return true;
}

} // namespace QNodeGraph::UI
