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
#include <osmscout/LocationDescriptionService.h>

#include <osmscout/ClientQtImportExport.h>
#include <iostream>

namespace osmscout {

class OSMSCOUT_CLIENT_QT_API AdminRegionInfo {
public:
  QString database;
  osmscout::AdminRegionRef adminRegion;
  QString name;
  QString altName;
  QString type;
  int adminLevel;
};

typedef std::shared_ptr<AdminRegionInfo> AdminRegionInfoRef;

/**
 * \ingroup QtAPI
 */
class OSMSCOUT_CLIENT_QT_API LookupModule:public QObject{
  Q_OBJECT

  friend class SearchRunnable;

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

  class ObjectInfo {
  public:
    QString type; // node / way / area
    QString objectType;
    QString name;
    QString phone;
    QString website;
    QString addressNumber;
    LocationDescriptionService::ReverseLookupRef reverseLookupRef;
    QStringList adminRegionList;
    uint64_t id;
    osmscout::GeoCoord center;
    std::vector<osmscout::Point> points;
  };

private:
  QMutex           mutex;
  QThread          *thread;
  DBThreadRef      dbThread;
  DBLoadJob        *loadJob;
  MapViewStruct    view;
  QRectF           filterRectangle;
  std::map<QString,std::map<osmscout::FileOffset,AdminRegionInfoRef>> adminRegionCache;

signals:
  void initialisationFinished(const DatabaseLoadedResponse& response);
  void viewObjectsLoaded(const MapViewStruct&,
                         const QList<LookupModule::ObjectInfo> &objects);

  void objectsLoaded(const LocationEntry&,
                     const QList<LookupModule::ObjectInfo> &objects);

  void locationDescription(const osmscout::GeoCoord location,
                           const QString database,
                           const osmscout::LocationDescription description,
                           const QStringList regions);
  void locationDescriptionFinished(const osmscout::GeoCoord location);

  void locationAdminRegions(const osmscout::GeoCoord location,
                            QList<AdminRegionInfoRef> adminRegionList);
  void locationAdminRegionFinished(const osmscout::GeoCoord location);

public slots:
  void requestObjectsOnView(const MapViewStruct&, const QRectF &filterRectangle);
  void requestObjects(const LocationEntry&, bool reverseLookupAddresses);
  void onDatabaseLoaded(QString dbPath,QList<osmscout::TileRef> tiles);
  void onLoadJobFinished(QMap<QString,QMap<osmscout::TileKey,osmscout::TileRef>> tiles);

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

  void addObjectInfo(QList<ObjectInfo> &objectList, // output
                     QString type,
                     const ObjectFileRef &ref,
                     const std::vector<osmscout::Point> &points,
                     const osmscout::GeoCoord &center,
                     const osmscout::TypeInfoRef &objectType,
                     const osmscout::FeatureValueBuffer &features,
                     const std::map<ObjectFileRef,LocationDescriptionService::ReverseLookupResult> &reverseLookupMap,
                     LocationServiceRef &locationService,
                     std::map<osmscout::FileOffset,osmscout::AdminRegionRef> &regionMap)
  {
    ObjectInfo info;
    //std::cout << " - "<<type.toStdString()<<": " << o->GetType()->GetName() << " " << ref.GetName();

    info.type=type;
    info.objectType=QString::fromStdString(objectType->GetName());
    info.id=ref.GetFileOffset();

    const osmscout::NameFeatureValue *name=features.findValue<osmscout::NameFeatureValue>();
    if (name!=nullptr){
      info.name=QString::fromStdString(name->GetLabel(Locale(), 0));
    }
    const osmscout::PhoneFeatureValue *phone=features.findValue<osmscout::PhoneFeatureValue>();
    if (phone!=nullptr){
      info.phone=QString::fromStdString(phone->GetPhone());
    }
    const osmscout::WebsiteFeatureValue *website=features.findValue<osmscout::WebsiteFeatureValue>();
    if (website!=nullptr){
      info.website=QString::fromStdString(website->GetWebsite());
    }
    const osmscout::AddressFeatureValue *address=features.findValue<osmscout::AddressFeatureValue>();
    if (address!=nullptr){
      info.addressNumber=QString::fromStdString(address->GetAddress());
    }
    const auto &it=reverseLookupMap.find(ref);
    if (it!=reverseLookupMap.end()){
      info.adminRegionList=BuildAdminRegionList(locationService, it->second.adminRegion, regionMap);
      info.reverseLookupRef=std::make_shared<LocationDescriptionService::ReverseLookupResult>(it->second);
    }
    info.center=center;
    info.points=points;

    objectList << info;
  }

  template<class T> void addObjectInfo(QList<ObjectInfo> &objectList, // output
                                       QString type,
                                       const ObjectFileRef &ref,
                                       const std::vector<osmscout::Point> &points,
                                       const osmscout::GeoCoord &center,
                                       const T &o,
                                       const std::map<ObjectFileRef,LocationDescriptionService::ReverseLookupResult> &reverseLookupMap,
                                       LocationServiceRef &locationService,
                                       std::map<osmscout::FileOffset,osmscout::AdminRegionRef> &regionMap)
  {
    addObjectInfo(objectList,type,ref,points,center,o->GetType(),o->GetFeatureValueBuffer(),reverseLookupMap,locationService,regionMap);
  }

  void addObjectInfo(QList<ObjectInfo> &objectList, // output
                     const NodeRef &node,
                     const std::map<ObjectFileRef,LocationDescriptionService::ReverseLookupResult> &reverseLookupMap,
                     LocationServiceRef &locationService,
                     std::map<osmscout::FileOffset,osmscout::AdminRegionRef> &regionMap);

  void addObjectInfo(QList<ObjectInfo> &objectList, // output
                     const WayRef &way,
                     const std::map<ObjectFileRef,LocationDescriptionService::ReverseLookupResult> &reverseLookupMap,
                     LocationServiceRef &locationService,
                     std::map<osmscout::FileOffset,osmscout::AdminRegionRef> &regionMap);

  void addObjectInfo(QList<ObjectInfo> &objectList, // output
                     const AreaRef &area,
                     const std::map<ObjectFileRef,LocationDescriptionService::ReverseLookupResult> &reverseLookupMap,
                     LocationServiceRef &locationService,
                     std::map<osmscout::FileOffset,osmscout::AdminRegionRef> &regionMap);

  AdminRegionInfoRef buildAdminRegionInfo(DBInstanceRef &db,const osmscout::AdminRegionRef &region);

  QList<AdminRegionInfoRef> BuildAdminRegionInfoList(AdminRegionInfoRef &bottom,
                                                     std::map<osmscout::FileOffset,AdminRegionInfoRef> &regionInfoMap);

  static QStringList BuildAdminRegionList(const osmscout::LocationServiceRef& locationService,
                                          const osmscout::AdminRegionRef& adminRegion,
                                          std::map<osmscout::FileOffset,osmscout::AdminRegionRef> &regionMap);

  static QStringList BuildAdminRegionList(const osmscout::AdminRegionRef& adminRegion,
                                          std::map<osmscout::FileOffset,osmscout::AdminRegionRef> regionMap);

  void filterObjectInView(const osmscout::MapData &data,
                          QList<ObjectInfo> &objectList);
};

/**
 * \ingroup QtAPI
 */
typedef std::shared_ptr<LookupModule> LookupModuleRef;

}

#endif /* OSMSCOUT_CLIENT_QT_LOOKUPMODULE_H */
