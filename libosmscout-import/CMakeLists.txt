set(OSMSCOUT_BUILD_IMPORT ON CACHE INTERNAL "" FORCE)

set(HEADER_FILES
    include/osmscout/import/GenAreaAreaIndex.h
    include/osmscout/import/GenAreaNodeIndex.h
    include/osmscout/import/GenAreaWayIndex.h
    include/osmscout/import/GenCoordDat.h
    include/osmscout/import/GenCoverageIndex.h
    include/osmscout/import/GenIntersectionIndex.h
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
    include/osmscout/import/GenTypeDat.h
    include/osmscout/import/GenWaterIndex.h
    include/osmscout/import/GenWayAreaDat.h
    include/osmscout/import/GenWayWayDat.h
    include/osmscout/import/Import.h
    include/osmscout/import/ImportErrorReporter.h
    include/osmscout/import/MergeAreaData.h
    include/osmscout/import/Preprocess.h
    include/osmscout/import/Preprocessor.h
    include/osmscout/import/PreprocessPoly.h
    include/osmscout/import/RawCoastline.h
    include/osmscout/import/RawCoord.h
    include/osmscout/import/RawNode.h
    include/osmscout/import/RawRelation.h
    include/osmscout/import/RawRelIndexedDataFile.h
    include/osmscout/import/RawWay.h
    include/osmscout/import/RawWayIndexedDataFile.h
    include/osmscout/import/ShapeFileScanner.h
    include/osmscout/import/WaterIndexProcessor.h
    include/osmscout/import/SortDat.h
    include/osmscout/import/SortNodeDat.h
    include/osmscout/import/SortWayDat.h
    include/osmscout/import/ImportImportExport.h
)

set(SOURCE_FILES
    src/osmscout/import/GenAreaAreaIndex.cpp
    src/osmscout/import/GenAreaNodeIndex.cpp
    src/osmscout/import/GenAreaWayIndex.cpp
    src/osmscout/import/GenCoordDat.cpp
    src/osmscout/import/GenCoverageIndex.cpp
    src/osmscout/import/GenIntersectionIndex.cpp
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
    src/osmscout/import/GenTypeDat.cpp
    src/osmscout/import/GenWaterIndex.cpp
    src/osmscout/import/GenWayAreaDat.cpp
    src/osmscout/import/GenWayWayDat.cpp
    src/osmscout/import/Import.cpp
    src/osmscout/import/ImportErrorReporter.cpp
    src/osmscout/import/MergeAreaData.cpp
    src/osmscout/import/Preprocess.cpp
    src/osmscout/import/Preprocessor.cpp
    src/osmscout/import/PreprocessPoly.cpp
    src/osmscout/import/RawCoastline.cpp
    src/osmscout/import/RawCoord.cpp
    src/osmscout/import/RawNode.cpp
    src/osmscout/import/RawRelation.cpp
    src/osmscout/import/RawRelIndexedDataFile.cpp
    src/osmscout/import/RawWay.cpp
    src/osmscout/import/RawWayIndexedDataFile.cpp
    src/osmscout/import/ShapeFileScanner.cpp
    src/osmscout/import/WaterIndexProcessor.cpp
    src/osmscout/import/SortDat.cpp
    src/osmscout/import/SortNodeDat.cpp
    src/osmscout/import/SortWayDat.cpp
)

if(MARISA_FOUND)
    list(APPEND HEADER_FILES include/osmscout/import/GenTextIndex.h)
    list(APPEND SOURCE_FILES src/osmscout/import/GenTextIndex.cpp)
endif()

if(LIBXML2_FOUND)
    list(APPEND HEADER_FILES include/osmscout/import/PreprocessOSM.h)
    list(APPEND SOURCE_FILES src/osmscout/import/PreprocessOSM.cpp)
endif()

if (PROTOBUF_FOUND)
    set(PROTOBUF_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/include/osmscout/import/pbf)
    file(MAKE_DIRECTORY "${PROTOBUF_OUT_DIR}")
    protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS
            "${PROTOBUF_OUT_DIR}"
            src/protobuf/fileformat.proto src/protobuf/osmformat.proto)

    list(APPEND HEADER_FILES include/osmscout/import/PreprocessPBF.h)
    list(APPEND SOURCE_FILES src/osmscout/import/PreprocessPBF.cpp)
    list(APPEND HEADER_FILES ${PROTO_HDRS})
    list(APPEND SOURCE_FILES ${PROTO_SRCS})
endif()

if(IOS)
  add_library(OSMScoutImport STATIC ${SOURCE_FILES} ${HEADER_FILES})
else()
  add_library(OSMScoutImport ${SOURCE_FILES} ${HEADER_FILES})
endif()

set_target_properties(OSMScoutImport PROPERTIES
        CXX_STANDARD 14
        OUTPUT_NAME "osmscout_import")

target_include_directories(OSMScoutImport PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout/import/pbf)

# Public features Header
configure_file(include/osmscout/import/ImportFeatures.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/include/osmscout/import/ImportFeatures.h)
target_include_directories(OSMScoutImport PUBLIC include ${CMAKE_CURRENT_BINARY_DIR}/include)

# Private config header
create_private_config("${CMAKE_CURRENT_BINARY_DIR}/privateinclude/osmscout/private/Config.h" "OSMScoutImport")
target_include_directories(OSMScoutImport PRIVATE include ${CMAKE_CURRENT_BINARY_DIR}/privateinclude)

target_link_libraries(OSMScoutImport OSMScout)

if(ZLIB_FOUND AND ICONV_FOUND AND LIBXML2_FOUND)
    target_include_directories(OSMScoutImport PRIVATE ${LIBXML2_INCLUDE_DIR})
    target_link_libraries(OSMScoutImport ${LIBXML2_LIBRARIES})
endif()

if (PROTOBUF_FOUND)
    target_include_directories(OSMScoutImport PRIVATE ${PROTOBUF_INCLUDE_DIRS})
    target_link_libraries(OSMScoutImport ${PROTOBUF_LIBRARIES})
endif()

if (ICONV_FOUND)
    target_include_directories(OSMScoutImport PRIVATE ${ICONV_INCLUDE_DIRS})
    target_link_libraries(OSMScoutImport ${ICONV_LIBRARIES})
endif()

if (LIBLZMA_FOUND)
    target_include_directories(OSMScoutImport PRIVATE ${LIBLZMA_INCLUDE_DIRS})
  target_link_libraries(OSMScoutImport ${LIBLZMA_LIBRARIES})
endif()

if (ZLIB_FOUND)
    target_include_directories(OSMScoutImport PRIVATE ${ZLIB_INCLUDE_DIRS})
  target_link_libraries(OSMScoutImport ${ZLIB_LIBRARIES})
endif()

if(MARISA_FOUND)
    target_include_directories(OSMScoutImport PRIVATE ${MARISA_INCLUDE_DIRS})
    target_link_libraries(OSMScoutImport ${MARISA_LIBRARIES})
endif()

if(WIN32)
  target_link_libraries(OSMScoutImport Ws2_32.lib)
  target_compile_definitions(OSMScoutImport PRIVATE -DPROTOBUF_USE_DLLS)
endif()

#if(${IOS})
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fembed-bitcode")
#endif()

if(APPLE AND BUILD_FRAMEWORKS)
set_target_properties(OSMScoutImport PROPERTIES
        FRAMEWORK TRUE
        FRAMEWORK_VERSION C
        MACOSX_FRAMEWORK_IDENTIFIER com.cmake.dynamicFramework
        #MACOSX_FRAMEWORK_INFO_PLIST Info.plist
        PUBLIC_HEADER     "${HEADER_FILES}"
        CODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer"
        OUTPUT_NAME "OSMScoutImport")
endif()

target_compile_definitions(OSMScoutImport PRIVATE -DOSMSCOUT_IMPORT_EXPORT_SYMBOLS ${LIBXML2_DEFINITIONS})

install(TARGETS OSMScoutImport
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        FRAMEWORK DESTINATION lib)

if(MARISA_FOUND)
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout DESTINATION include FILES_MATCHING PATTERN "*.h" PATTERN "private" EXCLUDE)
else()
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout DESTINATION include FILES_MATCHING PATTERN "*.h" PATTERN "private" EXCLUDE PATTERN GenTextIndex.h EXCLUDE)
endif()
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/osmscout/import/ImportFeatures.h DESTINATION include/osmscout)
