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

# 求值计划与脏传播：独立可执行文件。只用 Core + Qt Core（不依赖 Gui/Concurrent），
# 保持它在无 GUI 环境下也能跑。
add_executable(QNodeGraph.GraphPlan.Tests
    graph_plan_test.cpp
)

target_link_libraries(QNodeGraph.GraphPlan.Tests
    PRIVATE
        QNodeGraph.Lib.Execution
        QNodeGraph.Lib.Image
        Qt6::Core
        Qt6::Test
)

add_test(NAME QNodeGraph.GraphPlan.Tests COMMAND QNodeGraph.GraphPlan.Tests)

