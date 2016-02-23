set(OSMSCOUT_BUILD_TOOL_OSMSCOUT2 ON CACHE INTERNAL "" FORCE)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5Core 5.0 REQUIRED)
find_package(Qt5Gui 5.0 REQUIRED)
find_package(Qt5Widgets 5.0 REQUIRED)
find_package(Qt5Qml 5.0 REQUIRED)
find_package(Qt5Quick 5.0 REQUIRED)
find_package(Qt5Svg 5.0 REQUIRED)
find_package(Qt5Positioning 5.0 REQUIRED)

set(HEADER_FILES
    src/DBThread.h
    src/MapWidget.h
    src/RoutingModel.h
    src/SearchLocationModel.h
    src/Settings.h
    src/Theme.h
)

set(SOURCE_FILES
    src/DBThread.cpp
    src/MapWidget.cpp
    src/OSMScout.cpp
    src/RoutingModel.cpp
    src/SearchLocationModel.cpp
    src/Settings.cpp
    src/Theme.cpp
)

set(RESOURCE_FILES
    res.qrc
)

add_executable(OSMScout2 ${SOURCE_FILES} ${HEADER_FILES} ${RESOURCE_FILES})

target_include_directories(OSMScout2 PRIVATE 
		${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include
		${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-map/include
		${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-map-qt/include)

target_link_libraries(OSMScout2
        libosmscout
        libosmscout_map
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Qml
        Qt5::Quick
        Qt5::Svg
        Qt5::Positioning
        libosmscout_map_qt)

set_property(TARGET OSMScout2 PROPERTY CXX_STANDARD 11)
