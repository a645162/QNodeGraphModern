add_executable(QNodeGraph.Core.Tests
    graph_document_test.cpp
)

target_link_libraries(QNodeGraph.Core.Tests
    PRIVATE
        QNodeGraph.Lib.Core
        Qt6::Core
        Qt6::Test
)

add_test(NAME QNodeGraph.Core.Tests COMMAND QNodeGraph.Core.Tests)

