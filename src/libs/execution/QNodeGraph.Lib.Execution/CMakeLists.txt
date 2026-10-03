add_library(QNodeGraph.Lib.Execution STATIC
    include/QNodeGraph/Lib/Execution/image_execution_service.h
    include/QNodeGraph/Lib/Execution/image_pipeline.h
    src/image_execution_service.cpp
    src/image_pipeline.cpp
)

target_include_directories(QNodeGraph.Lib.Execution
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(QNodeGraph.Lib.Execution
    PUBLIC
        QNodeGraph.Lib.Image
        Qt6::Core
        Qt6::Concurrent
)

target_compile_features(QNodeGraph.Lib.Execution PUBLIC cxx_std_23)
