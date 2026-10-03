#include <QNodeGraph/Lib/UI/QtQuick/graph_connections_item.h>

#include <QtTest/QtTest>

class GraphConnectionsItemTest final : public QObject {
    Q_OBJECT

private slots:
    void exposesSceneGraphProperties();
};

void GraphConnectionsItemTest::exposesSceneGraphProperties() {
    QNodeGraph::UI::GraphConnectionsItem item;
    QVERIFY(item.flags().testFlag(QQuickItem::ItemHasContents));
    item.setZoomFactor(1.5);
    QCOMPARE(item.zoomFactor(), 1.5);
    item.setLayoutMode(1);
    QCOMPARE(item.layoutMode(), 1);
    item.setLayoutMode(99);
    QCOMPARE(item.layoutMode(), 2);
    item.setPanOffset(QPointF(12.0, 18.0));
    QCOMPARE(item.panOffset(), QPointF(12.0, 18.0));
    item.setConnections({QVariantMap{{QStringLiteral("outputX"), 20.0},
                                     {QStringLiteral("outputY"), 30.0},
                                     {QStringLiteral("inputX"), 120.0},
                                     {QStringLiteral("inputY"), 90.0},
                                     {QStringLiteral("outputWidth"), 180.0}}});
    QCOMPARE(item.connections().size(), qsizetype{1});
}

QTEST_MAIN(GraphConnectionsItemTest)
#include "graph_connections_item_test.moc"
