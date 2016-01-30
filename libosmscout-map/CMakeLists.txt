cmake_minimum_required(VERSION 3.2)
project(libosmscout_map)

set(HEADER_FILES
	include/osmscout/oss/Parser.h
	include/osmscout/oss/Scanner.h
	#include/osmscout/private/Config.h
	#include/osmscout/private/MapImportExport.h
	include/osmscout/MapFeatures.h
	include/osmscout/MapPainter.h
	include/osmscout/MapParameter.h
	include/osmscout/MapService.h
	include/osmscout/StyleConfig.h
	include/osmscout/TiledDataCache.h
	include/osmscout/MapPainterNoOp.h
)

set(SOURCE_FILES
	src/osmscout/oss/Parser.cpp
	src/osmscout/oss/Scanner.cpp
	src/osmscout/MapPainter.cpp
	src/osmscout/MapParameter.cpp
	src/osmscout/MapService.cpp
	src/osmscout/StyleConfig.cpp
	src/osmscout/TiledDataCache.cpp
	src/osmscout/MapPainterNoOp.cpp
)

add_library(libosmscout_map SHARED ${SOURCE_FILES} ${HEADER_FILES})
set_property(TARGET libosmscout_map PROPERTY CXX_STANDARD 11)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout/MapFeatures.h.cmake ${OSMSCOUT_BASE_DIR_BUILD}/include/osmscout/MapFeatures.h)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout/private/Config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/include/osmscout/private/Config.h)
target_include_directories(libosmscout_map PRIVATE 
		include
		${CMAKE_CURRENT_BINARY_DIR}/include
		${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include)
target_link_libraries(libosmscout_map libosmscout)
target_compile_definitions(libosmscout_map PRIVATE -DOSMSCOUT_MAP_EXPORT_SYMBOLS)
install(TARGETS libosmscout_map
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout DESTINATION include FILES_MATCHING PATTERN "*.h" PATTERN "private" EXCLUDE)
install(FILES ${OSMSCOUT_BASE_DIR_BUILD}/include/osmscout/MapFeatures.h DESTINATION include/osmscout)
