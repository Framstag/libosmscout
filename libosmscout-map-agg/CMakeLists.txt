set(OSMSCOUT_BUILD_MAP_AGG ON CACHE INTERNAL "" FORCE)

set(HEADER_FILES
    #include/osmscout/private/Config.h
    include/osmscout/private/MapAggImportExport.h
    include/osmscout/MapAggFeatures.h
    include/osmscout/MapPainterAgg.h
)

set(SOURCE_FILES
    src/osmscout/MapPainterAgg.cpp
)

add_library(libosmscout_map_agg SHARED ${SOURCE_FILES} ${HEADER_FILES})
set_property(TARGET libosmscout_map_agg PROPERTY CXX_STANDARD 11)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout/MapAggFeatures.h.cmake ${OSMSCOUT_BASE_DIR_BUILD}/include/osmscout/MapAggFeatures.h)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout/private/Config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/include/osmscout/private/Config.h)
target_include_directories(libosmscout_map_agg PRIVATE 
		include
		${CMAKE_CURRENT_BINARY_DIR}/include
		${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include
		${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-map/include
		${LIBAGG_INCLUDE_DIRS}
		${FREETYPE_INCLUDE_DIRS})
target_link_libraries(libosmscout_map_agg
		libosmscout
		libosmscout_map
		${LIBAGG_LIBRARIES}
		${FREETYPE_LIBRARIES})
target_compile_definitions(libosmscout_map_agg PRIVATE -DOSMSCOUT_MAP_AGG_EXPORT_SYMBOLS)
install(TARGETS libosmscout_map_agg
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout DESTINATION include FILES_MATCHING PATTERN "*.h" PATTERN "private" EXCLUDE)
install(FILES ${OSMSCOUT_BASE_DIR_BUILD}/include/osmscout/MapAggFeatures.h DESTINATION include/osmscout)
