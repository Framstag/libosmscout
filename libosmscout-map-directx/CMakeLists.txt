if(NOT ${OSMSCOUT_BUILD_MAP})
	message(SEND_ERROR "The main map drawing interface is required for DirectX map drawing backend")
endif()

set(HEADER_FILES
	include/osmscout/MapDirectXImportExport.h
    include/osmscout/MapPainterDirectX.h
    include/osmscout/PathTextRenderer.h
)

set(SOURCE_FILES
    src/osmscout/MapPainterDirectX.cpp
    src/osmscout/PathTextRenderer.cpp
)

add_library(OSMScoutMapDirectX SHARED ${SOURCE_FILES} ${HEADER_FILES})

set_target_properties(OSMScoutMapDirectX PROPERTIES
		CXX_STANDARD 17
		OUTPUT_NAME "osmscout_map_directx")

target_include_directories(OSMScoutMapDirectX PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/include
		${Direct2D_INCLUDE_DIRS})

# Public features Header
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout/MapDirectXFeatures.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/include/osmscout/MapDirectXFeatures.h)
target_include_directories(OSMScoutMapDirectX PUBLIC include ${CMAKE_CURRENT_BINARY_DIR}/include)

# Private config header
create_private_config("${CMAKE_CURRENT_BINARY_DIR}/privateinclude/osmscout/private/Config.h" "OSMScoutMapDirectX")
target_include_directories(OSMScoutMapDirectX PRIVATE include ${CMAKE_CURRENT_BINARY_DIR}/privateinclude)

target_link_libraries(OSMScoutMapDirectX
		OSMScout
		OSMScoutMap
		${Direct2D_LIBRARIES}
		Windowscodecs)

target_compile_definitions(OSMScoutMapDirectX PRIVATE -DOSMSCOUT_MAP_DIRECTX_EXPORT_SYMBOLS -DUNICODE -D_UNICODE)

install(TARGETS OSMScoutMapDirectX
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout DESTINATION include FILES_MATCHING PATTERN "*.h" PATTERN "private/Config.h" EXCLUDE)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/osmscout/MapDirectXFeatures.h DESTINATION include/osmscout)
