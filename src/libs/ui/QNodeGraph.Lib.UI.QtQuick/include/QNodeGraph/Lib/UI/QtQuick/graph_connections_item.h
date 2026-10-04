#pragma once

#include <QQuickItem>
#include <QVariantList>
#include <QVariantMap>

#include <QtQml/qqmlregistration.h>

namespace QNodeGraph::UI {

class GraphConnectionsItem : public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(QVariantList connections READ connections WRITE setConnections
                   NOTIFY connectionsChanged)
    Q_PROPERTY(QVariantMap preview READ preview WRITE setPreview
                   NOTIFY previewChanged)
    Q_PROPERTY(QPointF panOffset READ panOffset WRITE setPanOffset
                   NOTIFY panOffsetChanged)
    Q_PROPERTY(qreal zoomFactor READ zoomFactor WRITE setZoomFactor
                   NOTIFY zoomFactorChanged)
    Q_PROPERTY(int layoutMode READ layoutMode WRITE setLayoutMode
                   NOTIFY layoutModeChanged)

public:
    explicit GraphConnectionsItem(QQuickItem* parent = nullptr);

    [[nodiscard]] QVariantList connections() const;
    [[nodiscard]] QVariantMap preview() const;
    [[nodiscard]] QPointF panOffset() const;
    [[nodiscard]] qreal zoomFactor() const noexcept;
    [[nodiscard]] int layoutMode() const noexcept;

    void setConnections(QVariantList value);
    void setPreview(QVariantMap value);
    void setPanOffset(QPointF value);
    void setZoomFactor(qreal value);
    void setLayoutMode(int value);

signals:
    void connectionsChanged();
    void previewChanged();
    void panOffsetChanged();
    void zoomFactorChanged();
    void layoutModeChanged();

protected:
    QSGNode* updatePaintNode(QSGNode* oldNode,
                             UpdatePaintNodeData* updateData) override;

private:
    QVariantList m_connections;
    QVariantMap m_preview;
    QPointF m_panOffset;
    qreal m_zoomFactor = 1.0;
    int m_layoutMode = 0;
};

} // namespace QNodeGraph::UI
