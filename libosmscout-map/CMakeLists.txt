set(HEADER_FILES
	include/osmscout/MapImportExport.h
	include/osmscout/oss/Parser.h
	include/osmscout/oss/Scanner.h
	include/osmscout/LabelLayouter.h
	include/osmscout/MapPainter.h
	include/osmscout/MapParameter.h
	include/osmscout/MapData.h
	include/osmscout/MapService.h
	include/osmscout/LabelProvider.h
	include/osmscout/LabelPath.h
	include/osmscout/Styles.h
	include/osmscout/StyleDescription.h
	include/osmscout/StyleConfig.h
	include/osmscout/StyleProcessor.h
	include/osmscout/DataTileCache.h
	include/osmscout/MapTileCache.h
	include/osmscout/MapPainterNoOp.h
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
	TEMPLATE ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout/MapFeatures.h.cmake
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
