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
#include <QtGlobal>
#include <QMetaType>
#include <QQmlEngine>

#include <osmscout/OSMScoutQt.h>
#include <osmscout/Settings.h>
#include <osmscout/DBThread.h>
#include <osmscout/DataTileCache.h>
#include <osmscout/MapWidget.h>
#include <osmscout/PlaneMapRenderer.h>
#include <osmscout/TiledMapRenderer.h>
#include <osmscout/OverlayObject.h>
#include <osmscout/util/Distance.h>

#include <osmscout/AvailableMapsModel.h>
#include <osmscout/LocationInfoModel.h>
#include <osmscout/MapDownloadsModel.h>
#include <osmscout/MapObjectInfoModel.h>
#include <osmscout/MapStyleModel.h>
#include <osmscout/OnlineTileProviderModel.h>
#include <osmscout/RoutingModel.h>
#include <osmscout/SearchLocationModel.h>
#include <osmscout/StyleFlagsModel.h>
#include <osmscout/TiledMapOverlay.h>
#include <osmscout/Router.h>
#include <osmscout/NavigationModel.h>
#include <osmscout/NearPOIModel.h>
#include <osmscout/InstalledMapsModel.h>
#include <osmscout/AvailableVoicesModel.h>
#include <osmscout/InstalledVoicesModel.h>

#include <optional>

namespace osmscout {

static OSMScoutQt* osmScoutInstance=nullptr;

OSMScoutQtBuilder::OSMScoutQtBuilder()
{
  QString documentsLocation = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
  mapLookupDirectories << QDir::currentPath();
  mapLookupDirectories << QDir(documentsLocation).filePath("Maps");

  cacheLocation = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);

  voiceLookupDirectory = QStandardPaths::writableLocation(QStandardPaths::DataLocation) + QDir::separator() + "voices";
}

OSMScoutQtBuilder::~OSMScoutQtBuilder()
{
}

bool OSMScoutQtBuilder::Init()
{
  if (osmScoutInstance!=nullptr){
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
  if (!voiceProviders.isEmpty()){
    settings->loadVoiceProviders(voiceProviders);
  }

  // setup style sheet
  if (styleSheetFileConfigured){
    settings->SetStyleSheetFile(styleSheetFile);
  }
  if (styleSheetDirectoryConfigured){
    settings->SetStyleSheetDirectory(styleSheetDirectory);
  }

  // setup voice
  settings->SetVoiceLookupDirectory(voiceLookupDirectory);

  MapManagerRef mapManager=std::make_shared<MapManager>(mapLookupDirectories, settings);

  QString userAgent=QString("%1/%2 libosmscout/%3 Qt/%4")
      .arg(appName).arg(appVersion)
      .arg(LIBOSMSCOUT_VERSION_STRING)
      .arg(qVersion());

  if(strcmp(qVersion(), QT_VERSION_STR) != 0) {
    qWarning() << "Runtime Qt version" << qVersion() << "is different that compile time version" << QT_VERSION_STR;
  }

  osmScoutInstance=new OSMScoutQt(settings,
                                  mapManager,
                                  basemapLookupDirectory,
                                  iconDirectory,
                                  cacheLocation,
                                  onlineTileCacheSize,
                                  offlineTileCacheSize,
                                  userAgent,
                                  customPoiTypes);
                                  
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
  qRegisterMetaType<osmscout::Distance>("osmscout::Distance");
  qRegisterMetaType<osmscout::Bearing>("osmscout::Bearing");
  qRegisterMetaType<std::shared_ptr<osmscout::Bearing>>("std::shared_ptr<osmscout::Bearing>");
  qRegisterMetaType<std::optional<osmscout::Bearing>>("std::optional<osmscout::Bearing>");
  qRegisterMetaType<osmscout::GeoBox>("osmscout::GeoBox");
  qRegisterMetaType<osmscout::GeoCoord>("osmscout::GeoCoord");
  qRegisterMetaType<osmscout::LocationDescription>("osmscout::LocationDescription");
  qRegisterMetaType<osmscout::MapData>("osmscout::MapData");
  qRegisterMetaType<osmscout::TileRef>("osmscout::TileRef");
  qRegisterMetaType<osmscout::Vehicle>("osmscout::Vehicle");
  qRegisterMetaType<osmscout::PositionAgent::PositionState>("osmscout::PositionAgent::PositionState");
  qRegisterMetaType<osmscout::LaneAgent::Lane>("osmscout::LaneAgent::Lane");
  qRegisterMetaType<QList<LocationEntry>>("QList<LocationEntry>");
  qRegisterMetaType<QList<QDir>>("QList<QDir>");
  qRegisterMetaType<MapViewStruct>("MapViewStruct");
  qRegisterMetaType<QtRouteData>("QtRouteData");
  qRegisterMetaType<uint32_t>("uint32_t");
  qRegisterMetaType<uint64_t>("uint64_t");
  qRegisterMetaType<AdminRegionInfoRef>("AdminRegionInfoRef");
  qRegisterMetaType<QList<AdminRegionInfoRef>>("QList<AdminRegionInfoRef>");
  qRegisterMetaType<std::unordered_map<std::string,bool>>("std::unordered_map<std::string,bool>");
  qRegisterMetaType<QMap<QString,bool>>("QMap<QString,bool>");
  qRegisterMetaType<LocationEntry>("LocationEntry");
  qRegisterMetaType<OnlineTileProvider>("OnlineTileProvider");
  qRegisterMetaType<RouteStep>("RouteStep");
  qRegisterMetaType<std::list<RouteStep>>("std::list<RouteStep>");
  qRegisterMetaType<OverlayWay*>("OverlayWay*");
  qRegisterMetaType<OverlayArea*>("OverlayArea*");
  qRegisterMetaType<OverlayNode*>("OverlayNode*");
  qRegisterMetaType<QList<LookupModule::ObjectInfo>>("QList<LookupModule::ObjectInfo>");

  // register osmscout types for usage in QML
  qmlRegisterType<AvailableMapsModel>(uri, versionMajor, versionMinor, "AvailableMapsModel");
  qmlRegisterType<LocationEntry>(uri, versionMajor, versionMinor, "LocationEntry");
  qmlRegisterType<LocationInfoModel>(uri, versionMajor, versionMinor, "LocationInfoModel");
  qmlRegisterType<LocationListModel>(uri, versionMajor, versionMinor, "LocationListModel");
  qmlRegisterType<MapDownloadsModel>(uri, versionMajor, versionMinor, "MapDownloadsModel");
  qmlRegisterType<MapObjectInfoModel>(uri, versionMajor, versionMinor, "MapObjectInfoModel");
  qmlRegisterType<MapStyleModel>(uri, versionMajor, versionMinor, "MapStyleModel");
  qmlRegisterType<MapWidget>(uri, versionMajor, versionMinor, "Map");
  qmlRegisterType<NavigationModel>(uri, versionMajor, versionMinor, "NavigationModel");
  qmlRegisterType<OnlineTileProviderModel>(uri, versionMajor, versionMinor, "OnlineTileProviderModel");
  qmlRegisterType<OverlayWay>(uri, versionMajor, versionMinor, "OverlayWay");
  qmlRegisterType<OverlayArea>(uri, versionMajor, versionMinor, "OverlayArea");
  qmlRegisterType<OverlayNode>(uri, versionMajor, versionMinor, "OverlayNode");
  qmlRegisterType<QmlSettings>(uri, versionMajor, versionMinor, "Settings");
  qmlRegisterType<RouteStep>(uri, versionMajor, versionMinor, "RouteStep");
  qmlRegisterType<RoutingListModel>(uri, versionMajor, versionMinor, "RoutingListModel");
  qmlRegisterType<StyleFlagsModel>(uri, versionMajor, versionMinor, "StyleFlagsModel");
  qmlRegisterType<TiledMapOverlay>(uri, versionMajor, versionMinor, "TiledMapOverlay");
  qmlRegisterType<NearPOIModel>(uri, versionMajor, versionMinor, "NearPOIModel");
  qmlRegisterType<InstalledMapsModel>(uri, versionMajor, versionMinor, "InstalledMapsModel");
  qmlRegisterType<AvailableVoicesModel>(uri, versionMajor, versionMinor, "AvailableVoicesModel");
  qmlRegisterType<InstalledVoicesModel>(uri, versionMajor, versionMinor, "InstalledVoicesModel");
}

OSMScoutQtBuilder OSMScoutQt::NewInstance()
{
  return OSMScoutQtBuilder();
}

OSMScoutQt& OSMScoutQt::GetInstance()
{
  assert(osmScoutInstance);
  return *osmScoutInstance;
}

void OSMScoutQt::FreeInstance()
{
  // wait up to 5 seconds for release dbThread from other threads
  if (!osmScoutInstance->waitForReleasingResources(100, 50)){
    osmscout::log.Warn() << "Some resources still acquired by other components";
  }
  delete osmScoutInstance;
  osmScoutInstance=nullptr;
  osmscout::log.Debug() << "OSMScoutQt freed";
}

OSMScoutQt::OSMScoutQt(SettingsRef settings,
                       MapManagerRef mapManager,
                       QString basemapLookupDirectory,
                       QString iconDirectory,
                       QString cacheLocation,
                       size_t onlineTileCacheSize,
                       size_t offlineTileCacheSize,
                       QString userAgent,
                       QStringList customPoiTypes):
        settings(settings),
        mapManager(mapManager),
        iconDirectory(iconDirectory),
        cacheLocation(cacheLocation),
        onlineTileCacheSize(onlineTileCacheSize),
        offlineTileCacheSize(offlineTileCacheSize),
        userAgent(userAgent),
        liveBackgroundThreads(0)
{

  std::vector<std::string> customPoiTypeVector;
  for (const auto &typeName:customPoiTypes){
    customPoiTypeVector.push_back(typeName.toStdString());
  }

  QThread *thread=makeThread("DBThread");
  dbThread=std::make_shared<DBThread>(thread,
                                      basemapLookupDirectory,
                                      iconDirectory,
                                      settings,
                                      mapManager,
                                      customPoiTypeVector);

  connect(thread, &QThread::started,
          dbThread.get(), &DBThread::Initialize);

  dbThread->moveToThread(thread);

  thread->start();

  // move itself to DBThread event loop,
  // we need to receive threadFinished slot
  // even main loop is shutdown

  // DBThread is responsible for thread shutdown
  this->moveToThread(thread);
}

OSMScoutQt::~OSMScoutQt()
{
}

bool OSMScoutQt::waitForReleasingResources(unsigned long mSleep, unsigned long maxCount) const
{
  for (unsigned long count=0;
       count < maxCount && (dbThread.use_count()>1 || liveBackgroundThreads>1);
       count++){
    QThread::msleep(mSleep);
  };
  return dbThread.use_count() == 1;
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

VoiceManagerRef OSMScoutQt::GetVoiceManager()
{
  if (!voiceManager){
    voiceManager = std::make_shared<VoiceManager>();
  }
  return voiceManager;
}

QThread *OSMScoutQt::makeThread(QString name)
{
  QThread *thread=new QThread();
  thread->setObjectName(name);
  QObject::connect(thread, &QThread::finished,
                   thread, &QThread::deleteLater);
  connect(thread, &QThread::finished,
          this, &OSMScoutQt::threadFinished);

  liveBackgroundThreads++;
  return thread;
}

void OSMScoutQt::threadFinished()
{
  liveBackgroundThreads--;
}

LookupModule* OSMScoutQt::MakeLookupModule()
{
  QThread *thread=makeThread("LookupModule");
  LookupModule *module=new LookupModule(thread,dbThread);
  module->moveToThread(thread);
  thread->start();
  return module;
}

SearchModule* OSMScoutQt::MakeSearchModule()
{
  QThread *thread=makeThread("SearchModule");
  SearchModule *module=new SearchModule(thread,dbThread,MakeLookupModule());
  module->moveToThread(thread);
  thread->start();
  return module;
}

StyleModule* OSMScoutQt::MakeStyleModule()
{
  QThread *thread=makeThread("StyleModule");
  StyleModule *module=new StyleModule(thread,dbThread);
  module->moveToThread(thread);
  thread->start();
  return module;
}

POILookupModule *OSMScoutQt::MakePOILookupModule()
{
  QThread *thread=makeThread("POILookupModule");
  POILookupModule *module=new POILookupModule(thread,dbThread);
  module->moveToThread(thread);
  thread->start();
  return module;
}

MapRenderer* OSMScoutQt::MakeMapRenderer(RenderingType type)
{
  QThread *thread=makeThread("MapRenderer");
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

  return mapRenderer;
}

Router* OSMScoutQt::MakeRouter()
{
  QThread *thread=makeThread("Router");

  Router *router=new Router(thread,settings,dbThread);
  router->moveToThread(thread);
  thread->start();
  return router;
}

NavigationModule* OSMScoutQt::MakeNavigation()
{
  QThread *thread=makeThread("Navigation");

  NavigationModule *navigation=new NavigationModule(thread,settings,dbThread);
  navigation->moveToThread(thread);
  thread->start();
  return navigation;
}

QString OSMScoutQt::GetUserAgent() const
{
  return userAgent;
}

QString OSMScoutQt::GetCacheLocation() const
{
  return cacheLocation;
}

size_t OSMScoutQt::GetOnlineTileCacheSize() const
{
  return onlineTileCacheSize;
}

QString OSMScoutQt::GetIconDirectory() const
{
  return iconDirectory;
}
}
