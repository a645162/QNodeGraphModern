#pragma once

#include <QNodeGraph/Lib/Core/graph_document.h>

#include <QObject>

namespace QNodeGraph::UI {

class GraphController : public QObject {
    Q_OBJECT
    Q_PROPERTY(int nodeCount READ nodeCount NOTIFY nodeCountChanged)

public:
    explicit GraphController(QObject* parent = nullptr);

    [[nodiscard]] int nodeCount() const noexcept;

    Q_INVOKABLE void addDemoNode();
    Q_INVOKABLE void clearGraph();

signals:
    void nodeCountChanged();

private:
    Core::GraphDocument m_document;
};

} // namespace QNodeGraph::UI
