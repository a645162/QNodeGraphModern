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
        // 写入描述符声明的初值。仅在调用方未显式提供该属性时写入，避免覆盖
        // 宿主在创建前预设的值。
        if (property.defaultValue.has_value() &&
            document.property(*node, property.name) == nullptr) {
            static_cast<void>(document.setProperty(*node, property.name,
                                                   *property.defaultValue));
        }
    }
    // deletable 与 fixed 都表达「不可删除」，后者是新的显式声明方式；
    // 保留 deletable 的语义以兼容既有描述符。
    if (!descriptor->deletable || descriptor->fixed) {
        static_cast<void>(document.setProperty(*node, "fixed", true));
    }
    return *node;
}

std::size_t NodeRegistry::countNodesOfType(const Core::GraphDocument& document,
                                           std::string_view typeId) {
    std::size_t count = 0;
    for (const auto nodeId : document.nodeIds()) {
        const auto* node = document.node(nodeId);
        if (node != nullptr && node->type == typeId) {
            ++count;
        }
    }
    return count;
}

NodeRegistry NodeRegistry::withBuiltins() {
    NodeRegistry registry;

    // 外观元数据（iconGlyph / iconSource / accent）此前硬编码在
    // graph_controller.cpp 的三个自由函数里，按 typeId 做 if 链。现迁移到
    // 描述符，宿主注册自定义节点时即可一并声明外观，无需修改库代码。
    constexpr auto kIconPathPrefix =
        "qrc:/qt/qml/QNodeGraph/UI/qml/icons/";

    NodeDescriptor demo;
    demo.typeId = "demo";
    demo.displayName = "Demo Node";
    demo.ports = {imagePort("in", Core::PortDirection::Input),
                  imagePort("out", Core::PortDirection::Output)};
    demo.properties = {{"label", PropertyPlacement::Panel}};
    demo.iconGlyph = "NODE";
    demo.iconSource = std::string(kIconPathPrefix) + "node.svg";
    demo.accent = "#426b91";
    registerBuiltin(registry, std::move(demo));

    NodeDescriptor load;
    load.typeId = "load_image";
    load.displayName = "Load Image";
    load.ports = {imagePort("image", Core::PortDirection::Output)};
    load.properties = {
        {"label", PropertyPlacement::Panel},
        {.name = "path", .placement = PropertyPlacement::Panel,
         .editor = PropertyEditor::FilePath}};
    load.deletable = false;
    load.iconGlyph = "IMG";
    load.iconSource = std::string(kIconPathPrefix) + "load_image.svg";
    load.accent = "#4f8fc6";
    registerBuiltin(registry, std::move(load));

    NodeDescriptor grayscale;
    grayscale.typeId = "grayscale";
    grayscale.displayName = "Grayscale";
    grayscale.ports = {imagePort("image", Core::PortDirection::Input),
                       imagePort("image", Core::PortDirection::Output)};
    grayscale.properties = {{"label", PropertyPlacement::Panel}};
    grayscale.iconGlyph = "FX";
    grayscale.iconSource = std::string(kIconPathPrefix) + "grayscale.svg";
    grayscale.accent = "#6f7bd8";
    registerBuiltin(registry, std::move(grayscale));

    NodeDescriptor blur;
    blur.typeId = "blur";
    blur.displayName = "Blur";
    blur.ports = {imagePort("image", Core::PortDirection::Input),
                  imagePort("image", Core::PortDirection::Output)};
    blur.properties = {
        {"label", PropertyPlacement::Panel},
        // 半径用滑块呈现，范围 1..50；默认 1 与既有的 int 语义一致。
        {.name = "radius", .placement = PropertyPlacement::NodeAndPanel,
         .editor = PropertyEditor::Slider, .minimum = 1.0, .maximum = 50.0,
         .step = 1.0, .choices = {}, .defaultValue = std::int64_t{1}}};
    blur.qmlContentUrl = "qrc:/qt/qml/QNodeGraph/UI/qml/content/NodeImageContent.qml";
    blur.iconGlyph = "BLR";
    blur.iconSource = std::string(kIconPathPrefix) + "blur.svg";
    blur.accent = "#9568c7";
    registerBuiltin(registry, std::move(blur));

    NodeDescriptor edge;
    edge.typeId = "edge_detect";
    edge.displayName = "Edge Detect";
    edge.ports = {imagePort("image", Core::PortDirection::Input),
                  imagePort("image", Core::PortDirection::Output)};
    edge.properties = {
        {"label", PropertyPlacement::Panel},
        // 阈值滑块 0..255，默认 100。
        {.name = "threshold", .placement = PropertyPlacement::NodeAndPanel,
         .editor = PropertyEditor::Slider, .minimum = 0.0, .maximum = 255.0,
         .step = 1.0, .choices = {}, .defaultValue = std::int64_t{100}}};
    edge.iconGlyph = "EDG";
    edge.iconSource = std::string(kIconPathPrefix) + "edge_detect.svg";
    edge.accent = "#b26d5c";
    registerBuiltin(registry, std::move(edge));

    NodeDescriptor preview;
    preview.typeId = "image_preview";
    preview.displayName = "Image Preview";
    preview.ports = {imagePort("image", Core::PortDirection::Input)};
    preview.properties = {{"label", PropertyPlacement::Panel}};
    preview.iconGlyph = "VIEW";
    preview.iconSource = std::string(kIconPathPrefix) + "image_preview.svg";
    preview.accent = "#4aaf85";
    registerBuiltin(registry, std::move(preview));

    NodeDescriptor save;
    save.typeId = "save_image";
    save.displayName = "Save Image";
    save.ports = {imagePort("image", Core::PortDirection::Input)};
    save.properties = {
        {"label", PropertyPlacement::Panel},
        {.name = "path", .placement = PropertyPlacement::Panel,
         .editor = PropertyEditor::FilePath}};
    save.deletable = false;
    save.iconGlyph = "SAVE";
    save.iconSource = std::string(kIconPathPrefix) + "save_image.svg";
    save.accent = "#71859a";
    registerBuiltin(registry, std::move(save));

    // group / backdrop 此前在 addNodeType 里按 typeId 硬编码尺寸与颜色。现由
    // 描述符声明，addNodeType 的特判可随之移除。
    NodeDescriptor group;
    group.typeId = "group";
    group.displayName = "Group";
    group.ports = {imagePort("in", Core::PortDirection::Input),
                   imagePort("out", Core::PortDirection::Output)};
    group.visualRole = NodeVisualRole::Group;
    group.defaultWidth = 240.0;
    group.defaultHeight = 140.0;
    group.iconGlyph = "GRP";
    group.iconSource = std::string(kIconPathPrefix) + "group.svg";
    group.accent = "#4a718c";
    registerBuiltin(registry, std::move(group));

    NodeDescriptor backdrop;
    backdrop.typeId = "backdrop";
    backdrop.displayName = "Backdrop";
    backdrop.visualRole = NodeVisualRole::Backdrop;
    backdrop.defaultWidth = 560.0;
    backdrop.defaultHeight = 300.0;
    backdrop.iconGlyph = "BG";
    backdrop.iconSource = std::string(kIconPathPrefix) + "backdrop.svg";
    backdrop.accent = "#557081";
    registerBuiltin(registry, std::move(backdrop));
    return registry;
}

} // namespace QNodeGraph::Graph
