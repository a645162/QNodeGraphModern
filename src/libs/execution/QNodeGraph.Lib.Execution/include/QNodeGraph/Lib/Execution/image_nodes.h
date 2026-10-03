#pragma once

#include <QNodeGraph/Lib/Image/image_frame.h>

#include <QString>

namespace QNodeGraph::Execution {

using ImageNodeResult = Core::GraphResult<Image::ImageFrame>;

class LoadImageNode final {
public:
    [[nodiscard]] static ImageNodeResult execute(const QString& path);
};

class GrayscaleNode final {
public:
    [[nodiscard]] static ImageNodeResult execute(
        const Image::ImageFrame& input);
};

class BlurNode final {
public:
    [[nodiscard]] static ImageNodeResult execute(
        const Image::ImageFrame& input, int radius = 1);
};

class EdgeDetectNode final {
public:
    [[nodiscard]] static ImageNodeResult execute(
        const Image::ImageFrame& input);
};

class ImagePreviewNode final {
public:
    [[nodiscard]] static ImageNodeResult execute(
        const Image::ImageFrame& input);
};

class SaveImageNode final {
public:
    [[nodiscard]] static Core::GraphResult<void> execute(
        const Image::ImageFrame& input, const QString& path);
};

} // namespace QNodeGraph::Execution
