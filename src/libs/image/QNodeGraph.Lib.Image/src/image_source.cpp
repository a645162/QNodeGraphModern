#include <QNodeGraph/Lib/Image/image_source.h>

#include <utility>

namespace QNodeGraph::Image {

QImageSource::QImageSource(QImage image) : m_image(std::move(image)) {}

QSize QImageSource::size() const noexcept { return m_image.size(); }

Core::GraphResult<QImage> QImageSource::readTile(const QRect& region) const {
    const QRect bounds(QPoint(0, 0), m_image.size());
    if (m_image.isNull() || region.isEmpty() ||
        !bounds.contains(region.topLeft()) ||
        !bounds.contains(region.bottomRight())) {
        return std::unexpected(Core::GraphError{
            Core::GraphErrorCode::InvalidImage,
            "The requested image tile is outside the source bounds."});
    }
    return m_image.copy(region);
}

} // namespace QNodeGraph::Image
