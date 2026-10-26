#include <QNodeGraph/Lib/Execution/graph_plan.h>

#include <QNodeGraph/Lib/Graph/node_registry.h>

#include <algorithm>
#include <queue>
#include <unordered_map>

namespace QNodeGraph::Execution {

namespace {

std::vector<Core::PortId> inputPorts(const Core::GraphDocument& document,
                                     Core::NodeId nodeId);

// 某节点的第一个输出端口 id。无输出端口 → 0。
Core::PortId firstOutputPort(const Core::GraphDocument& document,
                             Core::NodeId nodeId) {
    const auto* node = document.node(nodeId);
    if (node == nullptr) {
        return 0;
    }
    for (const auto portId : node->ports) {
        const auto* port = document.port(portId);
        if (port != nullptr && port->direction == Core::PortDirection::Output) {
            return portId;
        }
    }
    return 0;
}

// 每个计划阶段至多生成一份不透明输出载荷。普通节点以首个输出端口决定
// 类型；无输出端口的汇点沿用首个输入类型，便于宿主保留其消费到的标量、
// 特征或对象结果。
Core::PortDataType planOutputDataType(const Core::GraphDocument& document,
                                      Core::NodeId nodeId) {
    const auto outputPort = firstOutputPort(document, nodeId);
    if (const auto* port = document.port(outputPort); port != nullptr) {
        return port->dataType;
    }
    const auto inputs = inputPorts(document, nodeId);
    if (!inputs.empty()) {
        if (const auto* port = document.port(inputs.front()); port != nullptr) {
            return port->dataType;
        }
    }
    return Core::PortDataType::Any;
}

// 某节点的输入端口（保持声明顺序，供输入槽排列使用）。
std::vector<Core::PortId> inputPorts(const Core::GraphDocument& document,
                                     Core::NodeId nodeId) {
    std::vector<Core::PortId> ports;
    const auto* node = document.node(nodeId);
    if (node == nullptr) {
        return ports;
    }
    for (const auto portId : node->ports) {
        const auto* port = document.port(portId);
        if (port != nullptr && port->direction == Core::PortDirection::Input) {
            ports.push_back(portId);
        }
    }
    return ports;
}

bool hasOutputPort(const Core::GraphDocument& document, Core::NodeId nodeId) {
    return firstOutputPort(document, nodeId) != 0;
}

// 某节点的全部输出端口（保持节点端口顺序）。
//
// 逐个端口建槽而不是只取 primary：Type D/E 节点会同时产出图像与结构化载荷，
// 下游按端口取用，一个槽位无法区分两条分支。
std::vector<Core::PortId> outputPorts(const Core::GraphDocument& document,
                                      Core::NodeId nodeId) {
    std::vector<Core::PortId> ports;
    const auto* node = document.node(nodeId);
    if (node == nullptr) {
        return ports;
    }
    for (const auto portId : node->ports) {
        const auto* port = document.port(portId);
        if (port != nullptr && port->direction == Core::PortDirection::Output) {
            ports.push_back(portId);
        }
    }
    return ports;
}

// 某输出端口声明的载荷类型；端口缺失时退回 Any。
Core::PortDataType outputDataType(const Core::GraphDocument& document,
                                  Core::PortId portId) {
    if (const auto* port = document.port(portId); port != nullptr) {
        return port->dataType;
    }
    return Core::PortDataType::Any;
}

} // namespace

std::set<Core::NodeId>
GraphPlanBuilder::reachableFromSinks(const Core::GraphDocument& document) {
    // 汇点 = 没有输出端口的节点（图像预览 / 保存 / 视口输出）。它们的输入被
    // 消费，因此其上游「输出被使用」，沿连接反向可达。
    //
    // 反向邻接表一次构建、反复查询 —— 若在遍历中每次扫描全部连接，长链上是
    // O(N*E)，这里降到 O(V+E)。
    std::unordered_map<Core::PortId, std::vector<Core::NodeId>> upstreamOf;
    for (const auto& connection : document.connections()) {
        const auto* outputPort = document.port(connection.outputPort);
        if (outputPort == nullptr) {
            continue;
        }
        upstreamOf[connection.inputPort].push_back(outputPort->nodeId);
    }

    std::set<Core::NodeId> reachable;
    std::queue<Core::NodeId> pending;
    for (const auto nodeId : document.nodeIds()) {
        if (!hasOutputPort(document, nodeId) &&
            reachable.insert(nodeId).second) {
            pending.push(nodeId);
        }
    }
    while (!pending.empty()) {
        const auto nodeId = pending.front();
        pending.pop();
        for (const auto portId : inputPorts(document, nodeId)) {
            const auto iterator = upstreamOf.find(portId);
            if (iterator == upstreamOf.end()) {
                continue;
            }
            for (const auto upstreamNode : iterator->second) {
                if (reachable.insert(upstreamNode).second) {
                    pending.push(upstreamNode);
                }
            }
        }
    }
    return reachable;
}

std::vector<Core::NodeId>
GraphPlanBuilder::topologicalOrder(const Core::GraphDocument& document) {
    // Kahn 算法。边方向为 上游 → 下游。
    const auto nodeIds = document.nodeIds();
    std::unordered_map<Core::NodeId, int> indegree;
    std::unordered_map<Core::NodeId, std::vector<Core::NodeId>> downstream;
    for (const auto nodeId : nodeIds) {
        indegree[nodeId] = 0;
    }
    for (const auto& connection : document.connections()) {
        const auto* outputPort = document.port(connection.outputPort);
        const auto* inputPort = document.port(connection.inputPort);
        if (outputPort == nullptr || inputPort == nullptr) {
            continue;
        }
        downstream[outputPort->nodeId].push_back(inputPort->nodeId);
        ++indegree[inputPort->nodeId];
    }

    // 稳定序：就绪集始终按 id 升序，保证同一张图每次编译得到相同计划
    // （便于缓存命中与测试断言）。
    std::vector<Core::NodeId> ready;
    for (const auto nodeId : nodeIds) {
        if (indegree[nodeId] == 0) {
            ready.push_back(nodeId);
        }
    }
    std::sort(ready.begin(), ready.end());

    std::vector<Core::NodeId> order;
    order.reserve(nodeIds.size());
    while (!ready.empty()) {
        const auto nodeId = ready.front();
        ready.erase(ready.begin());
        order.push_back(nodeId);
        for (const auto target : downstream[nodeId]) {
            if (--indegree[target] == 0) {
                // 就地按升序插入，避免每轮整体重排。
                const auto position =
                    std::lower_bound(ready.begin(), ready.end(), target);
                ready.insert(position, target);
            }
        }
    }
    return order;
}

std::vector<Core::NodeId>
GraphPlanBuilder::downstreamClosure(const Core::GraphDocument& document,
                                    Core::NodeId origin) {
    // 正向邻接表一次构建。
    std::unordered_map<Core::NodeId, std::vector<Core::NodeId>> downstream;
    for (const auto& connection : document.connections()) {
        const auto* outputPort = document.port(connection.outputPort);
        const auto* inputPort = document.port(connection.inputPort);
        if (outputPort == nullptr || inputPort == nullptr) {
            continue;
        }
        downstream[outputPort->nodeId].push_back(inputPort->nodeId);
    }

    std::set<Core::NodeId> closure;
    std::queue<Core::NodeId> pending;
    if (closure.insert(origin).second) {
        pending.push(origin);
    }
    while (!pending.empty()) {
        const auto nodeId = pending.front();
        pending.pop();
        const auto iterator = downstream.find(nodeId);
        if (iterator == downstream.end()) {
            continue;
        }
        for (const auto target : iterator->second) {
            if (closure.insert(target).second) {
                pending.push(target);
            }
        }
    }

    // 按拓扑序返回，调用方可直接顺序重算。
    std::vector<Core::NodeId> result;
    for (const auto nodeId : topologicalOrder(document)) {
        if (closure.contains(nodeId)) {
            result.push_back(nodeId);
        }
    }
    return result;
}

EvaluationPlan GraphPlanBuilder::build(const Core::GraphDocument& document,
                                       const Graph::NodeRegistry* registry) {
    EvaluationPlan plan;
    const auto order = topologicalOrder(document);
    const auto reachable = reachableFromSinks(document);

    for (const auto nodeId : order) {
        const auto* node = document.node(nodeId);
        if (node == nullptr) {
            continue;
        }
        // 被剪枝（输出无人使用）的节点不进计划，也不分配帧。
        if (!reachable.contains(nodeId)) {
            plan.prunedNodes.push_back(nodeId);
            continue;
        }
        PlanStep step;
        step.node = nodeId;
        // 算子键取节点类型；宿主按 typeId 注册对应算子实现。
        step.operatorKey = node->type;
        step.outputDataType = planOutputDataType(document, nodeId);
        // 每个输出端口一槽：多输出算子（图像 + Objects/Features）要靠端口
        // 区分分支，单一 primary 槽位无法表达。
        for (const auto portId : outputPorts(document, nodeId)) {
            OutputSlot slot;
            slot.port = portId;
            slot.dataType = outputDataType(document, portId);
            step.outputs.push_back(slot);
        }
        // 终端汇点没有输出端口，但仍要分配一块承载「被消费结果」的帧：运行时按
        // 槽位分配帧，空槽位会让汇点拿不到帧（主视口因此整片黑）。这里补一个
        // port 0 的虚拟槽位，其载荷类型沿用 outputDataType 的输入类型回退。
        if (step.outputs.empty()) {
            OutputSlot sink;
            sink.port = 0;
            sink.dataType = step.outputDataType;
            step.outputs.push_back(sink);
        }
        // 描述符声明的执行契约（代价等级、后端偏好等）。没有注册表时保持
        // 默认策略，让只关心拓扑的调用方不必构造注册表。
        if (registry != nullptr) {
            if (const auto* descriptor = registry->find(node->type);
                descriptor != nullptr) {
                step.policy = descriptor->executionPolicy;
            }
        }
        // 参数：带出节点属性中除 ".display"（呈现元数据）与纯 UI 键以外的
        // 全部，供宿主算子读取。二值化阈值这类参数由此传入算子。
        for (const auto& [name, value] : node->properties) {
            if (name.size() >= 8 &&
                name.compare(name.size() - 8, 8, ".display") == 0) {
                continue;
            }
            if (name == "fixed" || name == "groupId") {
                continue;
            }
            step.params.emplace(name, value);
        }
        plan.steps.push_back(std::move(step));
    }
    return plan;
}

} // namespace QNodeGraph::Execution
