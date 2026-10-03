add_executable(QNodeGraph.UI.Tests
    graph_controller_test.cpp
)

target_link_libraries(QNodeGraph.UI.Tests
    PRIVATE
        QNodeGraph.Lib.UI.QtQuick
        Qt6::Core
        Qt6::Qml
        Qt6::Quick
        Qt6::Test
)

add_test(NAME QNodeGraph.UI.Tests COMMAND QNodeGraph.UI.Tests)

