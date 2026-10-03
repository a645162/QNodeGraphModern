#include <QNodeGraph/Lib/Image/image_processors.h>

namespace QNodeGraph::Image {

Core::GraphResult<ImageFrame> ImageProcessors::grayscale(
    const ImageFrame& input) {
    const auto converted = input.image().convertToFormat(QImage::Format_Grayscale8);
    return ImageFrame::fromQImage(converted, input.source());
}

} // namespace QNodeGraph::Image

