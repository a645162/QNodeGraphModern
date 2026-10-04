/*
 * File: graph_controller_edit.cpp
 * Brief: Node creation, document operations, movement, and layout for
 * GraphController. Author: Haomin Kong Email: konghm@beim.com.cn /
 * a645162@qq.com Created: 2026-10-04
 */
#include <QNodeGraph/Lib/UI/QtQuick/graph_controller.h>

#include <QFileInfo>

#include <QHash>
#include <QMetaType>
#include <QString>
#include <QVariantMap>

#include "graph_controller_helpers.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <queue>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>

namespace QNodeGraph::UI {

using namespace detail;

void GraphController::addDemoNode(bool connectToPrevious) {
  const auto row = static_cast<int>(m_nodeOrder.size());
  const auto node =
      m_document.addNode("demo", "Demo Node " + std::to_string(row + 1));
  if (!node) {
    return;
  }
  const auto input = m_document.addPort(*node, "in", Core::PortDirection::Input,
                                        Core::PortDataType::Any);
  const auto output = m_document.addPort(
      *node, "out", Core::PortDirection::Output, Core::PortDataType::Any);
  if (!input || !output ||
      !m_document.setNodePosition(
          *node, {80.0 + (row % 3) * 250.0, 90.0 + (row / 3) * 180.0})) {
    static_cast<void>(m_document.removeNode(*node));
    return;
  }
  static_cast<void>(m_document.setProperty(*node, "enabled", true));
  static_cast<void>(m_document.setProperty(
      *node, "label", "Demo Node " + std::to_string(row + 1)));
  beginInsertRows(QModelIndex(), row, row);
  m_nodeOrder.push_back(*node);
  endInsertRows();
  if (connectToPrevious && row > 0) {
    const auto previousNode = m_nodeOrder[static_cast<std::size_t>(row - 1)];
    const auto previousOutput =
        portFor(previousNode, Core::PortDirection::Output);
    if (previousOutput != 0) {
      static_cast<void>(m_document.connect(previousOutput, *input));
    }
  }
  emit nodeCountChanged();
  notifyTopologyChanged();
}

bool GraphController::addNodeType(QString typeId, bool connectToPrevious) {
  const auto *descriptor = m_registry.find(typeId.toStdString());
  if (descriptor == nullptr) {
    return false;
  }
  // 唯一节点（如《图像输入》《图像输出》）在同一文档中只允许存在一个。
  if (descriptor->unique && Graph::NodeRegistry::countNodesOfType(
                                m_document, descriptor->typeId) > 0) {
    setError(QString::fromStdString(descriptor->displayName) +
             QStringLiteral(" already exists; only one instance is allowed."));
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
  static_cast<void>(m_document.setProperty(*node, "enabled", true));
  static_cast<void>(m_document.setProperty(*node, "label", name));
  // 尺寸与背景色改由描述符声明，取代此前按 backdrop/group 的 typeId 特判。
  static_cast<void>(m_document.setProperty(
      *node, "width",
      std::int64_t{static_cast<std::int64_t>(descriptor->defaultWidth)}));
  static_cast<void>(m_document.setProperty(
      *node, "height",
      std::int64_t{static_cast<std::int64_t>(descriptor->defaultHeight)}));
  if (const auto *existing = m_document.property(*node, "color");
      existing == nullptr &&
      descriptor->visualRole != Graph::NodeVisualRole::Standard) {
    static_cast<void>(m_document.setProperty(
        *node, "color",
        std::string{descriptor->visualRole == Graph::NodeVisualRole::Backdrop
                        ? "#304554"
                        : "#384b5b"}));
  }
  const auto input = portFor(*node, Core::PortDirection::Input);
  if (connectToPrevious && row > 0 && input != 0) {
    const auto previousNode = m_nodeOrder[static_cast<std::size_t>(row - 1)];
    const auto previousOutput =
        portFor(previousNode, Core::PortDirection::Output);
    if (previousOutput != 0) {
      static_cast<void>(m_document.connect(previousOutput, input));
    }
  }
  beginInsertRows(QModelIndex(), row, row);
  m_nodeOrder.push_back(*node);
  endInsertRows();
  emit nodeCountChanged();
  notifyTopologyChanged();
  return true;
}

bool GraphController::addNodeTypeAt(QString typeId, double x, double y,
                                    bool connectToPrevious) {
  if (!addNodeType(std::move(typeId), connectToPrevious)) {
    return false;
  }
  const auto row = nodeCount() - 1;
  return moveNode(row, x - kDemoNodeWidth / 2.0, y - kDemoNodeHeight / 2.0);
}

bool GraphController::deleteNode(int row) {
  if (row < 0 || row >= nodeCount()) {
    return false;
  }
  const auto nodeId = m_nodeOrder[static_cast<std::size_t>(row)];
  if (isFixedNode(nodeId)) {
    setError(QStringLiteral("This node is fixed and cannot be deleted."));
    return false;
  }
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
  notifyTopologyChanged();
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
    const auto *node = m_document.node(m_nodeOrder[row]);
    if (node == nullptr) {
      continue;
    }
    left = std::min(left, node->position.x);
    top = std::min(top, node->position.y);
    right =
        std::max(right, node->position.x +
                            nodeNumberProperty(*node, "width", kDemoNodeWidth));
    bottom = std::max(bottom,
                      node->position.y +
                          nodeNumberProperty(*node, "height", kDemoNodeHeight));
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
  const auto oldCount = nodeCount();
  beginResetModel();
  // `clear` is a bulk deletion command, not an escape hatch around the fixed
  // node contract. Keep fixed nodes (for example a host's viewport input and
  // output) so the user cannot make them impossible to restore from the UI.
  for (const auto nodeId : m_nodeOrder) {
    if (!isFixedNode(nodeId)) {
      static_cast<void>(m_document.removeNode(nodeId));
    }
  }
  m_nodeOrder = m_document.nodeIds();
  m_nodePreviews.clear();
  m_commandStack.clear();
  endResetModel();
  if (nodeCount() != oldCount) {
    emit nodeCountChanged();
  }
  notifyTopologyChanged();
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
  notifyTopologyChanged();
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
  const auto *anchor =
      m_document.node(m_nodeOrder[static_cast<std::size_t>(anchorRow)]);
  if (anchor == nullptr) {
    return false;
  }
  const auto dx = x - anchor->position.x;
  const auto dy = y - anchor->position.y;
  std::vector<std::pair<Core::NodeId, Core::Point>> targets;
  targets.reserve(m_selectedRows.size());
  for (const auto row : m_selectedRows) {
    const auto *node =
        m_document.node(m_nodeOrder[static_cast<std::size_t>(row)]);
    if (node == nullptr) {
      return false;
    }
    targets.emplace_back(
        node->id, Core::Point{node->position.x + dx, node->position.y + dy});
  }
  const auto result = m_commandStack.execute(
      std::make_unique<Core::MoveNodesCommand>(std::move(targets)), m_document);
  if (!result) {
    return false;
  }
  for (const auto row : m_selectedRows) {
    const auto modelIndex = index(row, 0);
    emit dataChanged(modelIndex, modelIndex, {NodeXRole, NodeYRole});
  }
  notifyTopologyChanged();
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
  for (const auto &connection : m_document.connections()) {
    const auto *output = m_document.port(connection.outputPort);
    const auto *input = m_document.port(connection.inputPort);
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
      auto &successorColumn = columns[static_cast<std::size_t>(successor)];
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
  std::vector<double> columnWidths(static_cast<std::size_t>(maxColumn + 1),
                                   0.0);
  constexpr double kColumnGap = 80.0;
  constexpr double kRowGap = 36.0;
  for (int row = 0; row < count; ++row) {
    const auto *node =
        m_document.node(m_nodeOrder[static_cast<std::size_t>(row)]);
    if (node == nullptr) {
      continue;
    }
    const auto column = columns[static_cast<std::size_t>(row)];
    columnWidths[static_cast<std::size_t>(column)] = std::max(
        columnWidths[static_cast<std::size_t>(column)], nodeWidth(*node));
  }
  std::vector<double> columnX(static_cast<std::size_t>(maxColumn + 1), 80.0);
  for (int column = 1; column <= maxColumn; ++column) {
    columnX[static_cast<std::size_t>(column)] =
        columnX[static_cast<std::size_t>(column - 1)] +
        columnWidths[static_cast<std::size_t>(column - 1)] + kColumnGap;
  }
  std::vector<double> nextY(static_cast<std::size_t>(maxColumn + 1), 90.0);
  bool changed = false;
  for (int row = 0; row < count; ++row) {
    const auto column = columns[static_cast<std::size_t>(row)];
    const auto *node =
        m_document.node(m_nodeOrder[static_cast<std::size_t>(row)]);
    if (node == nullptr) {
      continue;
    }
    const Core::Point position{columnX[static_cast<std::size_t>(column)],
                               nextY[static_cast<std::size_t>(column)]};
    nextY[static_cast<std::size_t>(column)] += nodeHeight(*node) + kRowGap;
    if (node->position.x == position.x && node->position.y == position.y) {
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
  emit dataChanged(index(0, 0), index(count - 1, 0), {NodeXRole, NodeYRole});
  notifyTopologyChanged();
  if (connectionPending()) {
    emit connectionPreviewChanged();
  }
  emit historyChanged();
  return true;
}

} // namespace QNodeGraph::UI
