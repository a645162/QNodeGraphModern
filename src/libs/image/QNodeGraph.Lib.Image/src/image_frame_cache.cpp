#include <QNodeGraph/Lib/Image/image_frame_cache.h>

#include <QMutexLocker>

#include <algorithm>

namespace QNodeGraph::Image {

ImageFrameCache::ImageFrameCache(int capacity) {
    m_frames.setMaxCost(std::max(0, capacity));
}

void ImageFrameCache::setCapacity(int capacity) {
    QMutexLocker locker(&m_mutex);
    m_frames.setMaxCost(std::max(0, capacity));
}

int ImageFrameCache::capacity() const {
    QMutexLocker locker(&m_mutex);
    return m_frames.maxCost();
}

void ImageFrameCache::put(QString key, const ImageFrame& frame) {
    if (key.isEmpty()) {
        return;
    }
    QMutexLocker locker(&m_mutex);
    m_frames.insert(std::move(key), new ImageFrame(frame));
}

std::optional<ImageFrame> ImageFrameCache::get(const QString& key) const {
    QMutexLocker locker(&m_mutex);
    const auto* frame = m_frames.object(key);
    if (frame == nullptr) {
        return std::nullopt;
    }
    return *frame;
}

bool ImageFrameCache::contains(const QString& key) const {
    QMutexLocker locker(&m_mutex);
    return m_frames.contains(key);
}

int ImageFrameCache::size() const {
    QMutexLocker locker(&m_mutex);
    return m_frames.size();
}

void ImageFrameCache::clear() {
    QMutexLocker locker(&m_mutex);
    m_frames.clear();
}

} // namespace QNodeGraph::Image
