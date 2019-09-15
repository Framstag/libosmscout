if(NOT ${OSMSCOUT_BUILD_MAP})
	message(SEND_ERROR "The main map drawing interface is required for SVG map drawing backend")
endif()

set(HEADER_FILES
    include/osmscout/MapSVGImportExport.h
    include/osmscout/MapPainterSVG.h
)

set(SOURCE_FILES
    src/osmscout/MapPainterSVG.cpp
)

if(IOS)
  add_library(OSMScoutMapSVG STATIC ${SOURCE_FILES} ${HEADER_FILES})
else()
  add_library(OSMScoutMapSVG ${SOURCE_FILES} ${HEADER_FILES})
endif()

set_target_properties(OSMScoutMapSVG PROPERTIES
        CXX_STANDARD 14
        OUTPUT_NAME "osmscout_map_svg")

target_include_directories(OSMScoutMapSVG PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Public features Header
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout/MapSVGFeatures.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/include/osmscout/MapSVGFeatures.h)
target_include_directories(OSMScoutMapSVG PUBLIC include ${CMAKE_CURRENT_BINARY_DIR}/include)

# Private config header
create_private_config("${CMAKE_CURRENT_BINARY_DIR}/privateinclude/osmscout/private/Config.h" "OSMScoutMapSVG")
target_include_directories(OSMScoutMapSVG PRIVATE include ${CMAKE_CURRENT_BINARY_DIR}/privateinclude)

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

target_link_libraries(OSMScoutMapSVG
		OSMScout
		OSMScoutMap)

if(PANGOFT2_FOUND)
  target_include_directories(OSMScoutMapSVG PUBLIC ${PANGOFT2_INCLUDE_DIRS})
  target_link_libraries(OSMScoutMapSVG ${PANGOFT2_LIBRARIES})

  if(HARFBUZZ_FOUND)
      target_include_directories(OSMScoutMapSVG PUBLIC ${HARFBUZZ_INCLUDE_DIRS})
      target_link_libraries(OSMScoutMapSVG ${HARFBUZZ_LIBRARIES})
  endif()
endif()

target_compile_definitions(OSMScoutMapSVG PRIVATE -DOSMSCOUT_MAP_SVG_EXPORT_SYMBOLS)

install(TARGETS OSMScoutMapSVG
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        FRAMEWORK DESTINATION lib)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout DESTINATION include FILES_MATCHING PATTERN "*.h" PATTERN "private" EXCLUDE)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/osmscout/MapSVGFeatures.h DESTINATION include/osmscout)
