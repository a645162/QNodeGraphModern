#include <QNodeGraph/Lib/UI/QtQuick/graph_connections_item.h>

#include <QColor>
#include <QSGFlatColorMaterial>
#include <QSGGeometryNode>

#include <algorithm>
#include <cmath>
#include <vector>

namespace QNodeGraph::UI {

namespace {

constexpr int kCurveSamples = 24;

QPointF screenPoint(double x, double y, const QPointF& pan, qreal zoom) {
    return {pan.x() + x * zoom, pan.y() + y * zoom};
}

std::pair<QPointF, QPointF> endpoints(const QVariantMap& connection,
                                      const QPointF& pan, qreal zoom) {
    const auto start = screenPoint(
        connection.value(QStringLiteral("outputX")).toDouble() +
            connection.value(QStringLiteral("outputWidth")).toDouble(),
        connection.value(QStringLiteral("outputY")).toDouble(), pan, zoom);
    const auto end = screenPoint(
        connection.value(QStringLiteral("inputX")).toDouble(),
        connection.value(QStringLiteral("inputY")).toDouble(), pan, zoom);
    return {start, end};
}

void appendPath(QSGNode* root, const std::vector<QPointF>& points,
                const QColor& color) {
    if (points.size() < 2) {
        return;
    }
    auto* geometry = new QSGGeometry(
        QSGGeometry::defaultAttributes_Point2D(),
        static_cast<int>(points.size()));
    geometry->setDrawingMode(QSGGeometry::DrawLineStrip);
    auto* vertices = geometry->vertexDataAsPoint2D();
    for (std::size_t index = 0; index < points.size(); ++index) {
        vertices[index].set(points[index].x(), points[index].y());
    }

    auto* node = new QSGGeometryNode();
    node->setGeometry(geometry);
    node->setFlag(QSGNode::OwnsGeometry);
    auto* material = new QSGFlatColorMaterial();
    material->setColor(color);
    node->setMaterial(material);
    node->setFlag(QSGNode::OwnsMaterial);
    root->appendChildNode(node);
}

void appendConnection(QSGNode* root, const QVariantMap& connection,
                      const QPointF& pan, qreal zoom, int layoutMode,
                      const QColor& color) {
    const auto [start, end] = endpoints(connection, pan, zoom);
    if (layoutMode == 2) {
        appendPath(root, {start, end}, color);
        return;
    }
    if (layoutMode == 1) {
        const auto middleX = (start.x() + end.x()) * 0.5;
        appendPath(root, {start, {middleX, start.y()},
                          {middleX, end.y()}, end}, color);
        return;
    }
    const auto distance = std::max(40.0, std::abs(end.x() - start.x()) * 0.5);
    const QPointF controlStart(start.x() + distance * zoom, start.y());
    const QPointF controlEnd(end.x() - distance * zoom, end.y());

    std::vector<QPointF> points;
    points.reserve(kCurveSamples + 1);
    for (int index = 0; index <= kCurveSamples; ++index) {
        const auto t = static_cast<float>(index) / kCurveSamples;
        const auto inverse = 1.0f - t;
        const auto point = inverse * inverse * inverse * start +
                           3.0f * inverse * inverse * t * controlStart +
                           3.0f * inverse * t * t * controlEnd +
                           t * t * t * end;
        points.emplace_back(point.x(), point.y());
    }
    appendPath(root, points, color);
}

} // namespace

GraphConnectionsItem::GraphConnectionsItem(QQuickItem* parent)
    : QQuickItem(parent) {
    setFlag(ItemHasContents, true);
}

QVariantList GraphConnectionsItem::connections() const { return m_connections; }

QVariantMap GraphConnectionsItem::preview() const { return m_preview; }

QPointF GraphConnectionsItem::panOffset() const { return m_panOffset; }

qreal GraphConnectionsItem::zoomFactor() const noexcept { return m_zoomFactor; }

int GraphConnectionsItem::layoutMode() const noexcept { return m_layoutMode; }

void GraphConnectionsItem::setConnections(QVariantList value) {
    if (m_connections == value) {
        return;
    }
    m_connections = std::move(value);
    emit connectionsChanged();
    update();
}

void GraphConnectionsItem::setPreview(QVariantMap value) {
    if (m_preview == value) {
        return;
    }
    m_preview = std::move(value);
    emit previewChanged();
    update();
}

void GraphConnectionsItem::setPanOffset(QPointF value) {
    if (m_panOffset == value) {
        return;
    }
    m_panOffset = value;
    emit panOffsetChanged();
    update();
}

void GraphConnectionsItem::setZoomFactor(qreal value) {
    if (qFuzzyCompare(m_zoomFactor, value)) {
        return;
    }
    m_zoomFactor = value;
    emit zoomFactorChanged();
    update();
}

void GraphConnectionsItem::setLayoutMode(int value) {
    value = std::clamp(value, 0, 2);
    if (m_layoutMode == value) {
        return;
    }
    m_layoutMode = value;
    emit layoutModeChanged();
    update();
}

QSGNode* GraphConnectionsItem::updatePaintNode(
    QSGNode* oldNode, UpdatePaintNodeData*) {
    auto* root = oldNode == nullptr ? new QSGNode() : oldNode;
    while (auto* child = root->firstChild()) {
        root->removeChildNode(child);
        delete child;
    }
    for (const auto& value : m_connections) {
        appendConnection(root, value.toMap(), m_panOffset, m_zoomFactor,
                         m_layoutMode, QColor(QStringLiteral("#7d9bb8")));
    }
    if (!m_preview.isEmpty()) {
        appendConnection(root, m_preview, m_panOffset, m_zoomFactor,
                         m_layoutMode, QColor(QStringLiteral("#c8d9e8")));
    }
    return root;
}

} // namespace QNodeGraph::UI
