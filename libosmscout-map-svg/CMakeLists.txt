cmake_minimum_required(VERSION 3.2)
project(libosmscout_map_svg)

set(HEADER_FILES
    #include/osmscout/private/Config.h
    include/osmscout/private/MapSVGImportExport.h
    #include/osmscout/MapSVGFeatures.h
    include/osmscout/MapPainterSVG.h
)

set(SOURCE_FILES
    src/osmscout/MapPainterSVG.cpp
)

add_library(libosmscout_map_svg SHARED ${SOURCE_FILES} ${HEADER_FILES})
set_property(TARGET libosmscout_map_svg PROPERTY CXX_STANDARD 11)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout/MapSVGFeatures.h.cmake ${OSMSCOUT_BASE_DIR_BUILD}/include/osmscout/MapSVGFeatures.h)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout/private/Config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/include/osmscout/private/Config.h)
target_include_directories(libosmscout_map_svg PRIVATE 
		include
		${CMAKE_CURRENT_BINARY_DIR}/include
		${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include
		${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-map/include)
target_link_libraries(libosmscout_map_svg
		libosmscout
		libosmscout_map)
if(PANGO_FOUND)
  target_include_directories(libosmscout_map_svg PRIVATE ${PANGO_INCLUDE_DIRS})
  target_link_libraries(libosmscout_map_svg ${PANGO_LIBRARIES})
endif()
target_compile_definitions(libosmscout_map_svg PRIVATE -DOSMSCOUT_MAP_SVG_EXPORT_SYMBOLS)
install(TARGETS libosmscout_map_svg
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout DESTINATION include FILES_MATCHING PATTERN "*.h" PATTERN "private" EXCLUDE)
install(FILES ${OSMSCOUT_BASE_DIR_BUILD}/include/osmscout/MapSVGFeatures.h DESTINATION include/osmscout)
