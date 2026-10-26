#include <QNodeGraph/Lib/UI/QtQuick/graph_controller.h>

#include <QtTest/QtTest>
#include <QTemporaryDir>

#include <algorithm>

class GraphControllerTest final : public QObject {
    Q_OBJECT

private slots:
    void startsWithEmptyModel();
    void addsNodeWithRoles();
    void connectsAdjacentDemoNodes();
    void addsNodeAtCanvasPosition();
    void fansOutOneOutputToMultipleInputs();
    void connectsNodesThroughPortInteraction();
    void reportsPortHitTestingResults();
    void exposesAndClearsConnectionPreview();
    void selectsNodeAndListsProperties();
    void selectsNodesByRectangleAndTogglesSelection();
    void editsSelectedNodeProperty();
    void addsRegisteredNodeType();
    void deletesNodeAndRestoresWithUndo();
    void savesAndLoadsGraphThroughController();
    void movesNodeWithUndoAndRedo();
    void movesSelectedNodesAsOneCommand();
    void exposesNodeVisualRolesAndDisabledState();
    void exposesNodeImagePreviewRoles();
    void exposesPortNameRoles();
    void reportsGraphBounds();
    void autoLayoutsConnectedNodes();
    void slicesConnectionsAlongGesture();
    void exposesGroupAndBackdropVisualRoles();
    void assignsNodeToGroupAndExposesProxyPorts();
    void movesNodeAndEmitsData();
    void clearsModel();
    void emitsConnectionSignals();
    void replacesExistingInputConnection();
    void blocksDeletionOfFixedNode();
    void resizesNodeWithUndo();
    void resizesNodeFromTopLeftCorner();
    void followsNodeSizeInConnectionGeometry();
    void declaresAppearanceFromDescriptor();
    void usesDefaultSizeFromDescriptor();
    void rejectsDuplicateUniqueNode();
    void exposesPropertyEditorMetadata();
    void exposesExtendedPropertySchemaMetadata();
    void usesInjectedRegistryForCustomNodes();
    void marksPrunedNodesAsIneffective();
};

void GraphControllerTest::startsWithEmptyModel() {
    const QNodeGraph::UI::GraphController controller;
    QCOMPARE(controller.rowCount(), 0);
    QCOMPARE(controller.nodeCount(), 0);
}

void GraphControllerTest::addsNodeWithRoles() {
    QNodeGraph::UI::GraphController controller;
    controller.addDemoNode();
    QCOMPARE(controller.rowCount(), 1);

    const auto index = controller.index(0, 0);
    QCOMPARE(
        index.data(QNodeGraph::UI::GraphController::NodeNameRole).toString(),
        QStringLiteral("Demo Node 1"));
    QCOMPARE(
        index.data(QNodeGraph::UI::GraphController::NodeTypeRole).toString(),
        QStringLiteral("demo"));
    QCOMPARE(
        index.data(QNodeGraph::UI::GraphController::InputPortCountRole).toInt(),
        1);
    QCOMPARE(index.data(QNodeGraph::UI::GraphController::OutputPortCountRole)
                 .toInt(),
             1);
}

void GraphControllerTest::connectsAdjacentDemoNodes() {
    QNodeGraph::UI::GraphController controller;
    controller.addDemoNode(true);
    controller.addDemoNode(true);
    QCOMPARE(controller.connections().size(), qsizetype{1});
    const auto connection = controller.connections().constFirst().toMap();
    QCOMPARE(connection.value(QStringLiteral("outputRow")).toInt(), 0);
    QCOMPARE(connection.value(QStringLiteral("inputRow")).toInt(), 1);
}

void GraphControllerTest::addsNodeAtCanvasPosition() {
    QNodeGraph::UI::GraphController controller;
    QVERIFY(
        controller.addNodeTypeAt(QStringLiteral("grayscale"), 420.0, 260.0));
    QCOMPARE(controller.nodeCount(), 1);
    const auto index = controller.index(0, 0);
    QCOMPARE(index.data(QNodeGraph::UI::GraphController::NodeXRole).toDouble(),
             330.0);
    QCOMPARE(index.data(QNodeGraph::UI::GraphController::NodeYRole).toDouble(),
             206.0);
}

void GraphControllerTest::fansOutOneOutputToMultipleInputs() {
    QNodeGraph::UI::GraphController controller;
    controller.addDemoNode(false);
    controller.addDemoNode(false);
    controller.addDemoNode(false);

    QVERIFY(controller.beginConnection(0));
    QVERIFY(controller.completeConnectionAt(330.0, 144.0));
    QVERIFY(controller.beginConnection(0));
    QVERIFY(controller.completeConnectionAt(580.0, 144.0));
    QCOMPARE(controller.connections().size(), qsizetype{2});
}

void GraphControllerTest::connectsNodesThroughPortInteraction() {
    QNodeGraph::UI::GraphController controller;
    controller.addDemoNode(false);
    controller.addDemoNode(false);

    QVERIFY(controller.beginConnection(0));
    controller.updateConnectionPreview(330.0, 144.0);
    QVERIFY(controller.completeConnectionAt(330.0, 144.0));
    QCOMPARE(controller.connections().size(), qsizetype{1});
    QVERIFY(!controller.connectionPending());
}

void GraphControllerTest::reportsPortHitTestingResults() {
    QNodeGraph::UI::GraphController controller;
    controller.addDemoNode();

    const auto input = controller.portAt(80.0, 144.0);
    QCOMPARE(input.value(QStringLiteral("row")).toInt(), 0);
    QCOMPARE(input.value(QStringLiteral("direction")).toString(),
             QStringLiteral("input"));

    const auto output = controller.portAt(260.0, 144.0);
    QCOMPARE(output.value(QStringLiteral("row")).toInt(), 0);
    QCOMPARE(output.value(QStringLiteral("direction")).toString(),
             QStringLiteral("output"));
    QVERIFY(controller.portAt(400.0, 400.0).isEmpty());
}

void GraphControllerTest::exposesAndClearsConnectionPreview() {
    QNodeGraph::UI::GraphController controller;
    controller.addDemoNode();
    QSignalSpy previewSpy(
        &controller,
        &QNodeGraph::UI::GraphController::connectionPreviewChanged);

    QVERIFY(controller.beginConnection(0));
    controller.updateConnectionPreview(300.0, 210.0);
    const auto preview = controller.connectionPreview();
    QCOMPARE(preview.value(QStringLiteral("outputRow")).toInt(), 0);
    QCOMPARE(preview.value(QStringLiteral("inputX")).toDouble(), 300.0);
    QCOMPARE(preview.value(QStringLiteral("inputY")).toDouble(), 210.0);
    QVERIFY(!previewSpy.isEmpty());

    controller.cancelConnection();
    QVERIFY(!controller.connectionPending());
    QVERIFY(controller.connectionPreview().isEmpty());
}

void GraphControllerTest::selectsNodeAndListsProperties() {
    QNodeGraph::UI::GraphController controller;
    controller.addDemoNode(false);

    QVERIFY(controller.selectNode(0));
    QCOMPARE(controller.selectedRow(), 0);
    const auto properties = controller.selectedProperties();
    QVERIFY(!properties.isEmpty());
    const auto first = properties.constFirst().toMap();
    QVERIFY(first.contains(QStringLiteral("name")));
    QVERIFY(first.contains(QStringLiteral("valueType")));
}

void GraphControllerTest::selectsNodesByRectangleAndTogglesSelection() {
    QNodeGraph::UI::GraphController controller;
    controller.addDemoNode(false);
    controller.addDemoNode(false);
    QVERIFY(controller.selectNodesInRect(60.0, 70.0, 600.0, 190.0));
    const QVariantList bothRows{0, 1};
    QCOMPARE(controller.selectedRows(), bothRows);
    QVERIFY(controller.toggleNodeSelection(0));
    const QVariantList secondRow{1};
    QCOMPARE(controller.selectedRows(), secondRow);
    QVERIFY(controller.index(1, 0)
                .data(QNodeGraph::UI::GraphController::NodeSelectedRole)
                .toBool());
    QVERIFY(!controller.index(0, 0)
                 .data(QNodeGraph::UI::GraphController::NodeSelectedRole)
                 .toBool());
}

void GraphControllerTest::editsSelectedNodeProperty() {
    QNodeGraph::UI::GraphController controller;
    controller.addDemoNode(false);
    QVERIFY(controller.selectNode(0));

    QVERIFY(controller.setNodeProperty(0, QStringLiteral("label"),
                                       QStringLiteral("Edited")));
    const auto properties = controller.selectedProperties();
    const auto edited = std::find_if(
        properties.cbegin(), properties.cend(), [](const QVariant& value) {
            return value.toMap().value(QStringLiteral("name")).toString() ==
                   QStringLiteral("label");
        });
    QVERIFY(edited != properties.cend());
    QCOMPARE(edited->toMap().value(QStringLiteral("value")).toString(),
             QStringLiteral("Edited"));
}

void GraphControllerTest::addsRegisteredNodeType() {
    QNodeGraph::UI::GraphController controller;
    QVERIFY(controller.addNodeType(QStringLiteral("grayscale"), false));
    QCOMPARE(controller.nodeCount(), 1);
    const auto index = controller.index(0, 0);
    QCOMPARE(
        index.data(QNodeGraph::UI::GraphController::NodeTypeRole).toString(),
        QStringLiteral("grayscale"));
    QCOMPARE(
        index.data(QNodeGraph::UI::GraphController::InputPortCountRole).toInt(),
        1);
    QCOMPARE(index.data(QNodeGraph::UI::GraphController::OutputPortCountRole)
                 .toInt(),
             1);
}

void GraphControllerTest::deletesNodeAndRestoresWithUndo() {
    QNodeGraph::UI::GraphController controller;
    controller.addDemoNode(false);
    controller.addDemoNode(true);
    QCOMPARE(controller.nodeCount(), 2);
    QCOMPARE(controller.connections().size(), qsizetype{1});

    QVERIFY(controller.deleteNode(0));
    QCOMPARE(controller.nodeCount(), 1);
    QCOMPARE(controller.connections().size(), qsizetype{0});
    QVERIFY(controller.undo());
    QCOMPARE(controller.nodeCount(), 2);
    QCOMPARE(controller.connections().size(), qsizetype{1});
    QVERIFY(controller.redo());
    QCOMPARE(controller.nodeCount(), 1);
    QCOMPARE(controller.connections().size(), qsizetype{0});
}

void GraphControllerTest::savesAndLoadsGraphThroughController() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath(QStringLiteral("session.json"));

    QNodeGraph::UI::GraphController source;
    source.addDemoNode(false);
    source.addDemoNode(true);
    QVERIFY(source.saveGraph(path));
    QVERIFY(source.lastError().isEmpty());

    QNodeGraph::UI::GraphController loaded;
    QVERIFY(loaded.loadGraph(path));
    QCOMPARE(loaded.nodeCount(), 2);
    QCOMPARE(loaded.connections().size(), qsizetype{1});
    QVERIFY(loaded.lastError().isEmpty());

    QVERIFY(
        !loaded.loadGraph(directory.filePath(QStringLiteral("missing.json"))));
    QVERIFY(!loaded.lastError().isEmpty());
}

void GraphControllerTest::movesNodeWithUndoAndRedo() {
    QNodeGraph::UI::GraphController controller;
    controller.addDemoNode(false);
    QVERIFY(controller.moveNode(0, 320.0, 150.0));
    QVERIFY(controller.canUndo());
    QVERIFY(controller.undo());
    QCOMPARE(controller.index(0, 0)
                 .data(QNodeGraph::UI::GraphController::NodeXRole)
                 .toDouble(),
             80.0);
    QVERIFY(controller.canRedo());
    QVERIFY(controller.redo());
    QCOMPARE(controller.index(0, 0)
                 .data(QNodeGraph::UI::GraphController::NodeXRole)
                 .toDouble(),
             320.0);
}

void GraphControllerTest::movesSelectedNodesAsOneCommand() {
    QNodeGraph::UI::GraphController controller;
    controller.addDemoNode(false);
    controller.addDemoNode(false);
    QVERIFY(controller.selectNodesInRect(0.0, 0.0, 600.0, 300.0));
    QVERIFY(controller.moveSelectedNodes(0, 120.0, 130.0));
    QCOMPARE(controller.index(0, 0)
                 .data(QNodeGraph::UI::GraphController::NodeXRole)
                 .toDouble(),
             120.0);
    QCOMPARE(controller.index(1, 0)
                 .data(QNodeGraph::UI::GraphController::NodeXRole)
                 .toDouble(),
             370.0);
    QVERIFY(controller.undo());
    QCOMPARE(controller.index(0, 0)
                 .data(QNodeGraph::UI::GraphController::NodeXRole)
                 .toDouble(),
             80.0);
    QCOMPARE(controller.index(1, 0)
                 .data(QNodeGraph::UI::GraphController::NodeXRole)
                 .toDouble(),
             330.0);
}

void GraphControllerTest::exposesNodeVisualRolesAndDisabledState() {
    QNodeGraph::UI::GraphController controller;
    QVERIFY(controller.addNodeType(QStringLiteral("edge_detect"), false));
    const auto index = controller.index(0, 0);
    QVERIFY(index.data(QNodeGraph::UI::GraphController::NodeIconRole)
                .toString()
                .size() > 0);
    QVERIFY(index.data(QNodeGraph::UI::GraphController::NodeIconSourceRole)
                .toString()
                .startsWith(QStringLiteral("qrc:/")));
    QVERIFY(index.data(QNodeGraph::UI::GraphController::NodeAccentRole)
                .toString()
                .startsWith(QStringLiteral("#")));
    QVERIFY(
        index.data(QNodeGraph::UI::GraphController::NodeEnabledRole).toBool());
    QVERIFY(controller.setNodeProperty(0, QStringLiteral("enabled"), false));
    QVERIFY(
        !index.data(QNodeGraph::UI::GraphController::NodeEnabledRole).toBool());
}

void GraphControllerTest::exposesNodeImagePreviewRoles() {
    QNodeGraph::UI::GraphController controller;
    controller.addNodeType(QStringLiteral("image_preview"), false);
    QVERIFY(controller.setNodePreview(
        0, QStringLiteral("image://pipeline/preview"), 64, 48, 4));
    const auto index = controller.index(0, 0);
    QCOMPARE(index.data(QNodeGraph::UI::GraphController::NodePreviewSourceRole)
                 .toString(),
             QStringLiteral("image://pipeline/preview"));
    QCOMPARE(index.data(QNodeGraph::UI::GraphController::NodePreviewWidthRole)
                 .toInt(),
             64);
    QCOMPARE(index.data(QNodeGraph::UI::GraphController::NodePreviewHeightRole)
                 .toInt(),
             48);
    QCOMPARE(
        index.data(QNodeGraph::UI::GraphController::NodePreviewChannelsRole)
            .toInt(),
        4);
    QVERIFY(controller.clearNodePreview(0));
    QVERIFY(index.data(QNodeGraph::UI::GraphController::NodePreviewSourceRole)
                .toString()
                .isEmpty());
}

void GraphControllerTest::exposesPortNameRoles() {
    QNodeGraph::UI::GraphController controller;
    controller.addNodeType(QStringLiteral("grayscale"), false);
    const auto index = controller.index(0, 0);
    const auto inputs =
        index.data(QNodeGraph::UI::GraphController::NodeInputPortsRole)
            .toList();
    const auto outputs =
        index.data(QNodeGraph::UI::GraphController::NodeOutputPortsRole)
            .toList();
    QCOMPARE(inputs.size(), qsizetype{1});
    QCOMPARE(outputs.size(), qsizetype{1});
    QCOMPARE(
        inputs.constFirst().toMap().value(QStringLiteral("name")).toString(),
        QStringLiteral("image"));
    QCOMPARE(outputs.constFirst()
                 .toMap()
                 .value(QStringLiteral("dataType"))
                 .toString(),
             QStringLiteral("image"));
}

void GraphControllerTest::reportsGraphBounds() {
    QNodeGraph::UI::GraphController controller;
    controller.addDemoNode(false);
    QVERIFY(controller.moveNode(0, 320.0, 150.0));
    const auto bounds = controller.graphBounds();
    QCOMPARE(bounds.value(QStringLiteral("x")).toDouble(), 320.0);
    QCOMPARE(bounds.value(QStringLiteral("y")).toDouble(), 150.0);
    QCOMPARE(bounds.value(QStringLiteral("width")).toDouble(), 180.0);
    QCOMPARE(bounds.value(QStringLiteral("height")).toDouble(), 108.0);
}

void GraphControllerTest::autoLayoutsConnectedNodes() {
    QNodeGraph::UI::GraphController controller;
    QVERIFY(controller.addNodeType(QStringLiteral("load_image"), false));
    QVERIFY(controller.addNodeType(QStringLiteral("grayscale"), true));
    QVERIFY(controller.addNodeType(QStringLiteral("edge_detect"), true));
    QVERIFY(controller.moveNode(0, 620.0, 480.0));
    QVERIFY(controller.moveNode(1, 120.0, 480.0));
    QVERIFY(controller.moveNode(2, 360.0, 480.0));

    QVERIFY(controller.autoLayout());
    QVERIFY(controller.index(0, 0)
                .data(QNodeGraph::UI::GraphController::NodeXRole)
                .toDouble() <
            controller.index(1, 0)
                .data(QNodeGraph::UI::GraphController::NodeXRole)
                .toDouble());
    QVERIFY(controller.index(1, 0)
                .data(QNodeGraph::UI::GraphController::NodeXRole)
                .toDouble() <
            controller.index(2, 0)
                .data(QNodeGraph::UI::GraphController::NodeXRole)
                .toDouble());
}

void GraphControllerTest::slicesConnectionsAlongGesture() {
    QNodeGraph::UI::GraphController controller;
    QVERIFY(controller.addNodeType(QStringLiteral("load_image"), false));
    QVERIFY(controller.addNodeType(QStringLiteral("grayscale"), true));
    QVERIFY(controller.addNodeType(QStringLiteral("edge_detect"), true));
    QCOMPARE(controller.connections().size(), qsizetype{2});

    const auto removed =
        controller.sliceConnections(295.0, 100.0, 295.0, 200.0);
    QCOMPARE(removed, 1);
    QCOMPARE(controller.connections().size(), qsizetype{1});
    QVERIFY(controller.undo());
    QCOMPARE(controller.connections().size(), qsizetype{2});
}

void GraphControllerTest::exposesGroupAndBackdropVisualRoles() {
    QNodeGraph::UI::GraphController controller;
    QVERIFY(controller.addNodeType(QStringLiteral("backdrop"), false));
    QVERIFY(controller.addNodeType(QStringLiteral("group"), false));

    const auto backdrop = controller.index(0, 0);
    QVERIFY(backdrop.data(QNodeGraph::UI::GraphController::NodeIsBackdropRole)
                .toBool());
    QVERIFY(backdrop.data(QNodeGraph::UI::GraphController::NodeWidthRole)
                .toDouble() > 180.0);
    QVERIFY(backdrop.data(QNodeGraph::UI::GraphController::NodeColorRole)
                .toString()
                .startsWith(QStringLiteral("#")));

    const auto group = controller.index(1, 0);
    QVERIFY(
        group.data(QNodeGraph::UI::GraphController::NodeIsGroupRole).toBool());
    QVERIFY(!group.data(QNodeGraph::UI::GraphController::NodeIsBackdropRole)
                 .toBool());
}

void GraphControllerTest::assignsNodeToGroupAndExposesProxyPorts() {
    QNodeGraph::UI::GraphController controller;
    QVERIFY(controller.addNodeType(QStringLiteral("group"), false));
    QVERIFY(controller.addNodeType(QStringLiteral("grayscale"), false));
    QCOMPARE(controller.index(0, 0)
                 .data(QNodeGraph::UI::GraphController::InputPortCountRole)
                 .toInt(),
             1);
    QCOMPARE(controller.index(0, 0)
                 .data(QNodeGraph::UI::GraphController::OutputPortCountRole)
                 .toInt(),
             1);

    QVERIFY(controller.assignNodeToGroup(1, 0));
    QCOMPARE(controller.index(1, 0)
                 .data(QNodeGraph::UI::GraphController::NodeGroupIdRole)
                 .toInt(),
             0);
    QVERIFY(controller.clearNodeGroup(1));
    QCOMPARE(controller.index(1, 0)
                 .data(QNodeGraph::UI::GraphController::NodeGroupIdRole)
                 .toInt(),
             -1);
}

void GraphControllerTest::movesNodeAndEmitsData() {
    QNodeGraph::UI::GraphController controller;
    controller.addDemoNode();
    QSignalSpy spy(&controller, &QAbstractItemModel::dataChanged);
    QVERIFY(controller.moveNode(0, 320.0, -20.0));
    QVERIFY(!spy.isEmpty());
    const auto index = controller.index(0, 0);
    QCOMPARE(index.data(QNodeGraph::UI::GraphController::NodeXRole).toDouble(),
             320.0);
    QCOMPARE(index.data(QNodeGraph::UI::GraphController::NodeYRole).toDouble(),
             -20.0);
}

void GraphControllerTest::clearsModel() {
    QNodeGraph::UI::GraphController controller;
    controller.addDemoNode();
    controller.addDemoNode();
    controller.clearGraph();
    QCOMPARE(controller.rowCount(), 0);
    QCOMPARE(controller.nodeCount(), 0);
}

void GraphControllerTest::emitsConnectionSignals() {
    QNodeGraph::UI::GraphController controller;
    controller.addDemoNode(false);
    controller.addDemoNode(false);
    QSignalSpy connectedSpy(&controller,
                            &QNodeGraph::UI::GraphController::nodeConnected);

    QVERIFY(controller.beginConnection(0));
    QVERIFY(controller.completeConnectionAt(330.0, 144.0));
    QCOMPARE(connectedSpy.count(), 1);
    const auto arguments = connectedSpy.takeFirst();
    QCOMPARE(arguments.at(0).toInt(), 0);
    QCOMPARE(arguments.at(1).toInt(), 1);
}

void GraphControllerTest::replacesExistingInputConnection() {
    QNodeGraph::UI::GraphController controller;
    controller.addDemoNode(false);
    controller.addDemoNode(false);
    controller.addDemoNode(false);
    QVERIFY(controller.beginConnection(2));
    QVERIFY(controller.completeConnectionAt(330.0, 144.0));
    QCOMPARE(controller.connections().size(), qsizetype{1});
    QCOMPARE(controller.connections()
                 .constFirst()
                 .toMap()
                 .value(QStringLiteral("outputRow"))
                 .toInt(),
             2);

    QSignalSpy disconnectedSpy(
        &controller, &QNodeGraph::UI::GraphController::nodeDisconnected);
    QVERIFY(controller.beginConnection(0));
    QVERIFY(controller.completeConnectionAt(330.0, 144.0));
    QCOMPARE(controller.connections().size(), qsizetype{1});
    QCOMPARE(controller.connections()
                 .constFirst()
                 .toMap()
                 .value(QStringLiteral("outputRow"))
                 .toInt(),
             0);
    QCOMPARE(disconnectedSpy.count(), 1);
}

void GraphControllerTest::blocksDeletionOfFixedNode() {
    QNodeGraph::UI::GraphController controller;
    QVERIFY(controller.addNodeType(QStringLiteral("load_image"), false));
    QVERIFY(controller.index(0, 0)
                .data(QNodeGraph::UI::GraphController::NodeFixedRole)
                .toBool());
    QVERIFY(!controller.deleteNode(0));
    QCOMPARE(controller.nodeCount(), 1);
    QVERIFY(!controller.lastError().isEmpty());
    QVERIFY(controller.setNodeFixed(0, false));
    QVERIFY(controller.deleteNode(0));
    QCOMPARE(controller.nodeCount(), 0);
}

void GraphControllerTest::resizesNodeWithUndo() {
    QNodeGraph::UI::GraphController controller;
    controller.addDemoNode(false);
    QVERIFY(controller.resizeNode(0, 320.0, 200.0));
    QCOMPARE(controller.index(0, 0)
                 .data(QNodeGraph::UI::GraphController::NodeWidthRole)
                 .toDouble(),
             320.0);
    QCOMPARE(controller.index(0, 0)
                 .data(QNodeGraph::UI::GraphController::NodeHeightRole)
                 .toDouble(),
             200.0);
    // 尺寸调整是单条原子命令：一次 undo 必须同时回退宽与高。旧实现用两条
    // SetPropertyCommand，需要按两次 Ctrl+Z 且中间会出现畸形的半改状态。
    QVERIFY(controller.undo());
    QCOMPARE(controller.index(0, 0)
                 .data(QNodeGraph::UI::GraphController::NodeWidthRole)
                 .toDouble(),
             180.0);
    QCOMPARE(controller.index(0, 0)
                 .data(QNodeGraph::UI::GraphController::NodeHeightRole)
                 .toDouble(),
             108.0);
    // 再次撤销应返回 false：addDemoNode 直接改文档、不入命令栈，栈里只有
    // resize 这一条命令。
    QVERIFY(!controller.undo());
}

void GraphControllerTest::resizesNodeFromTopLeftCorner() {
    QNodeGraph::UI::GraphController controller;
    controller.addDemoNode(false);
    // 从左/上侧拖拽会同时改变左上角坐标与尺寸，origin 必须与尺寸一起进入
    // 同一条撤销快照。addDemoNode 把首个节点放在 (80, 90)。
    QVERIFY(controller.resizeNodeAt(0, 40.0, 60.0, 300.0, 160.0));
    QCOMPARE(controller.index(0, 0)
                 .data(QNodeGraph::UI::GraphController::NodeWidthRole)
                 .toDouble(),
             300.0);
    QCOMPARE(controller.index(0, 0)
                 .data(QNodeGraph::UI::GraphController::NodeHeightRole)
                 .toDouble(),
             160.0);
    QCOMPARE(controller.index(0, 0)
                 .data(QNodeGraph::UI::GraphController::NodeXRole)
                 .toDouble(),
             40.0);
    QCOMPARE(controller.index(0, 0)
                 .data(QNodeGraph::UI::GraphController::NodeYRole)
                 .toDouble(),
             60.0);
    QVERIFY(controller.undo());
    QCOMPARE(controller.index(0, 0)
                 .data(QNodeGraph::UI::GraphController::NodeWidthRole)
                 .toDouble(),
             180.0);
    QCOMPARE(controller.index(0, 0)
                 .data(QNodeGraph::UI::GraphController::NodeXRole)
                 .toDouble(),
             80.0);
}

void GraphControllerTest::followsNodeSizeInConnectionGeometry() {
    QNodeGraph::UI::GraphController controller;
    controller.addDemoNode(true);
    controller.addDemoNode(true);
    // 放大首个节点后，连接几何必须跟随实际尺寸 —— 修复前这里恒用 180x108
    // 常量，导致改尺寸后连线端点不动。
    QVERIFY(controller.resizeNode(0, 400.0, 240.0));
    const auto connection = controller.connections().constFirst().toMap();
    // 首节点原点 (80, 90)：输出端口 x = 80 + 400，y = 90 + 240/2。
    QCOMPARE(connection.value(QStringLiteral("outputX")).toDouble(), 80.0);
    QCOMPARE(connection.value(QStringLiteral("outputWidth")).toDouble(), 400.0);
    QCOMPARE(connection.value(QStringLiteral("outputY")).toDouble(), 210.0);
}

void GraphControllerTest::declaresAppearanceFromDescriptor() {
    QNodeGraph::UI::GraphController controller;
    controller.addNodeType(QStringLiteral("blur"));
    // 外观元数据此前硬编码在三个自由函数里，现由描述符声明。
    QCOMPARE(controller.index(0, 0)
                 .data(QNodeGraph::UI::GraphController::NodeIconRole)
                 .toString(),
             QStringLiteral("BLR"));
    QCOMPARE(controller.index(0, 0)
                 .data(QNodeGraph::UI::GraphController::NodeAccentRole)
                 .toString(),
             QStringLiteral("#9568c7"));
    QVERIFY(controller.index(0, 0)
                .data(QNodeGraph::UI::GraphController::NodeIconSourceRole)
                .toString()
                .endsWith(QStringLiteral("blur.svg")));
}

void GraphControllerTest::usesDefaultSizeFromDescriptor() {
    QNodeGraph::UI::GraphController controller;
    // backdrop 描述符声明 560x300，取代 addNodeType 里按 typeId 的特判。
    controller.addNodeType(QStringLiteral("backdrop"));
    QCOMPARE(controller.index(0, 0)
                 .data(QNodeGraph::UI::GraphController::NodeWidthRole)
                 .toDouble(),
             560.0);
    QCOMPARE(controller.index(0, 0)
                 .data(QNodeGraph::UI::GraphController::NodeHeightRole)
                 .toDouble(),
             300.0);
    // 视觉角色也来自描述符，不再靠 type 字符串比较。
    QCOMPARE(controller.index(0, 0)
                 .data(QNodeGraph::UI::GraphController::NodeIsBackdropRole)
                 .toBool(),
             true);
}

void GraphControllerTest::rejectsDuplicateUniqueNode() {
    QNodeGraph::UI::GraphController controller;
    auto registry = QNodeGraph::Graph::NodeRegistry::withBuiltins();
    QNodeGraph::Graph::NodeDescriptor input;
    input.typeId = "viewer_image_input";
    input.displayName = "Viewport Image Input";
    input.fixed = true;
    input.unique = true;
    QVERIFY(registry.registerNode(std::move(input)));
    controller.setRegistry(std::move(registry));

    QVERIFY(controller.addNodeType(QStringLiteral("viewer_image_input")));
    QCOMPARE(controller.nodeCount(), 1);
    // 第二次添加必须被拒绝（唯一节点只允许一个实例）。
    QVERIFY(!controller.addNodeType(QStringLiteral("viewer_image_input")));
    QCOMPARE(controller.nodeCount(), 1);
    // 固定节点不可删除。
    QVERIFY(!controller.deleteNode(0));
    QCOMPARE(controller.nodeCount(), 1);
}

void GraphControllerTest::exposesPropertyEditorMetadata() {
    QNodeGraph::UI::GraphController controller;
    controller.addNodeType(QStringLiteral("blur"));
    const auto properties =
        controller.index(0, 0)
            .data(QNodeGraph::UI::GraphController::NodeOnNodePropertiesRole)
            .toList();
    bool sawRadius = false;
    for (const auto& entry : properties) {
        const auto map = entry.toMap();
        if (map.value(QStringLiteral("name")).toString() !=
            QStringLiteral("radius")) {
            continue;
        }
        sawRadius = true;
        // 节点内控件需要知道用滑块、以及取值范围。
        QCOMPARE(map.value(QStringLiteral("editor")).toString(),
                 QStringLiteral("slider"));
        QCOMPARE(map.value(QStringLiteral("minimum")).toDouble(), 1.0);
        QCOMPARE(map.value(QStringLiteral("maximum")).toDouble(), 50.0);
        QCOMPARE(map.value(QStringLiteral("valueType")).toString(),
                 QStringLiteral("int64"));
    }
    QVERIFY(sawRadius);
}

void GraphControllerTest::exposesExtendedPropertySchemaMetadata() {
    QNodeGraph::UI::GraphController controller;
    auto registry = QNodeGraph::Graph::NodeRegistry::withBuiltins();
    QNodeGraph::Graph::NodeDescriptor custom;
    custom.typeId = "custom.schema";
    custom.displayName = "Schema";
    custom.properties = {
        {.name = "kernel",
         .editor = QNodeGraph::Graph::PropertyEditor::Matrix,
         .valueKind = QNodeGraph::Graph::PropertyValueKind::Matrix,
         .visibility = QNodeGraph::Graph::PropertyVisibility::Advanced,
         .unit = "coefficients",
         .matrixRows = 3,
         .matrixCols = 3,
         .description = "Convolution kernel",
         .defaultValue = std::string("1,0,1")},
        {.name = "color",
         .editor = QNodeGraph::Graph::PropertyEditor::ColorPicker,
         .valueKind = QNodeGraph::Graph::PropertyValueKind::Color,
         .visibility = QNodeGraph::Graph::PropertyVisibility::Basic,
         .description = "Background color",
         .defaultValue = std::string("#112233")}};
    QVERIFY(registry.registerNode(std::move(custom)));
    controller.setRegistry(std::move(registry));
    QVERIFY(controller.addNodeType(QStringLiteral("custom.schema")));
    QVERIFY(controller.selectNode(0));

    const auto properties = controller.selectedProperties();
    const auto findProperty = [&properties](const QString& name) {
        return std::find_if(
            properties.cbegin(), properties.cend(),
            [&name](const QVariant& entry) {
                return entry.toMap().value(QStringLiteral("name")).toString() ==
                       name;
            });
    };
    const auto kernel = findProperty(QStringLiteral("kernel"));
    QVERIFY(kernel != properties.cend());
    QCOMPARE(kernel->toMap().value(QStringLiteral("editor")).toString(),
             QStringLiteral("matrix"));
    QCOMPARE(kernel->toMap().value(QStringLiteral("valueKind")).toString(),
             QStringLiteral("matrix"));
    QCOMPARE(kernel->toMap().value(QStringLiteral("visibility")).toString(),
             QStringLiteral("advanced"));
    QCOMPARE(kernel->toMap().value(QStringLiteral("matrixRows")).toInt(), 3);
    QCOMPARE(kernel->toMap().value(QStringLiteral("matrixCols")).toInt(), 3);
    QCOMPARE(kernel->toMap().value(QStringLiteral("description")).toString(),
             QStringLiteral("Convolution kernel"));

    const auto color = findProperty(QStringLiteral("color"));
    QVERIFY(color != properties.cend());
    QCOMPARE(color->toMap().value(QStringLiteral("editor")).toString(),
             QStringLiteral("color"));
    QCOMPARE(color->toMap().value(QStringLiteral("valueKind")).toString(),
             QStringLiteral("color"));
}

void GraphControllerTest::usesInjectedRegistryForCustomNodes() {
    QNodeGraph::UI::GraphController controller;
    auto registry = QNodeGraph::Graph::NodeRegistry::withBuiltins();
    QNodeGraph::Graph::NodeDescriptor custom;
    custom.typeId = "custom.invert";
    custom.displayName = "Custom Invert";
    custom.ports = {{"image", QNodeGraph::Core::PortDirection::Input,
                     QNodeGraph::Core::PortDataType::Image, false},
                    {"image", QNodeGraph::Core::PortDirection::Output,
                     QNodeGraph::Core::PortDataType::Image, false}};
    custom.iconGlyph = "INV";
    custom.accent = "#00aa88";
    custom.defaultWidth = 220.0;
    custom.defaultHeight = 130.0;
    QVERIFY(registry.registerNode(std::move(custom)));
    controller.setRegistry(std::move(registry));

    QVERIFY(controller.addNodeType(QStringLiteral("custom.invert")));
    QCOMPARE(controller.index(0, 0)
                 .data(QNodeGraph::UI::GraphController::NodeIconRole)
                 .toString(),
             QStringLiteral("INV"));
    QCOMPARE(controller.index(0, 0)
                 .data(QNodeGraph::UI::GraphController::NodeAccentRole)
                 .toString(),
             QStringLiteral("#00aa88"));
    QCOMPARE(controller.index(0, 0)
                 .data(QNodeGraph::UI::GraphController::NodeWidthRole)
                 .toDouble(),
             220.0);
}

void GraphControllerTest::marksPrunedNodesAsIneffective() {
    QNodeGraph::UI::GraphController controller;
    // 节点 0/1 是 demo（in+out），节点 2 是 image_preview（只有 in，是汇点）。
    controller.addNodeType(QStringLiteral("demo"));
    controller.addNodeType(QStringLiteral("demo"));
    controller.addNodeType(QStringLiteral("image_preview"));

    const auto effectiveRole =
        QNodeGraph::UI::GraphController::NodeEffectiveRole;
    // 未接线时：两个 demo 的输出无人使用 → 剪枝；preview 自身是汇点 → 有效。
    QCOMPARE(controller.index(0, 0).data(effectiveRole).toBool(), false);
    QCOMPARE(controller.index(1, 0).data(effectiveRole).toBool(), false);
    QCOMPARE(controller.index(2, 0).data(effectiveRole).toBool(), true);

    // demo0 → demo1 → preview 全链打通后，三者都有效。
    // 端口命中测试走 portAt(row 顺序首个命中)，故按实际节点坐标探测：
    // addNodeType 把第 row 个节点放在 (80 + (row%3)*250, 90 + (row/3)*180)。
    // 端口命中的是节点左侧中点（node.y + height/2），不是节点原点。
    const auto inputX = [](int row) { return 80.0 + (row % 3) * 250.0; };
    const auto inputY = [](int row) {
        return 90.0 + (row / 3) * 180.0 + 108.0 / 2.0;
    };
    QVERIFY(controller.beginConnection(0));
    QVERIFY(controller.completeConnectionAt(inputX(1), inputY(1)));
    QVERIFY(controller.beginConnection(1));
    QVERIFY(controller.completeConnectionAt(inputX(2), inputY(2)));
    QCOMPARE(controller.connections().size(), qsizetype{2});
    QCOMPARE(controller.index(0, 0).data(effectiveRole).toBool(), true);
    QCOMPARE(controller.index(1, 0).data(effectiveRole).toBool(), true);

    // 断开 demo0 → demo1，让 demo1 及其下游不可达 demo0，demo0 重新被剪枝。
    QVERIFY(controller.removeConnectionBetween(0, 1));
    QCOMPARE(controller.isNodeEffective(0), false);
}

QTEST_MAIN(GraphControllerTest)
#include "graph_controller_test.moc"
