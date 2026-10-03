#include <QNodeGraph/Lib/Image/image_processors.h>

#include <QColor>

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace QNodeGraph::Image {

Core::GraphResult<ImageFrame> ImageProcessors::grayscale(
    const ImageFrame& input) {
    const auto converted = input.image().convertToFormat(QImage::Format_Grayscale8);
    return ImageFrame::fromQImage(converted, input.source());
}

Core::GraphResult<ImageFrame> ImageProcessors::blur(const ImageFrame& input,
                                                    int radius) {
    if (radius < 0) {
        return std::unexpected(Core::GraphError{
            Core::GraphErrorCode::InvalidImage,
            "The blur radius cannot be negative."});
    }
    if (radius == 0) {
        return ImageFrame::fromQImage(input.image(), input.source());
    }

    const auto grayscale = input.channels() == 1;
    const auto format = grayscale ? QImage::Format_Grayscale8
                                  : QImage::Format_RGB888;
    const auto source = input.image().convertToFormat(format);
    QImage result(source.size(), format);
    const auto diameter = radius * 2 + 1;
    const auto sampleCount = diameter * diameter;
    for (int y = 0; y < source.height(); ++y) {
        for (int x = 0; x < source.width(); ++x) {
            int red = 0;
            int green = 0;
            int blue = 0;
            for (int offsetY = -radius; offsetY <= radius; ++offsetY) {
                const auto sampleY = std::clamp(y + offsetY, 0, source.height() - 1);
                for (int offsetX = -radius; offsetX <= radius; ++offsetX) {
                    const auto sampleX = std::clamp(x + offsetX, 0, source.width() - 1);
                    const auto color = source.pixelColor(sampleX, sampleY);
                    red += color.red();
                    green += color.green();
                    blue += color.blue();
                }
            }
            if (grayscale) {
                result.setPixelColor(x, y,
                                     QColor(red / sampleCount, red / sampleCount,
                                            red / sampleCount));
            } else {
                result.setPixelColor(x, y,
                                     QColor(red / sampleCount, green / sampleCount,
                                            blue / sampleCount));
            }
        }
    }
    return ImageFrame::fromQImage(result, input.source());
}

Core::GraphResult<ImageFrame> ImageProcessors::edgeDetect(
    const ImageFrame& input) {
    const auto source = input.image().convertToFormat(QImage::Format_Grayscale8);
    QImage result(source.size(), QImage::Format_Grayscale8);
    result.fill(0);
    for (int y = 1; y < source.height() - 1; ++y) {
        for (int x = 1; x < source.width() - 1; ++x) {
            const auto pixel = [&source](int sampleX, int sampleY) {
                return source.pixelColor(sampleX, sampleY).red();
            };
            const auto gradientX =
                -pixel(x - 1, y - 1) + pixel(x + 1, y - 1) -
                2 * pixel(x - 1, y) + 2 * pixel(x + 1, y) -
                pixel(x - 1, y + 1) + pixel(x + 1, y + 1);
            const auto gradientY =
                -pixel(x - 1, y - 1) - 2 * pixel(x, y - 1) -
                pixel(x + 1, y - 1) + pixel(x - 1, y + 1) +
                2 * pixel(x, y + 1) + pixel(x + 1, y + 1);
            const auto magnitude = static_cast<int>(std::sqrt(
                static_cast<double>(gradientX * gradientX +
                                    gradientY * gradientY)));
            const auto value = std::clamp(magnitude, 0, 255);
            result.setPixelColor(x, y, QColor(value, value, value));
        }
    }
    return ImageFrame::fromQImage(result, input.source());
}

} // namespace QNodeGraph::Image
