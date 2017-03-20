QT       += quick opengl

TARGET = osmscout-client-qt
TEMPLATE = lib
CONFIG += staticlib
INCLUDEPATH += include ../libosmscout-map/include ../libosmscout-map-qt/include ../libosmscout/include ../../../marisa/lib

SOURCES += \
    src/osmscout/AvailableMapsModel.cpp \
    src/osmscout/DBThread.cpp \
    src/osmscout/FileDownloader.cpp \
    src/osmscout/InputHandler.cpp \
    src/osmscout/LocationEntry.cpp \
    src/osmscout/LocationInfoModel.cpp \
    src/osmscout/MapDownloadsModel.cpp \
    src/osmscout/MapManager.cpp \
    src/osmscout/MapProvider.cpp \
    src/osmscout/MapWidget.cpp \
    src/osmscout/OnlineTileProvider.cpp \
    src/osmscout/OnlineTileProviderModel.cpp \
    src/osmscout/OSMTile.cpp \
    src/osmscout/OsmTileDownloader.cpp \
    src/osmscout/PlaneDBThread.cpp \
    src/osmscout/RoutingModel.cpp \
    src/osmscout/SearchLocationModel.cpp \
    src/osmscout/Settings.cpp \
    src/osmscout/TileCache.cpp \
    src/osmscout/TiledDBThread.cpp


HEADERS += \
    include/osmscout/AvailableMapsModel.h \
    include/osmscout/ClientQtFeatures.h \
    include/osmscout/DBThread.h \
    include/osmscout/FileDownloader.h \
    include/osmscout/InputHandler.h \
    include/osmscout/LocationEntry.h \
    include/osmscout/LocationInfoModel.h \
    include/osmscout/MapDownloadsModel.h \
    include/osmscout/MapManager.h \
    include/osmscout/MapProvider.h \
    include/osmscout/MapWidget.h \
    include/osmscout/OnlineTileProvider.h \
    include/osmscout/OnlineTileProviderModel.h \
    include/osmscout/OSMTile.h \
    include/osmscout/OsmTileDownloader.h \
    include/osmscout/PersistentCookieJar.h \
    include/osmscout/PlaneDBThread.h \
    include/osmscout/RoutingModel.h \
    include/osmscout/SearchLocationModel.h \
    include/osmscout/Settings.h \
    include/osmscout/TileCache.h \
    include/osmscout/TiledDBThread.h \
    include/osmscout/private/ClientQtImportExport.h \
    include/osmscout/private/Config.h

macx {
    QMAKE_CXXFLAGS = -mmacosx-version-min=10.7 -std=gnu0x -stdlib=libc++
    CONFIG +=c++11
}

unix {
    target.path = /usr/lib
    INSTALLS += target
}

windows {
    DEFINES += WIN32 OSMSCOUTMAPQTDLL DLL_EXPORT
    CONFIG(debug, debug|release): DESTDIR = ../windows/Debug
    CONFIG(release, debug|release): DESTDIR = ../windows/Release
}
