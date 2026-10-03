qt_add_executable(QNodeGraph.ImagePipelineDemo
    main.cpp
    pipeline_controller.h
    pipeline_controller.cpp
)

qt_add_qml_module(QNodeGraph.ImagePipelineDemo
    URI QNodeGraph.ImagePipelineDemo
    VERSION 1.0
    QML_FILES
        qml/Demo.qml
)

target_link_libraries(QNodeGraph.ImagePipelineDemo
    PRIVATE
        QNodeGraph.Lib.UI.QtQuick
        QNodeGraph.Lib.Execution
        QNodeGraph.Lib.Image
        Qt6::Gui
        Qt6::Quick
        Qt6::Qml
        Qt6::QuickControls2
)

target_compile_features(QNodeGraph.ImagePipelineDemo PRIVATE cxx_std_23)

set_target_properties(QNodeGraph.ImagePipelineDemo PROPERTIES
    WIN32_EXECUTABLE TRUE
)
