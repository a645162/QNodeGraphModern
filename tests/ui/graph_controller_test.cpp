#include <QNodeGraph/Lib/UI/QtQuick/graph_controller.h>

#include <QtTest/QtTest>

#include <algorithm>

class GraphControllerTest final : public QObject {
    Q_OBJECT

private slots:
    void startsWithEmptyModel();
    void addsNodeWithRoles();
    void connectsAdjacentDemoNodes();
    void connectsNodesThroughPortInteraction();
    void reportsPortHitTestingResults();
    void exposesAndClearsConnectionPreview();
    void selectsNodeAndListsProperties();
    void editsSelectedNodeProperty();
    void addsRegisteredNodeType();
    void movesNodeWithUndoAndRedo();
    void exposesNodeVisualRolesAndDisabledState();
    void autoLayoutsConnectedNodes();
    void slicesConnectionsAlongGesture();
    void exposesGroupAndBackdropVisualRoles();
    void movesNodeAndEmitsData();
    void clearsModel();
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
    QCOMPARE(index.data(QNodeGraph::UI::GraphController::NodeNameRole).toString(),
             QStringLiteral("Demo Node 1"));
    QCOMPARE(index.data(QNodeGraph::UI::GraphController::NodeTypeRole).toString(),
             QStringLiteral("demo"));
    QCOMPARE(index.data(QNodeGraph::UI::GraphController::InputPortCountRole).toInt(),
             1);
    QCOMPARE(index.data(QNodeGraph::UI::GraphController::OutputPortCountRole).toInt(),
             1);
}

void GraphControllerTest::connectsAdjacentDemoNodes() {
    QNodeGraph::UI::GraphController controller;
    controller.addDemoNode();
    controller.addDemoNode();
    QCOMPARE(controller.connections().size(), qsizetype{1});
    const auto connection = controller.connections().constFirst().toMap();
    QCOMPARE(connection.value(QStringLiteral("outputRow")).toInt(), 0);
    QCOMPARE(connection.value(QStringLiteral("inputRow")).toInt(), 1);
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
    QCOMPARE(index.data(QNodeGraph::UI::GraphController::NodeTypeRole)
                 .toString(),
             QStringLiteral("grayscale"));
    QCOMPARE(index.data(QNodeGraph::UI::GraphController::InputPortCountRole)
                 .toInt(),
             1);
    QCOMPARE(index.data(QNodeGraph::UI::GraphController::OutputPortCountRole)
                 .toInt(),
             1);
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

void GraphControllerTest::exposesNodeVisualRolesAndDisabledState() {
    QNodeGraph::UI::GraphController controller;
    QVERIFY(controller.addNodeType(QStringLiteral("edge_detect"), false));
    const auto index = controller.index(0, 0);
    QVERIFY(index.data(QNodeGraph::UI::GraphController::NodeIconRole)
                .toString()
                .size() > 0);
    QVERIFY(index.data(QNodeGraph::UI::GraphController::NodeAccentRole)
                .toString()
                .startsWith(QStringLiteral("#")));
    QVERIFY(index.data(QNodeGraph::UI::GraphController::NodeEnabledRole)
                .toBool());
    QVERIFY(controller.setNodeProperty(0, QStringLiteral("enabled"), false));
    QVERIFY(!index.data(QNodeGraph::UI::GraphController::NodeEnabledRole)
                 .toBool());
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

    const auto removed = controller.sliceConnections(295.0, 100.0, 295.0,
                                                     200.0);
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
    QVERIFY(backdrop.data(
                         QNodeGraph::UI::GraphController::NodeIsBackdropRole)
                .toBool());
    QVERIFY(backdrop.data(QNodeGraph::UI::GraphController::NodeWidthRole)
                .toDouble() > 180.0);
    QVERIFY(backdrop.data(QNodeGraph::UI::GraphController::NodeColorRole)
                .toString()
                .startsWith(QStringLiteral("#")));

    const auto group = controller.index(1, 0);
    QVERIFY(group.data(QNodeGraph::UI::GraphController::NodeIsGroupRole)
                .toBool());
    QVERIFY(!group.data(QNodeGraph::UI::GraphController::NodeIsBackdropRole)
                 .toBool());
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

QTEST_MAIN(GraphControllerTest)
#include "graph_controller_test.moc"
