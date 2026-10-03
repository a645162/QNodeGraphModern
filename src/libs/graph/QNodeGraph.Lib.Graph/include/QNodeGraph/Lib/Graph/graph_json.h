#pragma once

#include <QNodeGraph/Lib/Core/graph_document.h>

#include <QJsonObject>
#include <QString>

namespace QNodeGraph::Graph {

class GraphJson final {
public:
    static constexpr int kSchemaVersion = 1;

    [[nodiscard]] static Core::GraphResult<QJsonObject> toJson(
        const Core::GraphDocument& document);
    [[nodiscard]] static Core::GraphResult<Core::GraphDocument> fromJson(
        const QJsonObject& object);
    [[nodiscard]] static Core::GraphResult<void> save(
        const Core::GraphDocument& document, const QString& filePath);
    [[nodiscard]] static Core::GraphResult<Core::GraphDocument> load(
        const QString& filePath);
};

} // namespace QNodeGraph::Graph
