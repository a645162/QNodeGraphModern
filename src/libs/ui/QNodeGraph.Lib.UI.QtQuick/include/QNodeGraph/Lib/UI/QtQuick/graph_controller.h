#pragma once

#include <QNodeGraph/Lib/Core/graph_document.h>
#include <QNodeGraph/Lib/Core/graph_commands.h>
#include <QNodeGraph/Lib/Graph/graph_json.h>
#include <QNodeGraph/Lib/Graph/node_registry.h>

#include <QAbstractListModel>
#include <QtQml/qqmlregistration.h>
#include <QString>
#include <QVariant>

#include <map>
#include <set>
#include <vector>

namespace QNodeGraph::UI {

class GraphController : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int nodeCount READ nodeCount NOTIFY nodeCountChanged)
    Q_PROPERTY(QVariantList connections READ connections NOTIFY connectionsChanged)
    Q_PROPERTY(bool connectionPending READ connectionPending NOTIFY connectionPendingChanged)
    Q_PROPERTY(int pendingOutputRow READ pendingOutputRow NOTIFY connectionPendingChanged)
    Q_PROPERTY(QVariantMap connectionPreview READ connectionPreview NOTIFY connectionPreviewChanged)
    Q_PROPERTY(int selectedRow READ selectedRow NOTIFY selectedRowChanged)
    Q_PROPERTY(QVariantList selectedRows READ selectedRows NOTIFY selectionChanged)
    Q_PROPERTY(QVariantList selectedProperties READ selectedProperties NOTIFY propertiesChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY errorChanged)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY historyChanged)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY historyChanged)

public:
    enum NodeRole {
        NodeIdRole = Qt::UserRole + 1,
        NodeNameRole,
        NodeTypeRole,
        NodeXRole,
        NodeYRole,
        InputPortCountRole,
        OutputPortCountRole,
        NodeIconRole,
        NodeIconSourceRole,
        NodeAccentRole,
        NodeEnabledRole,
        NodeLabelRole,
        NodeWidthRole,
        NodeHeightRole,
        NodeColorRole,
        NodeIsBackdropRole,
        NodeIsGroupRole,
        NodeGroupIdRole,
        NodeSelectedRole,
        NodePreviewSourceRole,
        NodePreviewWidthRole,
        NodePreviewHeightRole,
        NodePreviewChannelsRole,
        NodeInputPortsRole,
        NodeOutputPortsRole,
        NodeFixedRole,
        NodeContentUrlRole,
        NodeOnNodePropertiesRole,
        // 该节点的输出是否被下游使用（从任一输出型节点反向可达）。
        // 未被使用的 in+out 节点会被标记为无效，UI 据此降级显示。
        NodeOutputUsedRole,
        // 该节点是否参与求值（已达且输出被使用）。等价于 !pruned。
        NodeEffectiveRole,
    };
    Q_ENUM(NodeRole)

    explicit GraphController(QObject* parent = nullptr);
    // 注入宿主注册表：宿主注册自定义节点类型后传入，使画布与调色板共享同一
    // 份类型定义。未注入时使用 NodeRegistry::withBuiltins()（保持既有行为）。
    void setRegistry(QNodeGraph::Graph::NodeRegistry registry);
    [[nodiscard]] const QNodeGraph::Graph::NodeRegistry& registry() const noexcept;
    // 注册表变更后需通知视图刷新（图标/强调色/尺寸等 role 全部来自描述符）。
    void refreshRegistryDependentRoles();

    [[nodiscard]] int rowCount(
        const QModelIndex& parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(
        const QModelIndex& index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;
    [[nodiscard]] int nodeCount() const noexcept;
    [[nodiscard]] QVariantList connections() const;
    [[nodiscard]] bool connectionPending() const noexcept;
    [[nodiscard]] int pendingOutputRow() const noexcept;
    [[nodiscard]] QVariantMap connectionPreview() const;
    [[nodiscard]] int selectedRow() const noexcept;
    [[nodiscard]] QVariantList selectedRows() const;
    [[nodiscard]] QVariantList selectedProperties() const;
    [[nodiscard]] QString lastError() const;
    [[nodiscard]] bool canUndo() const noexcept;
    [[nodiscard]] bool canRedo() const noexcept;

    Q_INVOKABLE void addDemoNode(bool connectToPrevious = false);
    Q_INVOKABLE bool addNodeType(QString typeId,
                                 bool connectToPrevious = false);
    Q_INVOKABLE bool addNodeTypeAt(QString typeId, double x, double y,
                                   bool connectToPrevious = false);
    Q_INVOKABLE bool deleteNode(int row);
    Q_INVOKABLE bool saveGraph(QString filePath);
    Q_INVOKABLE bool loadGraph(QString filePath);
    Q_INVOKABLE QVariantMap graphBounds() const;
    Q_INVOKABLE bool setNodePreview(int row, QString source, int width,
                                    int height, int channels);
    Q_INVOKABLE bool clearNodePreview(int row);
    Q_INVOKABLE void clearGraph();
    Q_INVOKABLE bool moveNode(int row, double x, double y);
    Q_INVOKABLE bool moveSelectedNodes(int anchorRow, double x, double y);
    Q_INVOKABLE bool autoLayout();
    Q_INVOKABLE int sliceConnections(double startX, double startY,
                                     double endX, double endY);
    Q_INVOKABLE QVariantMap portAt(double worldX, double worldY) const;
    Q_INVOKABLE bool beginConnection(int outputRow);
    Q_INVOKABLE void updateConnectionPreview(double worldX, double worldY);
    Q_INVOKABLE bool completeConnectionAt(double worldX, double worldY);
    Q_INVOKABLE void cancelConnection();
    Q_INVOKABLE bool selectNode(int row);
    Q_INVOKABLE bool toggleNodeSelection(int row);
    Q_INVOKABLE bool selectNodesInRect(double startX, double startY,
                                       double endX, double endY,
                                       bool additive = false);
    Q_INVOKABLE bool setNodeProperty(int row, QString name, QVariant value);
    Q_INVOKABLE bool resizeNode(int row, double width, double height);
    // 四角/四边 resize 的统一入口：同时指定调整后的左上角世界坐标与尺寸。
    // 右下角拖拽等价于 origin 不变；左/上侧拖拽会同时改变 origin。
    Q_INVOKABLE bool resizeNodeAt(int row, double x, double y, double width,
                                  double height);
    Q_INVOKABLE bool setNodeFixed(int row, bool fixed);
    Q_INVOKABLE bool assignNodeToGroup(int nodeRow, int groupRow);
    Q_INVOKABLE bool clearNodeGroup(int nodeRow);
    Q_INVOKABLE bool undo();
    Q_INVOKABLE bool redo();

signals:
    void nodeCountChanged();
    void connectionsChanged();
    void connectionPendingChanged();
    void connectionPreviewChanged();
    void selectedRowChanged();
    void selectionChanged();
    void propertiesChanged();
    void errorChanged();
    void historyChanged();
    void nodeConnected(int outputRow, int inputRow);
    void nodeDisconnected(int outputRow, int inputRow);

private:
    void setError(QString message);
    void setSelection(std::vector<int> rows);
    void syncModelOrder();
    [[nodiscard]] Core::PortId portFor(Core::NodeId nodeId,
                                       Core::PortDirection direction) const;
    [[nodiscard]] int rowFor(Core::NodeId nodeId) const;
    [[nodiscard]] bool connectRows(int outputRow, int inputRow);
    [[nodiscard]] bool isFixedNode(Core::NodeId nodeId) const;
    // 重算「输出被使用」集合：把「输出型节点」（无输出端口者，如图像预览、
    // 保存、视口输出）当作汇点，沿连接反向做可达性遍历。未出现在集合中的
    // 节点其输出无人使用，可被剪枝。
    void recomputeOutputUsage();
    // 拓扑或连接发生变化后的统一通知：重算可达性并发出 connectionsChanged +
    // 剪枝相关 role 的 dataChanged。所有改动连接的路径都必须走这里，否则
    // 剪枝标记会与图不同步。
    void notifyTopologyChanged();

    Core::GraphDocument m_document;
    QNodeGraph::Graph::NodeRegistry m_registry;
    Core::GraphCommandStack m_commandStack;
    std::vector<Core::NodeId> m_nodeOrder;
    std::vector<int> m_selectedRows;
    QString m_lastError;
    struct NodePreview {
        QString source;
        int width = 0;
        int height = 0;
        int channels = 0;
    };
    std::map<Core::NodeId, NodePreview> m_nodePreviews;
    // 输出被使用（从汇点反向可达）的节点集合，由 recomputeOutputUsage() 维护。
    std::set<Core::NodeId> m_outputUsed;
    int m_pendingOutputRow = -1;
    Core::Point m_previewPoint;
    int m_selectedRow = -1;
};

} // namespace QNodeGraph::UI
