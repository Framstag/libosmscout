set(HEADER_FILES
    include/osmscout/MapCairoImportExport.h
    include/osmscout/LoaderPNG.h
    include/osmscout/MapPainterCairo.h
)

set(SOURCE_FILES
    src/osmscout/LoaderPNG.cpp
    src/osmscout/MapPainterCairo.cpp
)

add_library(OSMScoutMapCairo ${SOURCE_FILES} ${HEADER_FILES})
add_library(OSMScout::MapCairo ALIAS OSMScoutMapCairo)
set_target_properties(OSMScoutMapCairo PROPERTIES
	OUTPUT_NAME "osmscout_map_cairo"
	VERSION ${OSMSCOUT_LIBRARY_VERSION}
	SOVERSION ${OSMSCOUT_LIBRARY_VERSION}
	FOLDER "libosmscout"
)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout/MapCairoFeatures.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/include/osmscout/MapCairoFeatures.h)
create_private_config("${CMAKE_CURRENT_BINARY_DIR}/privateinclude/osmscout/private/Config.h" "OSMScoutMapCairo")
target_include_directories(OSMScoutMapCairo PRIVATE
	include
	${CMAKE_SOURCE_DIR}/include
	${CMAKE_CURRENT_BINARY_DIR}/include
	${CMAKE_CURRENT_BINARY_DIR}/privateinclude
	${CMAKE_CURRENT_SOURCE_DIR}/include
	${CAIRO_INCLUDE_DIRS}
	${PNG_INCLUDE_DIRS}
)
target_include_directories(OSMScoutMapCairo INTERFACE ${CMAKE_SOURCE_DIR}/libosmscout-map-cairo/include ${CMAKE_BINARY_DIR}/libosmscout-map-cairo/include ${CAIRO_INCLUDE_DIRS} ${PNG_INCLUDE_DIRS})
target_link_libraries(OSMScoutMapCairo OSMScout::OSMScout OSMScout::Map ${CAIRO_LIBRARIES} ${PNG_LIBRARIES})

if(PANGOCAIRO_FOUND)
  target_include_directories(OSMScoutMapCairo PUBLIC ${PANGO_INCLUDE_DIRS})
  target_link_libraries(OSMScoutMapCairo ${PANGO_LIBRARIES})
  target_include_directories(OSMScoutMapCairo INTERFACE ${PANGO_INCLUDE_DIRS})

  if(HARFBUZZ_FOUND)
	  target_include_directories(OSMScoutMapCairo PUBLIC ${HARFBUZZ_INCLUDE_DIRS})
	  target_link_libraries(OSMScoutMapCairo ${HARFBUZZ_LIBRARIES})
	  target_include_directories(OSMScoutMapCairo INTERFACE ${HARFBUZZ_INCLUDE_DIRS})
  endif()
endif()

if(APPLE AND BUILD_FRAMEWORKS)
	set_target_properties(OSMScoutMapCairo PROPERTIES
  		FRAMEWORK TRUE
  		FRAMEWORK_VERSION C
  		MACOSX_FRAMEWORK_IDENTIFIER com.cmake.dynamicFramework
  		#MACOSX_FRAMEWORK_INFO_PLIST Info.plist
  		PUBLIC_HEADER     "${HEADER_FILES}"
  		CODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer"
		OUTPUT_NAME "OSMScoutMapCairo")
endif()

if(WIN32 AND BUILD_SHARED_LIBS)
	create_win32_dll_resource(OSMScoutMapCairo "osmscout_map_cairo")
endif()

install(TARGETS OSMScoutMapCairo
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        FRAMEWORK DESTINATION lib)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout DESTINATION include FILES_MATCHING PATTERN "*.h" PATTERN "private/Config.h" EXCLUDE)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/osmscout/MapCairoFeatures.h DESTINATION include/osmscout)
