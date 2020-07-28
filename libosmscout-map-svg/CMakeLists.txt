set(HEADER_FILES
    include/osmscout/MapSVGImportExport.h
    include/osmscout/MapPainterSVG.h
)

set(SOURCE_FILES
    src/osmscout/MapPainterSVG.cpp
)

add_library(OSMScoutMapSVG ${SOURCE_FILES} ${HEADER_FILES})
add_library(OSMScout::MapSVG ALIAS OSMScoutMapSVG)
set_target_properties(OSMScoutMapSVG PROPERTIES
	OUTPUT_NAME "osmscout_map_svg"
	VERSION ${OSMSCOUT_LIBRARY_VERSION}
	SOVERSION ${OSMSCOUT_LIBRARY_VERSION}
	FOLDER "libosmscout"
)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout/MapSVGFeatures.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/include/osmscout/MapSVGFeatures.h)
create_private_config("${CMAKE_CURRENT_BINARY_DIR}/privateinclude/osmscout/private/Config.h" "OSMScoutMapSVG")
target_include_directories(OSMScoutMapSVG PRIVATE
	include
	${CMAKE_SOURCE_DIR}/include
	${CMAKE_CURRENT_BINARY_DIR}/include
	${CMAKE_CURRENT_BINARY_DIR}/privateinclude
	${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_include_directories(OSMScoutMapSVG INTERFACE ${CMAKE_SOURCE_DIR}/libosmscout-map-svg/include ${CMAKE_BINARY_DIR}/libosmscout-map-svg/include)
target_link_libraries(OSMScoutMapSVG OSMScout::OSMScout OSMScout::Map)

if(APPLE AND BUILD_FRAMEWORKS)
    set_target_properties(OSMScoutMapSVG PROPERTIES
            FRAMEWORK TRUE
            FRAMEWORK_VERSION C
            MACOSX_FRAMEWORK_IDENTIFIER com.cmake.dynamicFramework
            #MACOSX_FRAMEWORK_INFO_PLIST Info.plist
            PUBLIC_HEADER     "${HEADER_FILES}"
            CODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer"
            OUTPUT_NAME "OSMScoutMapSVG")
endif()

if(PANGOFT2_FOUND)
  target_include_directories(OSMScoutMapSVG PUBLIC ${PANGOFT2_INCLUDE_DIRS})
  target_link_libraries(OSMScoutMapSVG ${PANGOFT2_LIBRARIES})

  if(HARFBUZZ_FOUND)
      target_include_directories(OSMScoutMapSVG PUBLIC ${HARFBUZZ_INCLUDE_DIRS})
      target_link_libraries(OSMScoutMapSVG ${HARFBUZZ_LIBRARIES})
  endif()
endif()

if(WIN32 AND BUILD_SHARED_LIBS)
	create_win32_dll_resource(OSMScoutMapSVG "osmscout_map_svg")
endif()

install(TARGETS OSMScoutMapSVG
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        FRAMEWORK DESTINATION lib)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout DESTINATION include FILES_MATCHING PATTERN "*.h" PATTERN "private" EXCLUDE)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/osmscout/MapSVGFeatures.h DESTINATION include/osmscout)
