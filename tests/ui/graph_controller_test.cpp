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
