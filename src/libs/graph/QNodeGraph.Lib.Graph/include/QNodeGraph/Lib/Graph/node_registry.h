#pragma once

#include <QNodeGraph/Lib/Core/graph_document.h>

#include <map>
#include <optional>
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

// 属性控件的呈现提示。UI 层据此选择编辑控件，而不是靠 valueType 猜 ——
// 例如阈值需要滑块、颜色需要取色器、路径需要文件选择框。
//
// 之所以单独建枚举而不复用 PropertyValue 的变体类型：PropertyValue 只表达
// 数据（bool/double/int64/string），无法承载「用哪种控件」「取值范围多少」，
// 而这些属于描述符层的元数据。
enum class PropertyEditor {
    Automatic, // 由 UI 按 valueType 推断（默认）。
    CheckBox,  // bool 开关。
    Slider,    // 数值滑块，配合 minimum/maximum/step。
    SpinBox,   // 数值输入框，配合 minimum/maximum/step。
    LineEdit,  // 单行文本。
    FilePath,  // 文件路径（宿主提供文件选择）。
    Directory, // 目录路径。
    ComboBox,  // 枚举选择，配合 choices。
    Matrix,    // 矩阵/卷积核编辑器，配合 matrixRows/matrixCols。
    ColorPicker, // 颜色选择器，值为 #RRGGBB 或 #AARRGGBB。
};

// 属性值的业务语义。PropertyValue 仍保持轻量的四种存储变体，
// 这里的类型用于校验、编辑器选择和文档生成，不改变持久化表示。
enum class PropertyValueKind {
    Automatic,
    Int,
    Real,
    Bool,
    String,
    Enum,
    Matrix,
    Color,
};

// 属性在面板中的可见性层级。Basic 默认展开，Advanced/Developer 可由 UI
// 折叠，便于复杂算子同时保留完整参数而不挤占主工作区。
enum class PropertyVisibility {
    Basic,
    Advanced,
    Developer,
};

struct PropertyDescriptor {
    std::string name;
    PropertyPlacement placement = PropertyPlacement::Panel;
    // --- 以下为控件元数据，均为可选（0 / 空表示未声明）---
    PropertyEditor editor = PropertyEditor::Automatic;
    PropertyValueKind valueKind = PropertyValueKind::Automatic;
    PropertyVisibility visibility = PropertyVisibility::Basic;
    std::string unit;
    bool logScale = false;
    int matrixRows = 0;
    int matrixCols = 0;
    std::string description;
    double minimum = 0.0;
    double maximum = 0.0;
    double step = 0.0;
    // 枚举候选项（editor == ComboBox 时使用）。
    std::vector<std::string> choices;
    // 未设置时的初值。未给出则创建节点时不写入该属性。
    std::optional<Core::PropertyValue> defaultValue;
};

// 节点的视觉角色。UI 层据此决定层级与容器语义，取代按 typeId 字符串硬比较
// （如 type == "backdrop"）。
enum class NodeVisualRole {
    Standard,  // 普通节点。
    Group,     // 分组容器：可容纳其它节点。
    Backdrop,  // 背景板：置于最底层。
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
    // 固定节点：不可删除。配合 unique 表达「不可新增多个」。
    bool fixed = false;
    // 唯一节点：同一文档中最多存在一个。用于《图像输入》《图像输出》这类
    // 必须唯一存在的固定组件；从调色板重复添加时会被拒绝。
    bool unique = false;

    // --- 外观元数据（由描述符声明，取代 UI 层按 typeId 的硬编码）---
    // 标题栏左上角的短标签，如 "FX"。空表示不显示字形。
    std::string iconGlyph;
    // 图标资源路径（SVG）。空表示回退到通用图标。
    std::string iconSource;
    // 标题栏强调色（#RRGGBB）。空表示回退到默认色。
    std::string accent;

    // --- 尺寸元数据 ---
    double defaultWidth = 180.0;
    double defaultHeight = 108.0;
    double minimumWidth = 120.0;
    double minimumHeight = 72.0;

    // 视觉角色（取代 typeId 字符串比较）。
    NodeVisualRole visualRole = NodeVisualRole::Standard;
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

    // 统计文档中某类型节点的数量。唯一性校验（Descriptor::unique）由调用方
    // 在创建前使用；放在注册表上是因为它同时需要描述符与文档。
    [[nodiscard]] static std::size_t countNodesOfType(
        const Core::GraphDocument& document, std::string_view typeId);

private:
    std::map<std::string, NodeDescriptor> m_descriptors;
};

} // namespace QNodeGraph::Graph
