set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(HEADER_FILES
    include/osmscoutclientqt/AdminRegionInfo.h
    include/osmscoutclientqt/ClientQtImportExport.h
    include/osmscoutclientqt/DBThread.h
    include/osmscoutclientqt/DBInstance.h
    include/osmscoutclientqt/ElevationChartWidget.h
    include/osmscoutclientqt/ElevationModule.h
    include/osmscoutclientqt/FileDownloader.h
    include/osmscoutclientqt/IconAnimation.h
    include/osmscoutclientqt/IconLookup.h
    include/osmscoutclientqt/InputHandler.h
    include/osmscoutclientqt/LocationEntry.h
    include/osmscoutclientqt/LocationInfoModel.h
    include/osmscoutclientqt/MapWidget.h
    include/osmscoutclientqt/OnlineTileProvider.h
    include/osmscoutclientqt/OnlineTileProviderModel.h
    include/osmscoutclientqt/OpeningHoursModel.h
    include/osmscoutclientqt/OsmTileDownloader.h
    include/osmscoutclientqt/OSMTile.h
    include/osmscoutclientqt/RoutingModel.h
    include/osmscoutclientqt/Router.h
    include/osmscoutclientqt/SearchLocationModel.h
    include/osmscoutclientqt/Settings.h
    include/osmscoutclientqt/TileCache.h
    include/osmscoutclientqt/MapProvider.h
    include/osmscoutclientqt/AvailableMapsModel.h
    include/osmscoutclientqt/PersistentCookieJar.h
    include/osmscoutclientqt/MapManager.h
    include/osmscoutclientqt/MapDownloadsModel.h
    include/osmscoutclientqt/MapObjectInfoModel.h
    include/osmscoutclientqt/MapStyleModel.h
    include/osmscoutclientqt/StyleFlagsModel.h
    include/osmscoutclientqt/OSMScoutQt.h
    include/osmscoutclientqt/DBJob.h
    include/osmscoutclientqt/LookupModule.h
    include/osmscoutclientqt/SearchModule.h
    include/osmscoutclientqt/StyleModule.h
    include/osmscoutclientqt/SunriseSunset.h
    include/osmscoutclientqt/MapRenderer.h
    include/osmscoutclientqt/PlaneMapRenderer.h
    include/osmscoutclientqt/TiledMapRenderer.h
    include/osmscoutclientqt/OverlayObject.h
    include/osmscoutclientqt/MapOverlay.h
    include/osmscoutclientqt/TiledMapOverlay.h
    include/osmscoutclientqt/TiledRenderingHelper.h
    include/osmscoutclientqt/NavigationModule.h
    include/osmscoutclientqt/RouteDescriptionBuilder.h
    include/osmscoutclientqt/NavigationModel.h
    include/osmscoutclientqt/VehiclePosition.h
    include/osmscoutclientqt/QmlRoutingProfile.h
    include/osmscoutclientqt/QtRouteData.h
    include/osmscoutclientqt/RouteStep.h
    include/osmscoutclientqt/NearPOIModel.h
    include/osmscoutclientqt/POILookupModule.h
    include/osmscoutclientqt/InstalledMapsModel.h
    include/osmscoutclientqt/Voice.h
    include/osmscoutclientqt/VoiceManager.h
    include/osmscoutclientqt/AvailableVoicesModel.h
    include/osmscoutclientqt/VoiceProvider.h
    include/osmscoutclientqt/InstalledVoicesModel.h
)

set(SOURCE_FILES
    src/osmscoutclientqt/DBThread.cpp
    src/osmscoutclientqt/DBInstance.cpp
    src/osmscoutclientqt/ElevationChartWidget.cpp
    src/osmscoutclientqt/ElevationModule.cpp
    src/osmscoutclientqt/FileDownloader.cpp
    src/osmscoutclientqt/IconAnimation.cpp
    src/osmscoutclientqt/IconLookup.cpp
    src/osmscoutclientqt/InputHandler.cpp
    src/osmscoutclientqt/LocationEntry.cpp
    src/osmscoutclientqt/LocationInfoModel.cpp
    src/osmscoutclientqt/MapWidget.cpp
    src/osmscoutclientqt/OnlineTileProvider.cpp
    src/osmscoutclientqt/OnlineTileProviderModel.cpp
    src/osmscoutclientqt/OpeningHoursModel.cpp
    src/osmscoutclientqt/OSMTile.cpp
    src/osmscoutclientqt/OsmTileDownloader.cpp
    src/osmscoutclientqt/RoutingModel.cpp
    src/osmscoutclientqt/Router.cpp
    src/osmscoutclientqt/SearchLocationModel.cpp
    src/osmscoutclientqt/Settings.cpp
    src/osmscoutclientqt/TileCache.cpp
    src/osmscoutclientqt/MapProvider.cpp
    src/osmscoutclientqt/AvailableMapsModel.cpp
    src/osmscoutclientqt/MapManager.cpp
    src/osmscoutclientqt/MapDownloadsModel.cpp
    src/osmscoutclientqt/MapObjectInfoModel.cpp
    src/osmscoutclientqt/MapStyleModel.cpp
    src/osmscoutclientqt/StyleFlagsModel.cpp
    src/osmscoutclientqt/OSMScoutQt.cpp
    src/osmscoutclientqt/DBJob.cpp
    src/osmscoutclientqt/LookupModule.cpp
    src/osmscoutclientqt/SearchModule.cpp
    src/osmscoutclientqt/StyleModule.cpp
    src/osmscoutclientqt/SunriseSunset.cpp
    src/osmscoutclientqt/MapRenderer.cpp
    src/osmscoutclientqt/PlaneMapRenderer.cpp
    src/osmscoutclientqt/TiledMapRenderer.cpp
    src/osmscoutclientqt/OverlayObject.cpp
    src/osmscoutclientqt/MapOverlay.cpp
    src/osmscoutclientqt/TiledMapOverlay.cpp
    src/osmscoutclientqt/TiledRenderingHelper.cpp
    src/osmscoutclientqt/NavigationModule.cpp
    src/osmscoutclientqt/RouteDescriptionBuilder.cpp
    src/osmscoutclientqt/NavigationModel.cpp
    src/osmscoutclientqt/VehiclePosition.cpp
    src/osmscoutclientqt/RouteStep.cpp
    src/osmscoutclientqt/QmlRoutingProfile.cpp
    src/osmscoutclientqt/QtRouteData.cpp
    src/osmscoutclientqt/NearPOIModel.cpp
    src/osmscoutclientqt/POILookupModule.cpp
    src/osmscoutclientqt/InstalledMapsModel.cpp
    src/osmscoutclientqt/Voice.cpp
    src/osmscoutclientqt/VoiceManager.cpp
    src/osmscoutclientqt/AvailableVoicesModel.cpp
    src/osmscoutclientqt/VoiceProvider.cpp
    src/osmscoutclientqt/InstalledVoicesModel.cpp
)

osmscout_library_project(
	NAME OSMScoutClientQt
	ALIAS ClientQt
	OUTPUT_NAME "osmscout_client_qt"
	SOURCE ${SOURCE_FILES}
	HEADER ${HEADER_FILES}
	INCLUDEDIR osmscoutclientqt
	TEMPLATE ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscoutclientqt/ClientQtFeatures.h.cmake
	TARGET OSMScout::OSMScout OSMScout::Map OSMScout::MapQt Qt5::Core Qt5::Gui Qt5::Quick Qt5::Multimedia
)

if(MARISA_FOUND)
    target_include_directories(OSMScoutClientQt PUBLIC ${MARISA_INCLUDE_DIRS})
    target_link_libraries(OSMScoutClientQt ${MARISA_LIBRARIES})
endif()

if(APPLE AND OSMSCOUT_BUILD_FRAMEWORKS)
	set_target_properties(OSMScoutClientQt PROPERTIES
		FRAMEWORK TRUE
		FRAMEWORK_VERSION C
		MACOSX_FRAMEWORK_IDENTIFIER com.cmake.dynamicFramework
		#MACOSX_FRAMEWORK_INFO_PLIST Info.plist
		PUBLIC_HEADER     "${HEADER_FILES}"
		CODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer"
		OUTPUT_NAME "OSMScoutClientQt")
endif()
