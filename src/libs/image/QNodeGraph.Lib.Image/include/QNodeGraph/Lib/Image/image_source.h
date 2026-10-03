#pragma once

#include <QNodeGraph/Lib/Core/graph_document.h>

#include <QImage>
#include <QRect>
#include <QSize>

namespace QNodeGraph::Image {

// Stable seam for tiled/remote images; the first adapter stores a QImage.
class ImageSource {
public:
    virtual ~ImageSource() = default;
    [[nodiscard]] virtual QSize size() const noexcept = 0;
    [[nodiscard]] virtual Core::GraphResult<QImage> readTile(
        const QRect& region) const = 0;
};

class QImageSource final : public ImageSource {
public:
    explicit QImageSource(QImage image);

    [[nodiscard]] QSize size() const noexcept override;
    [[nodiscard]] Core::GraphResult<QImage> readTile(
        const QRect& region) const override;

private:
    QImage m_image;
};

} // namespace QNodeGraph::Image
