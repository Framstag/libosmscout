/*
 OSMScout - a Qt backend for libosmscout and libosmscout-map
 Copyright (C) 2017 Lukas Karas

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 */

#include <QObject>
#include <QMetaType>
#include <QQmlEngine>

#include <osmscout/OSMScoutQt.h>
#include <osmscout/Settings.h>
#include <osmscout/DBThread.h>
#include <osmscout/DataTileCache.h>
#include <osmscout/MapWidget.h>
#include <osmscout/PlaneMapRenderer.h>
#include <osmscout/TiledMapRenderer.h>
#include <osmscout/OverlayWay.h>

#include <osmscout/AvailableMapsModel.h>
#include <osmscout/LocationInfoModel.h>
#include <osmscout/MapDownloadsModel.h>
#include <osmscout/MapObjectInfoModel.h>
#include <osmscout/MapStyleModel.h>
#include <osmscout/OnlineTileProviderModel.h>
#include <osmscout/RoutingModel.h>
#include <osmscout/SearchLocationModel.h>
#include <osmscout/StyleFlagsModel.h>
#include <osmscout/Router.h>

static OSMScoutQt* osmScoutInstance=NULL;

OSMScoutQtBuilder::OSMScoutQtBuilder():
  settingsStorage(NULL),
  onlineTileCacheSize(100),
  offlineTileCacheSize(200),
  styleSheetDirectoryConfigured(false),
  styleSheetFileConfigured(false)
{
  QString documentsLocation = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
  mapLookupDirectories << QDir::currentPath();
  mapLookupDirectories << QDir(documentsLocation).filePath("Maps");

  cacheLocation = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
}

  OSMScoutQtBuilder::~OSMScoutQtBuilder()
{
}

bool OSMScoutQtBuilder::Init()
{
  if (osmScoutInstance!=NULL){
    return false;
  }

  SettingsRef settings=std::make_shared<Settings>(settingsStorage);

  // load online tile providers
  if (!onlineTileProviders.isEmpty()){
    settings->loadOnlineTileProviders(onlineTileProviders);
  }
  if (!mapProviders.isEmpty()){
    settings->loadMapProviders(mapProviders);
  }

  // setup style sheet
  if (styleSheetFileConfigured){
    settings->SetStyleSheetFile(styleSheetFile);
  }
  if (styleSheetDirectoryConfigured){
    settings->SetStyleSheetDirectory(styleSheetDirectory);
  }

  MapManagerRef mapManager=std::make_shared<MapManager>(mapLookupDirectories, settings);

  QThread *thread=new QThread();
  DBThreadRef dbThread=std::make_shared<DBThread>(thread,
                                                  basemapLookupDirectory,
                                                  iconDirectory,
                                                  settings,
                                                  mapManager);

  thread->setObjectName("DBThread");

  dbThread->connect(thread, SIGNAL(started()), SLOT(Initialize()));
  QObject::connect(thread, SIGNAL(finished()),
                   thread, SLOT(deleteLater()));

  dbThread->moveToThread(thread);
  thread->start();

  osmScoutInstance=new OSMScoutQt(settings,
                                  mapManager,
                                  dbThread,
                                  iconDirectory,
                                  cacheLocation,
                                  onlineTileCacheSize,
                                  offlineTileCacheSize);
                                  
  return true;
}

void OSMScoutQt::RegisterQmlTypes(const char *uri,
                                  int versionMajor,
                                  int versionMinor)
{
  // register osmscout + standard types for usage in Qt signals/slots
  qRegisterMetaType<DatabaseLoadedResponse>("DatabaseLoadedResponse");
  qRegisterMetaType<LocationEntryRef>("LocationEntryRef");
  qRegisterMetaType<osmscout::BreakerRef>("osmscout::BreakerRef");
  qRegisterMetaType<osmscout::GeoBox>("osmscout::GeoBox");
  qRegisterMetaType<osmscout::GeoCoord>("osmscout::GeoCoord");
  qRegisterMetaType<osmscout::LocationDescription>("osmscout::LocationDescription");
  qRegisterMetaType<osmscout::MapData>("osmscout::MapData");
  qRegisterMetaType<osmscout::TileRef>("osmscout::TileRef");
  qRegisterMetaType<osmscout::Vehicle>("osmscout::Vehicle");
  qRegisterMetaType<QList<LocationEntry>>("QList<LocationEntry>");
  qRegisterMetaType<QList<QDir>>("QList<QDir>");
  qRegisterMetaType<MapViewStruct>("MapViewStruct");
  qRegisterMetaType<RouteSelectionRef>("RouteSelectionRef");
  qRegisterMetaType<RouteSelection>("RouteSelection");
  qRegisterMetaType<uint32_t>("uint32_t");
  qRegisterMetaType<AdminRegionInfoRef>("AdminRegionInfoRef");
  qRegisterMetaType<QList<AdminRegionInfoRef>>("QList<AdminRegionInfoRef>");
  qRegisterMetaType<std::unordered_map<std::string,bool>>("std::unordered_map<std::string,bool>");
  qRegisterMetaType<QMap<QString,bool>>("QMap<QString,bool>");

  // regiester osmscout types for usage in QML
  qmlRegisterType<AvailableMapsModel>(uri, versionMajor, versionMinor, "AvailableMapsModel");
  qmlRegisterType<LocationEntry>(uri, versionMajor, versionMinor, "LocationEntry");
  qmlRegisterType<LocationInfoModel>(uri, versionMajor, versionMinor, "LocationInfoModel");
  qmlRegisterType<LocationListModel>(uri, versionMajor, versionMinor, "LocationListModel");
  qmlRegisterType<MapDownloadsModel>(uri, versionMajor, versionMinor, "MapDownloadsModel");
  qmlRegisterType<MapObjectInfoModel>(uri, versionMajor, versionMinor, "MapObjectInfoModel");
  qmlRegisterType<MapStyleModel>(uri, versionMajor, versionMinor, "MapStyleModel");
  qmlRegisterType<MapWidget>(uri, versionMajor, versionMinor, "Map");
  qmlRegisterType<OnlineTileProviderModel>(uri, versionMajor, versionMinor, "OnlineTileProviderModel");
  qmlRegisterType<OverlayWay>(uri, versionMajor, versionMinor, "OverlayWay");
  qmlRegisterType<QmlSettings>(uri, versionMajor, versionMinor, "Settings");
  qmlRegisterType<RouteStep>(uri, versionMajor, versionMinor, "RouteStep");
  qmlRegisterType<RoutingListModel>(uri, versionMajor, versionMinor, "RoutingListModel");
  qmlRegisterType<StyleFlagsModel>(uri, versionMajor, versionMinor, "StyleFlagsModel");
}

OSMScoutQtBuilder OSMScoutQt::NewInstance()
{
  return OSMScoutQtBuilder();
}

OSMScoutQt& OSMScoutQt::GetInstance()
{
  return *osmScoutInstance;
}

void OSMScoutQt::FreeInstance()
{
  delete osmScoutInstance;
  osmScoutInstance=NULL;
}

OSMScoutQt::OSMScoutQt(SettingsRef settings,
                       MapManagerRef mapManager,
                       DBThreadRef dbThread,
                       QString iconDirectory,
                       QString cacheLocation,
                       size_t onlineTileCacheSize,
                       size_t offlineTileCacheSize):
        settings(settings),
        mapManager(mapManager),
        dbThread(dbThread),
        iconDirectory(iconDirectory),
        cacheLocation(cacheLocation),
        onlineTileCacheSize(onlineTileCacheSize),
        offlineTileCacheSize(offlineTileCacheSize)
{
}

OSMScoutQt::~OSMScoutQt()
{
}

DBThreadRef OSMScoutQt::GetDBThread() const
{
  return dbThread;
}

SettingsRef OSMScoutQt::GetSettings() const
{
  return settings;
}

MapManagerRef OSMScoutQt::GetMapManager() const
{
  return mapManager;
}

LookupModule* OSMScoutQt::MakeLookupModule()
{
  QThread *thread=new QThread();
  thread->setObjectName("LookupModule");
  LookupModule *module=new LookupModule(thread,dbThread);
  module->moveToThread(thread);
  thread->start();
  QObject::connect(thread, SIGNAL(finished()),
                   thread, SLOT(deleteLater()));
  return module;
}

SearchModule* OSMScoutQt::MakeSearchModule()
{
  QThread *thread=new QThread();
  thread->setObjectName("SearchModule");
  SearchModule *module=new SearchModule(thread,dbThread,MakeLookupModule());
  module->moveToThread(thread);
  thread->start();
  QObject::connect(thread, SIGNAL(finished()),
                   thread, SLOT(deleteLater()));
  return module;
}

StyleModule* OSMScoutQt::MakeStyleModule()
{
  QThread *thread=new QThread();
  thread->setObjectName("StyleModule");
  StyleModule *module=new StyleModule(thread,dbThread);
  module->moveToThread(thread);
  thread->start();
  QObject::connect(thread, SIGNAL(finished()),
                   thread, SLOT(deleteLater()));
  return module;
}

MapRenderer* OSMScoutQt::MakeMapRenderer(RenderingType type)
{
  QThread *thread=new QThread();
  thread->setObjectName("MapRenderer");
  MapRenderer* mapRenderer;
  if (type==RenderingType::TiledRendering){
    mapRenderer=new TiledMapRenderer(thread,
                                     settings,
                                     dbThread,
                                     iconDirectory,
                                     cacheLocation,
                                     onlineTileCacheSize,
                                     offlineTileCacheSize);
  }else{
    mapRenderer=new PlaneMapRenderer(thread,settings,dbThread,iconDirectory);
  }
  mapRenderer->moveToThread(thread);
  thread->start();

  QObject::connect(thread, SIGNAL(finished()),
                   thread, SLOT(deleteLater()));
  return mapRenderer;
}

Router* OSMScoutQt::MakeRouter()
{
  QThread *thread=new QThread();
  thread->setObjectName("Router");

  Router *router=new Router(thread,settings,dbThread);
  router->moveToThread(thread);
  thread->start();

  QObject::connect(thread, SIGNAL(finished()),
                   thread, SLOT(deleteLater()));
  return router;
}
