set(HEADER_FILES
    include/osmscout/MapSVGImportExport.h
    include/osmscout/MapPainterSVG.h
)

set(SOURCE_FILES
    src/osmscout/MapPainterSVG.cpp
)

osmscout_library_project(
	NAME OSMScoutMapSVG
	ALIAS MapSVG
	OUTPUT_NAME "osmscout_map_svg"
	SOURCE ${SOURCE_FILES}
	HEADER ${HEADER_FILES}
	TEMPLATE ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout/MapSVGFeatures.h.cmake
	TARGET OSMScout::OSMScout OSMScout::Map
)

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
