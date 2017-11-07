#ifndef OSMSCOUT_CLIENT_QT_LOOKUPMODULE_H
#define OSMSCOUT_CLIENT_QT_LOOKUPMODULE_H

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
#include <QThread>
#include <osmscout/DBThread.h>

#include <osmscout/private/ClientQtImportExport.h>

class OSMSCOUT_CLIENT_QT_API AdminRegionInfo {
public:
  QString database;
  osmscout::AdminRegionRef adminRegion;
  QString name;
  QString altName;
  int adminLevel;
};

typedef std::shared_ptr<AdminRegionInfo> AdminRegionInfoRef;

/**
 * \ingroup QtAPI
 */
class OSMSCOUT_CLIENT_QT_API LookupModule:public QObject{
  Q_OBJECT

  friend class SearchModule;

public:

  /**
   * Common sence of administrative region levels.
   *
   * Note that levels may have different meaning in various countries...
   * https://wiki.openstreetmap.org/wiki/Tag:boundary%3Dadministrative
   */
  enum AdminRegionLevel {
    NA1              = 1,
    Country          = 2,
    Territory        = 3,
    State            = 4,
    NA5              = 5,
    Region           = 6,
    District         = 7,
    Town             = 8,
    Neighbourhood    = 9
  };

private:
  QMutex           mutex;
  QThread          *thread;
  DBThreadRef      dbThread;
  DBLoadJob        *loadJob;
  MapViewStruct view;
  std::map<QString,std::map<osmscout::FileOffset,AdminRegionInfoRef>> adminRegionCache;

signals:
  void initialisationFinished(const DatabaseLoadedResponse& response);
  void viewObjectsLoaded(const MapViewStruct&, const osmscout::MapData&);
  void objectsLoaded(const LocationEntry&, const osmscout::MapData&);

  void locationDescription(const osmscout::GeoCoord location,
                           const QString database,
                           const osmscout::LocationDescription description,
                           const QStringList regions);
  void locationDescriptionFinished(const osmscout::GeoCoord location);

  void locationAdminRegions(const osmscout::GeoCoord location,
                            QList<AdminRegionInfoRef> adminRegionList);
  void locationAdminRegionFinished(const osmscout::GeoCoord location);

public slots:
  void requestObjectsOnView(const MapViewStruct&);
  void requestObjects(const LocationEntry&);
  void onDatabaseLoaded(QString dbPath,QList<osmscout::TileRef> tiles);
  void onLoadJobFinished(QMap<QString,QMap<osmscout::TileId,osmscout::TileRef>> tiles);

  /**
   * Start retrieving place information based on objects on or near the location.
   *
   * LookupModule then emits locationDescription signals followed by locationDescriptionFinished.
   *
   * User of this function should use Qt::QueuedConnection for invoking
   * this slot, operation may generate IO load and may tooks long time.
   *
   * @param location
   */
  void requestLocationDescription(const osmscout::GeoCoord location);

  /**
   * Start retrieving list of place admin regions.
   *
   * LookupModule then emits locationAdminRegions signals followed by locationAdminRegionFinished.
   *
   * User of this function should use Qt::QueuedConnection for invoking
   * this slot, operation may generate IO load and may tooks long time.
   *
   * @param location
   */
  void requestRegionLookup(const osmscout::GeoCoord location);

public:
  LookupModule(QThread *thread,DBThreadRef dbThread);
  virtual ~LookupModule();

private:
  AdminRegionInfoRef buildAdminRegionInfo(DBInstanceRef &db,const osmscout::AdminRegionRef &region);

  QList<AdminRegionInfoRef> BuildAdminRegionInfoList(AdminRegionInfoRef &bottom,
                                                     std::map<osmscout::FileOffset,AdminRegionInfoRef> &regionInfoMap);

  static QStringList BuildAdminRegionList(const osmscout::LocationServiceRef& locationService,
                                          const osmscout::AdminRegionRef& adminRegion,
                                          std::map<osmscout::FileOffset,osmscout::AdminRegionRef> &regionMap);

  static QStringList BuildAdminRegionList(const osmscout::AdminRegionRef& adminRegion,
                                          std::map<osmscout::FileOffset,osmscout::AdminRegionRef> regionMap);
};

/**
 * \ingroup QtAPI
 */
typedef std::shared_ptr<LookupModule> LookupModuleRef;

#endif /* OSMSCOUT_CLIENT_QT_LOOKUPMODULE_H */

