#pragma once

#include <QNodeGraph/Lib/Core/graph_document.h>

#include <QAbstractListModel>
#include <QString>
#include <QVariant>

#include <vector>

namespace QNodeGraph::UI {

class GraphController : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int nodeCount READ nodeCount NOTIFY nodeCountChanged)
    Q_PROPERTY(QVariantList connections READ connections NOTIFY connectionsChanged)
    Q_PROPERTY(bool connectionPending READ connectionPending NOTIFY connectionPendingChanged)
    Q_PROPERTY(QVariantMap connectionPreview READ connectionPreview NOTIFY connectionPreviewChanged)
    Q_PROPERTY(int selectedRow READ selectedRow NOTIFY selectedRowChanged)
    Q_PROPERTY(QVariantList selectedProperties READ selectedProperties NOTIFY propertiesChanged)

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
    [[nodiscard]] QVariantList connections() const;
    [[nodiscard]] bool connectionPending() const noexcept;
    [[nodiscard]] QVariantMap connectionPreview() const;
    [[nodiscard]] int selectedRow() const noexcept;
    [[nodiscard]] QVariantList selectedProperties() const;

    Q_INVOKABLE void addDemoNode(bool connectToPrevious = true);
    Q_INVOKABLE void clearGraph();
    Q_INVOKABLE bool moveNode(int row, double x, double y);
    Q_INVOKABLE QVariantMap portAt(double worldX, double worldY) const;
    Q_INVOKABLE bool beginConnection(int outputRow);
    Q_INVOKABLE void updateConnectionPreview(double worldX, double worldY);
    Q_INVOKABLE bool completeConnectionAt(double worldX, double worldY);
    Q_INVOKABLE void cancelConnection();
    Q_INVOKABLE bool selectNode(int row);
    Q_INVOKABLE bool setNodeProperty(int row, QString name, QVariant value);

signals:
    void nodeCountChanged();
    void connectionsChanged();
    void connectionPendingChanged();
    void connectionPreviewChanged();
    void selectedRowChanged();
    void propertiesChanged();

private:
    [[nodiscard]] Core::PortId portFor(Core::NodeId nodeId,
                                       Core::PortDirection direction) const;
    [[nodiscard]] int rowFor(Core::NodeId nodeId) const;
    [[nodiscard]] bool connectRows(int outputRow, int inputRow);

    Core::GraphDocument m_document;
    std::vector<Core::NodeId> m_nodeOrder;
    int m_pendingOutputRow = -1;
    Core::Point m_previewPoint;
    int m_selectedRow = -1;
};

} // namespace QNodeGraph::UI
