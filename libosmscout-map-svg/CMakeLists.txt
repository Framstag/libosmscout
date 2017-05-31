if(NOT ${OSMSCOUT_BUILD_MAP})
	message(SEND_ERROR "The main map drawing interface is required for SVG map drawing backend")
endif()

set(HEADER_FILES
    #include/osmscout/private/Config.h
    include/osmscout/private/MapSVGImportExport.h
    #include/osmscout/MapSVGFeatures.h
    include/osmscout/MapPainterSVG.h
)

set(SOURCE_FILES
    src/osmscout/MapPainterSVG.cpp
)

add_library(osmscout_map_svg ${SOURCE_FILES} ${HEADER_FILES})
set_property(TARGET osmscout_map_svg PROPERTY CXX_STANDARD 11)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout/MapSVGFeatures.h.cmake ${OSMSCOUT_BASE_DIR_BUILD}/include/osmscout/MapSVGFeatures.h)
create_private_config("${CMAKE_CURRENT_BINARY_DIR}/include/osmscout/private/Config.h" "osmscout-map-svg")
target_include_directories(osmscout_map_svg PRIVATE 
		include
		${CMAKE_CURRENT_BINARY_DIR}/include
		${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include
		${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-map/include)
target_link_libraries(osmscout_map_svg
		osmscout
		osmscout_map)
if(PANGOFT2_FOUND)
  target_include_directories(osmscout_map_svg PRIVATE ${PANGOFT2_INCLUDE_DIRS})
  target_link_libraries(osmscout_map_svg ${PANGOFT2_LIBRARIES})
endif()
target_compile_definitions(osmscout_map_svg PRIVATE -DOSMSCOUT_MAP_SVG_EXPORT_SYMBOLS)
install(TARGETS osmscout_map_svg
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout DESTINATION include FILES_MATCHING PATTERN "*.h" PATTERN "private" EXCLUDE)
install(FILES ${OSMSCOUT_BASE_DIR_BUILD}/include/osmscout/MapSVGFeatures.h DESTINATION include/osmscout)
