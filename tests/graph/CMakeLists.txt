add_executable(QNodeGraph.Graph.Tests
    graph_persistence_test.cpp
)

target_link_libraries(QNodeGraph.Graph.Tests
    PRIVATE
        QNodeGraph.Lib.Graph
        QNodeGraph.Lib.Core
        Qt6::Core
        Qt6::Test
)

add_test(NAME QNodeGraph.Graph.Tests COMMAND QNodeGraph.Graph.Tests)
