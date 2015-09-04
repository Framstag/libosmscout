cmake_minimum_required(VERSION 3.2)
project(OSMScout2)

include(FindPkgConfig)

pkg_check_modules(LIBOSMSCOUT REQUIRED libosmscout)
pkg_check_modules(LIBOSMSCOUTMAP REQUIRED libosmscout-map)
pkg_check_modules(LIBOSMSCOUTMAPQT REQUIRED libosmscout-map-qt)
pkg_check_modules(LIBQT Qt5Core>=5.0.0
                        Qt5Gui>=5.0.0
                        Qt5Widgets>=5.0.0
                        Qt5Qml>=5.0.0
                        Qt5Quick>=5.0.0
                        Qt5Svg>=5.0.0
                        Qt5Positioning>=5.0.0)

include_directories(${LIBOSMSCOUT_INCLUDE_DIRS}
                    ${LIBOSMSCOUTMAP_INCLUDE_DIRS}
                    ${LIBQT_INCLUDE_DIRS}
                    ${LIBOSMSCOUTMAPQT_INCLUDE_DIRS})

link_directories(${LIBOSMSCOUT_LIBRARY_DIRS}
                 ${LIBOSMSCOUTMAP_LIBRARY_DIRS}
                 ${LIBQT_LIBRARY_DIRS}
                 ${LIBOSMSCOUTMAPQT_LIBRARY_DIRS})

set(HEADER_FILES
    src/DBThread.h
    src/MainWindow.h
    src/MapWidget.h
    src/RoutingModel.h
    src/SearchLocationModel.h
    src/Settings.h
    src/Theme.h)

set(SOURCE_FILES
    src/DBThread.cpp
    src/MainWindow.cpp
    src/MapWidget.cpp
    src/OSMScout.cpp
    src/RoutingModel.cpp
    src/SearchLocationModel.cpp
    src/Settings.cpp
    src/Theme.cpp)

add_executable(OSMScout2 ${SOURCE_FILES} ${HEADER_FILES})

target_link_libraries(OSMScout2
                      ${LIBOSMSCOUT_LIBRARIES}
                      ${LIBOSMSCOUTMAP_LIBRARIES}
                      ${LIBQT_LIBRARIES}
                      ${LIBOSMSCOUTMAPQT_LIBRARIES})

set_property(TARGET OSMScout2 PROPERTY CXX_STANDARD 11)
