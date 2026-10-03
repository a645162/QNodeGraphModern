add_library(QNodeGraph.Lib.UI.QtQuick STATIC
    include/QNodeGraph/Lib/UI/QtQuick/graph_controller.h
    include/QNodeGraph/Lib/UI/QtQuick/image_frame_provider.h
    include/QNodeGraph/Lib/UI/QtQuick/node_palette_model.h
    include/QNodeGraph/Lib/UI/QtQuick/qml_registration.h
    src/graph_controller.cpp
    src/image_frame_provider.cpp
    src/node_palette_model.cpp
    src/qml_registration.cpp
)

set(QT_QML_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

qt_add_qml_module(QNodeGraph.Lib.UI.QtQuick
    URI QNodeGraph.UI
    VERSION 1.0
    QML_FILES
        qml/GraphCanvas.qml
        qml/PropertiesBin.qml
        qml/NodesPalette.qml
        qml/NodesTree.qml
        qml/TabSearch.qml
)

target_include_directories(QNodeGraph.Lib.UI.QtQuick
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(QNodeGraph.Lib.UI.QtQuick
    PUBLIC
        QNodeGraph.Lib.Core
        QNodeGraph.Lib.Graph
        QNodeGraph.Lib.Image
        Qt6::Core
        Qt6::Qml
        Qt6::Quick
)

target_compile_features(QNodeGraph.Lib.UI.QtQuick PUBLIC cxx_std_23)
