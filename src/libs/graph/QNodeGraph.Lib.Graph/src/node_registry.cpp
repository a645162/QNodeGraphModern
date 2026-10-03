#include <QNodeGraph/Lib/Graph/node_registry.h>

#include <algorithm>
#include <utility>

namespace QNodeGraph::Graph {

namespace {

Core::GraphError error(Core::GraphErrorCode code, std::string message) {
    return Core::GraphError{code, std::move(message)};
}

PortDescriptor imagePort(std::string name, Core::PortDirection direction) {
    return PortDescriptor{std::move(name), direction, Core::PortDataType::Image,
                          false};
}

void registerBuiltin(NodeRegistry& registry, NodeDescriptor descriptor) {
    static_cast<void>(registry.registerNode(std::move(descriptor)));
}

} // namespace

Core::GraphResult<void> NodeRegistry::registerNode(NodeDescriptor descriptor) {
    if (descriptor.typeId.empty()) {
        return std::unexpected(error(Core::GraphErrorCode::InvalidDocument,
                                      "A node type id cannot be empty."));
    }
    if (m_descriptors.contains(descriptor.typeId)) {
        return std::unexpected(error(
            Core::GraphErrorCode::DuplicateNodeType,
            "Node type " + descriptor.typeId + " is already registered."));
    }
    m_descriptors.emplace(descriptor.typeId, std::move(descriptor));
    return {};
}

const NodeDescriptor* NodeRegistry::find(std::string_view typeId) const noexcept {
    const auto iterator = m_descriptors.find(std::string(typeId));
    return iterator == m_descriptors.end() ? nullptr : &iterator->second;
}

std::vector<NodeDescriptor> NodeRegistry::descriptors() const {
    std::vector<NodeDescriptor> values;
    values.reserve(m_descriptors.size());
    for (const auto& [typeId, descriptor] : m_descriptors) {
        static_cast<void>(typeId);
        values.push_back(descriptor);
    }
    return values;
}

Core::GraphResult<Core::NodeId> NodeRegistry::createNode(
    Core::GraphDocument& document, std::string_view typeId, std::string name,
    Core::Point position) const {
    const auto* descriptor = find(typeId);
    if (descriptor == nullptr) {
        return std::unexpected(error(
            Core::GraphErrorCode::UnknownNodeType,
            "Node type " + std::string(typeId) + " is not registered."));
    }
    if (name.empty()) {
        name = descriptor->displayName;
    }
    const auto node = document.addNode(std::string(typeId), std::move(name));
    if (!node) {
        return std::unexpected(node.error());
    }
    if (const auto result = document.setNodePosition(*node, position); !result) {
        const auto failure = result.error();
        static_cast<void>(document.removeNode(*node));
        return std::unexpected(failure);
    }
    for (const auto& port : descriptor->ports) {
        const auto result = document.addPort(
            *node, port.name, port.direction, port.dataType,
            port.acceptsMultipleConnections);
        if (!result) {
            const auto failure = result.error();
            static_cast<void>(document.removeNode(*node));
            return std::unexpected(failure);
        }
    }
    return *node;
}

NodeRegistry NodeRegistry::withBuiltins() {
    NodeRegistry registry;
    registerBuiltin(registry,
                    {"demo", "Demo Node",
                     {imagePort("in", Core::PortDirection::Input),
                      imagePort("out", Core::PortDirection::Output)}});
    registerBuiltin(registry,
                    {"load_image", "Load Image",
                     {imagePort("image", Core::PortDirection::Output)}});
    registerBuiltin(registry,
                    {"grayscale", "Grayscale",
                     {imagePort("image", Core::PortDirection::Input),
                      imagePort("image", Core::PortDirection::Output)}});
    registerBuiltin(registry,
                    {"blur", "Blur",
                     {imagePort("image", Core::PortDirection::Input),
                      imagePort("image", Core::PortDirection::Output)}});
    registerBuiltin(registry,
                    {"edge_detect", "Edge Detect",
                     {imagePort("image", Core::PortDirection::Input),
                      imagePort("image", Core::PortDirection::Output)}});
    registerBuiltin(registry,
                    {"image_preview", "Image Preview",
                     {imagePort("image", Core::PortDirection::Input)}});
    registerBuiltin(registry,
                    {"save_image", "Save Image",
                     {imagePort("image", Core::PortDirection::Input)}});
    registerBuiltin(registry,
                    {"group", "Group",
                     {imagePort("in", Core::PortDirection::Input),
                      imagePort("out", Core::PortDirection::Output)}});
    registerBuiltin(registry, {"backdrop", "Backdrop", {}});
    return registry;
}

} // namespace QNodeGraph::Graph
