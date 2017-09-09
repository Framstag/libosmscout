set(OSMSCOUT_BUILD_IMPORT ON CACHE INTERNAL "" FORCE)

set(HEADER_FILES
    #include/osmscout/import/pbf/fileformat.pb.h
    #include/osmscout/import/pbf/osmformat.pb.h
    include/osmscout/import/GenAreaAreaIndex.h
    include/osmscout/import/GenAreaNodeIndex.h
    include/osmscout/import/GenAreaWayIndex.h
    include/osmscout/import/GenCoordDat.h
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
    #include/osmscout/private/Config.h
    include/osmscout/private/ImportImportExport.h
    #include/osmscout/ImportFeatures.h
)

set(SOURCE_FILES
    #src/osmscout/import/pbf/fileformat.pb.cc
    #src/osmscout/import/pbf/osmformat.pb.cc
    src/osmscout/import/GenAreaAreaIndex.cpp
    src/osmscout/import/GenAreaNodeIndex.cpp
    src/osmscout/import/GenAreaWayIndex.cpp
    src/osmscout/import/GenCoordDat.cpp
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
  protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout/import/pbf src/protobuf/fileformat.proto src/protobuf/osmformat.proto)
    list(APPEND HEADER_FILES include/osmscout/import/PreprocessPBF.h)
    list(APPEND SOURCE_FILES src/osmscout/import/PreprocessPBF.cpp)
    list(APPEND HEADER_FILES ${PROTO_HDRS})
    list(APPEND SOURCE_FILES ${PROTO_SRCS})
endif()

if(APPLE)
  set(THE_TARGET_NAME "OSMScoutImport") 
else()
  set(THE_TARGET_NAME "osmscout_import") 
endif()

if(IOS)
  add_library(${THE_TARGET_NAME} STATIC ${SOURCE_FILES} ${HEADER_FILES})
else()
  add_library(${THE_TARGET_NAME} ${SOURCE_FILES} ${HEADER_FILES})
endif()

set_property(TARGET ${THE_TARGET_NAME} PROPERTY CXX_STANDARD 11)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout/ImportFeatures.h.cmake ${OSMSCOUT_BASE_DIR_BUILD}/include/osmscout/ImportFeatures.h)
create_private_config("${CMAKE_CURRENT_BINARY_DIR}/include/osmscout/private/Config.h" "${THE_TARGET_NAME}")

target_include_directories(${THE_TARGET_NAME} PRIVATE
		include
		include/osmscout/import/pbf
		${CMAKE_CURRENT_BINARY_DIR}/include
		${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include
		)

if(APPLE)
  target_link_libraries(${THE_TARGET_NAME} OSMScout)
else()
  target_link_libraries(${THE_TARGET_NAME} osmscout)
endif()

if(ZLIB_FOUND AND ICONV_FOUND AND LIBXML2_FOUND)
    target_include_directories(${THE_TARGET_NAME} PRIVATE ${LIBXML2_INCLUDE_DIR})
    target_link_libraries(${THE_TARGET_NAME} ${LIBXML2_LIBRARIES})
endif()

if (PROTOBUF_FOUND)
    target_include_directories(${THE_TARGET_NAME} PRIVATE ${PROTOBUF_INCLUDE_DIRS})
    target_link_libraries(${THE_TARGET_NAME} ${PROTOBUF_LIBRARIES})
endif()

if (ICONV_FOUND)
    target_include_directories(${THE_TARGET_NAME} PRIVATE ${ICONV_INCLUDE_DIRS})
    target_link_libraries(${THE_TARGET_NAME} ${ICONV_LIBRARIES})
endif()

if (LIBLZMA_FOUND)
    target_include_directories(${THE_TARGET_NAME} PRIVATE ${LIBLZMA_INCLUDE_DIRS})
  target_link_libraries(${THE_TARGET_NAME} ${LIBLZMA_LIBRARIES})
endif()

if (ZLIB_FOUND)
    target_include_directories(${THE_TARGET_NAME} PRIVATE ${ZLIB_INCLUDE_DIRS})
  target_link_libraries(${THE_TARGET_NAME} ${ZLIB_LIBRARIES})
endif()

if(MARISA_FOUND)
    target_include_directories(${THE_TARGET_NAME} PRIVATE ${MARISA_INCLUDE_DIRS})
    target_link_libraries(${THE_TARGET_NAME} ${MARISA_LIBRARIES})
endif()

if(WIN32)
  target_link_libraries(${THE_TARGET_NAME} Ws2_32.lib)
  target_compile_definitions(${THE_TARGET_NAME} PRIVATE -DPROTOBUF_USE_DLLS)
endif()

#if(${IOS})
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fembed-bitcode")
#endif()

if(APPLE)
set_target_properties(${THE_TARGET_NAME} PROPERTIES
  FRAMEWORK TRUE
  FRAMEWORK_VERSION C
  MACOSX_FRAMEWORK_IDENTIFIER com.cmake.dynamicFramework
  #MACOSX_FRAMEWORK_INFO_PLIST Info.plist
  PUBLIC_HEADER     "${HEADER_FILES}"
  CODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer"
)
endif()

target_compile_definitions(${THE_TARGET_NAME} PRIVATE -DOSMSCOUT_IMPORT_EXPORT_SYMBOLS ${LIBXML2_DEFINITIONS})
install(TARGETS ${THE_TARGET_NAME}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        FRAMEWORK DESTINATION lib)
if(MARISA_FOUND)
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout DESTINATION include FILES_MATCHING PATTERN "*.h" PATTERN "private" EXCLUDE)
else()
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout DESTINATION include FILES_MATCHING PATTERN "*.h" PATTERN "private" EXCLUDE PATTERN GenTextIndex.h EXCLUDE)
endif()
install(FILES ${OSMSCOUT_BASE_DIR_BUILD}/include/osmscout/ImportFeatures.h DESTINATION include/osmscout)
