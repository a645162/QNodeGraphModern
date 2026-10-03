#include <QNodeGraph/Lib/UI/QtQuick/graph_controller.h>

#include <QtTest/QtTest>

class GraphControllerTest final : public QObject {
    Q_OBJECT

private slots:
    void startsWithEmptyModel();
    void addsNodeWithRoles();
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

