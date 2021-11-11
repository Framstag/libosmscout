set(HEADER_FILES
	include/osmscoutmapdirectx/MapDirectXImportExport.h
    include/osmscoutmapdirectx/MapPainterDirectX.h
    include/osmscoutmapdirectx/PathTextRenderer.h
)

set(SOURCE_FILES
    src/osmscoutmapdirectx/MapPainterDirectX.cpp
    src/osmscoutmapdirectx/PathTextRenderer.cpp
)

osmscout_library_project(
	NAME OSMScoutMapDirectX
	ALIAS MapDirectX
	OUTPUT_NAME "osmscout_map_directx"
	SOURCE ${SOURCE_FILES}
	HEADER ${HEADER_FILES}
	INCLUDEDIR osmscoutmapdirectx
	INCLUDES ${Direct2D_INCLUDE_DIRS}
	TEMPLATE ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscoutmapdirectx/MapDirectXFeatures.h.cmake
	TARGET OSMScout::OSMScout OSMScout::Map ${Direct2D_LIBRARIES} Windowscodecs
)
target_compile_definitions(OSMScoutMapDirectX PRIVATE -DUNICODE -D_UNICODE)
