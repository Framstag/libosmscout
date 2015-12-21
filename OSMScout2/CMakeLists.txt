cmake_minimum_required(VERSION 3.2)
project(OSMScout2)

include(FindPkgConfig)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

pkg_check_modules(LIBOSMSCOUT REQUIRED libosmscout)
pkg_check_modules(LIBOSMSCOUTMAP REQUIRED libosmscout-map)
pkg_check_modules(LIBOSMSCOUTMAPQT REQUIRED libosmscout-map-qt)

find_package(Qt5Core 5.0 REQUIRED)
find_package(Qt5Gui 5.0 REQUIRED)
find_package(Qt5Widgets 5.0 REQUIRED)
find_package(Qt5Qml 5.0 REQUIRED)
find_package(Qt5Quick 5.0 REQUIRED)
find_package(Qt5Svg 5.0 REQUIRED)
find_package(Qt5Positioning 5.0 REQUIRED)

include_directories(${LIBOSMSCOUT_INCLUDE_DIRS}
                    ${LIBOSMSCOUTMAP_INCLUDE_DIRS}
                    ${LIBOSMSCOUTMAPQT_INCLUDE_DIRS})

link_directories(${LIBOSMSCOUT_LIBRARY_DIRS}
                 ${LIBOSMSCOUTMAP_LIBRARY_DIRS}
                 ${LIBOSMSCOUTMAPQT_LIBRARY_DIRS})

set(HEADER_FILES
    src/DBThread.h
    src/MapWidget.h
    src/RoutingModel.h
    src/SearchLocationModel.h
    src/Settings.h
    src/Theme.h)

set(SOURCE_FILES
    src/DBThread.cpp
    src/MapWidget.cpp
    src/OSMScout.cpp
    src/RoutingModel.cpp
    src/SearchLocationModel.cpp
    src/Settings.cpp
    src/Theme.cpp)

set(RESOURCE_FILES
    res.qrc)

add_executable(OSMScout2 ${SOURCE_FILES} ${HEADER_FILES} ${RESOURCE_FILES})

target_link_libraries(OSMScout2
        ${LIBOSMSCOUT_LIBRARIES}
        ${LIBOSMSCOUTMAP_LIBRARIES}
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Qml
        Qt5::Quick
        Qt5::Svg
        Qt5::Positioning
        ${LIBOSMSCOUTMAPQT_LIBRARIES})

set_property(TARGET OSMScout2 PROPERTY CXX_STANDARD 11)
