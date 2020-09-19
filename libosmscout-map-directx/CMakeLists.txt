set(HEADER_FILES
	include/osmscout/MapDirectXImportExport.h
    include/osmscout/MapPainterDirectX.h
    include/osmscout/PathTextRenderer.h
)

set(SOURCE_FILES
    src/osmscout/MapPainterDirectX.cpp
    src/osmscout/PathTextRenderer.cpp
)

osmscout_library_project(
	NAME OSMScoutMapDirectX
	ALIAS MapDirectX
	OUTPUT_NAME "osmscout_map_directx"
	SOURCE ${SOURCE_FILES}
	HEADER ${HEADER_FILES}
	INCLUDES ${Direct2D_INCLUDE_DIRS}
	TEMPLATE ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout/MapDirectXFeatures.h.cmake
	TARGET OSMScout::OSMScout OSMScout::Map ${Direct2D_LIBRARIES} Windowscodecs
)
target_compile_definitions(OSMScoutMapDirectX PRIVATE -DUNICODE -D_UNICODE)
