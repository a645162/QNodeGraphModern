add_library(QNodeGraph.Lib.Graph STATIC
    include/QNodeGraph/Lib/Graph/graph_json.h
    include/QNodeGraph/Lib/Graph/node_registry.h
    src/graph_json.cpp
    src/node_registry.cpp
)

target_include_directories(QNodeGraph.Lib.Graph
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(QNodeGraph.Lib.Graph
    PUBLIC
        QNodeGraph.Lib.Core
        Qt6::Core
)

target_compile_features(QNodeGraph.Lib.Graph PUBLIC cxx_std_23)

install(TARGETS QNodeGraph.Lib.Graph
    EXPORT QNodeGraphTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
