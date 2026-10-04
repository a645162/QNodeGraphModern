cmake_minimum_required(VERSION 3.24)

project(QNodeGraphModern
    VERSION 0.1.0
    DESCRIPTION "C++23 and Qt6/QML node graph foundation"
    LANGUAGES CXX
)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(QNODEGRAPH_BUILD_EXAMPLES "Build QNodeGraph examples" ON)
option(QNODEGRAPH_BUILD_TESTS "Build QNodeGraph tests" ON)
option(QNODEGRAPH_DEPLOY_QT_RUNTIME
       "Deploy Qt runtime files beside example executables on Windows" ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Quick Qml QuickControls2 Concurrent)

if(WIN32 AND QNODEGRAPH_DEPLOY_QT_RUNTIME)
    get_filename_component(QNODEGRAPH_QT_PREFIX
                           "${Qt6_DIR}/../../.." ABSOLUTE)
    find_program(QNODEGRAPH_WINDEPLOYQT
                 NAMES windeployqt.exe windeployqt
                 HINTS "${QNODEGRAPH_QT_PREFIX}/bin"
                 NO_DEFAULT_PATH)
    if(QNODEGRAPH_WINDEPLOYQT)
        function(qnodegraph_deploy_qt_runtime target)
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND "${QNODEGRAPH_WINDEPLOYQT}"
                        --no-translations
                        --qmldir "${CMAKE_SOURCE_DIR}/examples"
                        "$<TARGET_FILE:${target}>"
                VERBATIM)
        endfunction()
    else()
        message(WARNING
                "windeployqt was not found; example executables will need Qt on PATH")
    endif()
endif()

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

set(QNODEGRAPH_CMAKE_INSTALL_DIR
    "${CMAKE_INSTALL_LIBDIR}/cmake/QNodeGraphModern")
configure_package_config_file(
    cmake/QNodeGraphModernConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/QNodeGraphModernConfig.cmake"
    INSTALL_DESTINATION "${QNODEGRAPH_CMAKE_INSTALL_DIR}")
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/QNodeGraphModernConfigVersion.cmake"
    COMPATIBILITY SameMajorVersion)
install(EXPORT QNodeGraphTargets
    FILE QNodeGraphTargets.cmake
    NAMESPACE QNodeGraph::
    DESTINATION "${QNODEGRAPH_CMAKE_INSTALL_DIR}")
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/QNodeGraphModernConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/QNodeGraphModernConfigVersion.cmake"
    DESTINATION "${QNODEGRAPH_CMAKE_INSTALL_DIR}")
