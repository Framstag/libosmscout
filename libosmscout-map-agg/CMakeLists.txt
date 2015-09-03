cmake_minimum_required(VERSION 3.2)
project(libosmscout_map_agg)

include(FindPkgConfig)

pkg_check_modules(LIBOSMSCOUT REQUIRED libosmscout)
pkg_check_modules(LIBOSMSCOUTMAP REQUIRED libosmscout-map)
pkg_check_modules(LIBFREETYPE2 freetype2)
pkg_check_modules(LIBAGG REQUIRED libagg)

include_directories(include
                    ${LIBOSMSCOUT_INCLUDE_DIRS}
                    ${LIBOSMSCOUTMAP_INCLUDE_DIRS}
                    ${LIBFREETYPE2_INCLUDE_DIRS}
                    ${LIBAGG_INCLUDE_DIRS})

set(HEADER_FILES
    include/osmscout/private/Config.h
    include/osmscout/private/MapAggImportExport.h
    include/osmscout/MapAggFeatures.h
    include/osmscout/MapPainterAgg.h)

set(SOURCE_FILES
    src/osmscout/MapPainterAgg.cpp)

add_library(libosmscout_map_agg SHARED ${SOURCE_FILES} ${HEADER_FILES})
set_property(TARGET libosmscout_map_agg PROPERTY CXX_STANDARD 11)