cmake_minimum_required(VERSION 3.2)
project(libosmscout_map_qt)

include(FindPkgConfig)

pkg_check_modules(LIBOSMSCOUT REQUIRED libosmscout)
pkg_check_modules(LIBOSMSCOUTMAP REQUIRED libosmscout-map)
pkg_check_modules(LIBQT REQUIRED "Qt5Gui >= 5.0.0")

include_directories(include ${LIBOSMSCOUT_INCLUDE_DIRS}
                    include ${LIBOSMSCOUTMAP_INCLUDE_DIRS}
                    include ${LIBQT_INCLUDE_DIRS})

set(HEADER_FILES
    include/osmscout/private/Config.h
    include/osmscout/private/MapQtImportExport.h
    include/osmscout/MapQtFeatures.h
    include/osmscout/MapPainterQt.h)

set(SOURCE_FILES
    src/osmscout/MapPainterQt.cpp)

add_library(libosmscout_map_qt SHARED ${SOURCE_FILES} ${HEADER_FILES})
set_property(TARGET libosmscout_map_qt PROPERTY CXX_STANDARD 11)