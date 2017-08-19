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

#include <QtGui>
#include <QThread>
#include <QMetaType>
#include <QMutex>
#include <QTime>
#include <QTimer>
#include <QReadWriteLock>

#include <osmscout/LocationEntry.h>
#include <osmscout/BasemapDatabase.h>
#include <osmscout/Database.h>
#include <osmscout/LocationService.h>
#include <osmscout/MapService.h>
#include <osmscout/MapPainterQt.h>

#include <osmscout/Settings.h>
#include <osmscout/TileCache.h>
#include <osmscout/OsmTileDownloader.h>
#include <osmscout/MapManager.h>
#include <osmscout/DBInstance.h>
#include <osmscout/DBJob.h>

/**
 * \ingroup QtAPI
 */
struct RenderMapRequest
{
  osmscout::GeoCoord      coord;
  double                  angle;
  osmscout::Magnification magnification;
  size_t                  width;
  size_t                  height;
};

Q_DECLARE_METATYPE(RenderMapRequest)

inline bool operator!=(const RenderMapRequest &r1, const RenderMapRequest &r2)
{
    return r1.coord!=r2.coord ||
      r1.angle!=r2.angle ||
      r1.magnification!=r2.magnification ||
      r1.width!=r2.width ||
      r1.height!=r2.height;
}

/**
 * \ingroup QtAPI
 */
struct DatabaseLoadedResponse
{
    osmscout::GeoBox boundingBox;
};

Q_DECLARE_METATYPE(DatabaseLoadedResponse)

enum DatabaseCoverage{
  Outside = 0,
  Covered = 1,
  Intersects = 2,
};

/**
 * \ingroup QtAPI
 *
 * Abstract object that manage osmscout database intances (\ref DBInstance)
 * and provides simple thread-safe, asynchronous api for it.
 * It don't provide map rendering, it is implented in its sublasses (\ref TiledDBThread
 * and \ref PlaneDBThread).
 *
 * DBThread is singleton, it should be initialized at application start by calling
 * static function DBThread::InitializeTiledInstance or DBThread::InitializePlaneInstance
 * (it depends what map redering implementation do you want to use).
 *
 * After initialization, it should be moved to some non gui thread, to be sure that
 * database operations will not block UI.
 *
 * ```
 * QThread thread;
 * DBThread* dbThread=DBThread::GetInstance();
 *
 * dbThread->connect(&thread, SIGNAL(started()), SLOT(Initialize()));
 * dbThread->connect(&thread, SIGNAL(finished()), SLOT(Finalize()));
 *
 * dbThread->moveToThread(&thread);
 * ```
 *
 * Before application exits, resources should be released by calling static function DBThread::FreeInstance.
 */
class OSMSCOUT_CLIENT_QT_API DBThread : public QObject
{
  friend class OSMScoutQt; // accessing to protected constructor

  Q_OBJECT
  Q_PROPERTY(QString stylesheetFilename READ GetStylesheetFilename NOTIFY stylesheetFilenameChanged)

public:
  typedef std::function<void(const std::list<DBInstanceRef>&)> SynchronousDBJob;

signals:
  void InitialisationFinished(const DatabaseLoadedResponse& response);
  void TriggerInitialRendering();
  void locationDescription(const osmscout::GeoCoord location,
                           const QString database,
                           const osmscout::LocationDescription description,
                           const QStringList regions);
  void locationDescriptionFinished(const osmscout::GeoCoord location);
  void stylesheetFilenameChanged();
  void databaseLoadFinished(osmscout::GeoBox boundingBox);
  void styleErrorsChanged();

  void searchResult(const QString searchPattern, const QList<LocationEntry>);

  void searchFinished(const QString searchPattern, bool error);

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
   * Start retrieving place informations based on objects on or near the location.
   *
   * DBThread then emits locationDescription signals followed by locationDescriptionFinished.
   *
   * User of this function should use Qt::QueuedConnection for invoking
   * this slot, operation may generate IO load and may tooks long time.
   *
   * @param location
   */
  void requestLocationDescription(const osmscout::GeoCoord location);

  /**
   * Start object search by some pattern.
   *
   * DBThread then emits searchResult signals followed by searchFinished
   * for this pattern.
   *
   * User of this function should use Qt::QueuedConnection for invoking
   * this slot, search may generate IO load and may tooks long time.
   *
   * Keep in mind that entries retrieved by searchResult signal can contains
   * duplicates, because search may use various databases and indexes.
   *
   * @param searchPattern
   * @param limit - suggested limit for count of retrieved entries from one database
   */
  void SearchForLocations(const QString searchPattern, int limit);

protected:
  QThread                            *backgroundThread;
  MapManagerRef                      mapManager;
  QString                            basemapLookupDirectory;
  SettingsRef                        settings;

  double                             mapDpi;
  double                             physicalDpi;

  mutable QReadWriteLock             lock;

  osmscout::BasemapDatabaseParameter basemapDatabaseParameter;
  osmscout::BasemapDatabaseRef       basemapDatabase;
  osmscout::DatabaseParameter        databaseParameter;
  std::list<DBInstanceRef>           databases;

  QString                            stylesheetFilename;
  QString                            iconDirectory;
  std::unordered_map<std::string,bool>
                                     stylesheetFlags;
  bool                               daylight;

  bool                               renderError;
  QList<StyleError>                  styleErrors;

protected:

  static QStringList BuildAdminRegionList(const osmscout::LocationServiceRef& locationService,
                                          const osmscout::AdminRegionRef& adminRegion,
                                          std::map<osmscout::FileOffset,osmscout::AdminRegionRef> regionMap);

  bool BuildLocationEntry(const osmscout::ObjectFileRef& object,
                          const QString title,
                          DBInstanceRef db,
                          std::map<osmscout::FileOffset,osmscout::AdminRegionRef> &adminRegionMap,
                          QList<LocationEntry> &locations
                          );
  bool BuildLocationEntry(const osmscout::LocationSearchResult::Entry &entry,
                          DBInstanceRef db,
                          std::map<osmscout::FileOffset,osmscout::AdminRegionRef> &adminRegionMap,
                          QList<LocationEntry> &locations
                          );

  bool GetObjectDetails(DBInstanceRef db, const osmscout::ObjectFileRef& object,
                        QString &typeName,
                        osmscout::GeoCoord& coordinates,
                        osmscout::GeoBox& bbox);

  void CancelCurrentDataLoading();

  bool isInitializedInternal();

public:
  DBThread(QThread *backgroundThread,
           QString basemapLookupDirectory,
           QStringList databaseLookupDirectories,
           QString iconDirectory,
           SettingsRef settings);

  virtual ~DBThread();

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

  inline MapManagerRef GetMapManager() const
  {
    return mapManager;
  }

  inline SettingsRef GetSettings() const
  {
    return settings;
  }

  inline QString GetStylesheetFilename() const
  {
    return stylesheetFilename;
  }

  const QList<StyleError> &GetStyleErrors() const
  {
      return styleErrors;
  }

  const QMap<QString,bool> GetStyleFlags() const;

  void RunJob(DBJob *job);
  void RunSynchronousJob(SynchronousDBJob job);

  static QStringList BuildAdminRegionList(const osmscout::AdminRegionRef& adminRegion,
                                          std::map<osmscout::FileOffset,osmscout::AdminRegionRef> regionMap);

};

typedef std::shared_ptr<DBThread> DBThreadRef;

#endif
