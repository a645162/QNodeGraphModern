#pragma once

#include <QNodeGraph/Lib/Image/image_frame_cache.h>

#include <QImage>
#include <QQuickImageProvider>

namespace QNodeGraph::UI {

class ImageFrameProvider final : public QQuickImageProvider {
public:
    explicit ImageFrameProvider(int cacheCapacity = 8);

    QImage requestImage(const QString& id, QSize* size,
                        const QSize& requestedSize) override;
    void setFrame(QString id, const Image::ImageFrame& frame);
    void clear();

private:
    Image::ImageFrameCache m_cache;
};

} // namespace QNodeGraph::UI
