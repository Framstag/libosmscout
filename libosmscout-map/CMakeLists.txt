cmake_minimum_required(VERSION 3.2)
project(libosmscout_map)

include(FindPkgConfig)

pkg_check_modules(LIBOSMSCOUT REQUIRED libosmscout)

include_directories(include ${LIBOSMSCOUT_INCLUDE_DIRS})

set(HEADER_FILES
        include/osmscout/oss/Parser.h
        include/osmscout/oss/Scanner.h
        include/osmscout/private/Config.h
        include/osmscout/private/MapImportExport.h
        include/osmscout/MapFeatures.h
        include/osmscout/MapPainter.h
        include/osmscout/MapParameter.h
        include/osmscout/MapService.h
        include/osmscout/StyleConfig.h
        include/osmscout/TiledDataCache.h
        include/osmscout/MapPainterNoOp.h)

set(SOURCE_FILES
        src/osmscout/oss/Parser.cpp
        src/osmscout/oss/Scanner.cpp
        src/osmscout/MapPainter.cpp
        src/osmscout/MapParameter.cpp
        src/osmscout/MapService.cpp
        src/osmscout/StyleConfig.cpp
        src/osmscout/TiledDataCache.cpp
        src/osmscout/MapPainterNoOp.cpp)

add_library(libosmscout_map SHARED ${SOURCE_FILES} ${HEADER_FILES})
set_property(TARGET libosmscout_map PROPERTY CXX_STANDARD 11)
