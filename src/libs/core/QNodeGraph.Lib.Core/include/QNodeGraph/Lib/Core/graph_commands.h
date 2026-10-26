#pragma once

#include <QNodeGraph/Lib/Core/graph_document.h>

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace QNodeGraph::Core {

class GraphCommand {
public:
    virtual ~GraphCommand() = default;
    virtual GraphResult<void> execute(GraphDocument& document) = 0;
    virtual GraphResult<void> undo(GraphDocument& document) = 0;
    [[nodiscard]] virtual const char* name() const noexcept = 0;
};

class CreateNodeCommand final : public GraphCommand {
public:
    CreateNodeCommand(std::string type, std::string name);
    GraphResult<void> execute(GraphDocument& document) override;
    GraphResult<void> undo(GraphDocument& document) override;
    [[nodiscard]] const char* name() const noexcept override;
    [[nodiscard]] std::optional<NodeId> nodeId() const noexcept;

private:
    std::string m_type;
    std::string m_name;
    std::optional<NodeId> m_nodeId;
};

class DeleteNodeCommand final : public GraphCommand {
public:
    explicit DeleteNodeCommand(NodeId nodeId);
    GraphResult<void> execute(GraphDocument& document) override;
    GraphResult<void> undo(GraphDocument& document) override;
    [[nodiscard]] const char* name() const noexcept override;

private:
    NodeId m_nodeId;
    std::optional<NodeSnapshot> m_snapshot;
};

class MoveNodeCommand final : public GraphCommand {
public:
    MoveNodeCommand(NodeId nodeId, Point position);
    GraphResult<void> execute(GraphDocument& document) override;
    GraphResult<void> undo(GraphDocument& document) override;
    [[nodiscard]] const char* name() const noexcept override;

private:
    NodeId m_nodeId;
    Point m_position;
    std::optional<Point> m_previousPosition;
};

class MoveNodesCommand final : public GraphCommand {
public:
    explicit MoveNodesCommand(std::vector<std::pair<NodeId, Point>> targets);
    GraphResult<void> execute(GraphDocument& document) override;
    GraphResult<void> undo(GraphDocument& document) override;
    [[nodiscard]] const char* name() const noexcept override;

private:
    std::vector<std::pair<NodeId, Point>> m_targets;
    std::vector<std::pair<NodeId, Point>> m_previousPositions;
};

class ConnectPortsCommand final : public GraphCommand {
public:
    ConnectPortsCommand(PortId outputPort, PortId inputPort);
    GraphResult<void> execute(GraphDocument& document) override;
    GraphResult<void> undo(GraphDocument& document) override;
    [[nodiscard]] const char* name() const noexcept override;

private:
    PortId m_outputPort;
    PortId m_inputPort;
};

class DisconnectPortsCommand final : public GraphCommand {
public:
    DisconnectPortsCommand(PortId outputPort, PortId inputPort);
    GraphResult<void> execute(GraphDocument& document) override;
    GraphResult<void> undo(GraphDocument& document) override;
    [[nodiscard]] const char* name() const noexcept override;

private:
    PortId m_outputPort;
    PortId m_inputPort;
};

class SetPropertyCommand final : public GraphCommand {
public:
    SetPropertyCommand(NodeId nodeId, std::string name, PropertyValue value);
    GraphResult<void> execute(GraphDocument& document) override;
    GraphResult<void> undo(GraphDocument& document) override;
    [[nodiscard]] const char* name() const noexcept override;

private:
    NodeId m_nodeId;
    std::string m_name;
    PropertyValue m_value;
    std::optional<PropertyValue> m_previousValue;
};

// 节点尺寸调整（宽 + 高）的原子命令。
//
// 为什么需要单独一条命令：尺寸是两个独立属性（"width" / "height"），若用两条
// SetPropertyCommand，撤销一次只回退一半尺寸 —— 用户要按两次 Ctrl+Z 才能复原，
// 且中间态会出现「宽度已改、高度未改」的畸形节点。此命令把两次写入合并为一次
// 可撤销操作。
//
// m_position 为调整后节点的左上角世界坐标：只有从右下角拖拽时左上角不动；
// 从左/上侧拖拽会同时改变左上角，因此 origin 必须与尺寸一起进入撤销快照。
class ResizeNodeCommand final : public GraphCommand {
public:
    ResizeNodeCommand(NodeId nodeId, Point position, double width,
                      double height);
    GraphResult<void> execute(GraphDocument& document) override;
    GraphResult<void> undo(GraphDocument& document) override;
    [[nodiscard]] const char* name() const noexcept override;

private:
    NodeId m_nodeId;
    Point m_position;
    double m_width;
    double m_height;
    std::optional<Point> m_previousPosition;
    std::optional<PropertyValue> m_previousWidth;
    std::optional<PropertyValue> m_previousHeight;
    bool m_previousWidthWasSet = false;
    bool m_previousHeightWasSet = false;
};

class GraphCommandStack final {
public:
    GraphResult<void> execute(std::unique_ptr<GraphCommand> command,
                              GraphDocument& document);
    GraphResult<void> undo(GraphDocument& document);
    GraphResult<void> redo(GraphDocument& document);
    [[nodiscard]] bool canUndo() const noexcept;
    [[nodiscard]] bool canRedo() const noexcept;
    void clear() noexcept;

private:
    std::vector<std::unique_ptr<GraphCommand>> m_undoStack;
    std::vector<std::unique_ptr<GraphCommand>> m_redoStack;
};

} // namespace QNodeGraph::Core
