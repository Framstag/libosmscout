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

add_library(OSMScoutGPX ${SOURCE_FILES} ${HEADER_FILES})
add_library(OSMScout::GPX ALIAS OSMScoutGPX)
set_target_properties(OSMScoutGPX PROPERTIES
	OUTPUT_NAME "osmscout_gpx"
	VERSION ${OSMSCOUT_LIBRARY_VERSION}
	SOVERSION ${OSMSCOUT_LIBRARY_VERSION}
	FOLDER "libosmscout"
)
configure_file(include/osmscout/gpx/GPXFeatures.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/include/osmscout/gpx/GPXFeatures.h)
create_private_config("${CMAKE_CURRENT_BINARY_DIR}/include/osmscout/private/Config.h" "OSMScoutGPX")
target_include_directories(OSMScoutGPX PRIVATE
	include
	${CMAKE_SOURCE_DIR}/include
	${CMAKE_CURRENT_BINARY_DIR}/include
	${CMAKE_CURRENT_BINARY_DIR}/privateinclude
	${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_include_directories(OSMScoutGPX INTERFACE ${CMAKE_SOURCE_DIR}/libosmscout-gpx/include ${CMAKE_BINARY_DIR}/libosmscout-gpx/include)
target_link_libraries(OSMScoutGPX OSMScout::OSMScout)

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

if(WIN32 AND BUILD_SHARED_LIBS)
	create_win32_dll_resource(OSMScoutGPX "osmscout_gpx")
endif()

install(TARGETS OSMScoutGPX
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        FRAMEWORK DESTINATION lib)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout DESTINATION include FILES_MATCHING PATTERN "*.h" PATTERN "private/Config.h" EXCLUDE)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/osmscout/gpx/GPXFeatures.h DESTINATION include/osmscout)
