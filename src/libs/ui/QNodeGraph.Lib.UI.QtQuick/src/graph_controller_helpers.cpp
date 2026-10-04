/*
 * File: graph_controller_helpers.cpp
 * Brief: Private helpers shared by GraphController implementation units.
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

namespace QNodeGraph::UI::detail {

QVariant propertyVariant(const Core::PropertyValue &value) {
  return std::visit(
      [](const auto &typed) -> QVariant {
        using Value = std::decay_t<decltype(typed)>;
        if constexpr (std::is_same_v<Value, std::int64_t>) {
          return QVariant::fromValue<qlonglong>(static_cast<qlonglong>(typed));
        } else if constexpr (std::is_same_v<Value, std::string>) {
          return QString::fromStdString(typed);
        } else {
          return QVariant::fromValue(typed);
        }
      },
      value);
}

QString propertyType(const Core::PropertyValue &value) {
  return std::visit(
      [](const auto &typed) {
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

// 控件类型名（QML 侧按此字符串分派编辑器）。与 PropertyEditor 枚举一一对应。
QString propertyEditorName(QNodeGraph::Graph::PropertyEditor editor) {
  switch (editor) {
  case QNodeGraph::Graph::PropertyEditor::Automatic:
    return QStringLiteral("automatic");
  case QNodeGraph::Graph::PropertyEditor::CheckBox:
    return QStringLiteral("checkbox");
  case QNodeGraph::Graph::PropertyEditor::Slider:
    return QStringLiteral("slider");
  case QNodeGraph::Graph::PropertyEditor::SpinBox:
    return QStringLiteral("spinbox");
  case QNodeGraph::Graph::PropertyEditor::LineEdit:
    return QStringLiteral("lineedit");
  case QNodeGraph::Graph::PropertyEditor::FilePath:
    return QStringLiteral("filepath");
  case QNodeGraph::Graph::PropertyEditor::Directory:
    return QStringLiteral("directory");
  case QNodeGraph::Graph::PropertyEditor::ComboBox:
    return QStringLiteral("combobox");
  }
  return QStringLiteral("automatic");
}

// 在描述符中查找某属性的控件元数据，未声明时返回 nullptr。
const QNodeGraph::Graph::PropertyDescriptor *
findPropertyDescriptor(const QNodeGraph::Graph::NodeDescriptor *descriptor,
                       const std::string &name) {
  if (descriptor == nullptr) {
    return nullptr;
  }
  for (const auto &property : descriptor->properties) {
    if (property.name == name) {
      return &property;
    }
  }
  return nullptr;
}

// 把控件元数据附加到属性条目上，供 QML 选择编辑器。未声明元数据时写入
// automatic/0，QML 会退回按 valueType 推断。
void appendPropertyEditorMetadata(
    QVariantMap &entry, const QNodeGraph::Graph::NodeDescriptor *descriptor,
    const std::string &name) {
  const auto *property = findPropertyDescriptor(descriptor, name);
  entry.insert(
      QStringLiteral("editor"),
      propertyEditorName(property != nullptr
                             ? property->editor
                             : QNodeGraph::Graph::PropertyEditor::Automatic));
  entry.insert(QStringLiteral("minimum"),
               property != nullptr ? property->minimum : 0.0);
  entry.insert(QStringLiteral("maximum"),
               property != nullptr ? property->maximum : 0.0);
  entry.insert(QStringLiteral("step"),
               property != nullptr ? property->step : 0.0);
  QVariantList choices;
  if (property != nullptr) {
    for (const auto &choice : property->choices) {
      choices.push_back(QString::fromStdString(choice));
    }
  }
  entry.insert(QStringLiteral("choices"), choices);
}

// 以下三个函数返回描述符未声明外观时的兜底值。此前这里是一整张按 typeId
// 的 if 链硬编码表，现由 NodeDescriptor 的 iconGlyph/iconSource/accent 声明，
// 宿主注册自定义节点时即可一并给出外观，无需修改库代码。
QString fallbackIconGlyph() { return QStringLiteral("NODE"); }

QString fallbackIconSource() {
  return QStringLiteral("qrc:/qt/qml/QNodeGraph/UI/qml/icons/node.svg");
}

QString fallbackAccent() { return QStringLiteral("#426b91"); }

double nodeNumberProperty(const Core::Node &node, const char *name,
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

// 节点实际尺寸。必须与 data() 的 NodeWidthRole/NodeHeightRole 保持同一语义，
// 否则连线端点会按默认尺寸计算、与画布上的真实节点错位。
//
// 兜底值刻意用 kDemoNodeWidth/Height 而非描述符默认尺寸：连接几何必须是
// const 上下文的纯函数（portAt 也被 const 方法使用），而注册表查询在节点刚
// 创建、属性尚未写入的瞬间可能拿不到描述符；画布尺寸与连线端点走同一个
// 兜底值，才不会出现两者基准不一致。
double nodeWidth(const Core::Node &node) {
  return nodeNumberProperty(node, "width", kDemoNodeWidth);
}

double nodeHeight(const Core::Node &node) {
  return nodeNumberProperty(node, "height", kDemoNodeHeight);
}

// 输出端口的世界坐标（节点右侧中点）。连线、命中测试、切片与预览共用同一
// 计算，避免任何一处漏改导致端点漂移。
Core::Point outputPortPoint(const Core::Node &node) {
  return Core::Point{node.position.x + nodeWidth(node),
                     node.position.y + nodeHeight(node) / 2.0};
}

// 输入端口的世界坐标（节点左侧中点）。
Core::Point inputPortPoint(const Core::Node &node) {
  return Core::Point{node.position.x, node.position.y + nodeHeight(node) / 2.0};
}

QString nodeStringProperty(const Core::Node &node, const char *name,
                           QString fallback) {
  const auto iterator = node.properties.find(name);
  if (iterator != node.properties.end() &&
      std::holds_alternative<std::string>(iterator->second)) {
    return QString::fromStdString(std::get<std::string>(iterator->second));
  }
  return fallback;
}

QString portDataTypeName(Core::PortDataType type) {
  switch (type) {
  case Core::PortDataType::Any:
    return QStringLiteral("any");
  case Core::PortDataType::Image:
    return QStringLiteral("image");
  case Core::PortDataType::Number:
    return QStringLiteral("number");
  case Core::PortDataType::Text:
    return QStringLiteral("text");
  case Core::PortDataType::Boolean:
    return QStringLiteral("boolean");
  }
  return {};
}

QVariantList nodePorts(const Core::Node &node,
                       const Core::GraphDocument &document,
                       Core::PortDirection direction) {
  QVariantList ports;
  for (const auto portId : node.ports) {
    const auto *port = document.port(portId);
    if (port == nullptr || port->direction != direction) {
      continue;
    }
    ports.push_back(QVariantMap{
        {QStringLiteral("name"), QString::fromStdString(port->name)},
        {QStringLiteral("dataType"), portDataTypeName(port->dataType)},
        {QStringLiteral("acceptsMultiple"), port->acceptsMultipleConnections}});
  }
  return ports;
}

double cross(const Core::Point &first, const Core::Point &second,
             const Core::Point &third) {
  return (second.x - first.x) * (third.y - first.y) -
         (second.y - first.y) * (third.x - first.x);
}

bool onSegment(const Core::Point &first, const Core::Point &second,
               const Core::Point &point) {
  constexpr double epsilon = 1e-6;
  return point.x >= std::min(first.x, second.x) - epsilon &&
         point.x <= std::max(first.x, second.x) + epsilon &&
         point.y >= std::min(first.y, second.y) - epsilon &&
         point.y <= std::max(first.y, second.y) + epsilon;
}

bool segmentsIntersect(const Core::Point &first, const Core::Point &second,
                       const Core::Point &third, const Core::Point &fourth) {
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
         (std::abs(secondCross) <= epsilon &&
          onSegment(first, second, fourth)) ||
         (std::abs(thirdCross) <= epsilon && onSegment(third, fourth, first)) ||
         (std::abs(fourthCross) <= epsilon && onSegment(third, fourth, second));
}

bool cubicIntersects(const Core::Point &start, const Core::Point &controlStart,
                     const Core::Point &controlEnd, const Core::Point &end,
                     const Core::Point &sliceStart,
                     const Core::Point &sliceEnd) {
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

} // namespace QNodeGraph::UI::detail
