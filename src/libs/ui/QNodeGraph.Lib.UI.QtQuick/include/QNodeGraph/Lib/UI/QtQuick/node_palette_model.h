#pragma once

#include <QNodeGraph/Lib/Graph/node_registry.h>

#include <QAbstractListModel>
#include <QtQml/qqmlregistration.h>
#include <QString>

#include <vector>

namespace QNodeGraph::UI {

class NodePaletteModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
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
    // 注入宿主注册表：必须与 GraphController 使用同一份，否则画布认识某类型
    // 而调色板不显示（或反之）。
    void setRegistry(QNodeGraph::Graph::NodeRegistry registry);
    // 隐藏固定/唯一节点（如《图像输入》《图像输出》）：它们由宿主预置，不应
    // 出现在可拖拽添加的调色板中。默认关闭，保持既有行为。
    void setHideFixedNodes(bool hide);

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
    bool m_hideFixedNodes = false;
};

} // namespace QNodeGraph::UI
