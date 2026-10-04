/*
 * File: graph_controller_commands.cpp
 * Brief: Selection, property, resize, and history commands for GraphController.
 * Author: Haomin Kong
 * Email: konghm@beim.com.cn / a645162@qq.com
 * Created: 2026-10-04
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
    const auto *node =
        m_document.node(m_nodeOrder[static_cast<std::size_t>(row)]);
    if (node == nullptr) {
      continue;
    }
    const auto width = data(index(row, 0), NodeWidthRole).toDouble();
    const auto height = data(index(row, 0), NodeHeightRole).toDouble();
    const auto intersects =
        node->position.x <= right && node->position.x + width >= left &&
        node->position.y <= bottom && node->position.y + height >= top;
    if (intersects &&
        std::find(rows.cbegin(), rows.cend(), row) == rows.cend()) {
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
  const auto *current = m_document.property(nodeId, name.toStdString());
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

  if (const auto *descriptor = m_registry.find(
          m_document.node(nodeId) != nullptr
              ? m_document.node(nodeId)->type
              : std::string{});
      descriptor != nullptr) {
    for (const auto &property : descriptor->properties) {
      if (property.name != name.toStdString() ||
          !std::holds_alternative<double>(converted) &&
              !std::holds_alternative<std::int64_t>(converted)) {
        continue;
      }
      const auto clamped = std::clamp(
          std::holds_alternative<double>(converted)
              ? std::get<double>(converted)
              : static_cast<double>(std::get<std::int64_t>(converted)),
          property.minimum, property.maximum);
      if (std::holds_alternative<double>(converted)) {
        converted = clamped;
      } else {
        converted = static_cast<std::int64_t>(std::llround(clamped));
      }
      break;
    }
  }

  const auto result = m_commandStack.execute(
      std::make_unique<Core::SetPropertyCommand>(nodeId, name.toStdString(),
                                                 std::move(converted)),
      m_document);
  if (!result) {
    return false;
  }
  const auto modelIndex = index(row, 0);
  if (name == QStringLiteral("enabled")) {
    emit dataChanged(modelIndex, modelIndex, {NodeEnabledRole});
  } else if (name == QStringLiteral("label")) {
    emit dataChanged(modelIndex, modelIndex, {NodeLabelRole, NodeNameRole});
  } else if (name == QStringLiteral("fixed")) {
    emit dataChanged(modelIndex, modelIndex, {NodeFixedRole});
  }
  // Node-local editors bind to this composite role. Without an explicit
  // refresh the side panel changes while the slider rendered inside the node
  // remains bound to its previous QVariantList value.
  emit dataChanged(modelIndex, modelIndex, {NodeOnNodePropertiesRole});
  emit propertiesChanged();
  emit historyChanged();
  return true;
}

bool GraphController::resizeNode(int row, double width, double height) {
  // 兼容旧签名：保持左上角不动（右下角拖拽语义）。
  if (row < 0 || row >= nodeCount()) {
    return false;
  }
  const auto *node =
      m_document.node(m_nodeOrder[static_cast<std::size_t>(row)]);
  if (node == nullptr) {
    return false;
  }
  return resizeNodeAt(row, node->position.x, node->position.y, width, height);
}

bool GraphController::resizeNodeAt(int row, double x, double y, double width,
                                   double height) {
  if (row < 0 || row >= nodeCount() || width <= 0.0 || height <= 0.0) {
    return false;
  }
  const auto nodeId = m_nodeOrder[static_cast<std::size_t>(row)];
  // 单条原子命令：宽、高与左上角一起写入同一次撤销快照。之前用两条
  // SetPropertyCommand 会导致撤销只回退一半尺寸。
  if (const auto result =
          m_commandStack.execute(std::make_unique<Core::ResizeNodeCommand>(
                                     nodeId, Core::Point{x, y}, width, height),
                                 m_document);
      !result) {
    return false;
  }
  const auto modelIndex = index(row, 0);
  emit dataChanged(modelIndex, modelIndex,
                   {NodeWidthRole, NodeHeightRole, NodeXRole, NodeYRole});
  notifyTopologyChanged();
  emit historyChanged();
  return true;
}

bool GraphController::setNodeFixed(int row, bool fixed) {
  return setNodeProperty(row, QStringLiteral("fixed"), fixed);
}

bool GraphController::assignNodeToGroup(int nodeRow, int groupRow) {
  if (nodeRow < 0 || groupRow < 0 || nodeRow >= nodeCount() ||
      groupRow >= nodeCount() || nodeRow == groupRow) {
    return false;
  }
  const auto group =
      m_document.node(m_nodeOrder[static_cast<std::size_t>(groupRow)]);
  const auto node =
      m_document.node(m_nodeOrder[static_cast<std::size_t>(nodeRow)]);
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
  const auto node =
      m_document.node(m_nodeOrder[static_cast<std::size_t>(nodeRow)]);
  if (node == nullptr ||
      node->properties.find("groupId") == node->properties.end()) {
    return false;
  }
  const auto result = m_commandStack.execute(
      std::make_unique<Core::SetPropertyCommand>(node->id, "groupId",
                                                 static_cast<std::int64_t>(-1)),
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
  notifyTopologyChanged();
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
  notifyTopologyChanged();
  emit propertiesChanged();
  emit historyChanged();
  return true;
}

} // namespace QNodeGraph::UI
