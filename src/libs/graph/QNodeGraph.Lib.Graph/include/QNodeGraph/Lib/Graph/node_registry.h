#pragma once

#include <QNodeGraph/Lib/Core/graph_document.h>

#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace QNodeGraph::Graph {

struct PortDescriptor {
    std::string name;
    Core::PortDirection direction = Core::PortDirection::Input;
    Core::PortDataType dataType = Core::PortDataType::Any;
    bool acceptsMultipleConnections = false;
};

struct NodeDescriptor {
    std::string typeId;
    std::string displayName;
    std::vector<PortDescriptor> ports;
};

class NodeRegistry final {
public:
    [[nodiscard]] Core::GraphResult<void> registerNode(
        NodeDescriptor descriptor);
    [[nodiscard]] const NodeDescriptor* find(
        std::string_view typeId) const noexcept;
    [[nodiscard]] std::vector<NodeDescriptor> descriptors() const;
    [[nodiscard]] Core::GraphResult<Core::NodeId> createNode(
        Core::GraphDocument& document, std::string_view typeId,
        std::string name = {}, Core::Point position = {}) const;

    [[nodiscard]] static NodeRegistry withBuiltins();

private:
    std::map<std::string, NodeDescriptor> m_descriptors;
};

} // namespace QNodeGraph::Graph
