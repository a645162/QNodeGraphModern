#include <QNodeGraph/Lib/Core/graph_commands.h>

#include <utility>

namespace QNodeGraph::Core {

namespace {

GraphError commandError(std::string message) {
    return GraphError{GraphErrorCode::CommandStackEmpty, std::move(message)};
}

} // namespace

CreateNodeCommand::CreateNodeCommand(std::string type, std::string name)
    : m_type(std::move(type)), m_name(std::move(name)) {}

GraphResult<void> CreateNodeCommand::execute(GraphDocument& document) {
    const auto result = m_nodeId.has_value()
                            ? document.addNode(m_type, m_name, *m_nodeId)
                            : document.addNode(m_type, m_name);
    if (!result) {
        return std::unexpected(result.error());
    }
    m_nodeId = *result;
    return {};
}

GraphResult<void> CreateNodeCommand::undo(GraphDocument& document) {
    if (!m_nodeId.has_value()) {
        return std::unexpected(
            commandError("The create command was not executed."));
    }
    const auto result = document.removeNode(*m_nodeId);
    if (!result) {
        return std::unexpected(result.error());
    }
    return {};
}

const char* CreateNodeCommand::name() const noexcept { return "Create Node"; }

std::optional<NodeId> CreateNodeCommand::nodeId() const noexcept {
    return m_nodeId;
}

DeleteNodeCommand::DeleteNodeCommand(NodeId nodeId) : m_nodeId(nodeId) {}

GraphResult<void> DeleteNodeCommand::execute(GraphDocument& document) {
    const auto result = document.removeNode(m_nodeId);
    if (!result) {
        return std::unexpected(result.error());
    }
    m_snapshot = *result;
    return {};
}

GraphResult<void> DeleteNodeCommand::undo(GraphDocument& document) {
    if (!m_snapshot.has_value()) {
        return std::unexpected(
            commandError("The delete command was not executed."));
    }
    return document.restoreNode(*m_snapshot);
}

const char* DeleteNodeCommand::name() const noexcept { return "Delete Node"; }

DeleteNodesCommand::DeleteNodesCommand(std::vector<NodeId> nodeIds)
    : m_nodeIds(std::move(nodeIds)) {}

GraphResult<void> DeleteNodesCommand::execute(GraphDocument& document) {
    m_snapshots.clear();
    m_snapshots.reserve(m_nodeIds.size());
    for (const auto nodeId : m_nodeIds) {
        auto removed = document.removeNode(nodeId);
        if (!removed) {
            // 中途失败时把已经删掉的节点按逆序还原，保证命令要么全做、
            // 要么全不做；否则撤销栈会记下一个「删了一半」的中间态。
            for (auto it = m_snapshots.rbegin(); it != m_snapshots.rend();
                 ++it) {
                static_cast<void>(document.restoreNode(*it));
            }
            m_snapshots.clear();
            return std::unexpected(removed.error());
        }
        m_snapshots.push_back(std::move(*removed));
    }
    return {};
}

GraphResult<void> DeleteNodesCommand::undo(GraphDocument& document) {
    if (m_snapshots.size() != m_nodeIds.size()) {
        return std::unexpected(
            commandError("The delete command was not executed."));
    }
    for (const auto& snapshot : m_snapshots) {
        const auto result = document.restoreNode(snapshot);
        if (!result) {
            return result;
        }
    }
    return {};
}

const char* DeleteNodesCommand::name() const noexcept { return "Delete Nodes"; }

MoveNodeCommand::MoveNodeCommand(NodeId nodeId, Point position)
    : m_nodeId(nodeId), m_position(position) {}

GraphResult<void> MoveNodeCommand::execute(GraphDocument& document) {
    const auto* value = document.node(m_nodeId);
    if (value == nullptr) {
        return std::unexpected(
            GraphError{GraphErrorCode::NodeNotFound,
                       "The move target node does not exist."});
    }
    if (!m_previousPosition.has_value()) {
        m_previousPosition = value->position;
    }
    return document.setNodePosition(m_nodeId, m_position);
}

GraphResult<void> MoveNodeCommand::undo(GraphDocument& document) {
    if (!m_previousPosition.has_value()) {
        return std::unexpected(
            commandError("The move command was not executed."));
    }
    return document.setNodePosition(m_nodeId, *m_previousPosition);
}

const char* MoveNodeCommand::name() const noexcept { return "Move Node"; }

MoveNodesCommand::MoveNodesCommand(
    std::vector<std::pair<NodeId, Point>> targets)
    : m_targets(std::move(targets)) {}

GraphResult<void> MoveNodesCommand::execute(GraphDocument& document) {
    if (m_targets.empty()) {
        return std::unexpected(commandError("A multi-node move is empty."));
    }
    if (m_previousPositions.empty()) {
        m_previousPositions.reserve(m_targets.size());
        for (const auto& [nodeId, target] : m_targets) {
            static_cast<void>(target);
            const auto* node = document.node(nodeId);
            if (node == nullptr) {
                m_previousPositions.clear();
                return std::unexpected(
                    GraphError{GraphErrorCode::NodeNotFound,
                               "A multi-node move target does not exist."});
            }
            m_previousPositions.emplace_back(nodeId, node->position);
        }
    }
    std::size_t moved = 0;
    for (const auto& [nodeId, target] : m_targets) {
        const auto result = document.setNodePosition(nodeId, target);
        if (!result) {
            for (std::size_t index = 0; index < moved; ++index) {
                static_cast<void>(document.setNodePosition(
                    m_previousPositions[index].first,
                    m_previousPositions[index].second));
            }
            return std::unexpected(result.error());
        }
        ++moved;
    }
    return {};
}

GraphResult<void> MoveNodesCommand::undo(GraphDocument& document) {
    if (m_previousPositions.empty()) {
        return std::unexpected(
            commandError("The multi-node move was not executed."));
    }
    for (const auto& [nodeId, position] : m_previousPositions) {
        const auto result = document.setNodePosition(nodeId, position);
        if (!result) {
            return std::unexpected(result.error());
        }
    }
    return {};
}

const char* MoveNodesCommand::name() const noexcept { return "Move Nodes"; }

ConnectPortsCommand::ConnectPortsCommand(PortId outputPort, PortId inputPort)
    : m_outputPort(outputPort), m_inputPort(inputPort) {}

GraphResult<void> ConnectPortsCommand::execute(GraphDocument& document) {
    return document.connect(m_outputPort, m_inputPort);
}

GraphResult<void> ConnectPortsCommand::undo(GraphDocument& document) {
    return document.disconnect(m_outputPort, m_inputPort);
}

const char* ConnectPortsCommand::name() const noexcept {
    return "Connect Ports";
}

DisconnectPortsCommand::DisconnectPortsCommand(PortId outputPort,
                                               PortId inputPort)
    : m_outputPort(outputPort), m_inputPort(inputPort) {}

GraphResult<void> DisconnectPortsCommand::execute(GraphDocument& document) {
    return document.disconnect(m_outputPort, m_inputPort);
}

GraphResult<void> DisconnectPortsCommand::undo(GraphDocument& document) {
    return document.connect(m_outputPort, m_inputPort);
}

const char* DisconnectPortsCommand::name() const noexcept {
    return "Disconnect Ports";
}

SetPropertyCommand::SetPropertyCommand(NodeId nodeId, std::string name,
                                       PropertyValue value)
    : m_nodeId(nodeId), m_name(std::move(name)), m_value(std::move(value)) {}

GraphResult<void> SetPropertyCommand::execute(GraphDocument& document) {
    if (!m_previousValue.has_value()) {
        if (const auto* value = document.property(m_nodeId, m_name);
            value != nullptr) {
            m_previousValue = *value;
        }
    }
    return document.setProperty(m_nodeId, m_name, m_value);
}

GraphResult<void> SetPropertyCommand::undo(GraphDocument& document) {
    if (m_previousValue.has_value()) {
        return document.setProperty(m_nodeId, m_name, *m_previousValue);
    }
    return document.clearProperty(m_nodeId, m_name);
}

const char* SetPropertyCommand::name() const noexcept { return "Set Property"; }

ResizeNodeCommand::ResizeNodeCommand(NodeId nodeId, Point position,
                                     double width, double height)
    : m_nodeId(nodeId), m_position(position), m_width(width), m_height(height) {
}

GraphResult<void> ResizeNodeCommand::execute(GraphDocument& document) {
    const auto* node = document.node(m_nodeId);
    if (node == nullptr) {
        return std::unexpected(
            GraphError{GraphErrorCode::NodeNotFound,
                       "Node " + std::to_string(m_nodeId) + " was not found."});
    }
    // 仅在首次执行时抓取快照：redo 必须回退到最初状态而不是上一次执行后的
    // 状态，否则反复 undo/redo 会累积漂移。
    if (!m_previousPosition.has_value()) {
        m_previousPosition = node->position;
        m_previousWidthWasSet = node->properties.contains("width");
        m_previousHeightWasSet = node->properties.contains("height");
        if (const auto iterator = node->properties.find("width");
            iterator != node->properties.end()) {
            m_previousWidth = iterator->second;
        }
        if (const auto iterator = node->properties.find("height");
            iterator != node->properties.end()) {
            m_previousHeight = iterator->second;
        }
    }
    if (const auto result = document.setNodePosition(m_nodeId, m_position);
        !result) {
        return std::unexpected(result.error());
    }
    if (const auto result = document.setProperty(
            m_nodeId, "width",
            std::int64_t{static_cast<std::int64_t>(m_width)});
        !result) {
        return std::unexpected(result.error());
    }
    if (const auto result = document.setProperty(
            m_nodeId, "height",
            std::int64_t{static_cast<std::int64_t>(m_height)});
        !result) {
        return std::unexpected(result.error());
    }
    return {};
}

GraphResult<void> ResizeNodeCommand::undo(GraphDocument& document) {
    if (!m_previousPosition.has_value()) {
        return std::unexpected(
            commandError("The resize command was not executed."));
    }
    if (const auto result =
            document.setNodePosition(m_nodeId, *m_previousPosition);
        !result) {
        return std::unexpected(result.error());
    }
    // 原先没有该属性时用 clearProperty 移除，而不是写回一个猜测的默认值 ——
    // 否则撤销后节点会多出本来不存在的 width/height 属性。
    if (m_previousWidthWasSet && m_previousWidth.has_value()) {
        if (const auto result =
                document.setProperty(m_nodeId, "width", *m_previousWidth);
            !result) {
            return std::unexpected(result.error());
        }
    } else if (const auto result = document.clearProperty(m_nodeId, "width");
               !result) {
        return std::unexpected(result.error());
    }
    if (m_previousHeightWasSet && m_previousHeight.has_value()) {
        if (const auto result =
                document.setProperty(m_nodeId, "height", *m_previousHeight);
            !result) {
            return std::unexpected(result.error());
        }
    } else if (const auto result = document.clearProperty(m_nodeId, "height");
               !result) {
        return std::unexpected(result.error());
    }
    return {};
}

const char* ResizeNodeCommand::name() const noexcept { return "Resize Node"; }

GraphResult<void>
GraphCommandStack::execute(std::unique_ptr<GraphCommand> command,
                           GraphDocument& document) {
    if (command == nullptr) {
        return std::unexpected(
            commandError("A null command cannot be executed."));
    }
    const auto result = command->execute(document);
    if (!result) {
        return std::unexpected(result.error());
    }
    m_undoStack.push_back(std::move(command));
    m_redoStack.clear();
    return {};
}

GraphResult<void> GraphCommandStack::undo(GraphDocument& document) {
    if (m_undoStack.empty()) {
        return std::unexpected(commandError("There is no command to undo."));
    }
    auto command = std::move(m_undoStack.back());
    const auto result = command->undo(document);
    if (!result) {
        m_undoStack.back() = std::move(command);
        return std::unexpected(result.error());
    }
    m_undoStack.pop_back();
    m_redoStack.push_back(std::move(command));
    return {};
}

GraphResult<void> GraphCommandStack::redo(GraphDocument& document) {
    if (m_redoStack.empty()) {
        return std::unexpected(commandError("There is no command to redo."));
    }
    auto command = std::move(m_redoStack.back());
    const auto result = command->execute(document);
    if (!result) {
        m_redoStack.back() = std::move(command);
        return std::unexpected(result.error());
    }
    m_redoStack.pop_back();
    m_undoStack.push_back(std::move(command));
    return {};
}

bool GraphCommandStack::canUndo() const noexcept {
    return !m_undoStack.empty();
}

bool GraphCommandStack::canRedo() const noexcept {
    return !m_redoStack.empty();
}

void GraphCommandStack::clear() noexcept {
    m_undoStack.clear();
    m_redoStack.clear();
}

} // namespace QNodeGraph::Core
