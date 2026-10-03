#include <QNodeGraph/Lib/Image/image_frame.h>

#include <utility>

namespace QNodeGraph::Image {

namespace {

int channelCount(QImage::Format format) {
    return static_cast<int>(QImage::toPixelFormat(format).channelCount());
}

} // namespace

ImageFrame::ImageFrame(QImage image, QString source, int channels)
    : m_image(std::move(image)), m_source(std::move(source)),
      m_channels(channels) {}

Core::GraphResult<ImageFrame> ImageFrame::fromQImage(QImage image,
                                                     QString source) {
    if (image.isNull()) {
        return std::unexpected(Core::GraphError{
            Core::GraphErrorCode::InvalidImage,
            "The image frame cannot be constructed from a null QImage."});
    }
    const auto channels = channelCount(image.format());
    return ImageFrame(std::move(image), std::move(source), channels);
}

const QImage& ImageFrame::image() const noexcept { return m_image; }

int ImageFrame::width() const noexcept { return m_image.width(); }

int ImageFrame::height() const noexcept { return m_image.height(); }

int ImageFrame::channels() const noexcept { return m_channels; }

const QString& ImageFrame::source() const noexcept { return m_source; }

} // namespace QNodeGraph::Image
