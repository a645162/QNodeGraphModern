#pragma once

#include <QNodeGraph/Lib/Image/image_frame.h>

#include <QCache>
#include <QMutex>

#include <optional>

namespace QNodeGraph::Image {

// Owns a bounded set of implicitly-shared image frames for preview consumers.
class ImageFrameCache final {
public:
    explicit ImageFrameCache(int capacity = 8);

    void setCapacity(int capacity);
    [[nodiscard]] int capacity() const;
    void put(QString key, const ImageFrame& frame);
    [[nodiscard]] std::optional<ImageFrame> get(const QString& key) const;
    [[nodiscard]] bool contains(const QString& key) const;
    [[nodiscard]] int size() const;
    void clear();

private:
    mutable QMutex m_mutex;
    QCache<QString, ImageFrame> m_frames;
};

} // namespace QNodeGraph::Image
