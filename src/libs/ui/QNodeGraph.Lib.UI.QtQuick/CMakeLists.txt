add_library(QNodeGraph.Lib.UI.QtQuick STATIC
    include/QNodeGraph/Lib/UI/QtQuick/graph_controller.h
    include/QNodeGraph/Lib/UI/QtQuick/graph_connections_item.h
    include/QNodeGraph/Lib/UI/QtQuick/image_frame_provider.h
    include/QNodeGraph/Lib/UI/QtQuick/node_palette_model.h
    include/QNodeGraph/Lib/UI/QtQuick/qml_registration.h
    src/graph_controller.cpp
    src/graph_connections_item.cpp
    src/image_frame_provider.cpp
    src/node_palette_model.cpp
    src/qml_registration.cpp
)

set(QT_QML_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

qt_add_qml_module(QNodeGraph.Lib.UI.QtQuick
    URI QNodeGraph.UI
    VERSION 1.0
    SOURCES
        include/QNodeGraph/Lib/UI/QtQuick/graph_controller.h
        include/QNodeGraph/Lib/UI/QtQuick/graph_connections_item.h
        include/QNodeGraph/Lib/UI/QtQuick/node_palette_model.h
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
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include/QNodeGraph/Lib/UI/QtQuick
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

install(TARGETS QNodeGraph.Lib.UI.QtQuick
    EXPORT QNodeGraphTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
foreach(QNODEGRAPH_QML_RESOURCE_TARGET
        QNodeGraph.Lib.UI.QtQuick_resources_1
        QNodeGraph.Lib.UI.QtQuick_resources_2
        QNodeGraph.Lib.UI.QtQuick_resources_3)
    if(TARGET ${QNODEGRAPH_QML_RESOURCE_TARGET})
        install(TARGETS ${QNODEGRAPH_QML_RESOURCE_TARGET}
            EXPORT QNodeGraphTargets
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    endif()
endforeach()
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY qml/
    DESTINATION ${CMAKE_INSTALL_DATADIR}/QNodeGraphModern/qml/QNodeGraph/UI
    FILES_MATCHING PATTERN "*.qml")
