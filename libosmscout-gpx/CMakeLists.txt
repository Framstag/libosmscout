set(HEADER_FILES
	include/osmscout/gpx/GpxFile.h
	include/osmscout/gpx/GPXImportExport.h
	include/osmscout/gpx/Route.h
	include/osmscout/gpx/Track.h
	include/osmscout/gpx/Waypoint.h
	include/osmscout/gpx/TrackPoint.h
	include/osmscout/gpx/TrackSegment.h
	include/osmscout/gpx/Utils.h
)

set(SOURCE_FILES
    src/osmscout/gpx/GpxFile.cpp
    src/osmscout/gpx/Track.cpp
	src/osmscout/gpx/TrackSegment.cpp
	src/osmscout/gpx/Utils.cpp
)

if(LIBXML2_FOUND)
    list(APPEND HEADER_FILES
        include/osmscout/gpx/Import.h
        include/osmscout/gpx/Export.h
    )
    list(APPEND SOURCE_FILES
        src/osmscout/gpx/Import.cpp
        src/osmscout/gpx/Export.cpp
    )
endif()

osmscout_library_project(
	NAME OSMScoutGPX
	ALIAS GPX
	OUTPUT_NAME "osmscout_gpx"
	SOURCE ${SOURCE_FILES}
	HEADER ${HEADER_FILES}
	TEMPLATE ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout/gpx/GPXFeatures.h.cmake
	FEATURE ${CMAKE_CURRENT_BINARY_DIR}/include/osmscout/gpx/GPXFeatures.h
	TARGET OSMScout::OSMScout
	SKIP_HEADER
)

if(LIBXML2_FOUND)
    target_include_directories(OSMScoutGPX PRIVATE ${LIBXML2_INCLUDE_DIR})
    target_link_libraries(OSMScoutGPX ${LIBXML2_LIBRARIES})
	target_include_directories(OSMScoutGPX INTERFACE ${LIBXML2_INCLUDE_DIR})
endif()

if (ICONV_FOUND)
	target_include_directories(OSMScoutGPX PRIVATE ${ICONV_INCLUDE_DIRS})
	target_link_libraries(OSMScoutGPX ${ICONV_LIBRARIES})
endif()

if (ZLIB_FOUND)
	target_include_directories(OSMScoutGPX PRIVATE ${ZLIB_INCLUDE_DIRS})
	target_link_libraries(OSMScoutGPX ${ZLIB_LIBRARIES})
endif()

if(APPLE AND BUILD_FRAMEWORKS)
	set_target_properties(OSMScoutGPX PROPERTIES
		FRAMEWORK TRUE
		FRAMEWORK_VERSION C
  		MACOSX_FRAMEWORK_IDENTIFIER com.cmake.dynamicFramework
  		#MACOSX_FRAMEWORK_INFO_PLIST Info.plist
  		PUBLIC_HEADER     "${HEADER_FILES}"
  		CODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer"
		OUTPUT_NAME "OSMScoutGPX")
endif()

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout DESTINATION include FILES_MATCHING PATTERN "*.h" PATTERN "private" EXCLUDE)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/osmscout/gpx/GPXFeatures.h DESTINATION include/osmscout)
