#include <QNodeGraph/Lib/Execution/image_pipeline.h>

#include <utility>

namespace QNodeGraph::Execution {

void ImagePipeline::addStep(std::string name, Processor processor) {
    if (!processor) {
        return;
    }
    m_steps.push_back(Step{std::move(name), std::move(processor)});
}

ImagePipeline::Result ImagePipeline::process(
    const Image::ImageFrame& input) const {
    auto current = input;
    for (const auto& step : m_steps) {
        const auto result = step.processor(current);
        if (!result) {
            auto failure = result.error();
            if (!step.name.empty()) {
                failure.message = "Step " + step.name + " failed: " +
                                  failure.message;
            }
            return std::unexpected(std::move(failure));
        }
        current = *result;
    }
    return current;
}

std::vector<std::string> ImagePipeline::stepNames() const {
    std::vector<std::string> names;
    names.reserve(m_steps.size());
    for (const auto& step : m_steps) {
        names.push_back(step.name);
    }
    return names;
}

} // namespace QNodeGraph::Execution
