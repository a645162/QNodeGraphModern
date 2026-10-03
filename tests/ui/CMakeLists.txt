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

add_executable(QNodeGraph.UI.Palette.Tests
    node_palette_model_test.cpp
)

target_link_libraries(QNodeGraph.UI.Palette.Tests
    PRIVATE
        QNodeGraph.Lib.UI.QtQuick
        Qt6::Core
        Qt6::Qml
        Qt6::Quick
        Qt6::Test
)

add_test(NAME QNodeGraph.UI.Palette.Tests COMMAND QNodeGraph.UI.Palette.Tests)

add_executable(QNodeGraph.UI.ImageProvider.Tests
    image_frame_provider_test.cpp
)

target_link_libraries(QNodeGraph.UI.ImageProvider.Tests
    PRIVATE
        QNodeGraph.Lib.UI.QtQuick
        Qt6::Core
        Qt6::Gui
        Qt6::Quick
        Qt6::Test
)

add_test(NAME QNodeGraph.UI.ImageProvider.Tests
         COMMAND QNodeGraph.UI.ImageProvider.Tests)

add_executable(QNodeGraph.UI.Visual.Tests
    graph_canvas_visual_test.cpp
)

target_link_libraries(QNodeGraph.UI.Visual.Tests
    PRIVATE
        QNodeGraph.Lib.UI.QtQuickplugin
        QNodeGraph.Lib.UI.QtQuick
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
        Qt6::Test
)

add_test(NAME QNodeGraph.UI.Visual.Tests
         COMMAND QNodeGraph.UI.Visual.Tests)
