set(HEADER_FILES
    include/osmscoutimport/AreaIndexGenerator.h
    include/osmscoutimport/GenAreaAreaIndex.h
    include/osmscoutimport/GenAreaNodeIndex.h
    include/osmscoutimport/GenAreaWayIndex.h
    include/osmscoutimport/GenCoordDat.h
    include/osmscoutimport/GenCoverageIndex.h
    include/osmscoutimport/GenIntersectionIndex.h
    include/osmscoutimport/GenLocationIndex.h
    include/osmscoutimport/GenMergeAreas.h
    include/osmscoutimport/GenNodeDat.h
    include/osmscoutimport/GenNumericIndex.h
    include/osmscoutimport/GenOptimizeAreasLowZoom.h
    include/osmscoutimport/GenOptimizeAreaWayIds.h
    include/osmscoutimport/GenOptimizeWaysLowZoom.h
    include/osmscoutimport/GenPTRouteDat.h
    include/osmscoutimport/GenRawNodeIndex.h
    include/osmscoutimport/GenRawRelIndex.h
    include/osmscoutimport/GenAreaRouteIndex.h
    include/osmscoutimport/GenRawWayIndex.h
    include/osmscoutimport/GenRelAreaDat.h
    include/osmscoutimport/GenRouteDat.h
    include/osmscoutimport/GenRoute2Dat.h
    include/osmscoutimport/GenTypeDat.h
    include/osmscoutimport/GenWaterIndex.h
    include/osmscoutimport/GenWayAreaDat.h
    include/osmscoutimport/GenWayWayDat.h
    include/osmscoutimport/Import.h
    include/osmscoutimport/ImportErrorReporter.h
    include/osmscoutimport/ImportModule.h
    include/osmscoutimport/ImportParameter.h
    include/osmscoutimport/ImportProgress.h
    include/osmscoutimport/MergeAreaData.h
    include/osmscoutimport/Preprocess.h
    include/osmscoutimport/Preprocessor.h
    include/osmscoutimport/PreprocessPoly.h
    include/osmscoutimport/RawCoastline.h
    include/osmscoutimport/RawCoord.h
    include/osmscoutimport/RawNode.h
    include/osmscoutimport/RawRelation.h
    include/osmscoutimport/RawRelIndexedDataFile.h
    include/osmscoutimport/RawWay.h
    include/osmscoutimport/RawWayIndexedDataFile.h
    include/osmscoutimport/ShapeFileScanner.h
    include/osmscoutimport/WaterIndexProcessor.h
    include/osmscoutimport/SortDat.h
    include/osmscoutimport/SortNodeDat.h
    include/osmscoutimport/SortWayDat.h
    include/osmscoutimport/ImportImportExport.h
)

set(SOURCE_FILES
    src/osmscoutimport/AreaIndexGenerator.cpp
    src/osmscoutimport/GenAreaAreaIndex.cpp
    src/osmscoutimport/GenAreaNodeIndex.cpp
    src/osmscoutimport/GenAreaWayIndex.cpp
    src/osmscoutimport/GenCoordDat.cpp
    src/osmscoutimport/GenCoverageIndex.cpp
    src/osmscoutimport/GenIntersectionIndex.cpp
    src/osmscoutimport/GenLocationIndex.cpp
    src/osmscoutimport/GenMergeAreas.cpp
    src/osmscoutimport/GenNodeDat.cpp
    src/osmscoutimport/GenNumericIndex.cpp
    src/osmscoutimport/GenOptimizeAreasLowZoom.cpp
    src/osmscoutimport/GenOptimizeAreaWayIds.cpp
    src/osmscoutimport/GenOptimizeWaysLowZoom.cpp
    src/osmscoutimport/GenRawNodeIndex.cpp
    src/osmscoutimport/GenRawRelIndex.cpp
    src/osmscoutimport/GenAreaRouteIndex.cpp
    src/osmscoutimport/GenRawWayIndex.cpp
    src/osmscoutimport/GenRelAreaDat.cpp
    src/osmscoutimport/GenPTRouteDat.cpp
    src/osmscoutimport/GenRouteDat.cpp
    src/osmscoutimport/GenRoute2Dat.cpp
    src/osmscoutimport/GenTypeDat.cpp
    src/osmscoutimport/GenWaterIndex.cpp
    src/osmscoutimport/GenWayAreaDat.cpp
    src/osmscoutimport/GenWayWayDat.cpp
    src/osmscoutimport/Import.cpp
    src/osmscoutimport/ImportErrorReporter.cpp
    src/osmscoutimport/ImportModule.cpp
    src/osmscoutimport/ImportParameter.cpp
    src/osmscoutimport/ImportProgress.cpp
    src/osmscoutimport/MergeAreaData.cpp
    src/osmscoutimport/Preprocess.cpp
    src/osmscoutimport/Preprocessor.cpp
    src/osmscoutimport/PreprocessPoly.cpp
    src/osmscoutimport/RawCoastline.cpp
    src/osmscoutimport/RawCoord.cpp
    src/osmscoutimport/RawNode.cpp
    src/osmscoutimport/RawRelation.cpp
    src/osmscoutimport/RawRelIndexedDataFile.cpp
    src/osmscoutimport/RawWay.cpp
    src/osmscoutimport/RawWayIndexedDataFile.cpp
    src/osmscoutimport/ShapeFileScanner.cpp
    src/osmscoutimport/WaterIndexProcessor.cpp
    src/osmscoutimport/SortDat.cpp
    src/osmscoutimport/SortNodeDat.cpp
    src/osmscoutimport/SortWayDat.cpp
)

set(EXCLUDE_HEADER)

if(MARISA_FOUND)
    list(APPEND HEADER_FILES include/osmscoutimport/GenTextIndex.h)
    list(APPEND SOURCE_FILES src/osmscoutimport/GenTextIndex.cpp)
else()
	list(APPEND EXCLUDE_HEADER GenTextIndex.h)
endif()

if(TARGET LibXml2::LibXml2)
    list(APPEND HEADER_FILES include/osmscoutimport/PreprocessOSM.h)
    list(APPEND SOURCE_FILES src/osmscoutimport/PreprocessOSM.cpp)
else()
	list(APPEND EXCLUDE_HEADER PreprocessOSM.h)
endif()

if (TARGET protobuf::libprotobuf AND EXISTS ${PROTOBUF_PROTOC_EXECUTABLE})
	protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS
		${CMAKE_CURRENT_SOURCE_DIR}/src/protobuf/fileformat.proto
		${CMAKE_CURRENT_SOURCE_DIR}/src/protobuf/osmformat.proto)
    list(APPEND HEADER_FILES include/osmscoutimport/PreprocessPBF.h)
    list(APPEND SOURCE_FILES src/osmscoutimport/PreprocessPBF.cpp)
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
	INCLUDEDIR "osmscoutimport"
	TEMPLATE ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscoutimport/ImportFeatures.h.cmake
	FEATURE ${CMAKE_CURRENT_BINARY_DIR}/include/osmscoutimport/ImportFeatures.h
	TARGET OSMScout::OSMScout
	SKIP_HEADER
)

if(TARGET LibXml2::LibXml2)
	target_link_libraries(OSMScoutImport LibXml2::LibXml2)
endif()

if (TARGET protobuf::libprotobuf AND EXISTS ${PROTOBUF_PROTOC_EXECUTABLE})
	target_include_directories(OSMScoutImport PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
	target_link_libraries(OSMScoutImport protobuf::libprotobuf)
	if(WIN32)
		target_compile_definitions(OSMScoutImport PRIVATE -DPROTOBUF_USE_DLLS)
	endif()
	target_compile_definitions(OSMScoutImport PRIVATE -DOSMSCOUT_IMPORT_CMAKE_BUILD)
endif()

if (TARGET Iconv::Iconv)
	target_link_libraries(OSMScoutImport Iconv::Iconv)
endif()

if (TARGET LibLZMA::LibLZMA)
	target_link_libraries(OSMScoutImport LibLZMA::LibLZMA)
endif()

if (TARGET ZLIB::ZLIB)
	target_link_libraries(OSMScoutImport ZLIB::ZLIB)
endif()

if(MARISA_FOUND)
	target_include_directories(OSMScoutImport PRIVATE ${MARISA_INCLUDE_DIRS})
	target_link_libraries(OSMScoutImport ${MARISA_LIBRARIES})
endif()

if (TBB_FOUND)
	target_link_libraries(OSMScoutImport TBB::tbb)
endif()

if(TARGET OpenMP::OpenMP_CXX)
	target_link_libraries(OSMScoutImport OpenMP::OpenMP_CXX)
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
	install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscoutimport DESTINATION include FILES_MATCHING PATTERN "*.h" PATTERN "private" EXCLUDE)
else()
	set(_exclude PATTERN "private" EXCLUDE)
	foreach(ef ${EXCLUDE_HEADER})
		set(_exclude ${_exclude} PATTERN ${ef} EXCLUDE)
	endforeach()
	install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscoutimport DESTINATION include FILES_MATCHING PATTERN "*.h" ${_exclude})
endif()
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/osmscoutimport/ImportFeatures.h DESTINATION include/osmscout)
