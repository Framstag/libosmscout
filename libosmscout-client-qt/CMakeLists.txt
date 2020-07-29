set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(HEADER_FILES
    include/osmscout/ClientQtImportExport.h
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
    include/osmscout/RoutingModel.h
    include/osmscout/Router.h
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
    include/osmscout/SearchModule.h
    include/osmscout/StyleModule.h
    include/osmscout/MapRenderer.h
    include/osmscout/PlaneMapRenderer.h
    include/osmscout/TiledMapRenderer.h
    include/osmscout/OverlayObject.h
    include/osmscout/MapOverlay.h
    include/osmscout/TiledMapOverlay.h
    include/osmscout/TiledRenderingHelper.h
    include/osmscout/NavigationModule.h
    include/osmscout/RouteDescriptionBuilder.h
    include/osmscout/NavigationModel.h
    include/osmscout/VehiclePosition.h
    include/osmscout/QtRouteData.h
    include/osmscout/RouteStep.h
    include/osmscout/NearPOIModel.h
    include/osmscout/POILookupModule.h
    include/osmscout/InstalledMapsModel.h
    include/osmscout/Voice.h
    include/osmscout/VoiceManager.h
    include/osmscout/AvailableVoicesModel.h
    include/osmscout/VoiceProvider.h
    include/osmscout/InstalledVoicesModel.h
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
    src/osmscout/Router.cpp
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
    src/osmscout/SearchModule.cpp
    src/osmscout/StyleModule.cpp
    src/osmscout/MapRenderer.cpp
    src/osmscout/PlaneMapRenderer.cpp
    src/osmscout/TiledMapRenderer.cpp
    src/osmscout/OverlayObject.cpp
    src/osmscout/MapOverlay.cpp
    src/osmscout/TiledMapOverlay.cpp
    src/osmscout/TiledRenderingHelper.cpp
    src/osmscout/NavigationModule.cpp
    src/osmscout/RouteDescriptionBuilder.cpp
    src/osmscout/NavigationModel.cpp
    src/osmscout/VehiclePosition.cpp
    src/osmscout/RouteStep.cpp
    src/osmscout/QtRouteData.cpp
    src/osmscout/NearPOIModel.cpp
    src/osmscout/POILookupModule.cpp
    src/osmscout/InstalledMapsModel.cpp
    src/osmscout/Voice.cpp
    src/osmscout/VoiceManager.cpp
    src/osmscout/AvailableVoicesModel.cpp
    src/osmscout/VoiceProvider.cpp
    src/osmscout/InstalledVoicesModel.cpp
)

osmscout_library_project(
	NAME OSMScoutClientQt
	ALIAS ClientQt
	OUTPUT_NAME "osmscout_client_qt"
	SOURCE ${SOURCE_FILES}
	HEADER ${HEADER_FILES}
	TEMPLATE ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout/ClientQtFeatures.h.cmake
	TARGET OSMScout::OSMScout OSMScout::Map OSMScout::MapQt Qt5::Core Qt5::Gui Qt5::Quick Qt5::Multimedia
)

if(MARISA_FOUND)
    target_include_directories(OSMScoutClientQt PRIVATE ${MARISA_INCLUDE_DIRS})
    target_link_libraries(OSMScoutClientQt ${MARISA_LIBRARIES})
endif()

if(APPLE AND BUILD_FRAMEWORKS)
	set_target_properties(OSMScoutClientQt PROPERTIES
		FRAMEWORK TRUE
		FRAMEWORK_VERSION C
		MACOSX_FRAMEWORK_IDENTIFIER com.cmake.dynamicFramework
		#MACOSX_FRAMEWORK_INFO_PLIST Info.plist
		PUBLIC_HEADER     "${HEADER_FILES}"
		CODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer"
		OUTPUT_NAME "OSMScoutClientQt")
endif()
