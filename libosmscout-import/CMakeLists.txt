cmake_minimum_required(VERSION 3.3)
project(libosmscout_import)

include(FindPkgConfig)

pkg_check_modules(LIBOSMSCOUT REQUIRED libosmscout)
pkg_check_modules(XML2 "libxml-2.0 >= 2.6.0")
pkg_check_modules(PROTOBUF protobuf)
pkg_check_modules(ZLIB zlib)
pkg_check_modules(MARISA maris)

include_directories(include
                    include/osmscout/import/pbf
                    ${LIBOSMSCOUT_INCLUDE_DIRS}
                    ${XML2_INCLUDE_DIRS}
                    ${PROTOBUF_INCLUDE_DIRS}
                    ${ZLIB_INCLUDE_DIRS}
                    ${MARISA_INCLUDE_DIRS})

set(HEADER_FILES
    include/osmscout/import/pbf/fileformat.pb.h
    include/osmscout/import/pbf/osmformat.pb.h
    include/osmscout/import/GenAreaAreaIndex.h
    include/osmscout/import/GenAreaNodeIndex.h
    include/osmscout/import/GenAreaWayIndex.h
    include/osmscout/import/GenLocationIndex.h
    include/osmscout/import/GenMergeAreas.h
    include/osmscout/import/GenNodeDat.h
    include/osmscout/import/GenNumericIndex.h
    include/osmscout/import/GenOptimizeAreasLowZoom.h
    include/osmscout/import/GenOptimizeAreaWayIds.h
    include/osmscout/import/GenOptimizeWaysLowZoom.h
    include/osmscout/import/GenRawNodeIndex.h
    include/osmscout/import/GenRawRelIndex.h
    include/osmscout/import/GenRawWayIndex.h
    include/osmscout/import/GenRelAreaDat.h
    include/osmscout/import/GenRouteDat.h
    include/osmscout/import/GenTextIndex.h
    include/osmscout/import/GenTypeDat.h
    include/osmscout/import/GenWaterIndex.h
    include/osmscout/import/GenWayAreaDat.h
    include/osmscout/import/GenWayWayDat.h
    include/osmscout/import/Import.h
    include/osmscout/import/MergeAreaData.h
    include/osmscout/import/Preprocess.h
    include/osmscout/import/Preprocessor.h
    include/osmscout/import/PreprocessOSM.h
    include/osmscout/import/PreprocessPBF.h
    include/osmscout/import/RawCoastline.h
    include/osmscout/import/RawNode.h
    include/osmscout/import/RawRelation.h
    include/osmscout/import/RawRelIndexedDataFile.h
    include/osmscout/import/RawWay.h
    include/osmscout/import/RawWayIndexedDataFile.h
    include/osmscout/import/SortDat.h
    include/osmscout/import/SortNodeDat.h
    include/osmscout/import/SortWayDat.h
    include/osmscout/private/Config.h
    include/osmscout/private/ImportImportExport.h
    include/osmscout/ImportFeatures.h
)

set(SOURCE_FILES
    src/osmscout/import/pbf/fileformat.pb.cc
    src/osmscout/import/pbf/osmformat.pb.cc
    src/osmscout/import/GenAreaAreaIndex.cpp
    src/osmscout/import/GenAreaNodeIndex.cpp
    src/osmscout/import/GenAreaWayIndex.cpp
    src/osmscout/import/GenLocationIndex.cpp
    src/osmscout/import/GenMergeAreas.cpp
    src/osmscout/import/GenNodeDat.cpp
    src/osmscout/import/GenNumericIndex.cpp
    src/osmscout/import/GenOptimizeAreasLowZoom.cpp
    src/osmscout/import/GenOptimizeAreaWayIds.cpp
    src/osmscout/import/GenOptimizeWaysLowZoom.cpp
    src/osmscout/import/GenRawNodeIndex.cpp
    src/osmscout/import/GenRawRelIndex.cpp
    src/osmscout/import/GenRawWayIndex.cpp
    src/osmscout/import/GenRelAreaDat.cpp
    src/osmscout/import/GenRouteDat.cpp
    src/osmscout/import/GenTextIndex.cpp
    src/osmscout/import/GenTypeDat.cpp
    src/osmscout/import/GenWaterIndex.cpp
    src/osmscout/import/GenWayAreaDat.cpp
    src/osmscout/import/GenWayWayDat.cpp
    src/osmscout/import/Import.cpp
    src/osmscout/import/MergeAreaData.cpp
    src/osmscout/import/Preprocess.cpp
    src/osmscout/import/Preprocessor.cpp
    src/osmscout/import/PreprocessOSM.cpp
    src/osmscout/import/PreprocessPBF.cpp
    src/osmscout/import/RawCoastline.cpp
    src/osmscout/import/RawNode.cpp
    src/osmscout/import/RawRelation.cpp
    src/osmscout/import/RawRelIndexedDataFile.cpp
    src/osmscout/import/RawWay.cpp
    src/osmscout/import/RawWayIndexedDataFile.cpp
    src/osmscout/import/SortDat.cpp
    src/osmscout/import/SortNodeDat.cpp
    src/osmscout/import/SortWayDat.cpp)

add_library(libosmscout_import SHARED ${SOURCE_FILES} ${HEADER_FILES})
set_property(TARGET libosmscout_import PROPERTY CXX_STANDARD 11)
