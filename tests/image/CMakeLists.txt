add_executable(QNodeGraph.Image.Tests
    image_frame_test.cpp
)

target_link_libraries(QNodeGraph.Image.Tests
    PRIVATE
        QNodeGraph.Lib.Image
        Qt6::Core
        Qt6::Gui
        Qt6::Test
)

add_test(NAME QNodeGraph.Image.Tests COMMAND QNodeGraph.Image.Tests)

