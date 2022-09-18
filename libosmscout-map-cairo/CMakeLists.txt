set(HEADER_FILES
    include/osmscoutmapcairo/MapCairoImportExport.h
    include/osmscoutmapcairo/LoaderPNG.h
    include/osmscoutmapcairo/MapPainterCairo.h
	include/osmscoutmapcairo/SymbolRendererCairo.h
)

set(SOURCE_FILES
    src/osmscoutmapcairo/LoaderPNG.cpp
    src/osmscoutmapcairo/MapPainterCairo.cpp
	src/osmscoutmapcairo/SymbolRendererCairo.cpp
)

osmscout_library_project(
	NAME OSMScoutMapCairo
	ALIAS MapCairo
	OUTPUT_NAME "osmscout_map_cairo"
	SOURCE ${SOURCE_FILES}
	HEADER ${HEADER_FILES}
	INCLUDEDIR osmscoutmapcairo
	INCLUDES ${CAIRO_INCLUDE_DIRS} ${PNG_INCLUDE_DIRS}
	TEMPLATE ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscoutmapcairo/MapCairoFeatures.h.cmake
	TARGET OSMScout::OSMScout OSMScout::Map ${CAIRO_LIBRARIES} ${PNG_LIBRARIES}
)
target_include_directories(OSMScoutMapCairo INTERFACE ${CAIRO_INCLUDE_DIRS})

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

if(APPLE AND OSMSCOUT_BUILD_FRAMEWORKS)
	set_target_properties(OSMScoutMapCairo PROPERTIES
  		FRAMEWORK TRUE
  		FRAMEWORK_VERSION C
  		MACOSX_FRAMEWORK_IDENTIFIER com.cmake.dynamicFramework
  		#MACOSX_FRAMEWORK_INFO_PLIST Info.plist
  		PUBLIC_HEADER     "${HEADER_FILES}"
  		CODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer"
		OUTPUT_NAME "OSMScoutMapCairo")
endif()
