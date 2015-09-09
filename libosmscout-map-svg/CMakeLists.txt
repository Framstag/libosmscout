cmake_minimum_required(VERSION 3.2)
project(libosmscout_map_svg)

include(FindPkgConfig)

pkg_check_modules(LIBOSMSCOUT REQUIRED libosmscout)
pkg_check_modules(LIBOSMSCOUTMAP REQUIRED libosmscout-map)
pkg_check_modules(LIBPANGO pangoft2)

include_directories(include
                    ${LIBOSMSCOUT_INCLUDE_DIRS}
                    ${LIBOSMSCOUTMAP_INCLUDE_DIRS}
                    ${LIBPANGO_INCLUDE_DIRS})

set(HEADER_FILES
    include/osmscout/private/Config.h
    include/osmscout/private/MapSVGImportExport.h
    include/osmscout/MapSVGFeatures.h
    include/osmscout/MapPainterSVG.h)

set(SOURCE_FILES
    src/osmscout/MapPainterSVG.cpp)

add_library(libosmscout_map_svg SHARED ${SOURCE_FILES} ${HEADER_FILES})
set_property(TARGET libosmscout_map_svg PROPERTY CXX_STANDARD 11)