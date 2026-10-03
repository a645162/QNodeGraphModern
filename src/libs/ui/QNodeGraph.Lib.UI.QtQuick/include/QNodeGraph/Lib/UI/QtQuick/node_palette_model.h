#pragma once

#include <QNodeGraph/Lib/Graph/node_registry.h>

#include <QAbstractListModel>
#include <QString>

#include <vector>

namespace QNodeGraph::UI {

class NodePaletteModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged)

public:
    enum NodeRole {
        NodeTypeRole = Qt::UserRole + 1,
        NodeNameRole,
        CategoryRole,
        InputPortCountRole,
        OutputPortCountRole,
    };
    Q_ENUM(NodeRole)

    explicit NodePaletteModel(QObject* parent = nullptr);

    [[nodiscard]] int rowCount(
        const QModelIndex& parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(
        const QModelIndex& index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;
    [[nodiscard]] QString filter() const;

    void setFilter(QString value);

signals:
    void filterChanged();

private:
    void rebuildVisibleRows();
    [[nodiscard]] static QString categoryFor(const QString& typeId);

    QNodeGraph::Graph::NodeRegistry m_registry;
    std::vector<QNodeGraph::Graph::NodeDescriptor> m_descriptors;
    std::vector<int> m_visibleRows;
    QString m_filter;
};

} // namespace QNodeGraph::UI
