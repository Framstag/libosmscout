set(HEADER_FILES
    include/osmscoutmapsvg/MapSVGImportExport.h
    include/osmscoutmapsvg/MapPainterSVG.h
    include/osmscoutmapsvg/SymbolRendererSVG.h
)

set(SOURCE_FILES
    src/osmscoutmapsvg/MapPainterSVG.cpp
    src/osmscoutmapsvg/SymbolRendererSVG.cpp
)

# The pango-less variant of the backend measures text with FreeType directly
if(NOT TARGET Freetype::Freetype)
    message(SEND_ERROR "freetype2 is required for the SVG map drawing backend")
endif()

# Font family names are resolved to font files with fontconfig where available;
# without it the configured font name has to be a font file
find_package(Fontconfig QUIET)
set(OSMSCOUT_MAP_SVG_HAVE_LIB_FONTCONFIG ${Fontconfig_FOUND})

osmscout_library_project(
	NAME OSMScoutMapSVG
	ALIAS MapSVG
	OUTPUT_NAME "osmscout_map_svg"
	SOURCE ${SOURCE_FILES}
	HEADER ${HEADER_FILES}
	INCLUDEDIR osmscoutmapsvg
	TEMPLATE ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscoutmapsvg/MapSVGFeatures.h.cmake
	TARGET OSMScout::OSMScout OSMScout::Map Freetype::Freetype
)

if(APPLE AND OSMSCOUT_BUILD_FRAMEWORKS)
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

  if(TARGET harfbuzz::harfbuzz)
      target_link_libraries(OSMScoutMapSVG harfbuzz::harfbuzz)
  endif()
endif()

if(TARGET Fontconfig::Fontconfig)
  target_link_libraries(OSMScoutMapSVG Fontconfig::Fontconfig)
endif()
