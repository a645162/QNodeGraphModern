qt_add_executable(QNodeGraph.BasicDemo
    main.cpp
)

qt_add_qml_module(QNodeGraph.BasicDemo
    URI QNodeGraph.BasicDemo
    VERSION 1.0
    QML_FILES
        qml/Main.qml
)

target_link_libraries(QNodeGraph.BasicDemo
    PRIVATE
        QNodeGraph.Lib.UI.QtQuickplugin
        QNodeGraph.Lib.UI.QtQuick
        Qt6::Gui
        Qt6::Quick
        Qt6::Qml
        Qt6::QuickControls2
)

target_compile_features(QNodeGraph.BasicDemo PRIVATE cxx_std_23)

set_target_properties(QNodeGraph.BasicDemo PROPERTIES
    WIN32_EXECUTABLE TRUE
)

if(COMMAND qnodegraph_deploy_qt_runtime)
    qnodegraph_deploy_qt_runtime(QNodeGraph.BasicDemo)
endif()
