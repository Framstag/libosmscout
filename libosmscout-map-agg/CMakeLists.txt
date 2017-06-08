if(NOT LIBAGG_FOUND)
	message(SEND_ERROR "libagg is required for agg map drawing backend")
endif()
if(NOT LIBAGGFT2_LIBRARIES)
	message(SEND_ERROR "libagg is required for agg map drawing backend")
endif()
if(NOT FREETYPE_FOUND)
	message(SEND_ERROR "freetype2 is required for agg map drawing backend")
endif()
if(NOT ${OSMSCOUT_BUILD_MAP})
	message(SEND_ERROR "The main map drawing interface is required for agg map drawing backend")
endif()

set(HEADER_FILES
    #include/osmscout/private/Config.h
    include/osmscout/private/MapAggImportExport.h
    include/osmscout/MapAggFeatures.h
    include/osmscout/MapPainterAgg.h
)

set(SOURCE_FILES
    src/osmscout/MapPainterAgg.cpp
)

add_library(osmscout_map_agg ${SOURCE_FILES} ${HEADER_FILES})
set_property(TARGET osmscout_map_agg PROPERTY CXX_STANDARD 11)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout/MapAggFeatures.h.cmake ${OSMSCOUT_BASE_DIR_BUILD}/include/osmscout/MapAggFeatures.h)
create_private_config("${CMAKE_CURRENT_BINARY_DIR}/include/osmscout/private/Config.h" "libosmscout-map-agg")
target_include_directories(osmscout_map_agg PRIVATE
		include
		${CMAKE_CURRENT_BINARY_DIR}/include
		${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include
		${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-map/include
		${LIBAGG_INCLUDE_DIRS}
		${FREETYPE_INCLUDE_DIRS})
target_link_libraries(osmscout_map_agg
		osmscout
		osmscout_map
		${LIBAGG_LIBRARIES}
		${LIBAGGFT2_LIBRARIES}
		${FREETYPE_LIBRARIES})
target_compile_definitions(osmscout_map_agg PRIVATE -DOSMSCOUT_MAP_AGG_EXPORT_SYMBOLS)
install(TARGETS osmscout_map_agg
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout DESTINATION include FILES_MATCHING PATTERN "*.h" PATTERN "private" EXCLUDE)
install(FILES ${OSMSCOUT_BASE_DIR_BUILD}/include/osmscout/MapAggFeatures.h DESTINATION include/osmscout)
