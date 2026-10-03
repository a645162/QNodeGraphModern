#include <QNodeGraph/Lib/UI/QtQuick/image_frame_provider.h>

namespace QNodeGraph::UI {

ImageFrameProvider::ImageFrameProvider(int cacheCapacity)
    : QQuickImageProvider(QQuickImageProvider::Image),
      m_cache(cacheCapacity) {}

QImage ImageFrameProvider::requestImage(const QString& id, QSize* size,
                                        const QSize& requestedSize) {
    const auto key = id.section(QLatin1Char('?'), 0, 0);
    const auto frame = m_cache.get(key);
    if (!frame.has_value()) {
        if (size != nullptr) {
            *size = {};
        }
        return {};
    }

    auto image = frame->image();
    if (!requestedSize.isEmpty() && !image.isNull()) {
        image = image.scaled(requestedSize, Qt::KeepAspectRatio,
                             Qt::SmoothTransformation);
    }
    if (size != nullptr) {
        *size = image.size();
    }
    return image;
}

void ImageFrameProvider::setFrame(QString id,
                                   const Image::ImageFrame& frame) {
    m_cache.put(std::move(id), frame);
}

void ImageFrameProvider::clear() { m_cache.clear(); }

} // namespace QNodeGraph::UI
