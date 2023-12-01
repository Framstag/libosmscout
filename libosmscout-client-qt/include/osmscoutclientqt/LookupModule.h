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

#include <iostream>

#include <QObject>
#include <QThread>
#include <QRectF>
#include <QMutex>

#include <osmscout/location/LocationDescriptionService.h>

#include <osmscout/feature/PhoneFeature.h>
#include <osmscout/feature/WebsiteFeature.h>
#include <osmscout/feature/OpeningHoursFeature.h>

#include <osmscoutclient/DBThread.h>

#include <osmscoutclientqt/ClientQtImportExport.h>
#include <osmscoutclientqt/LocationEntry.h>
#include <osmscoutclientqt/DBLoadJob.h>

namespace osmscout {

/**
 * \ingroup QtAPI
 */
class OSMSCOUT_CLIENT_QT_API LookupModule:public QObject{
  Q_OBJECT

  friend class SearchRunnable;
  friend class SearchLocationsRunnable;

public:

  /**
   * Common sense of administrative region levels.
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
    QString altLangName;
    QString phone;
    QString website;
    QString openingHours;
    QString addressNumber;
    LocationDescriptionService::ReverseLookupRef reverseLookupRef;
    QList<AdminRegionInfoRef> adminRegionList;
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

  Slot<osmscout::GeoBox> dbLoadedSlot{
    [this](const osmscout::GeoBox &b) {
      emit initialisationFinished(b);
    }
  };

signals:
  void initialisationFinished(const osmscout::GeoBox& response);
  void viewObjectsLoaded(const MapViewStruct&,
                         const QList<LookupModule::ObjectInfo> &objects);

  void objectsLoaded(const LocationEntry&,
                     const QList<LookupModule::ObjectInfo> &objects);

  void locationDescription(const osmscout::GeoCoord location,
                           const QString database,
                           const osmscout::LocationDescription description,
                           const QList<AdminRegionInfoRef> regions);
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
  ~LookupModule();

  /** Helper method that returns list of unique admin region names.
   * When region list contains two (following) administrative regions
   * with the same name (for example Prague "district", Prague "city"),
   * it will return such name just once.
   *
   * @param regionList
   * @return list of admin region names
   */
  static QStringList AdminRegionNames(const QList<AdminRegionInfoRef> &regionList, bool useAltNames);

  /** Helper method that returns names of admin region in indexed array.
   * Array length is 12. When some level is not present, empty string is used.
   * Level 2 are counties. Levels > 2 are country specific.
   * See https://wiki.openstreetmap.org/wiki/Tag:boundary%3Dadministrative for meaning
   * of individual levels.
   *
   * @param regionList
   * @param useAltNames
   * @return list of admin region names, indexed by admin region level
   */
  static QStringList IndexedAdminRegionNames(const QList<AdminRegionInfoRef> &regionList, bool useAltNames);

private:

  void addObjectInfo(QList<ObjectInfo> &objectList, // output
                     QString type,
                     const ObjectFileRef &ref,
                     const std::vector<osmscout::Point> &points,
                     const osmscout::GeoCoord &center,
                     const osmscout::TypeInfoRef &objectType,
                     const osmscout::FeatureValueBuffer &features,
                     const std::map<ObjectFileRef,LocationDescriptionService::ReverseLookupResult> &reverseLookupMap,
                     const DBInstanceRef &db,
                     std::map<osmscout::FileOffset,osmscout::AdminRegionRef> &regionMap)
  {
    ObjectInfo info;
    //std::cout << " - "<<type.toStdString()<<": " << o->GetType()->GetName() << " " << ref.GetName();

    info.type=type;
    info.objectType=QString::fromStdString(objectType->GetName());
    info.id=ref.GetFileOffset();

    if (const osmscout::NameFeatureValue *name=features.findValue<osmscout::NameFeatureValue>(); name!=nullptr){
      info.name=QString::fromStdString(name->GetLabel(Locale(), 0));
    }

    if (const osmscout::NameAltFeatureValue *nameAlt=features.findValue<osmscout::NameAltFeatureValue>(); nameAlt!=nullptr){
      info.altLangName=QString::fromStdString(nameAlt->GetLabel(Locale(), 0));
    }

    if (const osmscout::PhoneFeatureValue *phone=features.findValue<osmscout::PhoneFeatureValue>(); phone!=nullptr){
      info.phone=QString::fromStdString(phone->GetPhone());
    }

    if (const osmscout::WebsiteFeatureValue *website=features.findValue<osmscout::WebsiteFeatureValue>(); website!=nullptr){
      info.website=QString::fromStdString(website->GetWebsite());
    }

    if (const osmscout::OpeningHoursFeatureValue *openingHours=features.findValue<osmscout::OpeningHoursFeatureValue>(); openingHours!=nullptr){
      info.openingHours=QString::fromStdString(openingHours->GetValue());
    }

    if (const osmscout::AddressFeatureValue *address=features.findValue<osmscout::AddressFeatureValue>(); address!=nullptr){
      info.addressNumber=QString::fromStdString(address->GetAddress());
    }

    if (const auto &it=reverseLookupMap.find(ref); it!=reverseLookupMap.end()){
      info.adminRegionList=BuildAdminRegionList(db, it->second.adminRegion, regionMap);
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
                                       const DBInstanceRef &db,
                                       std::map<osmscout::FileOffset,osmscout::AdminRegionRef> &regionMap)
  {
    addObjectInfo(objectList,type,ref,points,center,o->GetType(),o->GetFeatureValueBuffer(),reverseLookupMap,db,regionMap);
  }

  void addObjectInfo(QList<ObjectInfo> &objectList, // output
                     const NodeRef &node,
                     const std::map<ObjectFileRef,LocationDescriptionService::ReverseLookupResult> &reverseLookupMap,
                     const DBInstanceRef &db,
                     std::map<osmscout::FileOffset,osmscout::AdminRegionRef> &regionMap);

  void addObjectInfo(QList<ObjectInfo> &objectList, // output
                     const WayRef &way,
                     const std::map<ObjectFileRef,LocationDescriptionService::ReverseLookupResult> &reverseLookupMap,
                     const DBInstanceRef &db,
                     std::map<osmscout::FileOffset,osmscout::AdminRegionRef> &regionMap);

  void addObjectInfo(QList<ObjectInfo> &objectList, // output
                     const AreaRef &area,
                     const std::map<ObjectFileRef,LocationDescriptionService::ReverseLookupResult> &reverseLookupMap,
                     const DBInstanceRef &db,
                     std::map<osmscout::FileOffset,osmscout::AdminRegionRef> &regionMap);

  static AdminRegionInfoRef buildAdminRegionInfo(const DBInstanceRef &db,const osmscout::AdminRegionRef &region);

  static QList<AdminRegionInfoRef> BuildAdminRegionInfoList(AdminRegionInfoRef &bottom,
                                                            std::map<osmscout::FileOffset,AdminRegionInfoRef> &regionInfoMap);

  static QList<AdminRegionInfoRef> BuildAdminRegionList(const DBInstanceRef &db,
                                                        const osmscout::AdminRegionRef& adminRegion,
                                                        std::map<osmscout::FileOffset,osmscout::AdminRegionRef> &regionMap);

  void filterObjectInView(const osmscout::MapData &data,
                          QList<ObjectInfo> &objectList);
};

/**
 * \ingroup QtAPI
 */
typedef std::shared_ptr<LookupModule> LookupModuleRef;

}

#endif /* OSMSCOUT_CLIENT_QT_LOOKUPMODULE_H */
