set(HEADER_FILES
    include/osmscoutgpx/GpxFile.h
    include/osmscoutgpx/GPXImportExport.h
    include/osmscoutgpx/Route.h
    include/osmscoutgpx/Track.h
    include/osmscoutgpx/Waypoint.h
    include/osmscoutgpx/TrackPoint.h
    include/osmscoutgpx/TrackSegment.h
    include/osmscoutgpx/Utils.h
    include/osmscoutgpx/Extensions.h
	${CMAKE_CURRENT_BINARY_DIR}/include/osmscoutgpx/GPXFeatures.h
)

set(SOURCE_FILES
    src/osmscoutgpx/GpxFile.cpp
    src/osmscoutgpx/Track.cpp
    src/osmscoutgpx/TrackSegment.cpp
    src/osmscoutgpx/Utils.cpp
    src/osmscoutgpx/Extensions.cpp
)

if(TARGET LibXml2::LibXml2)
    list(APPEND HEADER_FILES
        include/osmscoutgpx/Import.h
        include/osmscoutgpx/Export.h
    )
    list(APPEND SOURCE_FILES
        src/osmscoutgpx/Import.cpp
        src/osmscoutgpx/Export.cpp
    )
endif()

osmscout_library_project(
	NAME OSMScoutGPX
	ALIAS GPX
	OUTPUT_NAME "osmscout_gpx"
	SOURCE ${SOURCE_FILES}
	HEADER ${HEADER_FILES}
	INCLUDEDIR "osmscoutgpx"
	TEMPLATE ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscoutgpx/GPXFeatures.h.cmake
	FEATURE ${CMAKE_CURRENT_BINARY_DIR}/include/osmscoutgpx/GPXFeatures.h
	TARGET OSMScout::OSMScout
)

if(TARGET LibXml2::LibXml2)
    target_link_libraries(OSMScoutGPX LibXml2::LibXml2)
endif()

if (TARGET Iconv::Iconv)
	target_link_libraries(OSMScoutGPX Iconv::Iconv)
endif()

if (TARGET ZLIB::ZLIB)
	target_link_libraries(OSMScoutGPX ZLIB::ZLIB)
endif()

if(APPLE AND OSMSCOUT_BUILD_FRAMEWORKS)
	set_target_properties(OSMScoutGPX PROPERTIES
		FRAMEWORK TRUE
		FRAMEWORK_VERSION C
  		MACOSX_FRAMEWORK_IDENTIFIER com.cmake.dynamicFramework
  		#MACOSX_FRAMEWORK_INFO_PLIST Info.plist
  		PUBLIC_HEADER     "${HEADER_FILES}"
  		CODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer"
		OUTPUT_NAME "OSMScoutGPX")
endif()

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscoutgpx DESTINATION include FILES_MATCHING PATTERN "*.h" PATTERN "private" EXCLUDE)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/osmscoutgpx/GPXFeatures.h DESTINATION include/osmscout)
