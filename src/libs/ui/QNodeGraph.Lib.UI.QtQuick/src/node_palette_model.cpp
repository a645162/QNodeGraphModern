#include <QNodeGraph/Lib/UI/QtQuick/node_palette_model.h>
#include <QNodeGraph/Lib/UI/QtQuick/graph_controller.h>

#include <QVariantMap>

#include <algorithm>
#include <string>

namespace QNodeGraph::UI {

NodePaletteModel::NodePaletteModel(QObject* parent)
    : QAbstractListModel(parent),
      m_registry(QNodeGraph::Graph::NodeRegistry::withBuiltins()),
      m_descriptors(m_registry.descriptors()) {
    rebuildVisibleRows();
}

void NodePaletteModel::setRegistry(QNodeGraph::Graph::NodeRegistry registry) {
    m_registry = std::move(registry);
    beginResetModel();
    m_descriptors = m_registry.descriptors();
    rebuildVisibleRows();
    endResetModel();
}

void NodePaletteModel::setHideFixedNodes(bool hide) {
    if (m_hideFixedNodes == hide) {
        return;
    }
    m_hideFixedNodes = hide;
    beginResetModel();
    rebuildVisibleRows();
    endResetModel();
}

bool NodePaletteModel::hideFixedNodes() const noexcept {
    return m_hideFixedNodes;
}

QObject *NodePaletteModel::controller() const noexcept { return m_controller; }

void NodePaletteModel::setController(QObject *controller) {
    if (m_controller == controller) {
        return;
    }
    m_controller = controller;
    if (const auto *graph = qobject_cast<GraphController *>(controller)) {
        setRegistry(graph->registry());
        setHideFixedNodes(true);
        QObject::connect(graph, &GraphController::registryChanged, this,
                         [this, graph]() { setRegistry(graph->registry()); });
    }
    emit controllerChanged();
}

int NodePaletteModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(m_visibleRows.size());
}

QVariant NodePaletteModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 ||
        index.row() >= static_cast<int>(m_visibleRows.size())) {
        return {};
    }
    const auto descriptorIndex = m_visibleRows[static_cast<std::size_t>(index.row())];
    const auto& descriptor = m_descriptors[static_cast<std::size_t>(descriptorIndex)];
    switch (role) {
    case Qt::DisplayRole:
    case NodeNameRole:
        return QString::fromStdString(descriptor.displayName);
    case NodeTypeRole:
        return QString::fromStdString(descriptor.typeId);
    case CategoryRole:
        return categoryFor(QString::fromStdString(descriptor.typeId));
    case InputPortCountRole:
        return static_cast<int>(std::count_if(
            descriptor.ports.cbegin(), descriptor.ports.cend(),
            [](const auto& port) {
                return port.direction == Core::PortDirection::Input;
            }));
    case OutputPortCountRole:
        return static_cast<int>(std::count_if(
            descriptor.ports.cbegin(), descriptor.ports.cend(),
            [](const auto& port) {
                return port.direction == Core::PortDirection::Output;
            }));
    default:
        return {};
    }
}

QHash<int, QByteArray> NodePaletteModel::roleNames() const {
    return {{NodeTypeRole, "nodeType"},
            {NodeNameRole, "nodeName"},
            {CategoryRole, "category"},
            {InputPortCountRole, "inputPortCount"},
            {OutputPortCountRole, "outputPortCount"}};
}

QString NodePaletteModel::filter() const { return m_filter; }

void NodePaletteModel::setFilter(QString value) {
    if (m_filter == value) {
        return;
    }
    m_filter = std::move(value);
    beginResetModel();
    rebuildVisibleRows();
    endResetModel();
    emit filterChanged();
}

void NodePaletteModel::rebuildVisibleRows() {
    m_visibleRows.clear();
    for (int index = 0; index < static_cast<int>(m_descriptors.size()); ++index) {
        const auto& descriptor = m_descriptors[static_cast<std::size_t>(index)];
        // 固定/唯一节点由宿主预置，不出现在可拖拽添加的调色板里。
        // deletable=false 与 fixed=true 都表达「不可删除」，两者都要判 ——
        // 既有描述符（load_image/save_image）用的是前者。
        if (m_hideFixedNodes &&
            (!descriptor.deletable || descriptor.fixed || descriptor.unique)) {
            continue;
        }
        const auto type = QString::fromStdString(descriptor.typeId);
        const auto name = QString::fromStdString(descriptor.displayName);
        const auto category = categoryFor(type);
        if (m_filter.isEmpty() || type.contains(m_filter, Qt::CaseInsensitive) ||
            name.contains(m_filter, Qt::CaseInsensitive) ||
            category.contains(m_filter, Qt::CaseInsensitive)) {
            m_visibleRows.push_back(index);
        }
    }
}

QString NodePaletteModel::categoryFor(const QString& typeId) {
    const auto separator = typeId.indexOf(QLatin1Char('.'));
    if (separator > 0) {
        return typeId.left(separator);
    }
    return QStringLiteral("Image");
}

} // namespace QNodeGraph::UI
