#pragma once

#include <QNodeGraph/Lib/Core/execution_policy.h>
#include <QNodeGraph/Lib/Core/graph_document.h>

#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace QNodeGraph::Graph {
struct NodeRegistry;
} // namespace QNodeGraph::Graph

// 求值计划与脏传播：把 GraphDocument 编译成一份可执行的阶段序列，并描述
// 「哪些节点需要重算」。
//
// 为什么放在 Core 侧的 Execution 层（不依赖具体帧类型）：调度是纯图算法，
// 与帧是 QImage、CL buffer 还是 GL 纹理无关。宿主通过 FramePool /
// OperatorExecutor 两个抽象提供帧与算子，库对帧表示零假设 —— 这样同一份调度
// 逻辑既能服务逐帧渲染路径（同步），也能服务离线 CPU 参考实现。
//
// 与既有 ImageExecutionService / ImagePipeline 的关系：后者是「一张图 + 一个
// 函数」的线性执行（面向 QImage，Demo/测试用）；本文件是「整张图按拓扑求值」
// 的调度（面向不透明帧句柄）。两者并存，互不替代。

namespace QNodeGraph::Execution {

// 不透明帧句柄。0 表示无效/未分配。
using FrameHandle = std::uint64_t;

// 执行契约（ROI / 边界 / 精度 / 后端 / 代价等级）由 Core 定义，因为节点描述符
// （Graph 层）也要声明它。这里重新导出，既有的
// `QNodeGraph::Execution::Precision` 等写法继续可用，且与 Core 是同一类型。
using Core::Backend;
using Core::BorderMode;
using Core::CostClass;
using Core::ExecutionPolicy;
using Core::Precision;
using Core::Roi;

// 帧规格：库只知道尺寸与格式标签，具体含义由宿主的 FramePool 解释。
//
// 字段顺序即聚合初始化顺序：宿主写 `FrameSpec{w, h, "bgra32", dataType,
// precision, roi}` 时必须能一眼读出每个位置的含义，所以载荷类型紧跟在格式标签
// 之后，精度在它之后（类型比精度更重要：Number 帧根本谈不上像素精度）。
struct FrameSpec {
    int width = 0;
    int height = 0;
    // 格式标签，如 "bgra32" / "rgba8-texture" / "gray8"。库不解释其语义。
    std::string format;
    // The graph-level payload type carried by this frame. Image remains the
    // compatibility default; Number/Features/Objects let a host allocate an
    // appropriate opaque payload instead of treating every plan step as an
    // image buffer.
    Core::PortDataType dataType = Core::PortDataType::Image;
    Precision precision = Precision::U8;
    // Source region this frame holds. It is part of the storage contract, not
    // merely metadata: a 512x512 tile at a different source position must
    // allocate (and invalidate) a different frame, otherwise two tiles would
    // alias one cache entry and present stale pixels.
    Roi roi;
};

// 一个具体输出端口的槽位。
//
// 为什么计划阶段要为「每个输出端口」而不是「每个节点」保留槽位：Type D/E 节点
// （例如连通域）同时产出标签图与 Objects，下游按端口取用。只留一个 primary
// 槽位会让「Objects 端口被接到汇点」与「图像端口被接到汇点」无法区分。
struct OutputSlot {
    // 输出端口 id；0 表示该节点没有输出端口（终端汇点）。
    Core::PortId port = 0;
    // 该端口声明的载荷类型。汇点没有输出端口时取第一个输入端口类型，使透传
    // 汇点仍能保留并暴露被消费的结果。
    Core::PortDataType dataType = Core::PortDataType::Image;
    FrameHandle handle = 0;
};

// 由宿主实现：帧的分配与生命周期。库只持有句柄，不持有帧数据。
class FramePool {
public:
    virtual ~FramePool() = default;
    // 分配一个符合 spec 的帧。
    [[nodiscard]] virtual Core::GraphResult<FrameHandle> allocate(
        const FrameSpec& spec) = 0;
    // 引用计数 +1 / -1。库在多个阶段复用同一帧时会 retain。
    virtual void retain(FrameHandle handle) = 0;
    virtual void release(FrameHandle handle) = 0;
    // 标记帧内容失效（脏）。宿主可据此丢弃缓存或延迟释放。
    virtual void invalidate(FrameHandle handle) = 0;
};

// 一次算子调用：算子键 + 参数 + 输入槽 + 输出槽。
//
// `output` 是 primary（第一个已连接输出），保留给只实现单输出语义的执行器；
// 需要区分多输出的执行器读 `outputs`。
struct OperatorInvocation {
    std::string operatorKey;
    std::map<std::string, Core::PropertyValue> params;
    std::vector<FrameHandle> inputs;
    FrameHandle output = 0;
    std::vector<OutputSlot> outputs;
    ExecutionPolicy policy;
};

// 由宿主实现：单个算子的一次执行。库负责调度，宿主负责算法。
class OperatorExecutor {
public:
    virtual ~OperatorExecutor() = default;
    [[nodiscard]] virtual Core::GraphResult<void> execute(
        const OperatorInvocation& invocation) = 0;
};

// 求值计划中的一个阶段。
struct PlanStep {
    Core::NodeId node = 0;
    // 宿主注册的算子键（通常即节点描述符的 typeId）。
    std::string operatorKey;
    // Declared output payload type. For terminal nodes with no output port,
    // this is their first input type so a pass-through sink can still retain
    // and expose the consumed result.
    Core::PortDataType outputDataType = Core::PortDataType::Image;
    // 输入槽：按该节点输入端口的顺序排列。
    std::vector<FrameHandle> inputs;
    // primary 输出：第一个输出端口的槽位句柄（无输出端口时为 0）。
    FrameHandle output = 0;
    // 每个具体输出端口一槽，按节点输出端口顺序排列；终端汇点没有输出端口，
    // 因此这里是空的，其语义由 outputDataType 的回退承载。
    std::vector<OutputSlot> outputs;
    std::map<std::string, Core::PropertyValue> params;
    ExecutionPolicy policy;
};

// 由 GraphDocument 编译出的求值计划。
struct EvaluationPlan {
    // 已按拓扑序排列的阶段（不含被剪枝的节点）。
    std::vector<PlanStep> steps;
    // 被剪枝的节点：从「输出型节点」（汇点）反向不可达，不参与求值。
    std::vector<Core::NodeId> prunedNodes;

    [[nodiscard]] const PlanStep* stepFor(Core::NodeId node) const {
        for (const auto& step : steps) {
            if (step.node == node) {
                return &step;
            }
        }
        return nullptr;
    }
};

// 一次图变更的描述。宿主据此决定「重编译计划」还是「增量重算」。
//
// 拓扑脏与输入帧脏必须分开：前者是增量（只重跑下游），后者是全量（视口移动
// 改变了输入帧，所有节点都要重算）。
struct GraphChangeSet {
    // 脏节点：变更节点自身 + 其下游闭包，已按拓扑序。空表示无节点级变更。
    std::vector<Core::NodeId> dirtyNodes;
    // 连接增删导致拓扑变化 → 计划需重新编译。
    bool topologyChanged = false;
    // 输入帧（视口帧）变化 → 全部节点需重算。
    bool inputFrameChanged = false;
    // 属性变化 → 通常只需重算 dirtyNodes。
    bool propertiesChanged = false;

    [[nodiscard]] bool empty() const noexcept {
        return dirtyNodes.empty() && !topologyChanged && !inputFrameChanged &&
               !propertiesChanged;
    }
};

// 由 GraphDocument 编译求值计划的纯函数集合。
class GraphPlanBuilder final {
public:
    // 编译计划。汇点定义为「没有输出端口的节点」。
    //
    // `registry` 是可选的节点描述符表：提供时每个阶段的 ExecutionPolicy 取自
    // 描述符声明的 executionPolicy（代价等级、后端偏好等），否则保持默认策略，
    // 使只关心拓扑的调用方不必构造注册表。
    [[nodiscard]] static EvaluationPlan build(
        const Core::GraphDocument& document,
        const Graph::NodeRegistry* registry = nullptr);

    // 计算某节点的下游闭包（含自身），按拓扑序返回。
    [[nodiscard]] static std::vector<Core::NodeId> downstreamClosure(
        const Core::GraphDocument& document, Core::NodeId origin);

    // 从汇点反向可达的节点集合（即「输出被使用」的节点）。
    [[nodiscard]] static std::set<Core::NodeId> reachableFromSinks(
        const Core::GraphDocument& document);

    // 拓扑序。若图有环则返回已排序部分（Core 已禁止成环连接，故正常不发生）。
    [[nodiscard]] static std::vector<Core::NodeId> topologicalOrder(
        const Core::GraphDocument& document);
};

} // namespace QNodeGraph::Execution
