set(OSMSCOUT_BUILD_MAP ON CACHE INTERNAL "" FORCE)

set(HEADER_FILES
	include/osmscout/oss/Parser.h
	include/osmscout/oss/Scanner.h
	#include/osmscout/private/Config.h
	#include/osmscout/private/MapImportExport.h
	include/osmscout/MapFeatures.h
	include/osmscout/LabelLayouter.h
	include/osmscout/MapPainter.h
	include/osmscout/MapParameter.h
	include/osmscout/MapService.h
	include/osmscout/LabelProvider.h
	include/osmscout/StyleDescription.h
	include/osmscout/StyleConfig.h
	include/osmscout/TileId.h
	include/osmscout/DataTileCache.h
	include/osmscout/MapTileCache.h
	include/osmscout/MapPainterNoOp.h
)

set(SOURCE_FILES
	src/osmscout/oss/Parser.cpp
	src/osmscout/oss/Scanner.cpp
	src/osmscout/LabelLayouter.cpp
	src/osmscout/MapPainter.cpp
	src/osmscout/MapParameter.cpp
	src/osmscout/MapService.cpp
	src/osmscout/LabelProvider.cpp
	src/osmscout/StyleDescription.cpp
	src/osmscout/StyleConfig.cpp
	src/osmscout/TileId.cpp
	src/osmscout/DataTileCache.cpp
	src/osmscout/MapTileCache.cpp
	src/osmscout/MapPainterNoOp.cpp
)

add_library(osmscout_map ${SOURCE_FILES} ${HEADER_FILES})
set_property(TARGET osmscout_map PROPERTY CXX_STANDARD 11)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout/MapFeatures.h.cmake ${OSMSCOUT_BASE_DIR_BUILD}/include/osmscout/MapFeatures.h)
create_private_config("${CMAKE_CURRENT_BINARY_DIR}/include/osmscout/private/Config.h" "osmscout-map")
target_include_directories(osmscout_map PRIVATE
		include
		${CMAKE_CURRENT_BINARY_DIR}/include
		${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include)
target_link_libraries(osmscout_map osmscout)
target_compile_definitions(osmscout_map PRIVATE -DOSMSCOUT_MAP_EXPORT_SYMBOLS)
install(TARGETS osmscout_map
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout DESTINATION include FILES_MATCHING PATTERN "*.h" PATTERN "private/Config.h" EXCLUDE)
install(FILES ${OSMSCOUT_BASE_DIR_BUILD}/include/osmscout/MapFeatures.h DESTINATION include/osmscout)
