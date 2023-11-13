/*
  OSMScout - a Qt backend for libosmscout and libosmscout-map
  Copyright (C) 2010  Tim Teulings
  Copyright (C) 2016  Lukáš Karas

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

#include <osmscoutmap/MapService.h>

#include <osmscoutclientqt/DBThread.h>
#include <osmscoutclientqt/private/Config.h>
#include <osmscoutclientqt/MapDownloader.h>

#ifdef OSMSCOUT_HAVE_LIB_MARISA
#include <osmscout/db/TextSearchIndex.h>
#endif

#include <osmscout/log/Logger.h>

namespace osmscout {

DBThread::DBThread(QThread *backgroundThread,
                   QString basemapLookupDirectory,
                   QString iconDirectory,
                   SettingsRef settings,
                   MapManagerRef mapManager,
                   const std::vector<std::string> &customPoiTypes)
  : backgroundThread(backgroundThread),
    mapManager(mapManager),
    basemapLookupDirectory(basemapLookupDirectory),
    settings(settings),
    mapDpi(-1),
    iconDirectory(iconDirectory),
    daylight(true),
    customPoiTypes(customPoiTypes)
{
  double physicalDpi = settings->GetPhysicalDPI();
  osmscout::log.Debug() << "Reported screen DPI: " << physicalDpi;
  mapDpi = settings->GetMapDPI();
  osmscout::log.Debug() << "Map DPI override: " << mapDpi;

  stylesheetFilename=QString::fromStdString(settings->GetStyleSheetAbsoluteFile());
  stylesheetFlags=settings->GetStyleSheetFlags();
  osmscout::log.Debug() << "Using stylesheet: " << stylesheetFilename.toStdString();

  emptyTypeConfig=std::make_shared<TypeConfig>();
  registerCustomPoiTypes(emptyTypeConfig);
  emptyStyleConfig=makeStyleConfig(emptyTypeConfig, true);

  settings->mapDPIChange.Connect(mapDpiSlot);
  connect(this, &DBThread::mapDpiSignal,
          this, &DBThread::onMapDPIChange,
          Qt::QueuedConnection);

  connect(this, &DBThread::databaseListChanged,
          this, &DBThread::onDatabaseListChanged,
          Qt::QueuedConnection);

  mapManager->databaseListChanged.Connect(databaseListChangedSlot);
}

DBThread::~DBThread()
{
  std::unique_lock locker(lock);
  osmscout::log.Debug() << "DBThread::~DBThread()";

  mapDpiSlot.Disconnect();

  if (basemapDatabase) {
    basemapDatabase->Close();
    basemapDatabase=nullptr;
  }

  for (auto& db:databases){
    db->Close();
  }
  databases.clear();
  backgroundThread->quit(); // deleteLater() is invoked when thread is finished
}

bool DBThread::isInitializedInternal()
{
  return std::all_of(databases.begin(), databases.end(),
                     [](const auto &db) { return db->IsOpen(); });
}

bool DBThread::isInitialized()
{
  std::shared_lock locker(lock);
  return isInitializedInternal();
}

double DBThread::GetMapDpi() const
{
    return mapDpi;
}

double DBThread::GetPhysicalDpi() const
{
    return settings->GetPhysicalDPI();
}

const DatabaseLoadedResponse DBThread::loadedResponse() const {
  std::shared_lock locker(lock);
  DatabaseLoadedResponse response;
  for (const auto& db:databases){
    response.boundingBox.Include(db->GetDBGeoBox());
  }
  return response;
}

DatabaseCoverage DBThread::databaseCoverage(const osmscout::Magnification &magnification,
                                            const osmscout::GeoBox &bbox)
{
  std::shared_lock locker(lock);

  osmscout::GeoBox boundingBox;
  for (const auto &db:databases){
    boundingBox.Include(db->GetDBGeoBox());
  }
  if (boundingBox.IsValid()) {
    /*
    qDebug() << "Database bounding box: " <<
                QString::fromStdString( boundingBox.GetDisplayText()) <<
            " test bounding box: " <<
                QString::fromStdString( tileBoundingBox.GetDisplayText() );
     */

    if (boundingBox.Includes(bbox.GetMinCoord()) &&
        boundingBox.Includes(bbox.GetMaxCoord())) {

      // test if some db has full coverage for this box
      bool fullCoverage=false;
      for (const auto &db:databases){
        std::list<osmscout::GroundTile> groundTiles;
        if (!db->GetMapService()->GetGroundTiles(bbox,magnification,groundTiles)){
          break;
        }
        bool mayContainsUnknown=false;
        for (const auto &tile:groundTiles){
          if (tile.type==osmscout::GroundTile::unknown ||
              tile.type==osmscout::GroundTile::coast){
            mayContainsUnknown=true;
            break;
          }
        }
        if (!mayContainsUnknown){
          fullCoverage=true;
          break;
        }
      }

      return fullCoverage? DatabaseCoverage::Covered: DatabaseCoverage::Intersects;
    }
    if (boundingBox.Intersects(bbox)){
      return DatabaseCoverage::Intersects;
    }
  }
  return DatabaseCoverage::Outside;
}

void DBThread::Initialize()
{
  std::shared_lock locker(lock);
  mapManager->LookupDatabases();
}

void DBThread::onDatabaseListChanged(QList<QDir> databaseDirectories)
{
  std::unique_lock locker(lock);

  if (basemapDatabase) {
    basemapDatabase->Close();
    basemapDatabase=nullptr;
  }

  for (const auto& db:databases){
    db->Close();
  }
  databases.clear();
  osmscout::GeoBox boundingBox;

#if defined(HAVE_MMAP)
  if (sizeof(void*)<=4){
    // we are on 32 bit system probably, we have to be careful with mmap
    qint64 mmapQuota=1.5 * (1<<30); // 1.5 GiB
    QStringList mmapFiles;
    mmapFiles << "bounding.dat" << "router2.dat" << "types.dat" << "textregion.dat" << "textpoi.dat"
              << "textother.dat" << "areasopt.dat" << "areanode.idx" << "textloc.dat" << "water.idx"
              << "areaway.idx" << "waysopt.dat" << "intersections.idx" << "areaarea.idx" << "arearoute.idx"
              << "location.idx" << "intersections.dat";

    for (auto &databaseDirectory:databaseDirectories){
      for (auto &file:mmapFiles){
        mmapQuota-=QFileInfo(databaseDirectory, file).size();
      }
    }
    if (mmapQuota<0){
      qWarning() << "Database is too huge to be mapped";
    }

    qint64 nodesSize=0;
    for (auto &databaseDirectory:databaseDirectories){
      nodesSize+=QFileInfo(databaseDirectory, "nodes.dat").size();
    }
    if (mmapQuota-nodesSize<0){
      qWarning() << "Nodes data files can't be mmapped";
      databaseParameter.SetNodesDataMMap(false);
    }else{
      mmapQuota-=nodesSize;
    }

    qint64 areasSize=0;
    for (auto &databaseDirectory:databaseDirectories){
      areasSize+=QFileInfo(databaseDirectory, "areas.dat").size();
    }
    if (mmapQuota-areasSize<0){
      qWarning() << "Areas data files can't be mmapped";
      databaseParameter.SetAreasDataMMap(false);
    }else{
      mmapQuota-=areasSize;
    }

    qint64 waysSize=0;
    for (auto &databaseDirectory:databaseDirectories){
      waysSize+=QFileInfo(databaseDirectory, "ways.dat").size();
    }
    if (mmapQuota-waysSize<0){
      qWarning() << "Ways data files can't be mmapped";
      databaseParameter.SetWaysDataMMap(false);
    }else{
      mmapQuota-=waysSize;
    }

    qint64 routeSize=0;
    for (auto &databaseDirectory:databaseDirectories){
      routeSize+=QFileInfo(databaseDirectory, "route.dat").size();
    }
    if (mmapQuota-routeSize<0){
      qWarning() << "Route data files can't be mmapped";
      databaseParameter.SetRoutesDataMMap(false);
    }else{
      mmapQuota-=routeSize;
    }

    qint64 routerSize=0;
    for (auto &databaseDirectory:databaseDirectories){
      routerSize+=QFileInfo(databaseDirectory, "router.dat").size();
    }
    if (mmapQuota-routerSize<0){
      qWarning() << "Router data files can't be mmapped";
      databaseParameter.SetRouterDataMMap(false);
    }
  }
#endif

  if (!basemapLookupDirectory.isEmpty()) {
    osmscout::BasemapDatabaseRef database = std::make_shared<osmscout::BasemapDatabase>(basemapDatabaseParameter);

    if (database->Open(basemapLookupDirectory.toLocal8Bit().data())) {
      basemapDatabase=database;
      qDebug() << "Basemap found and loaded from '" << basemapLookupDirectory << "'...";
    }
    else {
      qWarning() << "Cannot open basemap db '" << basemapLookupDirectory << "'!";
    }
  }

  for (auto &databaseDirectory:databaseDirectories){
    osmscout::DatabaseRef database = std::make_shared<osmscout::Database>(databaseParameter);
    osmscout::StyleConfigRef styleConfig;
    if (database->Open(databaseDirectory.absolutePath().toLocal8Bit().data())) {
      osmscout::TypeConfigRef typeConfig=database->GetTypeConfig();

      if (typeConfig) {
        registerCustomPoiTypes(typeConfig);
        styleConfig=makeStyleConfig(typeConfig);
      }
      else {
        qWarning() << "TypeConfig invalid!";
        styleConfig=nullptr;
      }
    }
    else {
      qWarning() << "Cannot open db '" << databaseDirectory.absolutePath() << "'!";
      continue;
    }

    if (!database->GetBoundingBox(boundingBox)) {
      qWarning() << "Cannot read initial bounding box";
      database->Close();
      continue;
    }

    databases.push_back(std::make_shared<DBInstance>(databaseDirectory.absolutePath().toStdString(),
                                                     database,
                                                     std::make_shared<osmscout::LocationService>(database),
                                                     std::make_shared<osmscout::LocationDescriptionService>(database),
                                                     std::make_shared<osmscout::MapService>(database),
                                                     styleConfig));
  }

  emit databaseLoadFinished(boundingBox);
  emit stylesheetFilenameChanged();
}

void DBThread::registerCustomPoiTypes(osmscout::TypeConfigRef typeConfig) const
{
  for (const std::string &typeName:customPoiTypes){
    osmscout::TypeInfoRef typeInfo=std::make_shared<osmscout::TypeInfo>(typeName);
    typeInfo->SetInternal()
      .CanBeWay(true)
      .CanBeArea(true)
      .CanBeNode(true);

    osmscout::FeatureRef nameFeature = typeConfig->GetFeature(NameFeature::NAME);
    if (nameFeature) {
      typeInfo->AddFeature(nameFeature);
    }

    osmscout::FeatureRef colorFeature = typeConfig->GetFeature(ColorFeature::NAME);
    if (colorFeature) {
      typeInfo->AddFeature(colorFeature);
    }

    typeConfig->RegisterType(typeInfo);
  }
}

StyleConfigRef DBThread::makeStyleConfig(TypeConfigRef typeConfig, bool suppressWarnings) const
{
  osmscout::StyleConfigRef styleConfig=std::make_shared<osmscout::StyleConfig>(typeConfig);

  // setup flag overrides before load
  for (const auto& flag : stylesheetFlags) {
    styleConfig->AddFlag(flag.first,flag.second);
  }

  Log log=osmscout::log;
  if (suppressWarnings) {
    log.Warn(false);
  }

  if (!styleConfig->Load(stylesheetFilename.toLocal8Bit().data(), nullptr, false, log)) {
    qWarning() << "Cannot load style sheet '" << stylesheetFilename << "'!";
    styleConfig=nullptr;
  }

  return styleConfig;
}

void DBThread::ToggleDaylight()
{
  std::unique_lock locker(lock);

  if (!isInitializedInternal()) {
      return;
  }

  qDebug() << "Toggling daylight from " << daylight << " to " << !daylight << "...";
  daylight=!daylight;
  stylesheetFlags["daylight"] = daylight;

  LoadStyleInternal(stylesheetFilename, stylesheetFlags);
  qDebug() << "Toggling daylight done.";
}

void DBThread::onMapDPIChange(double dpi)
{
  {
    std::unique_lock locker(lock);
    mapDpi = dpi;
  }
}

void DBThread::SetStyleFlag(const QString &key, bool value)
{
  qDebug() << "SetStyleFlag" << key << "to" << value;
  std::unique_lock locker(lock);

  if (!isInitializedInternal()) {
      return;
  }

  stylesheetFlags[key.toStdString()] = value;
  LoadStyleInternal(stylesheetFilename, stylesheetFlags);
}

void DBThread::ReloadStyle(const QString &suffix)
{
  qDebug() << "Reloading style " << stylesheetFilename << suffix << "...";
  LoadStyle(stylesheetFilename, stylesheetFlags,suffix);
  qDebug() << "Reloading style done.";
}

void DBThread::LoadStyle(QString stylesheetFilename,
                         std::unordered_map<std::string,bool> stylesheetFlags,
                         const QString &suffix)
{
  std::unique_lock locker(lock);
  LoadStyleInternal(stylesheetFilename, stylesheetFlags, suffix);
}

void DBThread::LoadStyleInternal(QString stylesheetFilename,
                                 std::unordered_map<std::string,bool> stylesheetFlags,
                                 const QString &suffix)
{
  this->stylesheetFilename = stylesheetFilename;
  this->stylesheetFlags = stylesheetFlags;

  emptyStyleConfig=makeStyleConfig(emptyTypeConfig, true);

  bool prevErrs = !styleErrors.empty();
  styleErrors.clear();
  std::string file = (stylesheetFilename+suffix).toStdString();
  for (const auto& db: databases){
    log.Debug() << "Loading style " << file << "...";
    db->LoadStyle(file, stylesheetFlags, styleErrors);
    log.Debug() << "Loading style done";
  }
  if (prevErrs || (!styleErrors.empty())){
    log.Warn() << "Failed to load stylesheet" << file;
    emit styleErrorsChanged();
  }
  emit stylesheetFilenameChanged();
}

const QMap<QString,bool> DBThread::GetStyleFlags() const
{
  std::shared_lock locker(lock);
  QMap<QString,bool> flags;
  // add flag overrides
  for (const auto& flag : stylesheetFlags){
    flags[QString::fromStdString(flag.first)]=flag.second;
  }

  // add flags defined by stylesheet
  for (const auto &db:databases){
    if (!db->GetStyleConfig()) {
      continue;
    }

    auto styleFlags = db->GetStyleConfig()->GetFlags(); // iterate temporary container is UB!
    for (const auto& flag : styleFlags){
      if (!flags.contains(QString::fromStdString(flag.first))){
        flags[QString::fromStdString(flag.first)]=flag.second;
      }
    }
  }

  return flags;
}

void DBThread::FlushCaches(qint64 idleMs)
{
  RunSynchronousJob([idleMs](const std::list<DBInstanceRef> &dbs){
    for (const auto &db:dbs){
      if (db->LastUsageMs().count() > idleMs){
        auto database=db->GetDatabase();
        osmscout::log.Debug() << "Flushing caches for " << database->GetPath();
        database->DumpStatistics();
        database->FlushCache();
        db->GetMapService()->FlushTileCache();
      }
    }
  });
}

void DBThread::RunJob(DBJob *job)
{
  std::shared_lock locker(lock);
  if (!isInitializedInternal()){
    locker.unlock();
    osmscout::log.Warn() << "ignore request, dbs is not initialized";
    return;
  }
  job->Run(basemapDatabase,databases,std::move(locker));
}

void DBThread::RunSynchronousJob(SynchronousDBJob job)
{
  std::shared_lock locker(lock);
  if (!isInitializedInternal()){
    osmscout::log.Warn() << "ignore request, dbs is not initialized";
    return;
  }
  job(databases);
}
}
