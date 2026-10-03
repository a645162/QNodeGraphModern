#include <QNodeGraph/Lib/Core/graph_document.h>

#include <QtTest/QtTest>

class GraphDocumentTest final : public QObject {
    Q_OBJECT

private slots:
    void startsEmpty();
    void countsAddedNodes();
    void clearRemovesNodes();
};

void GraphDocumentTest::startsEmpty() {
    const QNodeGraph::Core::GraphDocument document;
    QCOMPARE(document.name(), std::string("Untitled"));
    QCOMPARE(document.nodeCount(), std::size_t{0});
}

void GraphDocumentTest::countsAddedNodes() {
    QNodeGraph::Core::GraphDocument document("Test");
    document.addNode();
    document.addNode();
    QCOMPARE(document.name(), std::string("Test"));
    QCOMPARE(document.nodeCount(), std::size_t{2});
}

void GraphDocumentTest::clearRemovesNodes() {
    QNodeGraph::Core::GraphDocument document;
    document.addNode();
    document.clear();
    QCOMPARE(document.nodeCount(), std::size_t{0});
}

QTEST_MAIN(GraphDocumentTest)
#include "graph_document_test.moc"

