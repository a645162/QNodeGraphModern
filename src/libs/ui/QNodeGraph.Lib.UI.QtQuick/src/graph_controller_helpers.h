/*
 * File: graph_controller_helpers.h
 * Brief: Shared private geometry and property helpers for GraphController.
 * Author: Haomin Kong
 * Email: konghm@beim.com.cn / a645162@qq.com
 * Created: 2026-10-04
 */

#pragma once

#include <QNodeGraph/Lib/UI/QtQuick/graph_controller.h>

#include <QVariantList>
#include <QVariantMap>

#include <string>

namespace QNodeGraph::UI::detail {

inline constexpr double kDemoNodeWidth = 180.0;
inline constexpr double kDemoNodeHeight = 108.0;
inline constexpr double kPortHitRadius = 12.0;

QVariant propertyVariant(const Core::PropertyValue& value);
QString propertyType(const Core::PropertyValue& value);
QString propertyEditorName(Graph::PropertyEditor editor);
const Graph::PropertyDescriptor*
findPropertyDescriptor(const Graph::NodeDescriptor* descriptor,
                       const std::string& name);
void appendPropertyEditorMetadata(QVariantMap& entry,
                                  const Graph::NodeDescriptor* descriptor,
                                  const std::string& name);

QString fallbackIconGlyph();
QString fallbackIconSource();
QString fallbackAccent();

double nodeNumberProperty(const Core::Node& node, const char* name,
                          double fallback);
double nodeWidth(const Core::Node& node);
double nodeHeight(const Core::Node& node);
Core::Point outputPortPoint(const Core::Node& node);
Core::Point inputPortPoint(const Core::Node& node);
QString nodeStringProperty(const Core::Node& node, const char* name,
                           QString fallback);
QString portDataTypeName(Core::PortDataType type);
QVariantList nodePorts(const Core::Node& node,
                       const Core::GraphDocument& document,
                       Core::PortDirection direction);

bool cubicIntersects(const Core::Point& start, const Core::Point& control_start,
                     const Core::Point& control_end, const Core::Point& end,
                     const Core::Point& slice_start,
                     const Core::Point& slice_end);

} // namespace QNodeGraph::UI::detail
