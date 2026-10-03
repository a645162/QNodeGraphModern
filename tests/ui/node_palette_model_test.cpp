#include <QNodeGraph/Lib/UI/QtQuick/node_palette_model.h>

#include <QtTest/QtTest>

class NodePaletteModelTest final : public QObject {
    Q_OBJECT

private slots:
    void exposesBuiltInImageNodes();
    void filtersNodesBySearchText();
};

void NodePaletteModelTest::exposesBuiltInImageNodes() {
    const QNodeGraph::UI::NodePaletteModel model;
    QVERIFY(model.rowCount() >= 6);
    const auto first = model.index(0, 0);
    QVERIFY(first.data(QNodeGraph::UI::NodePaletteModel::NodeTypeRole)
                .toString()
                .size() > 0);
    QVERIFY(first.data(QNodeGraph::UI::NodePaletteModel::NodeNameRole)
                .toString()
                .size() > 0);
}

void NodePaletteModelTest::filtersNodesBySearchText() {
    QNodeGraph::UI::NodePaletteModel model;
    model.setFilter(QStringLiteral("edge"));
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.index(0, 0)
                 .data(QNodeGraph::UI::NodePaletteModel::NodeTypeRole)
                 .toString(),
             QStringLiteral("edge_detect"));

    model.setFilter({});
    QVERIFY(model.rowCount() >= 6);
}

QTEST_MAIN(NodePaletteModelTest)
#include "node_palette_model_test.moc"
