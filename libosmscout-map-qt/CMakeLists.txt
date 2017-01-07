if(NOT Qt5Gui_FOUND)
	message(SEND_ERROR "Qt5 is required for Qt map drawing backend")
endif()
if(NOT ${OSMSCOUT_BUILD_MAP})
	message(SEND_ERROR "The main map drawing interface is required for Qt map drawing backend")
endif()

set(HEADER_FILES
    #include/osmscout/private/Config.h
    include/osmscout/private/MapQtImportExport.h
    include/osmscout/SimplifiedPath.h
    include/osmscout/MapPainterQt.h
)

set(SOURCE_FILES
    src/osmscout/MapPainterQt.cpp
    src/osmscout/SimplifiedPath.cpp
)

add_library(osmscout_map_qt ${SOURCE_FILES} ${HEADER_FILES})
set_property(TARGET osmscout_map_qt PROPERTY CXX_STANDARD 11)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout/MapQtFeatures.h.cmake ${OSMSCOUT_BASE_DIR_BUILD}/include/osmscout/MapQtFeatures.h)
create_private_config("${CMAKE_CURRENT_BINARY_DIR}/include/osmscout/private/Config.h" "osmscout-map-qt")
target_include_directories(osmscout_map_qt PRIVATE 
		include
		${CMAKE_CURRENT_BINARY_DIR}/include
		${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include
		${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-map/include
		${Qt5Gui_INCLUDE_DIRS})
target_link_libraries(osmscout_map_qt
		osmscout
		osmscout_map
		${Qt5Gui_LIBRARIES})
target_compile_definitions(osmscout_map_qt PRIVATE -DOSMSCOUT_MAP_QT_EXPORT_SYMBOLS ${Qt5Gui_DEFINITIONS})
install(TARGETS osmscout_map_qt
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout DESTINATION include FILES_MATCHING PATTERN "*.h" PATTERN "private/Config.h" EXCLUDE)
install(FILES ${OSMSCOUT_BASE_DIR_BUILD}/include/osmscout/MapQtFeatures.h DESTINATION include/osmscout)
