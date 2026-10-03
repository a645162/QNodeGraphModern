#include <QNodeGraph/Lib/Execution/image_nodes.h>
#include <QNodeGraph/Lib/Image/image_processors.h>

#include <QImage>

namespace QNodeGraph::Execution {

namespace {

Core::GraphError ioError(const QString& message) {
    return {Core::GraphErrorCode::ImageIoError, message.toStdString()};
}

} // namespace

ImageNodeResult LoadImageNode::execute(const QString& path) {
    if (path.isEmpty()) {
        return std::unexpected(ioError(
            QStringLiteral("The image path cannot be empty.")));
    }
    QImage image;
    if (!image.load(path)) {
        return std::unexpected(ioError(
            QStringLiteral("Unable to load image: %1").arg(path)));
    }
    return Image::ImageFrame::fromQImage(image, path);
}

ImageNodeResult GrayscaleNode::execute(const Image::ImageFrame& input) {
    return Image::ImageProcessors::grayscale(input);
}

ImageNodeResult BlurNode::execute(const Image::ImageFrame& input, int radius) {
    return Image::ImageProcessors::blur(input, radius);
}

ImageNodeResult EdgeDetectNode::execute(const Image::ImageFrame& input) {
    return Image::ImageProcessors::edgeDetect(input);
}

ImageNodeResult ImagePreviewNode::execute(const Image::ImageFrame& input) {
    return Image::ImageFrame::fromQImage(input.image(), input.source());
}

Core::GraphResult<void> SaveImageNode::execute(const Image::ImageFrame& input,
                                               const QString& path) {
    if (path.isEmpty()) {
        return std::unexpected(ioError(
            QStringLiteral("The image output path cannot be empty.")));
    }
    if (!input.image().save(path)) {
        return std::unexpected(ioError(
            QStringLiteral("Unable to save image: %1").arg(path)));
    }
    return {};
}

} // namespace QNodeGraph::Execution
