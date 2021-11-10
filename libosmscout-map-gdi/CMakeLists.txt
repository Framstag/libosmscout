set(HEADER_FILES
    include/osmscoutmapgdi/MapGDIImportExport.h
    include/osmscoutmapgdi/MapPainterGDI.h
    include/osmscoutmapgdi/MapPainterGDIWindow.h
)

set(SOURCE_FILES
    src/osmscoutmapgdi/MapPainterGDI.cpp
    src/osmscoutmapgdi/MapPainterGDIWindow.cpp
)

osmscout_library_project(
	NAME OSMScoutMapGDI
	ALIAS MapGDI
	OUTPUT_NAME "osmscout_map_gdi"
	SOURCE ${SOURCE_FILES}
	HEADER ${HEADER_FILES}
	INCLUDEDIR osmscoutmapgdi
	TARGET OSMScout::OSMScout OSMScout::Map Gdiplus
)
