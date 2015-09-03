cmake_minimum_required(VERSION 3.2)
project(libosmscout_map_cairo)

include(FindPkgConfig)

pkg_check_modules(LIBOSMSCOUT REQUIRED libosmscout)
pkg_check_modules(LIBOSMSCOUTMAP REQUIRED libosmscout-map)
pkg_check_modules(LIBCAIRO REQUIRED cairo)
pkg_check_modules(LIBPANGO libpangocairo)
pkg_check_modules(LIBPNG libpng)

include_directories(include
                    ${LIBOSMSCOUT_INCLUDE_DIRS}
                    ${LIBOSMSCOUTMAP_INCLUDE_DIRS}
                    ${LIBCAIRO_INCLUDE_DIRS}
              q      ${LIBPANGO_INCLUDE_DIRS}
                    ${LIBPNG_INCLUDE_DIRS})

set(HEADER_FILES
    include/osmscout/private/Config.h
    include/osmscout/private/MapCairoImportExport.h
    include/osmscout/LoaderPNG.h
    include/osmscout/MapCairoFeatures.h
    include/osmscout/MapPainterCairo.h)

set(SOURCE_FILES
    src/osmscout/LoaderPNG.cpp
    src/osmscout/MapPainterCairo.cpp)

add_library(libosmscout_map_cairo SHARED ${SOURCE_FILES} ${HEADER_FILES})
set_property(TARGET libosmscout_map_cairo PROPERTY CXX_STANDARD 11)