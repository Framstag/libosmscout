cmake_minimum_required(VERSION 3.2)
project(libosmscout_map_opengl)

include(FindPkgConfig)

pkg_check_modules(LIBOSMSCOUT REQUIRED libosmscout)
pkg_check_modules(LIBOSMSCOUTMAP REQUIRED libosmscout-map)
pkg_check_modules(LIBOPENGL REQUIRED gl)

include_directories(include
                    ${LIBOSMSCOUT_INCLUDE_DIRS}
                    ${LIBOSMSCOUTMAP_INCLUDE_DIRS}
                    ${LIBOPENGL_INCLUDE_DIRS})

set(HEADER_FILES
    include/osmscout/private/Config.h
    include/osmscout/private/MapOpenGLImportExport.h
    include/osmscout/MapOpenGLFeatures.h
    include/osmscout/MapPainterOpenGL.h)

set(SOURCE_FILES
    src/osmscout/MapPainterOpenGL.cpp)

add_library(libosmscout_map_opengl SHARED ${SOURCE_FILES} ${HEADER_FILES})
set_property(TARGET libosmscout_map_opengl PROPERTY CXX_STANDARD 11)