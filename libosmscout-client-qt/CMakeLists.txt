if(NOT Qt5Core_FOUND)
	message(SEND_ERROR "Qt5Core is required for OSMScout Qt client library")
endif()
if(NOT Qt5Gui_FOUND)
	message(SEND_ERROR "Qt5Gui is required for OSMScout Qt client library")
endif()
if(NOT Qt5Quick_FOUND)
	message(SEND_ERROR "Qt5Quick is required for OSMScout Qt client library")
endif()
if(NOT ${OSMSCOUT_BUILD_MAP_QT})
	message(SEND_ERROR "The main map drawing interface is required for Qt map drawing backend")
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(HEADER_FILES
    include/osmscout/DBThread.h
    include/osmscout/DBInstance.h
    include/osmscout/FileDownloader.h
    include/osmscout/InputHandler.h
    include/osmscout/LocationEntry.h
    include/osmscout/LocationInfoModel.h
    include/osmscout/MapWidget.h
    include/osmscout/OnlineTileProvider.h
    include/osmscout/OnlineTileProviderModel.h
    include/osmscout/OsmTileDownloader.h
    include/osmscout/OSMTile.h
    include/osmscout/private
    include/osmscout/private/ClientQtImportExport.h
    include/osmscout/RoutingModel.h
    include/osmscout/SearchLocationModel.h
    include/osmscout/Settings.h
    include/osmscout/TileCache.h
    include/osmscout/MapProvider.h
    include/osmscout/AvailableMapsModel.h
    include/osmscout/PersistentCookieJar.h
    include/osmscout/MapManager.h
    include/osmscout/MapDownloadsModel.h
    include/osmscout/MapObjectInfoModel.h
    include/osmscout/MapStyleModel.h
    include/osmscout/StyleFlagsModel.h
    include/osmscout/OSMScoutQt.h
    include/osmscout/DBJob.h
    include/osmscout/LookupModule.h
    include/osmscout/MapRenderer.h
    include/osmscout/PlaneMapRenderer.h
    include/osmscout/TiledMapRenderer.h
)

set(SOURCE_FILES
    src/osmscout/DBThread.cpp
    src/osmscout/DBInstance.cpp
    src/osmscout/FileDownloader.cpp
    src/osmscout/InputHandler.cpp
    src/osmscout/LocationEntry.cpp
    src/osmscout/LocationInfoModel.cpp
    src/osmscout/MapWidget.cpp
    src/osmscout/OnlineTileProvider.cpp
    src/osmscout/OnlineTileProviderModel.cpp
    src/osmscout/OSMTile.cpp
    src/osmscout/OsmTileDownloader.cpp
    src/osmscout/RoutingModel.cpp
    src/osmscout/SearchLocationModel.cpp
    src/osmscout/Settings.cpp
    src/osmscout/TileCache.cpp
    src/osmscout/MapProvider.cpp
    src/osmscout/AvailableMapsModel.cpp
    src/osmscout/MapManager.cpp
    src/osmscout/MapDownloadsModel.cpp
    src/osmscout/MapObjectInfoModel.cpp
    src/osmscout/MapStyleModel.cpp
    src/osmscout/StyleFlagsModel.cpp
    src/osmscout/OSMScoutQt.cpp
    src/osmscout/DBJob.cpp
    src/osmscout/LookupModule.cpp
    src/osmscout/MapRenderer.cpp
    src/osmscout/PlaneMapRenderer.cpp
    src/osmscout/TiledMapRenderer.cpp
)

add_library(osmscout_client_qt ${SOURCE_FILES} ${HEADER_FILES})
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
if(MARISA_FOUND)
    target_include_directories(osmscout_client_qt PRIVATE ${MARISA_INCLUDE_DIRS})
endif()
target_link_libraries(osmscout_client_qt
		osmscout
		osmscout_map
		osmscout_map_qt
		Qt5::Core
		Qt5::Gui
		Qt5::Quick)
target_compile_definitions(osmscout_client_qt PRIVATE -DOSMSCOUT_CLIENT_QT_EXPORT_SYMBOLS ${Qt5Gui_DEFINITIONS})
install(TARGETS osmscout_client_qt
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout DESTINATION include FILES_MATCHING PATTERN "*.h" PATTERN "private/Config.h" EXCLUDE)
install(FILES ${OSMSCOUT_BASE_DIR_BUILD}/include/osmscout/ClientQtFeatures.h DESTINATION include/osmscout)
