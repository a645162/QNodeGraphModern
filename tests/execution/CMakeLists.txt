add_executable(QNodeGraph.Execution.Tests
    image_execution_test.cpp
)

target_link_libraries(QNodeGraph.Execution.Tests
    PRIVATE
        QNodeGraph.Lib.Execution
        QNodeGraph.Lib.Image
        Qt6::Core
        Qt6::Concurrent
        Qt6::Gui
        Qt6::Test
)

add_test(NAME QNodeGraph.Execution.Tests COMMAND QNodeGraph.Execution.Tests)

