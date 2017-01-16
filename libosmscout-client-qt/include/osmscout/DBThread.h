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

#include <osmscout/LocationEntry.h>
#include <osmscout/Database.h>
#include <osmscout/LocationService.h>
#include <osmscout/MapService.h>
#include <osmscout/RoutingService.h>
#include <osmscout/RoutePostprocessor.h>

#include <osmscout/MapPainterQt.h>

#include <osmscout/util/Breaker.h>

#include <osmscout/Settings.h>
#include <osmscout/TileCache.h>
#include <osmscout/OsmTileDownloader.h>
#include <osmscout/MapManager.h>

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

/**
 * \ingroup QtAPI
 */
class OSMSCOUT_CLIENT_QT_API QBreaker : public osmscout::Breaker
{
private:
  mutable QMutex mutex;
  bool           aborted;

public:
  QBreaker();

  void Break();
  bool IsAborted() const;
  void Reset();
};

/**
 * \ingroup QtAPI
 */
class OSMSCOUT_CLIENT_QT_API StyleError
{
    enum StyleErrorType {
        Symbol, Error, Warning, Exception
    };

public:
    StyleError(StyleErrorType type, int line, int column, const QString &text) :
        type(type), line(line), column(column), text(text){}
    StyleError(QString msg);

    StyleErrorType GetType(){ return type; }
    QString GetTypeName() const;
    int GetLine(){ return line; }
    int GetColumn(){ return column; }
    const QString &GetText(){ return text; }
    QString GetDescription(){return GetTypeName()+": "+GetText();}

private:
    StyleErrorType  type;
    int             line;
    int             column;
    QString         text;
};

/**
 * \ingroup QtAPI
 * 
 * Instance of one osmscout database and database specific objects.
 */
class DBInstance : public QObject
{
  Q_OBJECT

public:
  QString                       path;
  osmscout::DatabaseRef         database;
  
  osmscout::LocationServiceRef  locationService;
  osmscout::MapServiceRef       mapService;
  osmscout::MapService::CallbackId callbackId;
  osmscout::BreakerRef          dataLoadingBreaker;  
  
  osmscout::RoutingServiceRef   router;

  osmscout::StyleConfigRef      styleConfig;
  osmscout::MapPainterQt        *painter;
  
  inline DBInstance(QString path,
                    osmscout::DatabaseRef database,
                    osmscout::LocationServiceRef locationService,
                    osmscout::MapServiceRef mapService,
                    osmscout::MapService::CallbackId callbackId,
                    osmscout::BreakerRef dataLoadingBreaker,
                    osmscout::StyleConfigRef styleConfig):
    path(path),
    database(database),
    locationService(locationService),
    mapService(mapService),
    callbackId(callbackId),
    dataLoadingBreaker(dataLoadingBreaker),
    styleConfig(styleConfig),
    painter(NULL)
  {
    if (styleConfig){
      painter = new osmscout::MapPainterQt(styleConfig);
    }   
  };
  
  inline ~DBInstance()
  {
    close();
    if (painter!=NULL) {
      delete painter;
    }
  };

  bool LoadStyle(QString stylesheetFilename,
                 std::unordered_map<std::string,bool> stylesheetFlags, 
                 QList<StyleError> &errors);
  
  bool AssureRouter(osmscout::Vehicle vehicle, 
                    osmscout::RouterParameter routerParameter);  

  void close();
};

typedef std::shared_ptr<DBInstance> DBInstanceRef;

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
  Q_OBJECT
  Q_PROPERTY(QString stylesheetFilename READ GetStylesheetFilename NOTIFY stylesheetFilenameChanged)
  
signals:
  void InitialisationFinished(const DatabaseLoadedResponse& response);
  void TriggerInitialRendering();
  void TriggerDrawMap();
  void Redraw();
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
  void ReloadStyle(const QString &suffix="");
  void LoadStyle(QString stylesheetFilename,
                 std::unordered_map<std::string,bool> stylesheetFlags,
                 const QString &suffix="");
  virtual void Initialize() = 0;
  virtual void InvalidateVisualCache() = 0;
  void onDatabaseListChanged(QList<QDir> databaseDirectories);
  void Finalize();

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

  virtual void onMapDPIChange(double dpi);
  virtual void onRenderSeaChanged(bool);  

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
  MapManagerRef                 mapManager;

  double                        mapDpi;
  double                        physicalDpi;

  mutable QMutex                mutex;
  
  osmscout::DatabaseParameter   databaseParameter;
  QList<DBInstanceRef>          databases;
  osmscout::RouterParameter     routerParameter;
  osmscout::RoutePostprocessor  routePostprocessor;

  QString                       stylesheetFilename;
  QString                       iconDirectory;
  std::unordered_map<std::string,bool>
                                stylesheetFlags;
  bool                          daylight;
  
  bool                          renderSea;

  bool                          renderError;
  QList<StyleError>             styleErrors;

protected:
  
  DBThread(QStringList databaseLookupDirectories, QString stylesheetFilename, QString iconDirectory);

  virtual ~DBThread();

  bool AssureRouter(osmscout::Vehicle vehicle);

  virtual void TileStateCallback(const osmscout::TileRef& changedTile);
 
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

  bool InitializeDatabases();

  void CancelCurrentDataLoading();

  bool isInitializedInternal();

public:
  bool isInitialized(); 
  
  const DatabaseLoadedResponse loadedResponse() const;

  DatabaseCoverage databaseCoverage(const osmscout::Magnification &magnification,
                                    const osmscout::GeoBox &bbox);

  double GetMapDpi() const;
  
  double GetPhysicalDpi() const;
  
  /**
   * Render map defined by request to painter 
   * @param painter
   * @param request
   * @return true if rendered map is complete 
   */
  virtual bool RenderMap(QPainter& painter,
                         const RenderMapRequest& request) = 0;
  
  bool CalculateRoute(const QString databasePath,
                      const osmscout::RoutingProfile& routingProfile,
                      const osmscout::RoutePosition& start,
                      const osmscout::RoutePosition target,
                      osmscout::RouteData& route);

  bool TransformRouteDataToRouteDescription(const QString databasePath,
                                            const osmscout::RoutingProfile& routingProfile,
                                            const osmscout::RouteData& data,
                                            osmscout::RouteDescription& description,
                                            const std::string& start,
                                            const std::string& target);

  bool TransformRouteDataToWay(const QString databasePath,
                               osmscout::Vehicle vehicle,
                               const osmscout::RouteData& data,
                               osmscout::Way& way);

  osmscout::RoutePosition GetClosestRoutableNode(const QString databasePath,
                                                 const osmscout::ObjectFileRef& refObject,
                                                 const osmscout::RoutingProfile& routingProfile,
                                                 double radius);

  void ClearRoute();
  void AddRoute(const osmscout::Way& way);
  osmscout::TypeConfigRef GetTypeConfig(const QString databasePath) const;
  
  inline MapManagerRef GetMapManager() const
  {
    return mapManager;
  }

  inline QString GetStylesheetFilename() const
  {
    return stylesheetFilename;
  }

  const QList<StyleError> &GetStyleErrors() const
  {
      return styleErrors;
  }  

  static QStringList BuildAdminRegionList(const osmscout::AdminRegionRef& adminRegion,
                                          std::map<osmscout::FileOffset,osmscout::AdminRegionRef> regionMap);
  
  static bool InitializeTiledInstance(QStringList databaseDirectory, 
                                      QString stylesheetFilename, 
                                      QString iconDirectory,
                                      QString tileCacheDirectory,
                                      size_t onlineTileCacheSize = 20, 
                                      size_t offlineTileCacheSize = 50);

  static bool InitializePlaneInstance(QStringList databaseDirectory, 
                                      QString stylesheetFilename, 
                                      QString iconDirectory);
  
  static DBThread* GetInstance();
  static void FreeInstance();
};

#endif
