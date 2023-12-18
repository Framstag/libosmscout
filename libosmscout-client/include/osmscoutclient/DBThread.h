#ifndef OSMSCOUT_CLIENT_DBTHREAD_H
#define OSMSCOUT_CLIENT_DBTHREAD_H

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

#include <osmscoutclient/ClientImportExport.h>

#include <osmscout/db/BasemapDatabase.h>
#include <osmscout/db/Database.h>

#include <osmscout/location/LocationService.h>

#include <osmscout/async/Signal.h>
#include <osmscout/async/AsyncWorker.h>

#include <osmscoutmap/MapService.h>

#include <osmscoutclient/DBInstance.h>
#include <osmscoutclient/Settings.h>
#include <osmscoutclient/MapManager.h>

#include <shared_mutex>
#include <filesystem>
#include <map>
#include <string>

namespace osmscout {

/**
 * \ingroup ClientAPI
 */
struct MapViewStruct
{
  osmscout::GeoCoord      coord;
  osmscout::Bearing       angle; // canvas clockwise
  osmscout::Magnification magnification;
  size_t                  width;
  size_t                  height;
  double                  dpi;
};

inline bool operator!=(const MapViewStruct &r1, const MapViewStruct &r2)
{
    return r1.coord!=r2.coord ||
      r1.angle!=r2.angle ||
      r1.magnification!=r2.magnification ||
      r1.width!=r2.width ||
      r1.height!=r2.height ||
      int32_t(r1.dpi*1000)!=int32_t(r2.dpi*1000);
}

/**
 * \ingroup ClientAPI
 * \see DBThread::databaseCoverage
 */
enum class DatabaseCoverage{
  Outside = 0,
  Covered = 1,
  Intersects = 2,
};

/**
 * \ingroup ClientAPI
 *
 * Central object that manage db instances (\ref DBInstance),
 * its map styles (there is one global map style now)
 * and provides simple thread-safe, asynchronous api for accessing it.
 *
 * List of databases is protected by read-write lock. There may be multiple
 * readers at one time. DBThread warrants that none db will be closed
 * or modified (except thread-safe caches) when read lock is hold.
 * Databases may be accessed via \see AsynchronousDBJob or \see RunSynchronousJob methods.
 */
class OSMSCOUT_CLIENT_API DBThread: public AsyncWorker
{
public:
  using SynchronousDBJob = std::function<void (const std::list<DBInstanceRef> &)>;

  using AsynchronousDBJob = std::function<void (const osmscout::BasemapDatabaseRef& basemapDatabase,
                                                const std::list<DBInstanceRef> &databases,
                                                std::shared_lock<std::shared_mutex> &&locker)>;

  // signals
  Signal<> stylesheetFilenameChanged;
  Signal<osmscout::GeoBox> databaseLoadFinished;
  Signal<> styleErrorsChanged;

  // slots
  Slot<> toggleDaylight{
    std::bind(&DBThread::ToggleDaylight, this)
  };

  Slot<std::string, bool> setStyleFlag{
    std::bind(&DBThread::SetStyleFlag, this, std::placeholders::_1, std::placeholders::_2)
  };

  Slot<std::string> reloadStyle{
    std::bind(&DBThread::ReloadStyle, this, std::placeholders::_1)
  };

  Slot<std::string, std::unordered_map<std::string,bool>, std::string> loadStyle {
    std::bind(&DBThread::LoadStyle, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
  };

  Slot<> initialize{
    std::bind(&DBThread::Initialize, this)
  };

  Slot<std::vector<std::filesystem::path>> databaseListChangedSlot {
    std::bind(&DBThread::OnDatabaseListChanged, this, std::placeholders::_1)
  };

  /**
   * Flush all caches for db that was not used in recent idleMs
   */
  Slot<std::chrono::milliseconds> flushCaches {
    std::bind(&DBThread::FlushCaches, this, std::placeholders::_1)
  };

private:
  MapManagerRef                      mapManager;
  std::string                        basemapLookupDirectory;
  SettingsRef                        settings;

  double                             mapDpi;

  mutable std::shared_mutex          lock;

  osmscout::BasemapDatabaseParameter basemapDatabaseParameter;
  osmscout::BasemapDatabaseRef       basemapDatabase;
  osmscout::DatabaseParameter        databaseParameter;
  std::list<DBInstanceRef>           databases;

  TypeConfigRef                      emptyTypeConfig; // type config just with special and custom poi types
  StyleConfigRef                     emptyStyleConfig;

  std::string                        stylesheetFilename;
  std::string                        iconDirectory;
  std::unordered_map<std::string,bool>
                                     stylesheetFlags;
  bool                               daylight;

  std::list<StyleError>              styleErrors;

  std::vector<std::string>           customPoiTypes;

  Slot<double> mapDpiSlot {
    [this](const double &dpi) {
      OnMapDPIChange(dpi);
    }
  };

protected:

  /**
   * Check if DBThread is initialized without acquire lock
   *
   * @return true if all databases are open
   */
  bool isInitializedInternal();

  /**
   * Load stylesheet for all databases, write lock needs to be hold
   *
   * @param stylesheetFilename
   * @param stylesheetFlags
   * @param suffix
   */
  void LoadStyleInternal(const std::string &stylesheetFilename,
                         const std::unordered_map<std::string,bool> &stylesheetFlags,
                         const std::string &suffix="");

  void registerCustomPoiTypes(TypeConfigRef typeConfig) const;

  StyleConfigRef makeStyleConfig(TypeConfigRef typeConfig, bool suppressWarnings=false) const;

public:
  DBThread(const std::string &basemapLookupDirectory,
           const std::string &iconDirectory,
           SettingsRef settings,
           MapManagerRef mapManager,
           const std::vector<std::string> &customPoiTypes);

  ~DBThread() override;

  void Initialize();

  bool isInitialized();

  /** Return geographical bounding box contains all loaded databases.
   *
   * When there is no database, returned bounding box is invalid.
   *
   * @return geo box
   */
  const GeoBox databaseBoundingBox() const;

  /**
   * Test if some bounding box is covered by databases - fully, partially or not covered.
   * Database bounding box combined with water-index is used.
   *
   * @param magnification
   * @param bbox
   * @return DatabaseCoverage enum: Outside, Covered, Intersects
   */
  DatabaseCoverage databaseCoverage(const osmscout::Magnification &magnification,
                                    const osmscout::GeoBox &bbox);

  double GetMapDpi() const;

  double GetPhysicalDpi() const;

  std::string GetStylesheetFilename() const
  {
    return stylesheetFilename;
  }

  const std::list<StyleError> &GetStyleErrors() const
  {
      return styleErrors;
  }

  StyleConfigRef GetEmptyStyleConfig() const
  {
    std::shared_lock locker(lock);
    return emptyStyleConfig;
  }

  const std::map<std::string,bool> GetStyleFlags() const;

  /**
   * Submit asynchronous job that will retrieve list
   * of initialized databases and r-value reference to \ref std::shared_lock<std::shared_mutex>.
   * Job is responsible for releasing lock when its task
   * is finished.
   *
   * @param job
   */
  void RunJob(AsynchronousDBJob job);

  /**
   * Submit synchronous job (simple lambda function)
   * that will get access to list of initialized databases.
   * Database read lock is hold until job is running.
   * Lock is released automatically then. Database instances
   * should not be accessed after it.
   *
   * Example:
   * ```
   * dbThread->RunSynchronousJob(
   *   [&](const std::list<DBInstanceRef> &databases){
   *     // read data from databases...
   *   }
   * );
   * ```
   *
   * @param job
   */
  void RunSynchronousJob(SynchronousDBJob job);

  CancelableFuture<bool> FlushCaches(const std::chrono::milliseconds &idleMs);
  CancelableFuture<bool> OnDatabaseListChanged(const std::vector<std::filesystem::path> &databaseDirectories);
  CancelableFuture<bool> OnMapDPIChange(double dpi);
  CancelableFuture<bool> SetStyleFlag(const std::string &key, bool value);
  CancelableFuture<bool> LoadStyle(const std::string &stylesheetFilename,
                                   const std::unordered_map<std::string,bool> &stylesheetFlags,
                                   const std::string &suffix="");
  CancelableFuture<bool> ToggleDaylight();
  CancelableFuture<bool> ReloadStyle(const std::string &suffix="");
};

using DBThreadRef = std::shared_ptr<DBThread>;

}

#endif
