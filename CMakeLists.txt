cmake_minimum_required(VERSION 3.24)

project(QNodeGraphModern
    VERSION 0.1.0
    DESCRIPTION "C++23 and Qt6/QML node graph foundation"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(QNODEGRAPH_BUILD_EXAMPLES "Build QNodeGraph examples" ON)
option(QNODEGRAPH_BUILD_TESTS "Build QNodeGraph tests" ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Quick Qml QuickControls2)

qt_policy(SET QTP0001 NEW)
qt_policy(SET QTP0004 NEW)

include(CTest)

add_subdirectory(src)

if(QNODEGRAPH_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(QNODEGRAPH_BUILD_TESTS AND BUILD_TESTING)
    find_package(Qt6 6.5 REQUIRED COMPONENTS Test)
    add_subdirectory(tests)
endif()
