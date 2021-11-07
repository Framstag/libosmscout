set(HEADER_FILES
    include/osmscoutmapsvg/MapSVGImportExport.h
    include/osmscoutmapsvg/MapPainterSVG.h
)

set(SOURCE_FILES
    src/osmscoutmapsvg/MapPainterSVG.cpp
)

osmscout_library_project(
	NAME OSMScoutMapSVG
	ALIAS MapSVG
	OUTPUT_NAME "osmscout_map_svg"
	SOURCE ${SOURCE_FILES}
	HEADER ${HEADER_FILES}
	INCLUDEDIR osmscoutmapsvg
	TEMPLATE ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscoutmapsvg/MapSVGFeatures.h.cmake
	TARGET OSMScout::OSMScout OSMScout::Map
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

  if(HARFBUZZ_FOUND)
      target_include_directories(OSMScoutMapSVG PUBLIC ${HARFBUZZ_INCLUDE_DIRS})
      target_link_libraries(OSMScoutMapSVG ${HARFBUZZ_LIBRARIES})
  endif()
endif()
