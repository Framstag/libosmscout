set(HEADER_FILES
	include/osmscoutmap/MapImportExport.h
	include/osmscoutmap/oss/Parser.h
	include/osmscoutmap/oss/Scanner.h
	include/osmscoutmap/LabelLayouter.h
	include/osmscoutmap/MapPainter.h
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
	include/osmscoutmap/MapFeatures.h
)

set(SOURCE_FILES
	src/osmscout/oss/Parser.cpp
	src/osmscout/oss/Scanner.cpp
	src/osmscout/LabelLayouter.cpp
	src/osmscout/MapPainter.cpp
	src/osmscout/MapParameter.cpp
	src/osmscout/MapData.cpp
	src/osmscout/MapService.cpp
	src/osmscout/LabelProvider.cpp
	src/osmscout/LabelPath.cpp
	src/osmscout/Styles.cpp
	src/osmscout/StyleDescription.cpp
	src/osmscout/StyleConfig.cpp
	src/osmscout/StyleProcessor.cpp
	src/osmscout/DataTileCache.cpp
	src/osmscout/MapTileCache.cpp
	src/osmscout/MapPainterNoOp.cpp
)

osmscout_library_project(
	NAME OSMScoutMap
	ALIAS Map
	OUTPUT_NAME "osmscout_map"
	SOURCE ${SOURCE_FILES}
	HEADER ${HEADER_FILES}
	TEMPLATE ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscoutmap/MapFeatures.h.cmake
	FEATURE ${CMAKE_CURRENT_BINARY_DIR}/include/osmscoutmap//MapFeatures.h
	TARGET OSMScout::OSMScout
	SKIP_HEADER
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
