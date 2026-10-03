add_library(QNodeGraph.Lib.Image STATIC
    include/QNodeGraph/Lib/Image/image_frame.h
    include/QNodeGraph/Lib/Image/image_frame_cache.h
    include/QNodeGraph/Lib/Image/image_processors.h
    include/QNodeGraph/Lib/Image/image_source.h
    src/image_frame.cpp
    src/image_frame_cache.cpp
    src/image_processors.cpp
    src/image_source.cpp
)

target_include_directories(QNodeGraph.Lib.Image
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(QNodeGraph.Lib.Image
    PUBLIC
        QNodeGraph.Lib.Core
        Qt6::Gui
)

target_compile_features(QNodeGraph.Lib.Image PUBLIC cxx_std_23)

install(TARGETS QNodeGraph.Lib.Image
    EXPORT QNodeGraphTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
