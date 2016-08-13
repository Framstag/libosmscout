if(NOT Qt5Core_FOUND)
	message(SEND_ERROR "Qt5Core is required for OSMScout2 application")
endif()
if(NOT Qt5Gui_FOUND)
	message(SEND_ERROR "Qt5Gui is required for OSMScout2 application")
endif()
if(NOT Qt5Widgets_FOUND)
	message(SEND_ERROR "Qt5Widgets is required for OSMScout2 application")
endif()
if(NOT Qt5Quick_FOUND)
	message(SEND_ERROR "Qt5Quick is required for OSMScout2 application")
endif()
if(NOT ${OSMSCOUT_BUILD_MAP_QT})
	message(SEND_ERROR "The main map drawing interface is required for Qt map drawing backend")
endif()

set(HEADER_FILES
    #include/osmscout/private/Config.h
    include/osmscout/private/ClientQtImportExport.h
    #include/osmscout/ClientQtFeatures.h
    include/osmscout/DBThread.h
    include/osmscout/MapWidget.h
    include/osmscout/RoutingModel.h
    include/osmscout/SearchLocationModel.h
    include/osmscout/Settings.h
)

set(SOURCE_FILES
    src/osmscout/DBThread.cpp
    src/osmscout/MapWidget.cpp
    src/osmscout/RoutingModel.cpp
    src/osmscout/SearchLocationModel.cpp
    src/osmscout/Settings.cpp
)

add_library(osmscout_client_qt SHARED ${SOURCE_FILES} ${HEADER_FILES})
set_property(TARGET osmscout_client_qt PROPERTY CXX_STANDARD 11)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout/ClientQtFeatures.h.cmake ${OSMSCOUT_BASE_DIR_BUILD}/include/osmscout/ClientQtFeatures.h)
create_private_config("${CMAKE_CURRENT_BINARY_DIR}/include/osmscout/private/Config.h" "osmscout-client-qt")
target_include_directories(osmscout_client_qt PRIVATE 
		include
		${CMAKE_CURRENT_BINARY_DIR}/include
		${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include
		${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-map/include
		${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-map-qt/include
		${Qt5Gui_INCLUDE_DIRS})
target_link_libraries(osmscout_client_qt
		osmscout
		osmscout_map
		osmscout_map_qt
		Qt5::Core
		Qt5::Gui
		Qt5::Widgets
		Qt5::Quick)
target_compile_definitions(osmscout_client_qt PRIVATE -DOSMSCOUT_CLIENT_QT_EXPORT_SYMBOLS ${Qt5Gui_DEFINITIONS})
install(TARGETS osmscout_client_qt
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout DESTINATION include FILES_MATCHING PATTERN "*.h" PATTERN "private" EXCLUDE)
install(FILES ${OSMSCOUT_BASE_DIR_BUILD}/include/osmscout/ClientQtFeatures.h DESTINATION include/osmscout)
