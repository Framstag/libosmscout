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

if(APPLE AND BUILD_FRAMEWORKS)
  set(THE_TARGET_NAME "OSMScoutMapSVG")
else()
  set(THE_TARGET_NAME "osmscout_map_svg")
endif()

if(IOS)
  add_library(${THE_TARGET_NAME} STATIC ${SOURCE_FILES} ${HEADER_FILES})
else()
  add_library(${THE_TARGET_NAME} ${SOURCE_FILES} ${HEADER_FILES})
endif()

set_property(TARGET ${THE_TARGET_NAME} PROPERTY CXX_STANDARD 11)

target_include_directories(${THE_TARGET_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Public features Header
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout/MapSVGFeatures.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/include/osmscout/MapSVGFeatures.h)
target_include_directories(${THE_TARGET_NAME} PUBLIC include ${CMAKE_CURRENT_BINARY_DIR}/include)

# Private config header
create_private_config("${CMAKE_CURRENT_BINARY_DIR}/privateinclude/osmscout/private/Config.h" "${THE_TARGET_NAME}")
target_include_directories(${THE_TARGET_NAME} PRIVATE include ${CMAKE_CURRENT_BINARY_DIR}/privateinclude)

if(APPLE AND BUILD_FRAMEWORKS)
target_link_libraries(${THE_TARGET_NAME}
		OSMScout
		OSMScoutMap)
else()
target_link_libraries(${THE_TARGET_NAME}
		osmscout
		osmscout_map)
endif()

if(PANGOFT2_FOUND)
  target_include_directories(${THE_TARGET_NAME} PUBLIC ${PANGOFT2_INCLUDE_DIRS})
  target_link_libraries(${THE_TARGET_NAME} ${PANGOFT2_LIBRARIES})
endif()

target_compile_definitions(${THE_TARGET_NAME} PRIVATE -DOSMSCOUT_MAP_SVG_EXPORT_SYMBOLS)

install(TARGETS ${THE_TARGET_NAME}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        FRAMEWORK DESTINATION lib)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout DESTINATION include FILES_MATCHING PATTERN "*.h" PATTERN "private" EXCLUDE)
install(FILES ${OSMSCOUT_BASE_DIR_BUILD}/include/osmscout/MapSVGFeatures.h DESTINATION include/osmscout)
