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
    for (const auto& property : descriptor->properties) {
        const auto placement =
            property.placement == PropertyPlacement::Node
                ? "node"
                : (property.placement == PropertyPlacement::NodeAndPanel
                       ? "both"
                       : (property.placement == PropertyPlacement::Panel ? "panel"
                                                                         : "hidden"));
        static_cast<void>(document.setProperty(*node, property.name + ".display",
                                               std::string(placement)));
    }
    if (!descriptor->deletable) {
        static_cast<void>(document.setProperty(*node, "fixed", true));
    }
    return *node;
}

NodeRegistry NodeRegistry::withBuiltins() {
    NodeRegistry registry;

    NodeDescriptor demo;
    demo.typeId = "demo";
    demo.displayName = "Demo Node";
    demo.ports = {imagePort("in", Core::PortDirection::Input),
                  imagePort("out", Core::PortDirection::Output)};
    demo.properties = {{"label", PropertyPlacement::Panel}};
    registerBuiltin(registry, std::move(demo));

    NodeDescriptor load;
    load.typeId = "load_image";
    load.displayName = "Load Image";
    load.ports = {imagePort("image", Core::PortDirection::Output)};
    load.properties = {{"label", PropertyPlacement::Panel},
                       {"path", PropertyPlacement::Panel}};
    load.deletable = false;
    registerBuiltin(registry, std::move(load));

    NodeDescriptor grayscale;
    grayscale.typeId = "grayscale";
    grayscale.displayName = "Grayscale";
    grayscale.ports = {imagePort("image", Core::PortDirection::Input),
                       imagePort("image", Core::PortDirection::Output)};
    grayscale.properties = {{"label", PropertyPlacement::Panel}};
    registerBuiltin(registry, std::move(grayscale));

    NodeDescriptor blur;
    blur.typeId = "blur";
    blur.displayName = "Blur";
    blur.ports = {imagePort("image", Core::PortDirection::Input),
                  imagePort("image", Core::PortDirection::Output)};
    blur.properties = {{"label", PropertyPlacement::Panel},
                       {"radius", PropertyPlacement::NodeAndPanel}};
    blur.qmlContentUrl = "qrc:/qt/qml/QNodeGraph/UI/qml/content/NodeImageContent.qml";
    registerBuiltin(registry, std::move(blur));

    NodeDescriptor edge;
    edge.typeId = "edge_detect";
    edge.displayName = "Edge Detect";
    edge.ports = {imagePort("image", Core::PortDirection::Input),
                  imagePort("image", Core::PortDirection::Output)};
    edge.properties = {{"label", PropertyPlacement::Panel},
                       {"threshold", PropertyPlacement::NodeAndPanel}};
    registerBuiltin(registry, std::move(edge));

    NodeDescriptor preview;
    preview.typeId = "image_preview";
    preview.displayName = "Image Preview";
    preview.ports = {imagePort("image", Core::PortDirection::Input)};
    preview.properties = {{"label", PropertyPlacement::Panel}};
    registerBuiltin(registry, std::move(preview));

    NodeDescriptor save;
    save.typeId = "save_image";
    save.displayName = "Save Image";
    save.ports = {imagePort("image", Core::PortDirection::Input)};
    save.properties = {{"label", PropertyPlacement::Panel},
                       {"path", PropertyPlacement::Panel}};
    save.deletable = false;
    registerBuiltin(registry, std::move(save));

    registerBuiltin(registry,
                    {"group", "Group",
                     {imagePort("in", Core::PortDirection::Input),
                      imagePort("out", Core::PortDirection::Output)}});
    registerBuiltin(registry, {"backdrop", "Backdrop", {}});
    return registry;
}

} // namespace QNodeGraph::Graph
