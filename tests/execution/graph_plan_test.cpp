#include <QNodeGraph/Lib/Execution/graph_plan.h>

#include <QtTest/QtTest>

#include <algorithm>

// 求值计划与脏传播的单元测试。
//
// 覆盖：拓扑序稳定性、汇点反向可达性、下游闭包、剪枝、参数带出，以及
// 「上游变化不影响上游本身」这条「只重算下游」的依据。
class GraphPlanTest final : public QObject {
    Q_OBJECT

private slots:
    void topologicallyOrdersLinearChain();
    void marksUnreachableNodesAsPruned();
    void computesDownstreamClosure();
    void carriesNodePropertiesAsParams();
    void excludesDisplayAndUiKeysFromParams();
    void keepsPlanStableAcrossRebuilds();

private:
    // 建图辅助：load_image 只有输出端口，image_preview 只有输入端口（汇点）。
    struct Fixture {
        QNodeGraph::Core::GraphDocument document{"test"};
        QNodeGraph::Core::NodeId addNode(const std::string& type,
                                         const std::string& name) {
            const auto node = document.addNode(type, name);
            Q_ASSERT(node.has_value());
            return *node;
        }
        QNodeGraph::Core::PortId addPort(QNodeGraph::Core::NodeId node,
                                         const std::string& name,
                                         QNodeGraph::Core::PortDirection dir) {
            const auto port = document.addPort(
                node, name, dir, QNodeGraph::Core::PortDataType::Image);
            Q_ASSERT(port.has_value());
            return *port;
        }
        void connect(QNodeGraph::Core::PortId out,
                     QNodeGraph::Core::PortId in) {
            Q_ASSERT(document.connect(out, in).has_value());
        }
    };
};

void GraphPlanTest::topologicallyOrdersLinearChain() {
    Fixture fixture;
    const auto source = fixture.addNode("load_image", "Source");
    const auto middle = fixture.addNode("blur", "Middle");
    const auto sink = fixture.addNode("image_preview", "Sink");
    const auto sourceOut = fixture.addPort(
        source, "image", QNodeGraph::Core::PortDirection::Output);
    const auto middleIn = fixture.addPort(
        middle, "image", QNodeGraph::Core::PortDirection::Input);
    const auto middleOut = fixture.addPort(
        middle, "image", QNodeGraph::Core::PortDirection::Output);
    const auto sinkIn = fixture.addPort(
        sink, "image", QNodeGraph::Core::PortDirection::Input);
    fixture.connect(sourceOut, middleIn);
    fixture.connect(middleOut, sinkIn);

    const auto plan =
        QNodeGraph::Execution::GraphPlanBuilder::build(fixture.document);
    QCOMPARE(plan.steps.size(), std::size_t{3});
    QCOMPARE(plan.prunedNodes.size(), std::size_t{0});
    // 上游必须排在下游之前。
    QCOMPARE(plan.steps[0].node, source);
    QCOMPARE(plan.steps[1].node, middle);
    QCOMPARE(plan.steps[2].node, sink);
    QCOMPARE(QString::fromStdString(plan.steps[1].operatorKey),
             QStringLiteral("blur"));
}

void GraphPlanTest::marksUnreachableNodesAsPruned() {
    Fixture fixture;
    const auto orphan = fixture.addNode("blur", "Orphan");
    fixture.addPort(orphan, "image", QNodeGraph::Core::PortDirection::Input);
    fixture.addPort(orphan, "image", QNodeGraph::Core::PortDirection::Output);
    const auto sink = fixture.addNode("image_preview", "Sink");
    fixture.addPort(sink, "image", QNodeGraph::Core::PortDirection::Input);

    const auto plan =
        QNodeGraph::Execution::GraphPlanBuilder::build(fixture.document);
    // orphan 的输出无人使用 → 剪枝，且不进计划。
    QCOMPARE(plan.prunedNodes.size(), std::size_t{1});
    QCOMPARE(plan.prunedNodes[0], orphan);
    QCOMPARE(plan.steps.size(), std::size_t{1});
    QCOMPARE(plan.steps[0].node, sink);
}

void GraphPlanTest::computesDownstreamClosure() {
    Fixture fixture;
    const auto first = fixture.addNode("load_image", "First");
    const auto second = fixture.addNode("blur", "Second");
    const auto third = fixture.addNode("image_preview", "Third");
    const auto branch = fixture.addNode("image_preview", "Branch");
    const auto firstOut = fixture.addPort(
        first, "image", QNodeGraph::Core::PortDirection::Output);
    const auto secondIn = fixture.addPort(
        second, "image", QNodeGraph::Core::PortDirection::Input);
    const auto secondOut = fixture.addPort(
        second, "image", QNodeGraph::Core::PortDirection::Output);
    const auto thirdIn = fixture.addPort(
        third, "image", QNodeGraph::Core::PortDirection::Input);
    const auto branchIn = fixture.addPort(
        branch, "image", QNodeGraph::Core::PortDirection::Input);
    fixture.connect(firstOut, secondIn);
    fixture.connect(secondOut, thirdIn);
    fixture.connect(secondOut, branchIn);

    const auto closure =
        QNodeGraph::Execution::GraphPlanBuilder::downstreamClosure(
            fixture.document, second);
    // 含自身，且两路分支都在内（out 可多路输出）。
    QCOMPARE(closure.size(), std::size_t{3});
    QVERIFY(std::find(closure.begin(), closure.end(), second) != closure.end());
    QVERIFY(std::find(closure.begin(), closure.end(), third) != closure.end());
    QVERIFY(std::find(closure.begin(), closure.end(), branch) != closure.end());
    // 上游不受影响 —— 这正是「只重算下游」的依据。
    QVERIFY(std::find(closure.begin(), closure.end(), first) == closure.end());
}

void GraphPlanTest::carriesNodePropertiesAsParams() {
    Fixture fixture;
    const auto node = fixture.addNode("image_preview", "Sink");
    fixture.addPort(node, "image", QNodeGraph::Core::PortDirection::Input);
    QVERIFY(fixture.document
                .setProperty(node, "threshold", std::int64_t{128})
                .has_value());

    const auto plan =
        QNodeGraph::Execution::GraphPlanBuilder::build(fixture.document);
    QCOMPARE(plan.steps.size(), std::size_t{1});
    const auto iterator = plan.steps[0].params.find("threshold");
    QVERIFY(iterator != plan.steps[0].params.end());
    QVERIFY(std::holds_alternative<std::int64_t>(iterator->second));
    QCOMPARE(std::get<std::int64_t>(iterator->second), std::int64_t{128});
}

void GraphPlanTest::excludesDisplayAndUiKeysFromParams() {
    Fixture fixture;
    const auto node = fixture.addNode("image_preview", "Sink");
    fixture.addPort(node, "image", QNodeGraph::Core::PortDirection::Input);
    QVERIFY(fixture.document
                .setProperty(node, "radius.display", std::string{"both"})
                .has_value());
    QVERIFY(fixture.document.setProperty(node, "fixed", true).has_value());
    QVERIFY(fixture.document
                .setProperty(node, "groupId", std::int64_t{7})
                .has_value());

    const auto plan =
        QNodeGraph::Execution::GraphPlanBuilder::build(fixture.document);
    const auto& params = plan.steps[0].params;
    // 呈现元数据与 UI 专有键不进算子参数。
    QVERIFY(params.find("radius.display") == params.end());
    QVERIFY(params.find("fixed") == params.end());
    QVERIFY(params.find("groupId") == params.end());
}

void GraphPlanTest::keepsPlanStableAcrossRebuilds() {
    Fixture fixture;
    const auto a = fixture.addNode("load_image", "A");
    const auto b = fixture.addNode("blur", "B");
    const auto c = fixture.addNode("edge_detect", "C");
    const auto aOut = fixture.addPort(
        a, "image", QNodeGraph::Core::PortDirection::Output);
    const auto bIn = fixture.addPort(
        b, "image", QNodeGraph::Core::PortDirection::Input);
    const auto bOut = fixture.addPort(
        b, "image", QNodeGraph::Core::PortDirection::Output);
    const auto cIn = fixture.addPort(
        c, "image", QNodeGraph::Core::PortDirection::Input);
    const auto cOut = fixture.addPort(
        c, "image", QNodeGraph::Core::PortDirection::Output);
    fixture.connect(aOut, bIn);
    fixture.connect(bOut, cIn);
    static_cast<void>(cOut);

    const auto first = QNodeGraph::Execution::GraphPlanBuilder::topologicalOrder(
        fixture.document);
    const auto second = QNodeGraph::Execution::GraphPlanBuilder::topologicalOrder(
        fixture.document);
    // 同一张图多次编译必须得到相同顺序（缓存命中与测试断言依赖这一点）。
    QCOMPARE(first, second);
    QCOMPARE(first.size(), std::size_t{3});
}

QTEST_MAIN(GraphPlanTest)
#include "graph_plan_test.moc"
