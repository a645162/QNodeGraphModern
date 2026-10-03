#pragma once

#include <cstddef>
#include <string>

namespace QNodeGraph::Core {

class GraphDocument {
public:
    explicit GraphDocument(std::string name = "Untitled");

    [[nodiscard]] const std::string& name() const noexcept;
    void setName(std::string name);

    [[nodiscard]] std::size_t nodeCount() const noexcept;
    void addNode();
    void clear();

private:
    std::string m_name;
    std::size_t m_nodeCount = 0;
};

} // namespace QNodeGraph::Core

