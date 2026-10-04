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

enum class PropertyPlacement {
    Hidden,       // Not shown anywhere (internal storage only).
    Node,         // Rendered on the node body.
    Panel,        // Rendered in the properties panel only.
    NodeAndPanel, // Rendered both on the node and in the panel.
};

struct PropertyDescriptor {
    std::string name;
    PropertyPlacement placement = PropertyPlacement::Panel;
};

struct NodeDescriptor {
    std::string typeId;
    std::string displayName;
    std::vector<PortDescriptor> ports;
    std::vector<PropertyDescriptor> properties;
    // Optional QML component URL loaded as the node's custom content body.
    std::string qmlContentUrl;
    // Optional C++ content provider key resolved by the host application.
    std::string contentProviderKey;
    bool deletable = true;
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
