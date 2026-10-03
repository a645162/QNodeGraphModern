#include <QNodeGraph/Lib/Core/graph_commands.h>

#include <QtTest/QtTest>

class GraphCommandsTest final : public QObject {
    Q_OBJECT

private slots:
    void createCommandSupportsUndoAndRedo();
    void deleteCommandRestoresNodeAndConnections();
    void moveCommandSupportsUndoAndRedo();
    void moveNodesCommandSupportsUndoAndRedo();
    void connectionCommandSupportsUndoAndRedo();
    void propertyCommandSupportsUndoAndRedo();
};

void GraphCommandsTest::createCommandSupportsUndoAndRedo() {
    QNodeGraph::Core::GraphDocument document;
    QNodeGraph::Core::GraphCommandStack stack;
    QVERIFY(stack.execute(std::make_unique<QNodeGraph::Core::CreateNodeCommand>(
                             "blur", "Blur"), document)
                .has_value());
    QCOMPARE(document.nodeCount(), std::size_t{1});
    QVERIFY(stack.undo(document).has_value());
    QCOMPARE(document.nodeCount(), std::size_t{0});
    QVERIFY(stack.redo(document).has_value());
    QCOMPARE(document.nodeCount(), std::size_t{1});
}

void GraphCommandsTest::deleteCommandRestoresNodeAndConnections() {
    QNodeGraph::Core::GraphDocument document;
    const auto source = document.addNode("source", "Source");
    const auto sink = document.addNode("sink", "Sink");
    const auto output = document.addPort(*source, "out",
        QNodeGraph::Core::PortDirection::Output,
        QNodeGraph::Core::PortDataType::Image);
    const auto input = document.addPort(*sink, "in",
        QNodeGraph::Core::PortDirection::Input,
        QNodeGraph::Core::PortDataType::Image);
    QVERIFY(document.connect(*output, *input).has_value());

    QNodeGraph::Core::GraphCommandStack stack;
    QVERIFY(stack.execute(std::make_unique<QNodeGraph::Core::DeleteNodeCommand>(*sink),
                         document)
                .has_value());
    QCOMPARE(document.nodeCount(), std::size_t{1});
    QCOMPARE(document.connectionCount(), std::size_t{0});
    QVERIFY(stack.undo(document).has_value());
    QCOMPARE(document.nodeCount(), std::size_t{2});
    QCOMPARE(document.connectionCount(), std::size_t{1});
    QVERIFY(stack.redo(document).has_value());
    QCOMPARE(document.nodeCount(), std::size_t{1});
}

void GraphCommandsTest::moveCommandSupportsUndoAndRedo() {
    QNodeGraph::Core::GraphDocument document;
    const auto node = document.addNode("node", "Node");
    QVERIFY(node.has_value());
    QNodeGraph::Core::GraphCommandStack stack;

    QVERIFY(stack.execute(std::make_unique<QNodeGraph::Core::MoveNodeCommand>(
                             *node, QNodeGraph::Core::Point{80.0, 40.0}),
                         document)
                .has_value());
    QCOMPARE(document.node(*node)->position.x, 80.0);
    QVERIFY(stack.undo(document).has_value());
    QCOMPARE(document.node(*node)->position.x, 0.0);
    QVERIFY(stack.redo(document).has_value());
    QCOMPARE(document.node(*node)->position.y, 40.0);
}

void GraphCommandsTest::moveNodesCommandSupportsUndoAndRedo() {
    QNodeGraph::Core::GraphDocument document;
    const auto first = document.addNode("node", "First");
    const auto second = document.addNode("node", "Second");
    QNodeGraph::Core::GraphCommandStack stack;
    QVERIFY(stack.execute(std::make_unique<
                             QNodeGraph::Core::MoveNodesCommand>(
                             std::vector<std::pair<
                                 QNodeGraph::Core::NodeId,
                                 QNodeGraph::Core::Point>>{
                                 {*first, {10.0, 20.0}},
                                 {*second, {30.0, 40.0}}}),
                         document)
                .has_value());
    QCOMPARE(document.node(*first)->position.x, 10.0);
    QCOMPARE(document.node(*second)->position.y, 40.0);
    QVERIFY(stack.undo(document).has_value());
    QCOMPARE(document.node(*first)->position.x, 0.0);
    QCOMPARE(document.node(*second)->position.y, 0.0);
    QVERIFY(stack.redo(document).has_value());
    QCOMPARE(document.node(*first)->position.y, 20.0);
}

void GraphCommandsTest::connectionCommandSupportsUndoAndRedo() {
    QNodeGraph::Core::GraphDocument document;
    const auto source = document.addNode("source", "Source");
    const auto sink = document.addNode("sink", "Sink");
    const auto output = document.addPort(*source, "out",
        QNodeGraph::Core::PortDirection::Output,
        QNodeGraph::Core::PortDataType::Image);
    const auto input = document.addPort(*sink, "in",
        QNodeGraph::Core::PortDirection::Input,
        QNodeGraph::Core::PortDataType::Image);
    QNodeGraph::Core::GraphCommandStack stack;

    QVERIFY(stack.execute(std::make_unique<QNodeGraph::Core::ConnectPortsCommand>(
                             *output, *input),
                         document)
                .has_value());
    QCOMPARE(document.connectionCount(), std::size_t{1});
    QVERIFY(stack.undo(document).has_value());
    QCOMPARE(document.connectionCount(), std::size_t{0});
    QVERIFY(stack.redo(document).has_value());
    QCOMPARE(document.connectionCount(), std::size_t{1});
}

void GraphCommandsTest::propertyCommandSupportsUndoAndRedo() {
    QNodeGraph::Core::GraphDocument document;
    const auto node = document.addNode("node", "Node");
    QVERIFY(node.has_value());
    QNodeGraph::Core::GraphCommandStack stack;

    QVERIFY(stack.execute(std::make_unique<QNodeGraph::Core::SetPropertyCommand>(
                             *node, "amount", 0.75),
                         document)
                .has_value());
    QCOMPARE(std::get<double>(document.node(*node)->properties.at("amount")),
             0.75);
    QVERIFY(stack.undo(document).has_value());
    QVERIFY(document.node(*node)->properties.empty());
    QVERIFY(stack.redo(document).has_value());
    QCOMPARE(std::get<double>(document.node(*node)->properties.at("amount")),
             0.75);
}

QTEST_MAIN(GraphCommandsTest)
#include "graph_commands_test.moc"
