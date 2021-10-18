set(HEADER_FILES
    include/osmscoutmapagg/MapAggImportExport.h
    include/osmscoutmapagg/MapAggFeatures.h
    include/osmscoutmapagg/MapPainterAgg.h
)

set(SOURCE_FILES
    src/osmscoutmapagg/MapPainterAgg.cpp
)

osmscout_library_project(
	NAME OSMScoutMapAGG
	ALIAS MapAGG
	OUTPUT_NAME "osmscout_map_agg"
	SOURCE ${SOURCE_FILES}
	HEADER ${HEADER_FILES}
	INCLUDEDIR "osmscoutmapagg"
	INCLUDES ${LIBAGG_INCLUDE_DIRS} ${FREETYPE_INCLUDE_DIR_freetype2} ${FREETYPE_INCLUDE_DIR_ft2build}
	TEMPLATE ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscoutmapagg/MapAggFeatures.h.cmake
	TARGET OSMScout::OSMScout OSMScout::Map ${LIBAGG_LIBRARIES} ${LIBAGGFT2_LIBRARIES} ${FREETYPE_LIBRARIES}
)
target_include_directories(OSMScoutMapAGG INTERFACE ${LIBAGG_INCLUDE_DIRS} ${FREETYPE_INCLUDE_DIR_freetype2} ${FREETYPE_INCLUDE_DIR_ft2build})

if(APPLE AND OSMSCOUT_BUILD_FRAMEWORKS)
	set_target_properties(OSMScoutMapAGG PROPERTIES
  		FRAMEWORK TRUE
  		FRAMEWORK_VERSION C
  		MACOSX_FRAMEWORK_IDENTIFIER com.cmake.dynamicFramework
  		#MACOSX_FRAMEWORK_INFO_PLIST Info.plist
  		PUBLIC_HEADER     "${HEADER_FILES}"
  		CODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer"
		OUTPUT_NAME "OSMScoutMapAGG")
endif()
