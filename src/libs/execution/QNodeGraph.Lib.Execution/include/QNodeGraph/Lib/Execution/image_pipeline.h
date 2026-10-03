#pragma once

#include <QNodeGraph/Lib/Image/image_frame.h>

#include <functional>
#include <string>
#include <vector>

namespace QNodeGraph::Execution {

class ImagePipeline final {
public:
    using Result = Core::GraphResult<Image::ImageFrame>;
    using Processor = std::function<Result(const Image::ImageFrame&)>;

    void addStep(std::string name, Processor processor);
    [[nodiscard]] Result process(const Image::ImageFrame& input) const;
    [[nodiscard]] std::vector<std::string> stepNames() const;

private:
    struct Step {
        std::string name;
        Processor processor;
    };

    std::vector<Step> m_steps;
};

} // namespace QNodeGraph::Execution
