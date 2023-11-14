QT       += quick opengl

TARGET = osmscout-client-qt
TEMPLATE = lib
CONFIG += staticlib
INCLUDEPATH += include ../libosmscout-map/include ../libosmscout-map-qt/include ../libosmscout/include ../../../marisa/lib

SOURCES += \
    src/osmscoutclientqt/AvailableMapsModel.cpp \
    src/osmscoutclientqt/DBThread.cpp \
    src/osmscoutclientqt/FileDownloader.cpp \
    src/osmscoutclientqt/InputHandler.cpp \
    src/osmscoutclientqt/LocationEntry.cpp \
    src/osmscoutclientqt/LocationInfoModel.cpp \
    src/osmscoutclientqt/MapDownloadsModel.cpp \
    src/osmscoutclientqt/MapDownloader.cpp \
    src/osmscoutclientqt/MapProvider.cpp \
    src/osmscoutclientqt/MapWidget.cpp \
    src/osmscoutclientqt/OnlineTileProvider.cpp \
    src/osmscoutclientqt/OnlineTileProviderModel.cpp \
    src/osmscoutclientqt/OSMTile.cpp \
    src/osmscoutclientqt/OsmTileDownloader.cpp \
    src/osmscoutclientqt/PlaneDBThread.cpp \
    src/osmscoutclientqt/RoutingModel.cpp \
    src/osmscoutclientqt/SearchLocationModel.cpp \
    src/osmscoutclientqt/Settings.cpp \
    src/osmscoutclientqt/TileCache.cpp \
    src/osmscoutclientqt/TiledDBThread.cpp \
    src/osmscoutclientqt/MapStyleModel.cpp \
    src/osmscoutclientqt/StyleFlagsModel.cpp \
    src/osmscoutclientqt/MapObjectInfoModel.cpp \
    src/osmscoutclientqt/DBInstance.cpp \
    src/osmscoutclientqt/DBJob.cpp \
    src/osmscoutclientqt/LookupModule.cpp \
    src/osmscoutclientqt/OSMScoutQt.cpp


HEADERS += \
    include/osmscoutclientqt/AvailableMapsModel.h \
    include/osmscoutclientqt/ClientQtFeatures.h \
    include/osmscoutclientqt/DBThread.h \
    include/osmscoutclientqt/FileDownloader.h \
    include/osmscoutclientqt/InputHandler.h \
    include/osmscoutclientqt/LocationEntry.h \
    include/osmscoutclientqt/LocationInfoModel.h \
    include/osmscoutclientqt/MapDownloadsModel.h \
    include/osmscoutclientqt/MapDownloader.h \
    include/osmscoutclientqt/MapProvider.h \
    include/osmscoutclientqt/MapWidget.h \
    include/osmscoutclientqt/OnlineTileProvider.h \
    include/osmscoutclientqt/OnlineTileProviderModel.h \
    include/osmscoutclientqt/OSMTile.h \
    include/osmscoutclientqt/OsmTileDownloader.h \
    include/osmscoutclientqt/PersistentCookieJar.h \
    include/osmscoutclientqt/PlaneDBThread.h \
    include/osmscoutclientqt/RoutingModel.h \
    include/osmscoutclientqt/SearchLocationModel.h \
    include/osmscoutclientqt/Settings.h \
    include/osmscoutclientqt/TileCache.h \
    include/osmscoutclientqt/TiledDBThread.h \
    include/osmscoutclientqt/private/ClientQtImportExport.h \
    include/osmscoutclientqt/private/Config.h \
    include/osmscoutclientqt/MapStyleModel.h \
    include/osmscoutclientqt/StyleFlagsModel.h \
    include/osmscoutclientqt/MapObjectInfoModel.h \
    include/osmscoutclientqt/DBInstance.h \
    include/osmscoutclientqt/DBJob.h \
    include/osmscoutclientqt/LookupModule.h \
    include/osmscoutclientqt/MapRenderer.h \
    include/osmscoutclientqt/OSMScoutQt.h

macx {
    QMAKE_CXXFLAGS = -mmacosx-version-min=10.7 -std=gnu0x -stdlib=libc++
    CONFIG +=c++11
}

unix {
    target.path = /usr/lib
    INSTALLS += target
}

windows {
    DEFINES += WIN32 OSMScoutClientQt_EXPORTS DLL_EXPORT
    CONFIG(debug, debug|release): DESTDIR = ../windows/Debug
    CONFIG(release, debug|release): DESTDIR = ../windows/Release
}
