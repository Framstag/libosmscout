AM_CPPFLAGS = -DOSMSCOUTCLIENTQTDLL -I$(top_srcdir)/include \
              $(LIBOSMSCOUT_CFLAGS) \
              $(LIBQT_CFLAGS)

lib_LTLIBRARIES = libosmscoutclientqt.la
libosmscoutclientqt_la_LDFLAGS  = -no-undefined \
                               $(LIBOSMSCOUT_LIBS) \
                               $(LIBQT_LIBS)

libosmscoutclientqt_la_SOURCES = osmscout/DBThread.cpp \
                                 osmscout/moc_DBThread.cpp \
                                 osmscout/DBInstance.cpp \
                                 osmscout/moc_DBInstance.cpp \
                                 osmscout/RoutingModel.cpp \
                                 osmscout/moc_RoutingModel.cpp \
                                 osmscout/Router.cpp \
                                 osmscout/moc_Router.cpp \
                                 osmscout/InputHandler.cpp \
                                 osmscout/moc_InputHandler.cpp \
                                 osmscout/OnlineTileProvider.cpp \
                                 osmscout/moc_OnlineTileProvider.cpp \
                                 osmscout/OnlineTileProviderModel.cpp \
                                 osmscout/moc_OnlineTileProviderModel.cpp \
                                 osmscout/SearchLocationModel.cpp \
                                 osmscout/moc_SearchLocationModel.cpp \
                                 osmscout/Settings.cpp \
                                 osmscout/moc_Settings.cpp \
                                 osmscout/MapWidget.cpp \
                                 osmscout/moc_MapWidget.cpp \
                                 osmscout/LocationEntry.cpp \
                                 osmscout/moc_LocationEntry.cpp \
                                 osmscout/LocationInfoModel.cpp \
                                 osmscout/moc_LocationInfoModel.cpp \
                                 osmscout/OsmTileDownloader.cpp \
                                 osmscout/moc_OsmTileDownloader.cpp \
                                 osmscout/OSMTile.cpp \
                                 osmscout/moc_OSMTile.cpp \
                                 osmscout/TileCache.cpp \
                                 osmscout/moc_TileCache.cpp \
                                 osmscout/MapProvider.cpp \
                                 osmscout/moc_MapProvider.cpp \
                                 osmscout/AvailableMapsModel.cpp \
                                 osmscout/moc_AvailableMapsModel.cpp \
                                 osmscout/FileDownloader.cpp \
                                 osmscout/moc_FileDownloader.cpp \
                                 osmscout/MapManager.cpp \
                                 osmscout/moc_MapManager.cpp \
                                 osmscout/MapDownloadsModel.cpp \
                                 osmscout/moc_MapDownloadsModel.cpp \
                                 osmscout/MapObjectInfoModel.cpp \
                                 osmscout/moc_MapObjectInfoModel.cpp \
                                 osmscout/MapStyleModel.cpp \
                                 osmscout/moc_MapStyleModel.cpp \
                                 osmscout/StyleFlagsModel.cpp \
                                 osmscout/moc_StyleFlagsModel.cpp \
                                 osmscout/OSMScoutQt.cpp \
                                 osmscout/moc_OSMScoutQt.cpp \
                                 osmscout/DBJob.cpp \
                                 osmscout/moc_DBJob.cpp \
                                 osmscout/LookupModule.cpp \
                                 osmscout/moc_LookupModule.cpp \
                                 osmscout/MapRenderer.cpp \
                                 osmscout/moc_MapRenderer.cpp \
                                 osmscout/PlaneMapRenderer.cpp \
                                 osmscout/moc_PlaneMapRenderer.cpp \
                                 osmscout/TiledMapRenderer.cpp \
                                 osmscout/moc_TiledMapRenderer.cpp \
                                 osmscout/OverlayWay.cpp \
                                 osmscout/moc_OverlayWay.cpp

osmscout/moc_%.cpp: $(top_srcdir)/include/osmscout/%.h
	@MOC@ -o$@ $<

MOSTLYCLEANFILES = osmscout/moc_DBThread.cpp \
                   osmscout/moc_DBInstance.cpp \
                   osmscout/moc_MapWidget.cpp \
                   osmscout/moc_RoutingModel.cpp \
                   osmscout/moc_Router.cpp \
                   osmscout/moc_LocationEntry.cpp \
                   osmscout/moc_SearchLocationModel.cpp \
                   osmscout/moc_Settings.cpp \
                   osmscout/moc_LocationInfoModel.cpp \
                   osmscout/moc_OsmTileDownloader.cpp \
                   osmscout/moc_OSMTile.cpp \
                   osmscout/moc_TileCache.cpp \
                   osmscout/moc_MapProvider.cpp \
                   osmscout/moc_AvailableMapsModel.cpp \
                   osmscout/moc_MapManager.cpp \
                   osmscout/moc_MapDownloadsModel.cpp \
                   osmscout/moc_MapObjectInfoModel.cpp \
                   osmscout/moc_MapStyleModel.cpp \
                   osmscout/moc_StyleFlagsModel.cpp \
                   osmscout/moc_InputHandler.cpp \
                   osmscout/moc_OnlineTileProvider.cpp \
                   osmscout/moc_OnlineTileProviderModel.cpp \
                   osmscout/moc_OSMScoutQt.cpp \
                   osmscout/moc_DBJob.cpp \
                   osmscout/moc_LookupModule.cpp \
                   osmscout/moc_MapRenderer.cpp \
                   osmscout/moc_PlaneMapRenderer.cpp \
                   osmscout/moc_TiledMapRenderer.cpp \
                   osmscout/moc_OverlayWay.cpp
