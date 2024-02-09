/*
  This source is part of the libosmscout library
  Copyright (C) 2010  Tim Teulings
  Copyright (C) 2023  Lukáš Karas

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

#include <osmscoutclient/DBThread.h>
#include <osmscoutclient/private/Config.h>

#ifdef OSMSCOUT_HAVE_LIB_MARISA
#include <osmscout/db/TextSearchIndex.h>
#endif

#include <osmscout/log/Logger.h>

namespace osmscout {

DBThread::DBThread(const std::string &basemapLookupDirectory,
                   const std::string &iconDirectory,
                   SettingsRef settings,
                   MapManagerRef mapManager,
                   const std::vector<std::string> &customPoiTypes)
  : AsyncWorker("DBThread"),
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

  stylesheetFilename = settings->GetStyleSheetAbsoluteFile();
  stylesheetFlags=settings->GetStyleSheetFlags();
  osmscout::log.Debug() << "Using stylesheet: " << stylesheetFilename;

  emptyTypeConfig=std::make_shared<TypeConfig>();
  registerCustomPoiTypes(emptyTypeConfig);
  emptyStyleConfig=makeStyleConfig(emptyTypeConfig, true);

  settings->mapDPIChange.Connect(mapDpiSlot);
  mapManager->databaseListChanged.Connect(databaseListChangedSlot);
}

DBThread::~DBThread()
{
  WriteLock locker(latch);
  osmscout::log.Debug() << "DBThread::~DBThread()";

  mapDpiSlot.Disconnect();
  databaseListChangedSlot.Disconnect();

  if (basemapDatabase) {
    basemapDatabase->Close();
    basemapDatabase=nullptr;
  }

  for (auto& db:databases){
    db->Close();
  }
  databases.clear();
}

bool DBThread::isInitializedInternal()
{
  return std::all_of(databases.begin(), databases.end(),
                     [](const auto &db) { return db->IsOpen(); });
}

bool DBThread::isInitialized()
{
  ReadLock locker(latch);
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

const GeoBox DBThread::databaseBoundingBox() const {
    ReadLock locker(latch);
  GeoBox response;
  for (const auto& db:databases){
    response.Include(db->GetDBGeoBox());
  }
  return response;
}

DatabaseCoverage DBThread::databaseCoverage(const osmscout::Magnification &magnification,
                                            const osmscout::GeoBox &bbox)
{
  ReadLock locker(latch);

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
  mapManager->LookupDatabases();
}

CancelableFuture<bool> DBThread::OnDatabaseListChanged(const std::vector<std::filesystem::path> &databaseDirectories)
{
  return Async<bool>([this, databaseDirectories](const Breaker &breaker) -> bool {
    if (breaker.IsAborted()) {
      return false;
    }
    WriteLock locker(latch);

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
    constexpr bool haveMmap = true;
#else
    constexpr bool haveMmap = false;
#endif

    if constexpr (haveMmap && sizeof(void*)<=4){
      // we are on 32 bit system probably, we have to be careful with mmap
      int64_t mmapQuota=1.5 * (1<<30); // 1.5 GiB
      std::vector<std::string> mmapFiles{
        "bounding.dat", "router2.dat", "types.dat", "textregion.dat", "textpoi.dat",
        "textother.dat", "areasopt.dat", "areanode.idx", "textloc.dat", "water.idx",
        "areaway.idx", "waysopt.dat", "intersections.idx", "areaarea.idx", "arearoute.idx",
        "location.idx", "intersections.dat"
      };

      for (auto &databaseDirectory:databaseDirectories){
        for (auto &file:mmapFiles){
          mmapQuota -= GetFileSize((databaseDirectory / file).string());
        }
      }
      if (mmapQuota<0){
        log.Warn() << "Database is too huge to be mapped";
      }

      int64_t nodesSize=0;
      for (auto &databaseDirectory:databaseDirectories){
        nodesSize += GetFileSize((databaseDirectory / "nodes.dat").string());
      }
      if (mmapQuota-nodesSize<0){
        log.Warn() << "Nodes data files can't be mmapped";
        databaseParameter.SetNodesDataMMap(false);
      }else{
        mmapQuota-=nodesSize;
      }

      int64_t areasSize=0;
      for (auto &databaseDirectory:databaseDirectories){
        areasSize += GetFileSize((databaseDirectory / "areas.dat").string());
      }
      if (mmapQuota-areasSize<0){
        log.Warn() << "Areas data files can't be mmapped";
        databaseParameter.SetAreasDataMMap(false);
      }else{
        mmapQuota -= areasSize;
      }

      int64_t waysSize=0;
      for (auto &databaseDirectory:databaseDirectories){
        waysSize += GetFileSize((databaseDirectory / "ways.dat").string());
      }
      if (mmapQuota-waysSize<0){
        log.Warn() << "Ways data files can't be mmapped";
        databaseParameter.SetWaysDataMMap(false);
      }else{
        mmapQuota -= waysSize;
      }

      int64_t routeSize=0;
      for (auto &databaseDirectory:databaseDirectories){
        routeSize += GetFileSize((databaseDirectory /"route.dat").string());
      }
      if (mmapQuota-routeSize<0){
        log.Warn() << "Route data files can't be mmapped";
        databaseParameter.SetRoutesDataMMap(false);
      }else{
        mmapQuota -= routeSize;
      }

      int64_t routerSize=0;
      for (auto &databaseDirectory:databaseDirectories){
        routerSize += GetFileSize((databaseDirectory / "router.dat").string());
      }
      if (mmapQuota-routerSize<0){
        log.Warn() << "Router data files can't be mmapped";
        databaseParameter.SetRouterDataMMap(false);
      }
    }

    if (!basemapLookupDirectory.empty()) {
      osmscout::BasemapDatabaseRef database = std::make_shared<osmscout::BasemapDatabase>(basemapDatabaseParameter);

      if (database->Open(basemapLookupDirectory)) {
        basemapDatabase=database;
        log.Debug() << "Basemap found and loaded from '" << basemapLookupDirectory << "'...";
      }
      else {
        log.Warn() << "Cannot open basemap db '" << basemapLookupDirectory << "'!";
      }
    }

    for (auto &databaseDirectory:databaseDirectories){
      osmscout::DatabaseRef database = std::make_shared<osmscout::Database>(databaseParameter);
      osmscout::StyleConfigRef styleConfig;
      if (database->Open(databaseDirectory.string())) {
        osmscout::TypeConfigRef typeConfig=database->GetTypeConfig();

        if (typeConfig) {
          registerCustomPoiTypes(typeConfig);
          styleConfig=makeStyleConfig(typeConfig);
        }
        else {
          log.Warn() << "TypeConfig invalid!";
          styleConfig=nullptr;
        }
      }
      else {
        log.Warn() << "Cannot open db '" << databaseDirectory.string() << "'!";
        continue;
      }

      if (!database->GetBoundingBox(boundingBox)) {
        log.Warn() << "Cannot read initial bounding box";
        database->Close();
        continue;
      }

      databases.push_back(std::make_shared<DBInstance>(databaseDirectory.string(),
                                                       database,
                                                       std::make_shared<osmscout::LocationService>(database),
                                                       std::make_shared<osmscout::LocationDescriptionService>(database),
                                                       std::make_shared<osmscout::MapService>(database),
                                                       styleConfig));
    }

    databaseLoadFinished.Emit(boundingBox);

    return true;
  });
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

  if (!styleConfig->Load(stylesheetFilename, nullptr, false, log)) {
    log.Warn() << "Cannot load style sheet '" << stylesheetFilename << "'!";
    styleConfig=nullptr;
  }

  return styleConfig;
}

CancelableFuture<bool> DBThread::ToggleDaylight()
{
  return Async<bool>([this](const Breaker &) -> bool {
    WriteLock locker(latch);

    if (!isInitializedInternal()) {
      return false;
    }

    log.Debug() << "Toggling daylight from " << daylight << " to " << !daylight << "...";
    daylight=!daylight;
    stylesheetFlags["daylight"] = daylight;

    LoadStyleInternal(stylesheetFilename, stylesheetFlags);
    log.Debug() << "Toggling daylight done.";
    return true;
  });
}

CancelableFuture<bool> DBThread::OnMapDPIChange(double dpi)
{
  return Async<bool>([this, dpi](const Breaker&) -> bool{
    WriteLock locker(latch);
    mapDpi = dpi;
    return true;
  });
}

CancelableFuture<bool> DBThread::SetStyleFlag(const std::string &key, bool value)
{
  return Async<bool>([this, key, value](const Breaker&) -> bool{
    log.Debug() << "SetStyleFlag " << key << " to " << value;
    WriteLock locker(latch);

    if (!isInitializedInternal()) {
      return false;
    }

    stylesheetFlags[key] = value;
    LoadStyleInternal(stylesheetFilename, stylesheetFlags);

    return true;
  });
}

CancelableFuture<bool> DBThread::ReloadStyle(const std::string &suffix)
{
  return Async<bool>([this, suffix](const Breaker &) -> bool {
    log.Debug() << "Reloading style " << stylesheetFilename << suffix << "...";
    WriteLock locker(latch);
    LoadStyleInternal(stylesheetFilename, stylesheetFlags, suffix);
    log.Debug() << "Reloading style done.";
    return true;
  });
}

CancelableFuture<bool> DBThread::LoadStyle(const std::string &stylesheetFilename,
                                           const std::unordered_map<std::string,bool> &stylesheetFlags,
                                           const std::string &suffix)
{
  return Async<bool>([this, stylesheetFilename, stylesheetFlags, suffix](const Breaker&){
    WriteLock locker(latch);
    LoadStyleInternal(stylesheetFilename, stylesheetFlags, suffix);
    return true;
  });
}

void DBThread::LoadStyleInternal(const std::string &stylesheetFilename,
                                 const std::unordered_map<std::string,bool> &stylesheetFlags,
                                 const std::string &suffix)
{
  this->stylesheetFilename = stylesheetFilename;
  this->stylesheetFlags = stylesheetFlags;

  emptyStyleConfig=makeStyleConfig(emptyTypeConfig, true);

  bool prevErrs = !styleErrors.empty();
  styleErrors.clear();
  std::string file = stylesheetFilename+suffix;
  for (const auto& db: databases){
    log.Debug() << "Loading style " << file << "...";
    db->LoadStyle(file, stylesheetFlags, styleErrors);
    log.Debug() << "Loading style done";
  }
  if (prevErrs || (!styleErrors.empty())){
    log.Warn() << "Failed to load stylesheet" << file;
    styleErrorsChanged.Emit();
  }
  stylesheetFilenameChanged.Emit();
}

const std::map<std::string,bool> DBThread::GetStyleFlags() const
{
  ReadLock locker(latch);
  std::map<std::string,bool> flags;
  // add flag overrides
  for (const auto& flag : stylesheetFlags){
    flags[flag.first]=flag.second;
  }

  // add flags defined by stylesheet
  for (const auto &db:databases){
    if (!db->GetStyleConfig()) {
      continue;
    }

    auto styleFlags = db->GetStyleConfig()->GetFlags(); // iterate temporary container is UB!
    for (const auto& flag : styleFlags){
      if (flags.find(flag.first)==flags.end()){
        flags[flag.first]=flag.second;
      }
    }
  }

  return flags;
}

CancelableFuture<bool> DBThread::FlushCaches(const std::chrono::milliseconds &idleMs)
{
  return Async<bool>([this, idleMs](const Breaker& breaker) {
    if (breaker.IsAborted()) {
      return false;
    }
    bool result=true;
    RunSynchronousJob([&](const std::list<DBInstanceRef> &dbs){
      if (breaker.IsAborted()) {
        result = false;
        return;
      }

      for (const auto &db:dbs){
        if (db->LastUsageMs() > idleMs){
          auto database=db->GetDatabase();
          osmscout::log.Debug() << "Flushing caches for " << database->GetPath();
          database->DumpStatistics();
          database->FlushCache();
          db->GetMapService()->FlushTileCache();
        }
      }
    });
    return result;
  });
}

void DBThread::RunJob(AsynchronousDBJob job)
{
  ReadLock locker(latch);
  if (!isInitializedInternal()){
    locker.unlock();
    osmscout::log.Warn() << "ignore request, dbs is not initialized";
    return;
  }
  job(basemapDatabase,databases,std::move(locker));
}

void DBThread::RunSynchronousJob(SynchronousDBJob job)
{
  ReadLock locker(latch);
  if (!isInitializedInternal()){
    osmscout::log.Warn() << "ignore request, dbs is not initialized";
    return;
  }
  job(databases);
}
}
