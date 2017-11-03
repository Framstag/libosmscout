if(NOT ${OSMSCOUT_BUILD_MAP})
	message(SEND_ERROR "The main map drawing interface is required for DirectX map drawing backend")
endif()

set(HEADER_FILES
    include/osmscout/private/MapDirectXPaint.h
    include/osmscout/MapPainterDirectX.h
)

set(SOURCE_FILES
    src/osmscout/MapPainterDirectX.cpp
)

add_library(osmscout_map_directx SHARED ${SOURCE_FILES} ${HEADER_FILES})
set_property(TARGET osmscout_map_directx PROPERTY CXX_STANDARD 11)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout/MapDirectXFeatures.h.cmake ${OSMSCOUT_BASE_DIR_BUILD}/include/osmscout/MapDirectXFeatures.h)
create_private_config("${CMAKE_CURRENT_BINARY_DIR}/include/osmscout/private/Config.h" "osmscout-map-directx")
target_include_directories(osmscout_map_directx PRIVATE 
		include
		${CMAKE_CURRENT_BINARY_DIR}/include
		${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include
		${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-map/include
		${Direct2D_INCLUDE_DIRS})
target_link_libraries(osmscout_map_directx
		osmscout
		osmscout_map
		${Direct2D_LIBRARIES}
		Windowscodecs)
target_compile_definitions(osmscout_map_directx PRIVATE -DOSMSCOUT_MAP_DIRECTX_EXPORT_SYMBOLS -DUNICODE -D_UNICODE)
install(TARGETS osmscout_map_directx
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout DESTINATION include FILES_MATCHING PATTERN "*.h" PATTERN "private" EXCLUDE)
install(FILES ${OSMSCOUT_BASE_DIR_BUILD}/include/osmscout/MapDirectXFeatures.h DESTINATION include/osmscout)
