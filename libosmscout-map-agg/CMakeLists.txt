set(HEADER_FILES
    include/osmscout/MapAggImportExport.h
    include/osmscout/MapAggFeatures.h
    include/osmscout/MapPainterAgg.h
)

set(SOURCE_FILES
    src/osmscout/MapPainterAgg.cpp
)

add_library(OSMScoutMapAGG ${SOURCE_FILES} ${HEADER_FILES})
add_library(OSMScout::MapAGG ALIAS OSMScoutMapAGG)
set_target_properties(OSMScoutMapAGG PROPERTIES
	OUTPUT_NAME "osmscout_map_agg"
	VERSION ${OSMSCOUT_LIBRARY_VERSION}
	SOVERSION ${OSMSCOUT_LIBRARY_VERSION}
	FOLDER "libosmscout"
)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout/MapAggFeatures.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/include/osmscout/MapAggFeatures.h)
create_private_config("${CMAKE_CURRENT_BINARY_DIR}/privateinclude/osmscout/private/Config.h" "OSMScoutMapAGG")
target_include_directories(OSMScoutMapAGG PRIVATE
	include
	${CMAKE_SOURCE_DIR}/include
	${CMAKE_CURRENT_BINARY_DIR}/include
	${CMAKE_CURRENT_BINARY_DIR}/privateinclude
	${CMAKE_CURRENT_SOURCE_DIR}/include
	${LIBAGG_INCLUDE_DIRS}
	${FREETYPE_INCLUDE_DIR_freetype2}
	${FREETYPE_INCLUDE_DIR_ft2build}
)
target_include_directories(OSMScoutMapAGG INTERFACE ${CMAKE_SOURCE_DIR}/libosmscout-map-agg/include ${CMAKE_BINARY_DIR}/libosmscout-map-agg/include ${LIBAGG_INCLUDE_DIRS} ${FREETYPE_INCLUDE_DIR_freetype2} ${FREETYPE_INCLUDE_DIR_ft2build})
target_link_libraries(OSMScoutMapAGG OSMScout::OSMScout OSMScout::Map ${LIBAGG_LIBRARIES} ${LIBAGGFT2_LIBRARIES} ${FREETYPE_LIBRARIES})

if(APPLE AND BUILD_FRAMEWORKS)
	set_target_properties(OSMScoutMapAGG PROPERTIES
  		FRAMEWORK TRUE
  		FRAMEWORK_VERSION C
  		MACOSX_FRAMEWORK_IDENTIFIER com.cmake.dynamicFramework
  		#MACOSX_FRAMEWORK_INFO_PLIST Info.plist
  		PUBLIC_HEADER     "${HEADER_FILES}"
  		CODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer"
		OUTPUT_NAME "OSMScoutMapAGG")
endif()

if(WIN32 AND BUILD_SHARED_LIBS)
	create_win32_dll_resource(OSMScoutMapAGG "osmscout_map_agg")
endif()

install(TARGETS OSMScoutMapAGG
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        FRAMEWORK DESTINATION lib)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout DESTINATION include FILES_MATCHING PATTERN "*.h" PATTERN "private" EXCLUDE)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/osmscout/MapAggFeatures.h DESTINATION include/osmscout)
