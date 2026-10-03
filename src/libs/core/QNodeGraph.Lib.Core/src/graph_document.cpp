#include <QNodeGraph/Lib/Core/graph_document.h>

#include <utility>

namespace QNodeGraph::Core {

GraphDocument::GraphDocument(std::string name) : m_name(std::move(name)) {}

const std::string& GraphDocument::name() const noexcept {
    return m_name;
}

void GraphDocument::setName(std::string name) {
    m_name = std::move(name);
}

std::size_t GraphDocument::nodeCount() const noexcept {
    return m_nodeCount;
}

void GraphDocument::addNode() {
    ++m_nodeCount;
}

void GraphDocument::clear() {
    m_nodeCount = 0;
}

} // namespace QNodeGraph::Core

