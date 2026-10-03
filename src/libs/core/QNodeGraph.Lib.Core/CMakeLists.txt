add_library(QNodeGraph.Lib.Core STATIC
    include/QNodeGraph/Lib/Core/graph_document.h
    src/graph_document.cpp
)

target_include_directories(QNodeGraph.Lib.Core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_compile_features(QNodeGraph.Lib.Core PUBLIC cxx_std_23)

set_target_properties(QNodeGraph.Lib.Core PROPERTIES
    OUTPUT_NAME QNodeGraphCore
)

