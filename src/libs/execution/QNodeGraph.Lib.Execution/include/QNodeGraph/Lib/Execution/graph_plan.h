#pragma once

#include <QNodeGraph/Lib/Core/graph_document.h>

#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <vector>

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

// 执行区域。width/height <= 0 表示整帧，便于在没有 ROI 约束时保持现有
// FrameSpec/算子接口的默认行为。
struct Roi {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;

    [[nodiscard]] constexpr bool full() const noexcept {
        return width <= 0 || height <= 0;
    }
};

enum class BorderMode {
    Constant,
    Replicate,
    Reflect,
    Reflect101,
    Wrap,
};

enum class Precision {
    U8,
    U16,
    F32,
};

enum class Backend {
    Auto,
    Cpu,
    OpenCl,
};

enum class CostClass {
    Realtime,
    Interactive,
    Offline,
};

// 算子执行策略是调度契约，不绑定具体图像类型。业务执行器可据此选择
// ROI、边界扩展、精度和后端；未指定时保持 U8/整帧/自动后端的兼容路径。
struct ExecutionPolicy {
    Roi roi;
    BorderMode border = BorderMode::Reflect101;
    Precision precision = Precision::U8;
    Backend backend = Backend::Auto;
    CostClass cost = CostClass::Realtime;
};

// 帧规格：库只知道尺寸与格式标签，具体含义由宿主的 FramePool 解释。
struct FrameSpec {
    int width = 0;
    int height = 0;
    // 格式标签，如 "bgra32" / "rgba8-texture" / "gray8"。库不解释其语义。
    std::string format;
    Precision precision = Precision::U8;
    // The graph-level payload type carried by this frame. Image remains the
    // compatibility default; Number/Features/Objects let a host allocate an
    // appropriate opaque payload instead of treating every plan step as an
    // image buffer.
    Core::PortDataType dataType = Core::PortDataType::Image;
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
struct OperatorInvocation {
    std::string operatorKey;
    std::map<std::string, Core::PropertyValue> params;
    std::vector<FrameHandle> inputs;
    FrameHandle output = 0;
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
    FrameHandle output = 0;
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
    [[nodiscard]] static EvaluationPlan build(
        const Core::GraphDocument& document);

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
