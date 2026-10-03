#include <QNodeGraph/Lib/Core/graph_document.h>

#include <QtTest/QtTest>

#include <cstdint>

class GraphDocumentTest final : public QObject {
    Q_OBJECT

private slots:
    void startsEmpty();
    void rejectsDuplicateIdsAndMissingPorts();
    void rejectsInvalidDirectionAndDuplicateConnection();
    void createsNodeAndPorts();
    void rejectsIncompatiblePortTypes();
    void rejectsSecondConnectionToSingleInput();
    void rejectsCycles();
    void storesNodePositionAndProperties();
    void clearRemovesNodesAndConnections();
};

void GraphDocumentTest::startsEmpty() {
    const QNodeGraph::Core::GraphDocument document;
    QCOMPARE(document.name(), std::string("Untitled"));
    QCOMPARE(document.nodeCount(), std::size_t{0});
}

void GraphDocumentTest::rejectsDuplicateIdsAndMissingPorts() {
    QNodeGraph::Core::GraphDocument document;
    QVERIFY(document.addNode("node", "A", 42).has_value());
    const auto duplicate = document.addNode("node", "B", 42);
    QVERIFY(!duplicate.has_value());
    QCOMPARE(duplicate.error().code,
             QNodeGraph::Core::GraphErrorCode::DuplicateNodeId);

    const auto missingPort = document.connect(100, 101);
    QVERIFY(!missingPort.has_value());
    QCOMPARE(missingPort.error().code,
             QNodeGraph::Core::GraphErrorCode::PortNotFound);
}

void GraphDocumentTest::rejectsInvalidDirectionAndDuplicateConnection() {
    QNodeGraph::Core::GraphDocument document;
    const auto source = document.addNode("source", "Source");
    const auto sink = document.addNode("sink", "Sink");
    const auto output = document.addPort(*source, "out",
        QNodeGraph::Core::PortDirection::Output,
        QNodeGraph::Core::PortDataType::Image);
    const auto input = document.addPort(*sink, "in",
        QNodeGraph::Core::PortDirection::Input,
        QNodeGraph::Core::PortDataType::Image);

    const auto invalid = document.connect(*input, *output);
    QVERIFY(!invalid.has_value());
    QCOMPARE(invalid.error().code,
             QNodeGraph::Core::GraphErrorCode::InvalidPortDirection);
    QVERIFY(document.connect(*output, *input).has_value());
    const auto duplicate = document.connect(*output, *input);
    QVERIFY(!duplicate.has_value());
    QCOMPARE(duplicate.error().code,
             QNodeGraph::Core::GraphErrorCode::DuplicateConnection);
}

void GraphDocumentTest::createsNodeAndPorts() {
    QNodeGraph::Core::GraphDocument document("Test");
    const auto source = document.addNode("source", "Source");
    const auto sink = document.addNode("sink", "Sink");
    QVERIFY(source.has_value());
    QVERIFY(sink.has_value());

    const auto output = document.addPort(*source, "out",
        QNodeGraph::Core::PortDirection::Output,
        QNodeGraph::Core::PortDataType::Image);
    const auto input = document.addPort(*sink, "in",
        QNodeGraph::Core::PortDirection::Input,
        QNodeGraph::Core::PortDataType::Image);
    QVERIFY(output.has_value());
    QVERIFY(input.has_value());
    QVERIFY(document.connect(*output, *input).has_value());
    QCOMPARE(document.nodeCount(), std::size_t{2});
    QCOMPARE(document.connectionCount(), std::size_t{1});
}

void GraphDocumentTest::rejectsIncompatiblePortTypes() {
    QNodeGraph::Core::GraphDocument document;
    const auto source = document.addNode("source", "Source");
    const auto sink = document.addNode("sink", "Sink");
    const auto output = document.addPort(*source, "out",
        QNodeGraph::Core::PortDirection::Output,
        QNodeGraph::Core::PortDataType::Image);
    const auto input = document.addPort(*sink, "in",
        QNodeGraph::Core::PortDirection::Input,
        QNodeGraph::Core::PortDataType::Number);

    const auto result = document.connect(*output, *input);
    QVERIFY(!result.has_value());
    QCOMPARE(result.error().code,
             QNodeGraph::Core::GraphErrorCode::IncompatiblePortTypes);
}

void GraphDocumentTest::rejectsSecondConnectionToSingleInput() {
    QNodeGraph::Core::GraphDocument document;
    const auto sourceA = document.addNode("source", "Source A");
    const auto sourceB = document.addNode("source", "Source B");
    const auto sink = document.addNode("sink", "Sink");
    const auto outputA = document.addPort(*sourceA, "out",
        QNodeGraph::Core::PortDirection::Output,
        QNodeGraph::Core::PortDataType::Image);
    const auto outputB = document.addPort(*sourceB, "out",
        QNodeGraph::Core::PortDirection::Output,
        QNodeGraph::Core::PortDataType::Image);
    const auto input = document.addPort(*sink, "in",
        QNodeGraph::Core::PortDirection::Input,
        QNodeGraph::Core::PortDataType::Image);

    QVERIFY(document.connect(*outputA, *input).has_value());
    const auto result = document.connect(*outputB, *input);
    QVERIFY(!result.has_value());
    QCOMPARE(result.error().code,
             QNodeGraph::Core::GraphErrorCode::InputAlreadyConnected);
}

void GraphDocumentTest::rejectsCycles() {
    QNodeGraph::Core::GraphDocument document;
    const auto nodeA = document.addNode("node", "A");
    const auto nodeB = document.addNode("node", "B");
    const auto aOut = document.addPort(*nodeA, "out",
        QNodeGraph::Core::PortDirection::Output,
        QNodeGraph::Core::PortDataType::Any);
    const auto aIn = document.addPort(*nodeA, "in",
        QNodeGraph::Core::PortDirection::Input,
        QNodeGraph::Core::PortDataType::Any);
    const auto bOut = document.addPort(*nodeB, "out",
        QNodeGraph::Core::PortDirection::Output,
        QNodeGraph::Core::PortDataType::Any);
    const auto bIn = document.addPort(*nodeB, "in",
        QNodeGraph::Core::PortDirection::Input,
        QNodeGraph::Core::PortDataType::Any);

    QVERIFY(document.connect(*aOut, *bIn).has_value());
    const auto result = document.connect(*bOut, *aIn);
    QVERIFY(!result.has_value());
    QCOMPARE(result.error().code, QNodeGraph::Core::GraphErrorCode::CycleDetected);
}

void GraphDocumentTest::storesNodePositionAndProperties() {
    QNodeGraph::Core::GraphDocument document;
    const auto node = document.addNode("filter", "Filter");
    QVERIFY(node.has_value());
    QVERIFY(document.setNodePosition(*node, {120.0, -40.5}).has_value());
    QVERIFY(document.setProperty(*node, "strength", 0.75).has_value());

    const auto* stored = document.node(*node);
    QVERIFY(stored != nullptr);
    QCOMPARE(stored->position.x, 120.0);
    QCOMPARE(stored->position.y, -40.5);
    QCOMPARE(std::get<double>(stored->properties.at("strength")), 0.75);
}

void GraphDocumentTest::clearRemovesNodesAndConnections() {
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
    document.clear();
    QCOMPARE(document.nodeCount(), std::size_t{0});
    QCOMPARE(document.connectionCount(), std::size_t{0});
}

QTEST_MAIN(GraphDocumentTest)
#include "graph_document_test.moc"
