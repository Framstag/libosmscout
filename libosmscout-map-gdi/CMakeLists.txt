set(HEADER_FILES
    include/osmscout/MapGDIImportExport.h
    include/osmscout/MapPainterGDI.h
    include/osmscout/MapPainterGDIWindow.h
)

set(SOURCE_FILES
    src/osmscout/MapPainterGDI.cpp
    src/osmscout/MapPainterGDIWindow.cpp
)

osmscout_library_project(
	NAME OSMScoutMapGDI
	ALIAS MapGDI
	OUTPUT_NAME "osmscout_map_gdi"
	SOURCE ${SOURCE_FILES}
	HEADER ${HEADER_FILES}
	TARGET OSMScout::OSMScout OSMScout::Map Gdiplus
)
