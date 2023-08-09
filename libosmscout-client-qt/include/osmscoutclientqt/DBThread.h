#ifndef OSMSCOUT_CLIENT_QT_DBTHREAD_H
#define OSMSCOUT_CLIENT_QT_DBTHREAD_H

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

#include <QThread>
#include <QObject>
#include <QReadWriteLock>
#include <QDir>

#include <osmscout/db/BasemapDatabase.h>
#include <osmscout/db/Database.h>

#include <osmscout/location/LocationService.h>

#include <osmscoutmap/MapService.h>

#include <osmscoutclient/Settings.h>

#include <osmscoutclientqt/MapManager.h>
#include <osmscoutclientqt/DBInstance.h>
#include <osmscoutclientqt/DBJob.h>

namespace osmscout {

/**
 * \ingroup QtAPI
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
      r1.dpi!=r2.dpi;
}

/**
 * \ingroup QtAPI
 */
struct DatabaseLoadedResponse
{
    osmscout::GeoBox boundingBox;
};

/**
 * \ingroup QtAPI
 * \see DBThread::databaseCoverage
 */
enum DatabaseCoverage{
  Outside = 0,
  Covered = 1,
  Intersects = 2,
};

/**
 * \ingroup QtAPI
 *
 * Central object that manage db instances (\ref DBInstance),
 * its map styles (there is one global map style now)
 * and provides simple thread-safe, asynchronous api for accessing it.
 *
 * DBThread is de facto singleton, that is created and accessible by OSMScoutQt.
 *
 * List of databases is protected by read-write lock. There may be multiple
 * readers at one time. DBThread warrants that none db will be closed
 * or modified (except thread-safe caches) when read lock is hold.
 * Databases may be accessed via \see RunJob or \see RunSynchronousJob methods.
 */
class OSMSCOUT_CLIENT_QT_API DBThread : public QObject
{
  friend class OSMScoutQt; // accessing to protected constructor

  Q_OBJECT
  Q_PROPERTY(QString stylesheetFilename READ GetStylesheetFilename NOTIFY stylesheetFilenameChanged)

public:
  using SynchronousDBJob = std::function<void (const std::list<DBInstanceRef> &)>;

signals:
  void initialisationFinished(const DatabaseLoadedResponse& response);
  void stylesheetFilenameChanged();
  void databaseLoadFinished(osmscout::GeoBox boundingBox);
  void styleErrorsChanged();

  void mapDpiSignal(double);

public slots:
  void ToggleDaylight();
  void onMapDPIChange(double dpi);
  void SetStyleFlag(const QString &key, bool value);
  void ReloadStyle(const QString &suffix="");
  void LoadStyle(QString stylesheetFilename,
                 std::unordered_map<std::string,bool> stylesheetFlags,
                 const QString &suffix="");
  void Initialize();
  void onDatabaseListChanged(QList<QDir> databaseDirectories);

  /**
   * Flush all caches for db that was not used in recent idleMs
   */
  void FlushCaches(qint64 idleMs);

private:
  QThread                            *backgroundThread;
  MapManagerRef                      mapManager;
  QString                            basemapLookupDirectory;
  SettingsRef                        settings;

  double                             mapDpi;

  mutable QReadWriteLock             lock;

  osmscout::BasemapDatabaseParameter basemapDatabaseParameter;
  osmscout::BasemapDatabaseRef       basemapDatabase;
  osmscout::DatabaseParameter        databaseParameter;
  std::list<DBInstanceRef>           databases;

  TypeConfigRef                      emptyTypeConfig; // type config just with special and custom poi types
  StyleConfigRef                     emptyStyleConfig;

  QString                            stylesheetFilename;
  QString                            iconDirectory;
  std::unordered_map<std::string,bool>
                                     stylesheetFlags;
  bool                               daylight;

  bool                               renderError;
  QList<StyleError>                  styleErrors;

  std::vector<std::string>           customPoiTypes;

  Slot<double> mapDpiSlot{
    [this](const double &d) {
      mapDpiSignal(d);
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
  void LoadStyleInternal(QString stylesheetFilename,
                         std::unordered_map<std::string,bool> stylesheetFlags,
                         const QString &suffix="");

  void registerCustomPoiTypes(TypeConfigRef typeConfig) const;

  StyleConfigRef makeStyleConfig(TypeConfigRef typeConfig) const;

public:
  DBThread(QThread *backgroundThread,
           QString basemapLookupDirectory,
           QString iconDirectory,
           SettingsRef settings,
           MapManagerRef mapManager,
           const std::vector<std::string> &customPoiTypes);

  ~DBThread() override;

  bool isInitialized();

  const DatabaseLoadedResponse loadedResponse() const;

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

  QString GetStylesheetFilename() const
  {
    return stylesheetFilename;
  }

  const QList<StyleError> &GetStyleErrors() const
  {
      return styleErrors;
  }

  StyleConfigRef GetEmptyStyleConfig() const
  {
    QReadLocker locker(&lock);
    return emptyStyleConfig;
  }

  const QMap<QString,bool> GetStyleFlags() const;

  /**
   * Submit asynchronous job that will retrieve list
   * of initialized databases and pointer to \ref QReadLocker.
   * Job is responsible for releasing lock when its task
   * is finished.
   *
   * @param job
   */
  void RunJob(DBJob *job);

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

};

using DBThreadRef = std::shared_ptr<DBThread>;

}

Q_DECLARE_METATYPE(osmscout::MapViewStruct)
Q_DECLARE_METATYPE(osmscout::DatabaseLoadedResponse)

#endif
