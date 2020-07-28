set(HEADER_FILES
	include/osmscout/MapDirectXImportExport.h
    include/osmscout/MapPainterDirectX.h
    include/osmscout/PathTextRenderer.h
)

set(SOURCE_FILES
    src/osmscout/MapPainterDirectX.cpp
    src/osmscout/PathTextRenderer.cpp
)

add_library(OSMScoutMapDirectX ${SOURCE_FILES} ${HEADER_FILES})
add_library(OSMScout::MapDirectX ALIAS OSMScoutMapDirectX)
set_target_properties(OSMScoutMapDirectX PROPERTIES
	OUTPUT_NAME "osmscout_map_directx"
	VERSION ${OSMSCOUT_LIBRARY_VERSION}
	SOVERSION ${OSMSCOUT_LIBRARY_VERSION}
	FOLDER "libosmscout"
)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout/MapDirectXFeatures.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/include/osmscout/MapDirectXFeatures.h)
create_private_config("${CMAKE_CURRENT_BINARY_DIR}/privateinclude/osmscout/private/Config.h" "OSMScoutMapDirectX")
target_include_directories(OSMScoutMapDirectX PRIVATE
	include
	${CMAKE_SOURCE_DIR}/include
	${CMAKE_CURRENT_BINARY_DIR}/include
	${CMAKE_CURRENT_BINARY_DIR}/privateinclude
	${CMAKE_CURRENT_SOURCE_DIR}/include
	${Direct2D_INCLUDE_DIRS}
)
target_include_directories(OSMScoutMapDirectX INTERFACE ${CMAKE_SOURCE_DIR}/libosmscout-map-directx/include ${CMAKE_BINARY_DIR}/libosmscout-map-directx/include)
target_link_libraries(OSMScoutMapDirectX OSMScout::OSMScout OSMScout::Map ${Direct2D_LIBRARIES} Windowscodecs)
target_compile_definitions(OSMScoutMapDirectX PRIVATE -DUNICODE -D_UNICODE)

if(BUILD_SHARED_LIBS)
	create_win32_dll_resource(OSMScoutMapDirectX "osmscout_map_directx")
endif()

install(TARGETS OSMScoutMapDirectX
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout DESTINATION include FILES_MATCHING PATTERN "*.h" PATTERN "private/Config.h" EXCLUDE)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/osmscout/MapDirectXFeatures.h DESTINATION include/osmscout)
