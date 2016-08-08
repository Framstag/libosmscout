if(NOT ${OSMSCOUT_BUILD_MAP_QT})
	message(SEND_ERROR "The Qt map drawing backend is required for osmscout_client_qt application")
endif()
if(NOT Qt5Core_FOUND)
	message(SEND_ERROR "Qt5Core is required for osmscout_client_qt application")
endif()
if(NOT Qt5Gui_FOUND)
	message(SEND_ERROR "Qt5Gui is required for osmscout_client_qt application")
endif()
if(NOT Qt5Widgets_FOUND)
	message(SEND_ERROR "Qt5Widgets is required for osmscout_client_qt application")
endif()
if(NOT Qt5Qml_FOUND)
	message(SEND_ERROR "Qt5Qml is required for osmscout_client_qt application")
endif()
if(NOT Qt5Quick_FOUND)
	message(SEND_ERROR "Qt5Quick is required for osmscout_client_qt application")
endif()
if(NOT Qt5Svg_FOUND)
	message(SEND_ERROR "Qt5Svg is required for osmscout_client_qt application")
endif()
if(NOT Qt5Positioning_FOUND)
	message(SEND_ERROR "Qt5Positioning is required for osmscout_client_qt application")
endif()
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(HEADER_FILES
    include/osmscout/DBThread.h
    include/osmscout/MapWidget.h
    include/osmscout/RoutingModel.h
    include/osmscout/SearchLocationModel.h
    include/osmscout/Settings.h
)

set(SOURCE_FILES
    src/DBThread.cpp
    src/MapWidget.cpp
    src/RoutingModel.cpp
    src/SearchLocationModel.cpp
    src/Settings.cpp
)

add_library(osmscout_client_qt ${SOURCE_FILES} ${HEADER_FILES})
set_property(TARGET osmscout_client_qt PROPERTY CXX_STANDARD 11)

target_include_directories(osmscout_client_qt PRIVATE 
		${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include
		${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-map/include
		${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-map-qt/include
    ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-client-qt/include)

target_link_libraries(osmscout_client_qt
        osmscout
        osmscout_map
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Qml
        Qt5::Quick
        Qt5::Svg
        Qt5::Positioning
        osmscout_map_qt)

target_compile_definitions(osmscout_client_qt PRIVATE -DOSMSCOUT_CLIENT_QT_EXPORT_SYMBOLS ${Qt5Gui_DEFINITIONS})
install(TARGETS osmscout_client_qt
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout DESTINATION include FILES_MATCHING PATTERN "*.h" PATTERN "private" EXCLUDE)


