set(OSMSCOUT_BUILD_TOOL_STYLEEDITOR ON CACHE INTERNAL "" FORCE)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5Core 5.0 REQUIRED)
find_package(Qt5Gui 5.0 REQUIRED)
find_package(Qt5Widgets 5.0 REQUIRED)
find_package(Qt5Qml 5.0 REQUIRED)
find_package(Qt5Quick 5.0 REQUIRED)
find_package(Qt5Svg 5.0 REQUIRED)

set(HEADER_FILES
	src/DBThread.h
	src/FileIO.h
	src/Highlighter.h
	src/MainWindow.h
	src/MapWidget.h
	src/SearchLocationModel.h
	src/Settings.h
	src/SettingsDialog.h
)

set(SOURCE_FILES
	src/DBThread.cpp
	src/FileIO.cpp
	src/Highlighter.cpp
	src/MainWindow.cpp
	src/MapWidget.cpp
	src/SearchLocationModel.cpp
	src/Settings.cpp
	src/SettingsDialog.cpp
	src/StyleEditor.cpp
)

set(RESOURCE_FILES
    res.qrc
)

add_executable(StyleEditor ${SOURCE_FILES} ${HEADER_FILES} ${RESOURCE_FILES})

target_include_directories(StyleEditor PRIVATE 
		${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include
		${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-map/include
		${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-map-qt/include)

target_link_libraries(StyleEditor
        libosmscout
        libosmscout_map
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Qml
        Qt5::Quick
        Qt5::Svg
        libosmscout_map_qt)

set_property(TARGET StyleEditor PROPERTY CXX_STANDARD 11)
