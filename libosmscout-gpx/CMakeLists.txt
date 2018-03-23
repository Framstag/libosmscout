set(OSMSCOUT_BUILD_GPX ON CACHE INTERNAL "" FORCE)

set(HEADER_FILES
	include/osmscout/gpx/Optional.h
	include/osmscout/gpx/GpxFile.h
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

if(APPLE AND BUILD_FRAMEWORKS)
set(THE_TARGET_NAME "OSMScoutGPX")
else()
set(THE_TARGET_NAME "osmscout_gpx")
endif()

if(IOS)
  add_library(${THE_TARGET_NAME} STATIC ${SOURCE_FILES} ${HEADER_FILES})
else()
  add_library(${THE_TARGET_NAME} ${SOURCE_FILES} ${HEADER_FILES})
endif()

set_property(TARGET ${THE_TARGET_NAME} PROPERTY CXX_STANDARD 11)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout/GPXFeatures.h.cmake ${OSMSCOUT_BASE_DIR_BUILD}/include/osmscout/GPXFeatures.h)
create_private_config("${CMAKE_CURRENT_BINARY_DIR}/include/osmscout/private/Config.h" "osmscout-gpx")
target_include_directories(${THE_TARGET_NAME} PRIVATE
		include
		${CMAKE_CURRENT_BINARY_DIR}/include
		${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include
        )

if(LIBXML2_FOUND)
    target_include_directories(${THE_TARGET_NAME} PRIVATE ${LIBXML2_INCLUDE_DIR})
    target_link_libraries(${THE_TARGET_NAME} ${LIBXML2_LIBRARIES})

# Looks like libxml2 has additional dependencies under Windows
	if (ICONV_FOUND)
		target_include_directories(${THE_TARGET_NAME} PRIVATE ${ICONV_INCLUDE_DIRS})
		target_link_libraries(${THE_TARGET_NAME} ${ICONV_LIBRARIES})
	endif()

	if (ZLIB_FOUND)
		target_include_directories(${THE_TARGET_NAME} PRIVATE ${ZLIB_INCLUDE_DIRS})
		target_link_libraries(${THE_TARGET_NAME} ${ZLIB_LIBRARIES})
	endif()
endif()

if(APPLE AND BUILD_FRAMEWORKS)
  target_link_libraries(${THE_TARGET_NAME} OSMScout)
else()
  target_link_libraries(${THE_TARGET_NAME} osmscout)
endif()

if(${IOS})
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fembed-bitcode")
endif()

if(APPLE AND BUILD_FRAMEWORKS)
set_target_properties(${THE_TARGET_NAME} PROPERTIES
  FRAMEWORK TRUE
  FRAMEWORK_VERSION C
  MACOSX_FRAMEWORK_IDENTIFIER com.cmake.dynamicFramework
  #MACOSX_FRAMEWORK_INFO_PLIST Info.plist
  PUBLIC_HEADER     "${HEADER_FILES}"
  CODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer"
)
endif()

target_compile_definitions(${THE_TARGET_NAME} PRIVATE -DOSMSCOUT_GPX_EXPORT_SYMBOLS)
install(TARGETS ${THE_TARGET_NAME}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        FRAMEWORK DESTINATION lib)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout DESTINATION include FILES_MATCHING PATTERN "*.h" PATTERN "private/Config.h" EXCLUDE)
install(FILES ${OSMSCOUT_BASE_DIR_BUILD}/include/osmscout/GPXFeatures.h DESTINATION include/osmscout)
