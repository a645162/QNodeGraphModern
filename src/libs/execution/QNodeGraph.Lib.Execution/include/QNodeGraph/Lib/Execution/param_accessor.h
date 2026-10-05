#pragma once

#include <QNodeGraph/Lib/Core/graph_document.h>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <map>
#include <string>
#include <variant>

namespace QNodeGraph::Execution {

// 统一读取算子参数，兼容 GraphDocument 的 int64/double 存储差异，并在同一
// 个入口完成边界钳制。算子只依赖这个轻量视图，不再重复实现参数容错逻辑。
class ParamAccessor final {
public:
    explicit ParamAccessor(
        const std::map<std::string, Core::PropertyValue>& params) noexcept
        : m_params(params) {}

    [[nodiscard]] std::int64_t integer(
        const std::string& name, std::int64_t fallback,
        std::int64_t minimum = std::numeric_limits<std::int64_t>::min(),
        std::int64_t maximum = std::numeric_limits<std::int64_t>::max()) const
        noexcept {
        const auto iterator = m_params.find(name);
        if (iterator == m_params.end()) {
            return std::clamp(fallback, minimum, maximum);
        }
        std::int64_t value = fallback;
        if (std::holds_alternative<std::int64_t>(iterator->second)) {
            value = std::get<std::int64_t>(iterator->second);
        } else if (std::holds_alternative<double>(iterator->second)) {
            value = static_cast<std::int64_t>(std::get<double>(iterator->second));
        }
        return std::clamp(value, minimum, maximum);
    }

    [[nodiscard]] double real(
        const std::string& name, double fallback,
        double minimum = -std::numeric_limits<double>::max(),
        double maximum = std::numeric_limits<double>::max()) const noexcept {
        const auto iterator = m_params.find(name);
        if (iterator == m_params.end()) {
            return std::clamp(fallback, minimum, maximum);
        }
        double value = fallback;
        if (std::holds_alternative<double>(iterator->second)) {
            value = std::get<double>(iterator->second);
        } else if (std::holds_alternative<std::int64_t>(iterator->second)) {
            value = static_cast<double>(std::get<std::int64_t>(iterator->second));
        }
        return std::clamp(value, minimum, maximum);
    }

    [[nodiscard]] bool boolean(const std::string& name, bool fallback) const
        noexcept {
        const auto iterator = m_params.find(name);
        return iterator != m_params.end() &&
                       std::holds_alternative<bool>(iterator->second)
                   ? std::get<bool>(iterator->second)
                   : fallback;
    }

    [[nodiscard]] std::string text(const std::string& name,
                                   std::string fallback) const {
        const auto iterator = m_params.find(name);
        return iterator != m_params.end() &&
                       std::holds_alternative<std::string>(iterator->second)
                   ? std::get<std::string>(iterator->second)
                   : fallback;
    }

private:
    const std::map<std::string, Core::PropertyValue>& m_params;
};

} // namespace QNodeGraph::Execution
