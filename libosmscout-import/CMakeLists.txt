set(HEADER_FILES
    include/osmscout/import/AreaIndexGenerator.h
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
    include/osmscout/import/GenPTRouteDat.h
    include/osmscout/import/GenRawNodeIndex.h
    include/osmscout/import/GenRawRelIndex.h
    include/osmscout/import/GenAreaRouteIndex.h
    include/osmscout/import/GenRawWayIndex.h
    include/osmscout/import/GenRelAreaDat.h
    include/osmscout/import/GenRouteDat.h
    include/osmscout/import/GenRoute2Dat.h
    include/osmscout/import/GenTypeDat.h
    include/osmscout/import/GenWaterIndex.h
    include/osmscout/import/GenWayAreaDat.h
    include/osmscout/import/GenWayWayDat.h
    include/osmscout/import/Import.h
    include/osmscout/import/ImportErrorReporter.h
    include/osmscout/import/ImportModule.h
    include/osmscout/import/ImportParameter.h
    include/osmscout/import/ImportProgress.h
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
    src/osmscout/import/AreaIndexGenerator.cpp
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
    src/osmscout/import/GenAreaRouteIndex.cpp
    src/osmscout/import/GenRawWayIndex.cpp
    src/osmscout/import/GenRelAreaDat.cpp
    src/osmscout/import/GenPTRouteDat.cpp
    src/osmscout/import/GenRouteDat.cpp
    src/osmscout/import/GenRoute2Dat.cpp
    src/osmscout/import/GenTypeDat.cpp
    src/osmscout/import/GenWaterIndex.cpp
    src/osmscout/import/GenWayAreaDat.cpp
    src/osmscout/import/GenWayWayDat.cpp
    src/osmscout/import/Import.cpp
    src/osmscout/import/ImportErrorReporter.cpp
    src/osmscout/import/ImportModule.cpp
    src/osmscout/import/ImportParameter.cpp
    src/osmscout/import/ImportProgress.cpp
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

set(EXCLUDE_HEADER)

if(MARISA_FOUND)
    list(APPEND HEADER_FILES include/osmscout/import/GenTextIndex.h)
    list(APPEND SOURCE_FILES src/osmscout/import/GenTextIndex.cpp)
else()
	list(APPEND EXCLUDE_HEADER GenTextIndex.h)
endif()

if(LIBXML2_FOUND)
    list(APPEND HEADER_FILES include/osmscout/import/PreprocessOSM.h)
    list(APPEND SOURCE_FILES src/osmscout/import/PreprocessOSM.cpp)
else()
	list(APPEND EXCLUDE_HEADER PreprocessOSM.h)
endif()

if (PROTOBUF_FOUND)
    protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS
		${CMAKE_CURRENT_SOURCE_DIR}/src/protobuf/fileformat.proto
		${CMAKE_CURRENT_SOURCE_DIR}/src/protobuf/osmformat.proto)
    list(APPEND HEADER_FILES include/osmscout/import/PreprocessPBF.h)
    list(APPEND SOURCE_FILES src/osmscout/import/PreprocessPBF.cpp)
    list(APPEND HEADER_FILES ${PROTO_HDRS})
    list(APPEND SOURCE_FILES ${PROTO_SRCS})
else()
	list(APPEND EXCLUDE_HEADER PreprocessPBF.h ${PROTO_HDRS})
endif()

osmscout_library_project(
	NAME OSMScoutImport
	ALIAS Import
	OUTPUT_NAME "osmscout_import"
	SOURCE ${SOURCE_FILES}
	HEADER ${HEADER_FILES}
	TEMPLATE ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout/import/ImportFeatures.h.cmake
	FEATURE ${CMAKE_CURRENT_BINARY_DIR}/include/osmscout/import/ImportFeatures.h
	TARGET OSMScout::OSMScout
	SKIP_HEADER
)

if(LIBXML2_FOUND)
	target_include_directories(OSMScoutImport PRIVATE ${LIBXML2_INCLUDE_DIR})
	target_link_libraries(OSMScoutImport ${LIBXML2_LIBRARIES})
endif()

if (PROTOBUF_FOUND)
	target_include_directories(OSMScoutImport PRIVATE ${Protobuf_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})
	target_link_libraries(OSMScoutImport ${Protobuf_LIBRARIES})
	if(WIN32)
		target_compile_definitions(OSMScoutImport PRIVATE -DPROTOBUF_USE_DLLS)
	endif()
	target_compile_definitions(OSMScoutImport PRIVATE -DOSMSCOUT_IMPORT_CMAKE_BUILD)
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

if (HAVE_STD_EXECUTION)
	target_link_libraries(OSMScoutImport TBB::tbb)
endif()

if(APPLE AND OSMSCOUT_BUILD_FRAMEWORKS)
	set_target_properties(OSMScoutImport PROPERTIES
        FRAMEWORK TRUE
        FRAMEWORK_VERSION C
        MACOSX_FRAMEWORK_IDENTIFIER com.cmake.dynamicFramework
        #MACOSX_FRAMEWORK_INFO_PLIST Info.plist
        PUBLIC_HEADER     "${HEADER_FILES}"
        CODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer"
        OUTPUT_NAME "OSMScoutImport")
endif()

if(WIN32)
	target_link_libraries(OSMScoutImport Ws2_32.lib)
endif()

if(NOT EXCLUDE_HEADER)
	install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout DESTINATION include FILES_MATCHING PATTERN "*.h" PATTERN "private" EXCLUDE)
else()
	set(_exclude PATTERN "private" EXCLUDE)
	foreach(ef ${EXCLUDE_HEADER})
		set(_exclude ${_exclude} PATTERN ${ef} EXCLUDE)
	endforeach()
	install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout DESTINATION include FILES_MATCHING PATTERN "*.h" ${_exclude})
endif()
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/osmscout/import/ImportFeatures.h DESTINATION include/osmscout)
