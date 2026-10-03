#pragma once

#include <cstddef>
#include <cstdint>
#include <expected>
#include <map>
#include <string>
#include <variant>
#include <vector>

namespace QNodeGraph::Core {

using NodeId = std::uint64_t;
using PortId = std::uint64_t;
using PropertyValue = std::variant<bool, double, std::int64_t, std::string>;

enum class PortDirection {
    Input,
    Output,
};

enum class PortDataType {
    Any,
    Image,
    Number,
    Text,
    Boolean,
};

enum class GraphErrorCode {
    DuplicateNodeId,
    DuplicatePortId,
    NodeNotFound,
    PortNotFound,
    InvalidPortDirection,
    IncompatiblePortTypes,
    InputAlreadyConnected,
    DuplicateConnection,
    CycleDetected,
    CommandStackEmpty,
};

struct GraphError {
    GraphErrorCode code;
    std::string message;
};

template <typename T>
using GraphResult = std::expected<T, GraphError>;

struct Point {
    double x = 0.0;
    double y = 0.0;
};

struct Port {
    PortId id = 0;
    NodeId nodeId = 0;
    std::string name;
    PortDirection direction = PortDirection::Input;
    PortDataType dataType = PortDataType::Any;
    bool acceptsMultipleConnections = false;
};

struct Node {
    NodeId id = 0;
    std::string type;
    std::string name;
    Point position;
    std::map<std::string, PropertyValue> properties;
    std::vector<PortId> ports;
};

struct Connection {
    PortId outputPort = 0;
    PortId inputPort = 0;
};

struct NodeSnapshot {
    Node node;
    std::vector<Port> ports;
    std::vector<Connection> connections;
};

class GraphDocument {
public:
    explicit GraphDocument(std::string name = "Untitled");

    [[nodiscard]] const std::string& name() const noexcept;
    void setName(std::string name);

    [[nodiscard]] std::size_t nodeCount() const noexcept;
    [[nodiscard]] std::size_t connectionCount() const noexcept;
    [[nodiscard]] const std::vector<Connection>& connections() const noexcept;

    GraphResult<NodeId> addNode(std::string type = "generic",
                                std::string name = "Node",
                                NodeId id = 0);
    GraphResult<PortId> addPort(NodeId nodeId, std::string name,
                                PortDirection direction,
                                PortDataType dataType,
                                bool acceptsMultipleConnections = false,
                                PortId id = 0);
    GraphResult<void> connect(PortId outputPort, PortId inputPort);
    GraphResult<void> disconnect(PortId outputPort, PortId inputPort);
    GraphResult<NodeSnapshot> removeNode(NodeId nodeId);
    GraphResult<void> restoreNode(NodeSnapshot snapshot);
    GraphResult<void> setNodePosition(NodeId nodeId, Point position);
    GraphResult<void> setProperty(NodeId nodeId, std::string name,
                                  PropertyValue value);
    GraphResult<void> clearProperty(NodeId nodeId, const std::string& name);

    [[nodiscard]] const Node* node(NodeId id) const noexcept;
    [[nodiscard]] const Port* port(PortId id) const noexcept;
    [[nodiscard]] const PropertyValue* property(
        NodeId nodeId, const std::string& name) const noexcept;

    void clear();

private:
    [[nodiscard]] GraphResult<const Node*> requireNode(NodeId id) const;
    [[nodiscard]] GraphResult<const Port*> requirePort(PortId id) const;
    [[nodiscard]] bool wouldCreateCycle(NodeId outputNode,
                                        NodeId inputNode) const;
    [[nodiscard]] bool hasPath(NodeId from, NodeId target) const;

    std::string m_name;
    std::map<NodeId, Node> m_nodes;
    std::map<PortId, Port> m_ports;
    std::vector<Connection> m_connections;
    NodeId m_nextNodeId = 1;
    PortId m_nextPortId = 1;
};

} // namespace QNodeGraph::Core
