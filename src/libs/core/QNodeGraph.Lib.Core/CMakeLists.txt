add_library(QNodeGraph.Lib.Core STATIC
    include/QNodeGraph/Lib/Core/graph_commands.h
    include/QNodeGraph/Lib/Core/graph_document.h
    src/graph_commands.cpp
    src/graph_document.cpp
)

target_include_directories(QNodeGraph.Lib.Core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_compile_features(QNodeGraph.Lib.Core PUBLIC cxx_std_23)

install(TARGETS QNodeGraph.Lib.Core
    EXPORT QNodeGraphTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

set_target_properties(QNodeGraph.Lib.Core PROPERTIES
    OUTPUT_NAME QNodeGraphCore
)
