#pragma once

#include <QNodeGraph/Lib/Image/image_frame.h>

namespace QNodeGraph::Image {

class ImageProcessors final {
public:
    static Core::GraphResult<ImageFrame> grayscale(const ImageFrame& input);
    static Core::GraphResult<ImageFrame> blur(const ImageFrame& input,
                                              int radius = 1);
    static Core::GraphResult<ImageFrame> edgeDetect(const ImageFrame& input);
};

} // namespace QNodeGraph::Image
