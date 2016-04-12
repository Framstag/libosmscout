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
    #include/osmscout/private/Config.h
    include/osmscout/private/MapCairoImportExport.h
    include/osmscout/LoaderPNG.h
    #include/osmscout/MapCairoFeatures.h
    include/osmscout/MapPainterCairo.h
)

set(SOURCE_FILES
    src/osmscout/LoaderPNG.cpp
    src/osmscout/MapPainterCairo.cpp
)

add_library(libosmscout_map_cairo SHARED ${SOURCE_FILES} ${HEADER_FILES})
set_property(TARGET libosmscout_map_cairo PROPERTY CXX_STANDARD 11)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout/MapCairoFeatures.h.cmake ${OSMSCOUT_BASE_DIR_BUILD}/include/osmscout/MapCairoFeatures.h)
create_private_config("${CMAKE_CURRENT_BINARY_DIR}/include/osmscout/private/Config.h" "libosmscout-map-cairo")
target_include_directories(libosmscout_map_cairo PRIVATE 
		include
		${CMAKE_CURRENT_BINARY_DIR}/include
		${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include
		${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-map/include
		${CAIRO_INCLUDE_DIRS}
		${PNG_INCLUDE_DIRS})
target_link_libraries(libosmscout_map_cairo
		libosmscout
		libosmscout_map
		${CAIRO_LIBRARIES}
		${PNG_LIBRARIES})
if(PANGOCAIRO_FOUND)
  target_include_directories(libosmscout_map_cairo PRIVATE ${PANGO_INCLUDE_DIRS})
  target_link_libraries(libosmscout_map_cairo ${GLIB2_LIBRARIES} ${PANGO_LIBRARIES})
endif()
target_compile_definitions(libosmscout_map_cairo PRIVATE -DOSMSCOUT_MAP_CAIRO_EXPORT_SYMBOLS ${PNG_DEFINITIONS})
install(TARGETS libosmscout_map_cairo
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout DESTINATION include FILES_MATCHING PATTERN "*.h" PATTERN "private" EXCLUDE)
install(FILES ${OSMSCOUT_BASE_DIR_BUILD}/include/osmscout/MapCairoFeatures.h DESTINATION include/osmscout)
