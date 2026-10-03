cmake_minimum_required(VERSION 3.24)
project(QNodeGraphPackageSmoke LANGUAGES CXX)

find_package(QNodeGraphModern CONFIG REQUIRED)

if(NOT TARGET QNodeGraph::QNodeGraph.Lib.Core)
    message(FATAL_ERROR "QNodeGraph core target was not exported")
endif()
if(NOT TARGET QNodeGraph::QNodeGraph.Lib.UI.QtQuick)
    message(FATAL_ERROR "QNodeGraph QtQuick target was not exported")
endif()

add_library(package_consumer INTERFACE)
target_link_libraries(package_consumer INTERFACE
    QNodeGraph::QNodeGraph.Lib.Core
    QNodeGraph::QNodeGraph.Lib.UI.QtQuick)
