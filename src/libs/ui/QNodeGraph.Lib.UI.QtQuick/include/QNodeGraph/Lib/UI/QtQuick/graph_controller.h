#pragma once

#include <QNodeGraph/Lib/Core/graph_document.h>

#include <QAbstractListModel>

#include <vector>

namespace QNodeGraph::UI {

class GraphController : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int nodeCount READ nodeCount NOTIFY nodeCountChanged)

public:
    enum NodeRole {
        NodeIdRole = Qt::UserRole + 1,
        NodeNameRole,
        NodeTypeRole,
        NodeXRole,
        NodeYRole,
        InputPortCountRole,
        OutputPortCountRole,
    };
    Q_ENUM(NodeRole)

    explicit GraphController(QObject* parent = nullptr);

    [[nodiscard]] int rowCount(
        const QModelIndex& parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(
        const QModelIndex& index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;
    [[nodiscard]] int nodeCount() const noexcept;

    Q_INVOKABLE void addDemoNode();
    Q_INVOKABLE void clearGraph();
    Q_INVOKABLE bool moveNode(int row, double x, double y);

signals:
    void nodeCountChanged();

private:
    Core::GraphDocument m_document;
    std::vector<Core::NodeId> m_nodeOrder;
};

} // namespace QNodeGraph::UI
