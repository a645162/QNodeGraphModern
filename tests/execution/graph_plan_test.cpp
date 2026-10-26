#include <QNodeGraph/Lib/Execution/graph_plan.h>
#include <QNodeGraph/Lib/Execution/param_accessor.h>
#include <QNodeGraph/Lib/Graph/node_registry.h>

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
    void carriesOutputPayloadTypeToPlan();
    void carriesConcreteOutputSlotsToPlan();
    void appliesDescriptorExecutionPolicy();
    void excludesDisplayAndUiKeysFromParams();
    void keepsPlanStableAcrossRebuilds();
    void exposesExecutionPolicyDefaultsAndFields();
    void coercesAndClampsOperatorParameters();

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
        QNodeGraph::Core::PortId
        addPort(QNodeGraph::Core::NodeId node, const std::string& name,
                QNodeGraph::Core::PortDirection dir,
                QNodeGraph::Core::PortDataType type =
                    QNodeGraph::Core::PortDataType::Image) {
            const auto port = document.addPort(node, name, dir, type);
            Q_ASSERT(port.has_value());
            return *port;
        }
        void connect(QNodeGraph::Core::PortId out,
                     QNodeGraph::Core::PortId in) {
            const auto result = document.connect(out, in);
            if (!result.has_value()) {
                QFAIL(
                    qPrintable(QString::fromStdString(result.error().message)));
            }
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
    const auto sinkIn =
        fixture.addPort(sink, "image", QNodeGraph::Core::PortDirection::Input);
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
    const auto thirdIn =
        fixture.addPort(third, "image", QNodeGraph::Core::PortDirection::Input);
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
    QVERIFY(fixture.document.setProperty(node, "threshold", std::int64_t{128})
                .has_value());

    const auto plan =
        QNodeGraph::Execution::GraphPlanBuilder::build(fixture.document);
    QCOMPARE(plan.steps.size(), std::size_t{1});
    const auto iterator = plan.steps[0].params.find("threshold");
    QVERIFY(iterator != plan.steps[0].params.end());
    QVERIFY(std::holds_alternative<std::int64_t>(iterator->second));
    QCOMPARE(std::get<std::int64_t>(iterator->second), std::int64_t{128});
}

void GraphPlanTest::carriesOutputPayloadTypeToPlan() {
    using QNodeGraph::Core::PortDataType;
    Fixture fixture;
    const auto statistics = fixture.addNode("statistics", "Statistics");
    const auto valueView = fixture.addNode("value_view", "Value View");
    const auto statisticsInput = fixture.addPort(
        statistics, "image", QNodeGraph::Core::PortDirection::Input);
    static_cast<void>(statisticsInput);
    const auto statisticsOutput = fixture.addPort(
        statistics, "mean", QNodeGraph::Core::PortDirection::Output,
        PortDataType::Number);
    const auto valueInput = fixture.addPort(
        valueView, "value", QNodeGraph::Core::PortDirection::Input,
        PortDataType::Number);
    fixture.connect(statisticsOutput, valueInput);

    const auto plan =
        QNodeGraph::Execution::GraphPlanBuilder::build(fixture.document);
    QCOMPARE(plan.steps.size(), std::size_t{2});
    QCOMPARE(plan.steps[0].outputDataType, PortDataType::Number);
    // The sink has no output port, but it must keep its Number payload rather
    // than reverting to Image solely because it terminates the graph.
    QCOMPARE(plan.steps[1].outputDataType, PortDataType::Number);
}

void GraphPlanTest::carriesConcreteOutputSlotsToPlan() {
    using QNodeGraph::Core::PortDataType;
    Fixture fixture;
    // 一个多输出节点：图像与 Objects 同时产出，下游只消费 Objects 分支。
    const auto components = fixture.addNode("connected_components", "CC");
    const auto sink = fixture.addNode("objects_view", "Objects");
    const auto input = fixture.addPort(components, "image",
                                       QNodeGraph::Core::PortDirection::Input);
    static_cast<void>(input);
    const auto labelOutput = fixture.addPort(
        components, "labels", QNodeGraph::Core::PortDirection::Output);
    const auto objectsOutput = fixture.addPort(
        components, "objects", QNodeGraph::Core::PortDirection::Output,
        PortDataType::Objects);
    const auto sinkInput =
        fixture.addPort(sink, "objects", QNodeGraph::Core::PortDirection::Input,
                        PortDataType::Objects);
    fixture.connect(objectsOutput, sinkInput);

    const auto plan =
        QNodeGraph::Execution::GraphPlanBuilder::build(fixture.document);
    QCOMPARE(plan.steps.size(), std::size_t{2});
    // 每个输出端口一槽，且保留端口 id 与载荷类型，供执行器按分支取用。
    QCOMPARE(plan.steps[0].outputs.size(), std::size_t{2});
    QCOMPARE(plan.steps[0].outputs[0].port, labelOutput);
    QCOMPARE(plan.steps[0].outputs[0].dataType, PortDataType::Image);
    QCOMPARE(plan.steps[0].outputs[1].port, objectsOutput);
    QCOMPARE(plan.steps[0].outputs[1].dataType, PortDataType::Objects);
    // primary 输出仍是第一个输出端口。
    QCOMPARE(plan.steps[0].outputDataType, PortDataType::Image);
    // 终端汇点没有输出端口，但必须留一个 port 0 的虚拟槽位，否则运行时不分配
    // 帧，主视口/预览拿不到被消费的结果。
    QCOMPARE(plan.steps[1].outputs.size(), std::size_t{1});
    QCOMPARE(plan.steps[1].outputs[0].port, QNodeGraph::Core::PortId{0});
    QCOMPARE(plan.steps[1].outputs[0].dataType, PortDataType::Objects);
}

void GraphPlanTest::appliesDescriptorExecutionPolicy() {
    using QNodeGraph::Core::CostClass;
    Fixture fixture;
    const auto node = fixture.addNode("expensive", "Expensive");
    const auto output =
        fixture.addPort(node, "image", QNodeGraph::Core::PortDirection::Output);
    const auto sink = fixture.addNode("image_preview", "Sink");
    const auto sinkInput =
        fixture.addPort(sink, "image", QNodeGraph::Core::PortDirection::Input);
    fixture.connect(output, sinkInput);

    QNodeGraph::Graph::NodeRegistry registry;
    QNodeGraph::Graph::NodeDescriptor descriptor;
    descriptor.typeId = "expensive";
    descriptor.executionPolicy.costClass = CostClass::Offline;
    descriptor.executionPolicy.backend = QNodeGraph::Core::Backend::OpenCl;
    QVERIFY(registry.registerNode(descriptor).has_value());

    const auto plan = QNodeGraph::Execution::GraphPlanBuilder::build(
        fixture.document, &registry);
    QCOMPARE(plan.steps[0].policy.costClass, CostClass::Offline);
    QCOMPARE(plan.steps[0].policy.backend, QNodeGraph::Core::Backend::OpenCl);
    // 未登记的节点保持默认策略，说明注册表只是覆盖来源而不是必需条件。
    QCOMPARE(plan.steps[1].policy.costClass, CostClass::Realtime);

    const auto withoutRegistry =
        QNodeGraph::Execution::GraphPlanBuilder::build(fixture.document);
    QCOMPARE(withoutRegistry.steps[0].policy.costClass, CostClass::Realtime);
}

void GraphPlanTest::excludesDisplayAndUiKeysFromParams() {
    Fixture fixture;
    const auto node = fixture.addNode("image_preview", "Sink");
    fixture.addPort(node, "image", QNodeGraph::Core::PortDirection::Input);
    QVERIFY(fixture.document
                .setProperty(node, "radius.display", std::string{"both"})
                .has_value());
    QVERIFY(fixture.document.setProperty(node, "fixed", true).has_value());
    QVERIFY(fixture.document.setProperty(node, "groupId", std::int64_t{7})
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
    const auto aOut =
        fixture.addPort(a, "image", QNodeGraph::Core::PortDirection::Output);
    const auto bIn =
        fixture.addPort(b, "image", QNodeGraph::Core::PortDirection::Input);
    const auto bOut =
        fixture.addPort(b, "image", QNodeGraph::Core::PortDirection::Output);
    const auto cIn =
        fixture.addPort(c, "image", QNodeGraph::Core::PortDirection::Input);
    const auto cOut =
        fixture.addPort(c, "image", QNodeGraph::Core::PortDirection::Output);
    fixture.connect(aOut, bIn);
    fixture.connect(bOut, cIn);
    static_cast<void>(cOut);

    const auto first =
        QNodeGraph::Execution::GraphPlanBuilder::topologicalOrder(
            fixture.document);
    const auto second =
        QNodeGraph::Execution::GraphPlanBuilder::topologicalOrder(
            fixture.document);
    // 同一张图多次编译必须得到相同顺序（缓存命中与测试断言依赖这一点）。
    QCOMPARE(first, second);
    QCOMPARE(first.size(), std::size_t{3});
}

void GraphPlanTest::exposesExecutionPolicyDefaultsAndFields() {
    using namespace QNodeGraph::Execution;
    const ExecutionPolicy defaults;
    QVERIFY(defaults.roi.full());
    QCOMPARE(defaults.border, BorderMode::Reflect101);
    QCOMPARE(defaults.precision, Precision::U8);
    QCOMPARE(defaults.backend, Backend::Auto);
    QCOMPARE(defaults.costClass, CostClass::Realtime);

    ExecutionPolicy policy;
    policy.roi = Roi{4, 8, 32, 24};
    policy.border = BorderMode::Replicate;
    policy.precision = Precision::F32;
    policy.backend = Backend::OpenCl;
    policy.costClass = CostClass::Interactive;
    QVERIFY(!policy.roi.full());
    QCOMPARE(policy.roi.x, 4);
    QCOMPARE(policy.roi.height, 24);

    OperatorInvocation invocation;
    invocation.policy = policy;
    QCOMPARE(invocation.policy.backend, Backend::OpenCl);
    PlanStep step;
    step.policy = policy;
    QCOMPARE(step.policy.precision, Precision::F32);
    FrameSpec spec{640, 480, "bgra32", QNodeGraph::Core::PortDataType::Number,
                   Precision::U16};
    QCOMPARE(spec.precision, Precision::U16);
    QCOMPARE(spec.dataType, QNodeGraph::Core::PortDataType::Number);
}

void GraphPlanTest::coercesAndClampsOperatorParameters() {
    std::map<std::string, QNodeGraph::Core::PropertyValue> values{
        {"int", std::int64_t{400}},
        {"real", std::int64_t{2}},
        {"flag", true},
        {"text", std::string{"median"}}};
    const QNodeGraph::Execution::ParamAccessor params(values);
    QCOMPARE(params.integerClamped("int", 1, 0, 255), std::int64_t{255});
    QCOMPARE(params.integerClamped("real", 0, 0, 10), std::int64_t{2});
    QCOMPARE(params.integerClamped("missing", 7, 0, 5), std::int64_t{5});
    QCOMPARE(params.realClamped("real", 0.0, 0.0, 1.0), 1.0);
    // 不带范围的简写只做类型兼容读取，不做额外收窄。
    QCOMPARE(params.integer("int", 1), std::int64_t{400});
    QCOMPARE(params.real("real", 0.0), 2.0);
    QCOMPARE(params.boolean("flag", false), true);
    QCOMPARE(params.text("text", "fallback"), std::string{"median"});
    QCOMPARE(params.text("missing", "fallback"), std::string{"fallback"});
}

QTEST_MAIN(GraphPlanTest)
#include "graph_plan_test.moc"
