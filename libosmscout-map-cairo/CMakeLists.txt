if(NOT CAIRO_FOUND)
	message(SEND_ERROR "Cairo is required for cairo map drawing backend")
endif()
if(NOT PNG_FOUND)
	message(SEND_ERROR "libpng is required for cairo map drawing backend")
endif()
if(NOT ${OSMSCOUT_BUILD_MAP})
	message(SEND_ERROR "The main map drawing interface is required for cairo map drawing backend")
endif()

set(HEADER_FILES
    include/osmscout/MapCairoImportExport.h
    include/osmscout/LoaderPNG.h
    include/osmscout/MapPainterCairo.h
)

set(SOURCE_FILES
    src/osmscout/LoaderPNG.cpp
    src/osmscout/MapPainterCairo.cpp
)

if(IOS)
  add_library(OSMScoutMapCairo STATIC ${SOURCE_FILES} ${HEADER_FILES})
else()
  add_library(OSMScoutMapCairo ${SOURCE_FILES} ${HEADER_FILES})
endif()

set_target_properties(OSMScoutMapCairo PROPERTIES
		CXX_STANDARD 17
		OUTPUT_NAME "osmscout_map_cairo")

target_include_directories(OSMScoutMapCairo PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/include
		${CAIRO_INCLUDE_DIRS}
		${PNG_INCLUDE_DIRS})

# Public features Header
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout/MapCairoFeatures.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/include/osmscout/MapCairoFeatures.h)
target_include_directories(OSMScoutMapCairo PUBLIC include ${CMAKE_CURRENT_BINARY_DIR}/include)

# Private config header
create_private_config("${CMAKE_CURRENT_BINARY_DIR}/privateinclude/osmscout/private/Config.h" "OSMScoutMapCairo")
target_include_directories(OSMScoutMapCairo PRIVATE include ${CMAKE_CURRENT_BINARY_DIR}/privateinclude)

target_link_libraries(OSMScoutMapCairo
		OSMScout
		OSMScoutMap
		${CAIRO_LIBRARIES}
		${PNG_LIBRARIES})

if(PANGOCAIRO_FOUND)
  target_include_directories(OSMScoutMapCairo PUBLIC ${PANGO_INCLUDE_DIRS})
  target_link_libraries(OSMScoutMapCairo ${PANGO_LIBRARIES})

  if(HARFBUZZ_FOUND)
	  target_include_directories(OSMScoutMapCairo PUBLIC ${HARFBUZZ_INCLUDE_DIRS})
	  target_link_libraries(OSMScoutMapCairo ${HARFBUZZ_LIBRARIES})
  endif()
endif()

if(${IOS})
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fembed-bitcode")
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

target_compile_definitions(OSMScoutMapCairo PRIVATE -DOSMSCOUT_MAP_CAIRO_EXPORT_SYMBOLS ${PNG_DEFINITIONS})

install(TARGETS OSMScoutMapCairo
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        FRAMEWORK DESTINATION lib)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout DESTINATION include FILES_MATCHING PATTERN "*.h" PATTERN "private/Config.h" EXCLUDE)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/osmscout/MapCairoFeatures.h DESTINATION include/osmscout)
