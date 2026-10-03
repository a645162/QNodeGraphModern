#pragma once

#include <QNodeGraph/Lib/Image/image_frame.h>

namespace QNodeGraph::Image {

class ImageProcessors final {
public:
    static Core::GraphResult<ImageFrame> grayscale(const ImageFrame& input);
};

} // namespace QNodeGraph::Image

