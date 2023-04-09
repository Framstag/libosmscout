set(HEADER_FILES
	include/osmscoutmap/MapImportExport.h
	include/osmscoutmap/oss/Parser.h
	include/osmscoutmap/oss/Scanner.h
	include/osmscoutmap/BatchMapPainter.h
	include/osmscoutmap/LabelLayouter.h
	include/osmscoutmap/BatchMapPainter.h
	include/osmscoutmap/MapPainter.h
	include/osmscoutmap/MapPainterStatistics.h
	include/osmscoutmap/MapParameter.h
	include/osmscoutmap/MapData.h
	include/osmscoutmap/MapService.h
	include/osmscoutmap/LabelProvider.h
	include/osmscoutmap/LabelPath.h
	include/osmscoutmap/Styles.h
	include/osmscoutmap/StyleDescription.h
	include/osmscoutmap/StyleConfig.h
	include/osmscoutmap/StyleProcessor.h
	include/osmscoutmap/DataTileCache.h
	include/osmscoutmap/MapTileCache.h
	include/osmscoutmap/MapPainterNoOp.h
	include/osmscoutmap/SymbolRenderer.h
	${CMAKE_CURRENT_BINARY_DIR}/include/osmscoutmap/MapFeatures.h
)

set(SOURCE_FILES
	src/osmscoutmap/oss/Parser.cpp
	src/osmscoutmap/oss/Scanner.cpp
	src/osmscoutmap/LabelLayouter.cpp
	src/osmscoutmap/MapPainter.cpp
	src/osmscoutmap/MapPainterStatistics.cpp
	src/osmscoutmap/MapParameter.cpp
	src/osmscoutmap/MapData.cpp
	src/osmscoutmap/MapService.cpp
	src/osmscoutmap/LabelProvider.cpp
	src/osmscoutmap/LabelPath.cpp
	src/osmscoutmap/Styles.cpp
	src/osmscoutmap/StyleDescription.cpp
	src/osmscoutmap/StyleConfig.cpp
	src/osmscoutmap/StyleProcessor.cpp
	src/osmscoutmap/DataTileCache.cpp
	src/osmscoutmap/MapTileCache.cpp
	src/osmscoutmap/MapPainterNoOp.cpp
	src/osmscoutmap/SymbolRenderer.cpp
)

osmscout_library_project(
	NAME OSMScoutMap
	ALIAS Map
	OUTPUT_NAME "osmscout_map"
	SOURCE ${SOURCE_FILES}
	HEADER ${HEADER_FILES}
	INCLUDEDIR "osmscoutmap"
	TEMPLATE ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscoutmap/MapFeatures.h.cmake
	FEATURE ${CMAKE_CURRENT_BINARY_DIR}/include/osmscoutmap/MapFeatures.h
	TARGET OSMScout::OSMScout
)

if(APPLE AND OSMSCOUT_BUILD_FRAMEWORKS)
	set_target_properties(OSMScoutMap PROPERTIES
  		FRAMEWORK TRUE
  		FRAMEWORK_VERSION A
  		MACOSX_FRAMEWORK_IDENTIFIER com.cmake.dynamicFramework
  		#MACOSX_FRAMEWORK_INFO_PLIST Info.plist
  		PUBLIC_HEADER     "${HEADER_FILES}"
  		CODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer"
		OUTPUT_NAME "OSMScoutMap")
endif()
