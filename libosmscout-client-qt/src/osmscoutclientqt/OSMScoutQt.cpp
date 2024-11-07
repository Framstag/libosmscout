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

#include <osmscout/util/Distance.h>

#include <osmscoutmap/DataTileCache.h>

#include <osmscoutclient/Settings.h>
#include <osmscoutclient/DBThread.h>

#include <osmscoutclientqt/OSMScoutQt.h>
#include <osmscoutclientqt/QtSettingsStorage.h>
#include <osmscoutclientqt/QmlSettings.h>
#include <osmscoutclientqt/MapWidget.h>
#include <osmscoutclientqt/ElevationChartWidget.h>
#include <osmscoutclientqt/PlaneMapRenderer.h>
#include <osmscoutclientqt/TiledMapRenderer.h>
#include <osmscoutclientqt/OverlayObject.h>
#include <osmscoutclientqt/AvailableMapsModel.h>
#include <osmscoutclientqt/LocationInfoModel.h>
#include <osmscoutclientqt/MapDownloadsModel.h>
#include <osmscoutclientqt/MapObjectInfoModel.h>
#include <osmscoutclientqt/MapStyleModel.h>
#include <osmscoutclientqt/OnlineTileProviderModel.h>
#include <osmscoutclientqt/RoutingModel.h>
#include <osmscoutclientqt/SearchLocationModel.h>
#include <osmscoutclientqt/StyleFlagsModel.h>
#include <osmscoutclientqt/TiledMapOverlay.h>
#include <osmscoutclientqt/Router.h>
#include <osmscoutclientqt/NavigationModel.h>
#include <osmscoutclientqt/NearPOIModel.h>
#include <osmscoutclientqt/InstalledMapsModel.h>
#include <osmscoutclientqt/AvailableVoicesModel.h>
#include <osmscoutclientqt/InstalledVoicesModel.h>
#include <osmscoutclientqt/QmlRoutingProfile.h>
#include <osmscoutclientqt/SunriseSunset.h>
#include <osmscoutclientqt/OpeningHoursModel.h>

#include <QObject>
#include <QtGlobal>
#include <QMetaType>
#include <QQmlEngine>
#include <QStandardPaths>
#include <QGuiApplication>
#include <QScreen>
#include <QTemporaryFile>

#include <optional>

namespace osmscout {

static OSMScoutQt* osmScoutInstance=nullptr;

OSMScoutQtBuilder::OSMScoutQtBuilder()
{
  QString documentsLocation = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
  mapLookupDirectories << QDir::currentPath();
  mapLookupDirectories << QDir(documentsLocation).filePath("Maps");

  cacheLocation = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);

  voiceLookupDirectory = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QDir::separator() + "voices";
}

OSMScoutQtBuilder::~OSMScoutQtBuilder()
{
}

bool OSMScoutQtBuilder::Init()
{
  if (osmScoutInstance!=nullptr){
    return false;
  }

  /* Warning: Sailfish OS before version 2.0.1 reports incorrect DPI (100)
   *
   * Some DPI values:
   *
   * ~ 330 - Jolla tablet native
   *   242.236 - Jolla phone native
   *   130 - PC (24" FullHD)
   *   100 - Qt default (reported by SailfishOS < 2.0.1)
   */
  QScreen *srn=QGuiApplication::primaryScreen();
  double physicalDpi = srn ? (double)srn->physicalDotsPerInch() : 100;

  QLocale locale;
  QString defaultUnits;
  switch (locale.measurementSystem()){
    case QLocale::ImperialUSSystem:
    case QLocale::ImperialUKSystem:
      defaultUnits="imperial";
      break;
    case QLocale::MetricSystem:
    default:
      defaultUnits="metrics";
  }

  SettingsRef settings=std::make_shared<Settings>(std::make_shared<QtSettingsStorage>(settingsStorage), physicalDpi, defaultUnits.toStdString());
  settingsStorage = nullptr;

  // Provider files may be distributed as Qt resources. But Setting cannot read Qt resources,
  // so we make temporary copy of resource on standard filesystem (native file).
  std::vector<std::unique_ptr<QTemporaryFile>> resourceCopies;

  auto ProviderFiles = [&resourceCopies](const QStringList &list) -> std::vector<std::string> {
    std::vector<std::string> result;
    for (const auto &fileName: list) {
      QFile file(fileName);
      if (auto tmp=QTemporaryFile::createNativeFile(file); tmp!=nullptr) {
        resourceCopies.emplace_back(std::unique_ptr<QTemporaryFile>(tmp));
        result.push_back(tmp->fileName().toStdString());
      } else {
        result.push_back(fileName.toStdString());
      }
    }
    return result;
  };

  // load online tile providers
  if (!onlineTileProviders.isEmpty()){
    settings->loadOnlineTileProviders(ProviderFiles(onlineTileProviders));
  }
  if (!mapProviders.isEmpty()){
    settings->loadMapProviders(ProviderFiles(mapProviders));
  }
  if (!voiceProviders.isEmpty()){
    settings->loadVoiceProviders(ProviderFiles(voiceProviders));
  }

  // delete temporary files
  resourceCopies.clear();

  // setup style sheet
  if (styleSheetFileConfigured){
    settings->SetStyleSheetFile(styleSheetFile.toStdString());
  }
  if (styleSheetDirectoryConfigured){
    settings->SetStyleSheetDirectory(styleSheetDirectory.toStdString());
  }

  // setup voice
  settings->SetVoiceLookupDirectory(voiceLookupDirectory.toStdString());

  std::vector<std::filesystem::path> paths;
  for (const auto &dir: mapLookupDirectories) {
    paths.push_back(dir.toStdString());
  }
  MapManagerRef mapManager=std::make_shared<MapManager>(paths);

  QString userAgent=QString("%1/%2 libosmscout/%3 Qt/%4")
      .arg(appName).arg(appVersion)
      .arg(LIBOSMSCOUT_VERSION_STRING)
      .arg(qVersion());

  if(strcmp(qVersion(), QT_VERSION_STR) != 0) {
    qWarning() << "Runtime Qt version" << qVersion() << "is different to compile time version" << QT_VERSION_STR;
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
  // TODO: unify usage of osmscout namespace in signals and slots
  // register osmscout + standard types for usage in Qt signals/slots
  qRegisterMetaType<LocationEntryRef>("LocationEntryRef");
  qRegisterMetaType<osmscout::BreakerRef>("osmscout::BreakerRef");
  qRegisterMetaType<osmscout::Distance>("osmscout::Distance");
  qRegisterMetaType<osmscout::Bearing>("osmscout::Bearing");
  qRegisterMetaType<std::shared_ptr<osmscout::Bearing>>("std::shared_ptr<osmscout::Bearing>"); // TODO: use optional bearing
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
  qRegisterMetaType<size_t>("size_t");
  qRegisterMetaType<AdminRegionInfoRef>("AdminRegionInfoRef");
  qRegisterMetaType<QList<AdminRegionInfoRef>>("QList<AdminRegionInfoRef>");
  qRegisterMetaType<std::unordered_map<std::string,bool>>("std::unordered_map<std::string,bool>");
  qRegisterMetaType<QMap<QString,bool>>("QMap<QString,bool>");
  qRegisterMetaType<LocationEntry>("LocationEntry");
  qRegisterMetaType<OnlineTileProvider>("OnlineTileProvider");
  qRegisterMetaType<RouteStep>("RouteStep");
  qRegisterMetaType<std::list<RouteStep>>("std::list<RouteStep>");
  qRegisterMetaType<OverlayWay*>("OverlayWay*");
  qRegisterMetaType<std::shared_ptr<OverlayWay>>("std::shared_ptr<OverlayWay>");
  qRegisterMetaType<OverlayArea*>("OverlayArea*");
  qRegisterMetaType<OverlayNode*>("OverlayNode*");
  qRegisterMetaType<QList<LookupModule::ObjectInfo>>("QList<LookupModule::ObjectInfo>");
  qRegisterMetaType<ElevationModule::ElevationPoints>("ElevationModule::ElevationPoints");
  qRegisterMetaType<std::map<int,OverlayObjectRef>>("std::map<int,OverlayObjectRef>");
  qRegisterMetaType<MapIcon>("MapIcon");
  qRegisterMetaType<VoiceProvider>("VoiceProvider");
  qRegisterMetaType<MapProvider>("MapProvider");
  qRegisterMetaType<QmlRoutingProfileRef>("QmlRoutingProfileRef");
  qRegisterMetaType<VoicePlayer::PlaybackState>("VoicePlayer::PlaybackState");

  // register osmscout types for usage in QML
  qmlRegisterType<AvailableMapsModel>(uri, versionMajor, versionMinor, "AvailableMapsModel");
  qmlRegisterType<LocationEntry>(uri, versionMajor, versionMinor, "LocationEntry");
  qmlRegisterType<LocationInfoModel>(uri, versionMajor, versionMinor, "LocationInfoModel");
  qmlRegisterType<LocationListModel>(uri, versionMajor, versionMinor, "LocationListModel");
  qmlRegisterType<MapDownloadsModel>(uri, versionMajor, versionMinor, "MapDownloadsModel");
  qmlRegisterType<MapObjectInfoModel>(uri, versionMajor, versionMinor, "MapObjectInfoModel");
  qmlRegisterType<MapStyleModel>(uri, versionMajor, versionMinor, "MapStyleModel");
  qmlRegisterType<MapWidget>(uri, versionMajor, versionMinor, "Map");
  qmlRegisterType<ElevationChartWidget>(uri, versionMajor, versionMinor, "ElevationChart");
  qmlRegisterType<NavigationModel>(uri, versionMajor, versionMinor, "NavigationModel");
  qmlRegisterType<OnlineTileProviderModel>(uri, versionMajor, versionMinor, "OnlineTileProviderModel");
  qmlRegisterType<OverlayWay>(uri, versionMajor, versionMinor, "OverlayWay");
  qmlRegisterType<OverlayArea>(uri, versionMajor, versionMinor, "OverlayArea");
  qmlRegisterType<OverlayNode>(uri, versionMajor, versionMinor, "OverlayNode");
  qmlRegisterType<QmlSettings>(uri, versionMajor, versionMinor, "Settings");
  qmlRegisterType<QmlRoutingProfile>(uri, versionMajor, versionMinor, "RoutingProfile");
  qmlRegisterType<RouteStep>(uri, versionMajor, versionMinor, "RouteStep");
  qmlRegisterType<RoutingListModel>(uri, versionMajor, versionMinor, "RoutingListModel");
  qmlRegisterType<StyleFlagsModel>(uri, versionMajor, versionMinor, "StyleFlagsModel");
  qmlRegisterType<TiledMapOverlay>(uri, versionMajor, versionMinor, "TiledMapOverlay");
  qmlRegisterType<NearPOIModel>(uri, versionMajor, versionMinor, "NearPOIModel");
  qmlRegisterType<InstalledMapsModel>(uri, versionMajor, versionMinor, "InstalledMapsModel");
  qmlRegisterType<AvailableVoicesModel>(uri, versionMajor, versionMinor, "AvailableVoicesModel");
  qmlRegisterType<InstalledVoicesModel>(uri, versionMajor, versionMinor, "InstalledVoicesModel");
  qmlRegisterType<SunriseSunset>(uri, versionMajor, versionMinor, "SunriseSunset");
  qmlRegisterType<OpeningHoursModel>(uri, versionMajor, versionMinor, "OpeningHoursModel");
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

  dbThread=std::make_shared<DBThread>(basemapLookupDirectory.toStdString(),
                                      iconDirectory.toStdString(),
                                      settings,
                                      mapManager,
                                      customPoiTypeVector);

  dbThread->Initialize();

  // move itself to own event loop,
  // we need to receive slots
  // even when main loop is shutdown
  // QThread *thread=makeThread("OSMScoutQt");
  // this->moveToThread(thread);
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

MapDownloaderRef OSMScoutQt::GetMapDownloader()
{
  std::unique_lock<std::mutex> locker(mutex);
  if (!mapDownloader){
    mapDownloader = std::make_shared<MapDownloader>(mapManager, settings);
  }
  return mapDownloader;
}

VoiceManagerRef OSMScoutQt::GetVoiceManager()
{
  std::unique_lock<std::mutex> locker(mutex);
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
  return new POILookupModule(dbThread);
}

ElevationModule *OSMScoutQt::MakeElevationModule()
{
  QThread *thread=makeThread("ElevationModule");
  ElevationModule *module=new ElevationModule(thread,dbThread);
  module->moveToThread(thread);
  thread->start();
  return module;
}

IconLookup* OSMScoutQt::MakeIconLookup()
{
  QThread *thread=makeThread("IconLookup");
  IconLookup *iconLookup=new IconLookup(thread, dbThread, iconDirectory);
  iconLookup->moveToThread(thread);
  thread->start();
  return iconLookup;
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
