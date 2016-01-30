cmake_minimum_required(VERSION 3.2)
project(libosmscout_map_qt)

set(HEADER_FILES
    #include/osmscout/private/Config.h
    include/osmscout/private/MapQtImportExport.h
    #include/osmscout/MapQtFeatures.h
    include/osmscout/MapPainterQt.h
)

set(SOURCE_FILES
    src/osmscout/MapPainterQt.cpp
)

add_library(libosmscout_map_qt SHARED ${SOURCE_FILES} ${HEADER_FILES})
set_property(TARGET libosmscout_map_qt PROPERTY CXX_STANDARD 11)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout/MapQtFeatures.h.cmake ${OSMSCOUT_BASE_DIR_BUILD}/include/osmscout/MapQtFeatures.h)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout/private/Config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/include/osmscout/private/Config.h)
target_include_directories(libosmscout_map_qt PRIVATE 
		include
		${CMAKE_CURRENT_BINARY_DIR}/include
		${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include
		${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-map/include
		${Qt5Gui_INCLUDE_DIRS})
target_link_libraries(libosmscout_map_qt
		libosmscout
		libosmscout_map
		${Qt5Gui_LIBRARIES})
target_compile_definitions(libosmscout_map_qt PRIVATE -DOSMSCOUT_MAP_QT_EXPORT_SYMBOLS ${Qt5Gui_DEFINITIONS})
install(TARGETS libosmscout_map_qt
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout DESTINATION include FILES_MATCHING PATTERN "*.h" PATTERN "private" EXCLUDE)
install(FILES ${OSMSCOUT_BASE_DIR_BUILD}/include/osmscout/MapQtFeatures.h DESTINATION include/osmscout)
