#include <QNodeGraph/Lib/Core/graph_document.h>

#include <algorithm>
#include <sstream>
#include <utility>

namespace QNodeGraph::Core {

namespace {

GraphError error(GraphErrorCode code, std::string message) {
    return GraphError{code, std::move(message)};
}

std::string idText(std::uint64_t id) {
    std::ostringstream stream;
    stream << id;
    return stream.str();
}

bool compatible(PortDataType output, PortDataType input) {
    return output == PortDataType::Any || input == PortDataType::Any ||
           output == input;
}

} // namespace

GraphDocument::GraphDocument(std::string name) : m_name(std::move(name)) {}

const std::string& GraphDocument::name() const noexcept {
    return m_name;
}

void GraphDocument::setName(std::string name) {
    m_name = std::move(name);
}

std::size_t GraphDocument::nodeCount() const noexcept {
    return m_nodes.size();
}

std::size_t GraphDocument::connectionCount() const noexcept {
    return m_connections.size();
}

std::vector<NodeId> GraphDocument::nodeIds() const {
    std::vector<NodeId> ids;
    ids.reserve(m_nodes.size());
    for (const auto& [id, node] : m_nodes) {
        static_cast<void>(node);
        ids.push_back(id);
    }
    return ids;
}

const std::vector<Connection>& GraphDocument::connections() const noexcept {
    return m_connections;
}

GraphResult<NodeId> GraphDocument::addNode(std::string type, std::string name,
                                           NodeId id) {
    if (id == 0) {
        id = m_nextNodeId++;
    } else if (m_nodes.contains(id)) {
        return std::unexpected(error(GraphErrorCode::DuplicateNodeId,
                                      "Node id " + idText(id) +
                                          " is already registered."));
    } else {
        m_nextNodeId = std::max(m_nextNodeId, id + 1);
    }

    m_nodes.emplace(id, Node{id, std::move(type), std::move(name), {}, {}, {}});
    return id;
}

GraphResult<PortId> GraphDocument::addPort(
    NodeId nodeId, std::string name, PortDirection direction,
    PortDataType dataType, bool acceptsMultipleConnections, PortId id) {
    if (!m_nodes.contains(nodeId)) {
        return std::unexpected(error(GraphErrorCode::NodeNotFound,
                                      "Node id " + idText(nodeId) +
                                          " does not exist."));
    }
    if (id == 0) {
        id = m_nextPortId++;
    } else if (m_ports.contains(id)) {
        return std::unexpected(error(GraphErrorCode::DuplicatePortId,
                                      "Port id " + idText(id) +
                                          " is already registered."));
    } else {
        m_nextPortId = std::max(m_nextPortId, id + 1);
    }

    m_ports.emplace(id, Port{id, nodeId, std::move(name), direction, dataType,
                             acceptsMultipleConnections});
    m_nodes.at(nodeId).ports.push_back(id);
    return id;
}

GraphResult<void> GraphDocument::connect(PortId outputPort, PortId inputPort) {
    const auto output = requirePort(outputPort);
    if (!output) {
        return std::unexpected(output.error());
    }
    const auto input = requirePort(inputPort);
    if (!input) {
        return std::unexpected(input.error());
    }
    if ((*output)->direction != PortDirection::Output ||
        (*input)->direction != PortDirection::Input) {
        return std::unexpected(error(
            GraphErrorCode::InvalidPortDirection,
            "Connections require an output port followed by an input port."));
    }
    if (!compatible((*output)->dataType, (*input)->dataType)) {
        return std::unexpected(error(
            GraphErrorCode::IncompatiblePortTypes,
            "The output and input port data types are incompatible."));
    }
    const auto duplicate = std::find_if(
        m_connections.begin(), m_connections.end(),
        [outputPort, inputPort](const Connection& connection) {
            return connection.outputPort == outputPort &&
                   connection.inputPort == inputPort;
        });
    if (duplicate != m_connections.end()) {
        return std::unexpected(error(GraphErrorCode::DuplicateConnection,
                                      "The connection already exists."));
    }
    if (!(*input)->acceptsMultipleConnections) {
        const auto existing = std::find_if(
            m_connections.begin(), m_connections.end(),
            [inputPort](const Connection& connection) {
                return connection.inputPort == inputPort;
            });
        if (existing != m_connections.end()) {
            return std::unexpected(error(
                GraphErrorCode::InputAlreadyConnected,
                "The input port already has a connection."));
        }
    }
    if (wouldCreateCycle((*output)->nodeId, (*input)->nodeId)) {
        return std::unexpected(error(GraphErrorCode::CycleDetected,
                                      "The connection would create a cycle."));
    }

    m_connections.push_back(Connection{outputPort, inputPort});
    return {};
}

GraphResult<void> GraphDocument::disconnect(PortId outputPort,
                                            PortId inputPort) {
    const auto iterator = std::find_if(
        m_connections.begin(), m_connections.end(),
        [outputPort, inputPort](const Connection& connection) {
            return connection.outputPort == outputPort &&
                   connection.inputPort == inputPort;
        });
    if (iterator == m_connections.end()) {
        return std::unexpected(error(GraphErrorCode::PortNotFound,
                                      "The connection does not exist."));
    }
    m_connections.erase(iterator);
    return {};
}

GraphResult<void> GraphDocument::setNodePosition(NodeId nodeId,
                                                  Point position) {
    if (!m_nodes.contains(nodeId)) {
        return std::unexpected(error(GraphErrorCode::NodeNotFound,
                                      "Node id " + idText(nodeId) +
                                          " does not exist."));
    }
    m_nodes.at(nodeId).position = position;
    return {};
}

GraphResult<void> GraphDocument::setProperty(NodeId nodeId, std::string name,
                                             PropertyValue value) {
    if (!m_nodes.contains(nodeId)) {
        return std::unexpected(error(GraphErrorCode::NodeNotFound,
                                      "Node id " + idText(nodeId) +
                                          " does not exist."));
    }
    m_nodes.at(nodeId).properties.insert_or_assign(std::move(name),
                                                   std::move(value));
    return {};
}

GraphResult<void> GraphDocument::clearProperty(NodeId nodeId,
                                                const std::string& name) {
    if (!m_nodes.contains(nodeId)) {
        return std::unexpected(error(GraphErrorCode::NodeNotFound,
                                      "Node id " + idText(nodeId) +
                                          " does not exist."));
    }
    m_nodes.at(nodeId).properties.erase(name);
    return {};
}

const Node* GraphDocument::node(NodeId id) const noexcept {
    const auto iterator = m_nodes.find(id);
    return iterator == m_nodes.end() ? nullptr : &iterator->second;
}

const Port* GraphDocument::port(PortId id) const noexcept {
    const auto iterator = m_ports.find(id);
    return iterator == m_ports.end() ? nullptr : &iterator->second;
}

const PropertyValue* GraphDocument::property(
    NodeId nodeId, const std::string& name) const noexcept {
    const auto* value = node(nodeId);
    if (value == nullptr) {
        return nullptr;
    }
    const auto iterator = value->properties.find(name);
    return iterator == value->properties.end() ? nullptr : &iterator->second;
}

GraphResult<NodeSnapshot> GraphDocument::removeNode(NodeId nodeId) {
    const auto iterator = m_nodes.find(nodeId);
    if (iterator == m_nodes.end()) {
        return std::unexpected(error(GraphErrorCode::NodeNotFound,
                                      "Node id " + idText(nodeId) +
                                          " does not exist."));
    }

    NodeSnapshot snapshot;
    snapshot.node = iterator->second;
    snapshot.ports.reserve(snapshot.node.ports.size());
    for (const auto portId : snapshot.node.ports) {
        if (const auto* value = port(portId); value != nullptr) {
            snapshot.ports.push_back(*value);
        }
    }
    const auto touchesNode = [this, nodeId](const Connection& connection) {
        const auto* output = port(connection.outputPort);
        const auto* input = port(connection.inputPort);
        return (output != nullptr && output->nodeId == nodeId) ||
               (input != nullptr && input->nodeId == nodeId);
    };
    for (const auto& connection : m_connections) {
        if (touchesNode(connection)) {
            snapshot.connections.push_back(connection);
        }
    }

    m_connections.erase(
        std::remove_if(m_connections.begin(), m_connections.end(), touchesNode),
        m_connections.end());
    for (const auto& value : snapshot.ports) {
        m_ports.erase(value.id);
    }
    m_nodes.erase(iterator);
    return snapshot;
}

GraphResult<void> GraphDocument::restoreNode(NodeSnapshot snapshot) {
    if (m_nodes.contains(snapshot.node.id)) {
        return std::unexpected(error(GraphErrorCode::DuplicateNodeId,
                                      "The node snapshot id is already used."));
    }
    for (const auto& value : snapshot.ports) {
        if (m_ports.contains(value.id)) {
            return std::unexpected(error(GraphErrorCode::DuplicatePortId,
                                          "The port snapshot id is already used."));
        }
        if (value.nodeId != snapshot.node.id) {
            return std::unexpected(error(
                GraphErrorCode::NodeNotFound,
                "The port snapshot references another node."));
        }
    }

    m_nodes.emplace(snapshot.node.id, snapshot.node);
    for (const auto& value : snapshot.ports) {
        m_ports.emplace(value.id, value);
    }
    m_nextNodeId = std::max(m_nextNodeId, snapshot.node.id + 1);
    for (const auto& value : snapshot.ports) {
        m_nextPortId = std::max(m_nextPortId, value.id + 1);
    }
    for (const auto& connection : snapshot.connections) {
        m_connections.push_back(connection);
    }
    return {};
}

void GraphDocument::clear() {
    m_nodes.clear();
    m_ports.clear();
    m_connections.clear();
}

GraphResult<const Node*> GraphDocument::requireNode(NodeId id) const {
    const auto* value = node(id);
    if (value == nullptr) {
        return std::unexpected(error(GraphErrorCode::NodeNotFound,
                                      "Node id " + idText(id) +
                                          " does not exist."));
    }
    return value;
}

GraphResult<const Port*> GraphDocument::requirePort(PortId id) const {
    const auto* value = port(id);
    if (value == nullptr) {
        return std::unexpected(error(GraphErrorCode::PortNotFound,
                                      "Port id " + idText(id) +
                                          " does not exist."));
    }
    return value;
}

bool GraphDocument::wouldCreateCycle(NodeId outputNode, NodeId inputNode) const {
    return outputNode == inputNode || hasPath(inputNode, outputNode);
}

bool GraphDocument::hasPath(NodeId from, NodeId target) const {
    std::vector<NodeId> pending{from};
    std::vector<NodeId> visited;
    while (!pending.empty()) {
        const auto current = pending.back();
        pending.pop_back();
        if (current == target) {
            return true;
        }
        if (std::find(visited.begin(), visited.end(), current) != visited.end()) {
            continue;
        }
        visited.push_back(current);
        for (const auto& connection : m_connections) {
            const auto* output = port(connection.outputPort);
            const auto* input = port(connection.inputPort);
            if (output != nullptr && input != nullptr &&
                output->nodeId == current) {
                pending.push_back(input->nodeId);
            }
        }
    }
    return false;
}

} // namespace QNodeGraph::Core
