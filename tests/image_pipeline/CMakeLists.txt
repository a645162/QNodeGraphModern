add_executable(QNodeGraph.ImagePipeline.Tests
    pipeline_controller_test.cpp
    ${CMAKE_SOURCE_DIR}/examples/QNodeGraph.ImagePipelineDemo/pipeline_controller.cpp
)

target_include_directories(QNodeGraph.ImagePipeline.Tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/examples/QNodeGraph.ImagePipelineDemo
)

target_link_libraries(QNodeGraph.ImagePipeline.Tests
    PRIVATE
        QNodeGraph.Lib.Execution
        QNodeGraph.Lib.Image
        Qt6::Core
        Qt6::Concurrent
        Qt6::Gui
        Qt6::Quick
        Qt6::Test
)

add_test(NAME QNodeGraph.ImagePipeline.Tests COMMAND QNodeGraph.ImagePipeline.Tests)

