#include <QNodeGraph/Lib/UI/QtQuick/node_palette_model.h>

#include <QtTest/QtTest>

class NodePaletteModelTest final : public QObject {
    Q_OBJECT

private slots:
    void exposesBuiltInImageNodes();
    void filtersNodesBySearchText();
    void hidesFixedNodesWhenRequested();
    void usesInjectedRegistry();
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

void NodePaletteModelTest::hidesFixedNodesWhenRequested() {
    QNodeGraph::UI::NodePaletteModel model;
    const auto defaultRows = model.rowCount();
    model.setHideFixedNodes(true);
    // load_image / save_image 声明了 deletable=false，属于固定节点，隐藏后
    // 行数必须减少。
    QVERIFY(model.rowCount() < defaultRows);
    for (int row = 0; row < model.rowCount(); ++row) {
        const auto type = model.index(row, 0)
                              .data(QNodeGraph::UI::NodePaletteModel::NodeTypeRole)
                              .toString();
        QVERIFY(type != QStringLiteral("load_image"));
        QVERIFY(type != QStringLiteral("save_image"));
    }
}

void NodePaletteModelTest::usesInjectedRegistry() {
    // 宿主注册自定义类型后，调色板必须能列出它 —— 注入前两个模型各持私有
    // 注册表，会出现「画布认识、调色板不显示」的不一致。
    auto registry = QNodeGraph::Graph::NodeRegistry::withBuiltins();
    QNodeGraph::Graph::NodeDescriptor custom;
    custom.typeId = "custom.threshold";
    custom.displayName = "Custom Threshold";
    custom.iconGlyph = "THR";
    custom.accent = "#ff8800";
    QVERIFY(registry.registerNode(std::move(custom)));

    QNodeGraph::UI::NodePaletteModel model;
    model.setRegistry(std::move(registry));
    model.setFilter(QStringLiteral("custom.threshold"));
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.index(0, 0)
                 .data(QNodeGraph::UI::NodePaletteModel::NodeTypeRole)
                 .toString(),
             QStringLiteral("custom.threshold"));
}

QTEST_MAIN(NodePaletteModelTest)
#include "node_palette_model_test.moc"