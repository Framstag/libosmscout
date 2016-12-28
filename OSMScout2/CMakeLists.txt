if(NOT ${OSMSCOUT_BUILD_MAP_QT})
	message(SEND_ERROR "The Qt map drawing backend is required for OSMScout2 application")
endif()
if(NOT ${OSMSCOUT_BUILD_CLIENT_QT})
	message(SEND_ERROR "The Qt client lib is required for OSMScout2 application")
endif()
if(NOT Qt5Core_FOUND)
	message(SEND_ERROR "Qt5Core is required for OSMScout2 application")
endif()
if(NOT Qt5Gui_FOUND)
	message(SEND_ERROR "Qt5Gui is required for OSMScout2 application")
endif()
if(NOT Qt5Widgets_FOUND)
	message(SEND_ERROR "Qt5Widgets is required for OSMScout2 application")
endif()
if(NOT Qt5Qml_FOUND)
	message(SEND_ERROR "Qt5Qml is required for OSMScout2 application")
endif()
if(NOT Qt5Quick_FOUND)
	message(SEND_ERROR "Qt5Quick is required for OSMScout2 application")
endif()
if(NOT Qt5Svg_FOUND)
	message(SEND_ERROR "Qt5Svg is required for OSMScout2 application")
endif()
if(NOT Qt5Positioning_FOUND)
	message(SEND_ERROR "Qt5Positioning is required for OSMScout2 application")
endif()
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

string(TIMESTAMP OSMSCOUT_VERSION_STRING "%Y-%m-%d")
set(OSMSCOUT_USER_AGENT "OSMScout demo app %1")

set(HEADER_FILES
    src/Theme.h
)

set(SOURCE_FILES
    src/Theme.cpp
    src/OSMScout.cpp

    # qml files in CMake sources make it visible in QtCreator
    qml/AboutDialog.qml
    qml/main.qml
    qml/SearchDialog.qml
    qml/MapDownloadDialog.qml
    qml/custom/MapDialog.qml
    qml/custom/ScrollIndicator.qml
    qml/custom/LineEdit.qml
    qml/custom/LocationSearch.qml
    qml/custom/DialogActionButton.qml
    qml/custom/MapButton.qml
)

qt5_add_resources(RESOURCE_FILES
    res.qrc
)

add_executable(OSMScout2 ${SOURCE_FILES} ${HEADER_FILES} ${RESOURCE_FILES})

target_include_directories(OSMScout2 PRIVATE
		${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include
		${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-map/include
		${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-map-qt/include
		${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-client-qt/include
)

target_link_libraries(OSMScout2
        osmscout
        osmscout_map
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Qml
        Qt5::Quick
        Qt5::Svg
        Qt5::Positioning
        osmscout_map_qt
        osmscout_client_qt
)
# enable QML debuger
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DQT_QML_DEBUG ")
set_property(TARGET OSMScout2 PROPERTY CXX_STANDARD 11)
