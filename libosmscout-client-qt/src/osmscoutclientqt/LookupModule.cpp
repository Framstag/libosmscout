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

#include <osmscoutclientqt/LookupModule.h>
#include <osmscoutclientqt/OSMScoutQt.h>
#include <iostream>

namespace osmscout {

LookupModule::LookupModule(QThread *thread,DBThreadRef dbThread):
  QObject(),
  thread(thread),
  dbThread(dbThread),
  loadJob(nullptr)
{

  connect(dbThread.get(), &DBThread::initialisationFinished,
          this, &LookupModule::initialisationFinished);
}

LookupModule::~LookupModule()
{
  if (thread!=QThread::currentThread()){
    qWarning() << "Destroy" << this << "from non incorrect thread;" << thread << "!=" << QThread::currentThread();
  }
  if (thread!=nullptr){
    thread->quit();
  }
}

void LookupModule::requestObjectsOnView(const MapViewStruct &view,
                                        const QRectF &filterRectangle)
{
  // setup projection for data lookup
  osmscout::MercatorProjection lookupProjection;
  lookupProjection.Set(view.coord,  0, view.magnification, view.dpi, view.width*1.5, view.height*1.5);
  lookupProjection.SetLinearInterpolationUsage(view.magnification.GetLevel() >= 10);

  if (loadJob!=nullptr){
    delete loadJob;
  }

  unsigned long maximumAreaLevel=4;
  if (view.magnification.GetLevel() >= 15) {
    maximumAreaLevel=6;
  }

  loadJob=new DBLoadJob(lookupProjection,maximumAreaLevel,/* lowZoomOptimization */ true);
  this->view=view;
  this->filterRectangle=filterRectangle;

  connect(loadJob, &DBLoadJob::databaseLoaded,
          this, &LookupModule::onDatabaseLoaded);
  connect(loadJob, &DBLoadJob::finished,
          this, &LookupModule::onLoadJobFinished);

  dbThread->RunJob(loadJob);
}

void LookupModule::addObjectInfo(QList<ObjectInfo> &objectList, // output
                                 const NodeRef &n,
                                 const std::map<ObjectFileRef,LocationDescriptionService::ReverseLookupResult> &reverseLookupMap,
                                 const DBInstanceRef &db,
                                 std::map<osmscout::FileOffset,osmscout::AdminRegionRef> &regionMap)
{
  std::vector<osmscout::Point> nodes;
  nodes.emplace_back(0, n->GetCoords());
  addObjectInfo(objectList,
                "node",
                n->GetObjectFileRef(),
                nodes,
                n->GetCoords(),
                n,
                reverseLookupMap,
                db,
                regionMap);
}

void LookupModule::addObjectInfo(QList<ObjectInfo> &objectList, // output
                                 const WayRef &w,
                                 const std::map<ObjectFileRef,LocationDescriptionService::ReverseLookupResult> &reverseLookupMap,
                                 const DBInstanceRef &db,
                                 std::map<osmscout::FileOffset,osmscout::AdminRegionRef> &regionMap)
{
  addObjectInfo(objectList,
                "way",
                w->GetObjectFileRef(),
                w->nodes,
                w->GetBoundingBox().GetCenter(),
                w,
                reverseLookupMap,
                db,
                regionMap);
}

void LookupModule::addObjectInfo(QList<ObjectInfo> &objectList, // output
                                 const AreaRef &a,
                                 const std::map<ObjectFileRef,LocationDescriptionService::ReverseLookupResult> &reverseLookupMap,
                                 const DBInstanceRef &db,
                                 std::map<osmscout::FileOffset,osmscout::AdminRegionRef> &regionMap)
{
  for (const auto &ring:a->rings) {
    TypeInfoRef type=a->GetRingType(ring);
    if (type->GetIgnore()) {
      continue;
    }
    // Master ring don't contain nodes! Use intersection of all outer rings instead
    osmscout::GeoBox bbox=(ring.nodes.empty() ? a->GetBoundingBox() : ring.GetBoundingBox());

    addObjectInfo(objectList,
                  "area",
                  a->GetObjectFileRef(),
                  ring.nodes,
                  bbox.GetCenter(),
                  type,
                  ring.GetFeatureValueBuffer(),
                  reverseLookupMap,
                  db,
                  regionMap);
  }
}


void LookupModule::requestObjects(const LocationEntry &entry, bool reverseLookupAddresses)
{
  QList<ObjectInfo> objectList;

  dbThread->RunSynchronousJob(
    [&](const std::list<DBInstanceRef> &databases){
      for (const auto &db:databases) {
        if (db->path==entry.getDatabase()){

          std::set<osmscout::FileOffset>         areaOffsets;
          std::set<osmscout::FileOffset>         wayOffsets;
          std::set<osmscout::FileOffset>         nodeOffsets;

          for (const osmscout::ObjectFileRef &ref: entry.getReferences()){
            switch (ref.type){
              case osmscout::RefType::refArea:
                areaOffsets.insert(ref.offset);
                break;
              case osmscout::RefType::refWay:
                wayOffsets.insert(ref.offset);
                break;
              case osmscout::RefType::refNode:
                nodeOffsets.insert(ref.offset);
                break;
              default:
                break;
            }
          }

          qDebug() << "Lookup objects for location entry" << entry.getLabel() << ":"
                   << areaOffsets.size() << wayOffsets.size() << nodeOffsets.size()
                   << "(areas, ways, nodes) in database" << entry.getDatabase();

          MapData mapData;

          auto database=db->GetDatabase();
          database->GetAreasByOffset(areaOffsets, mapData.areas);
          database->GetWaysByOffset(wayOffsets, mapData.ways);
          database->GetNodesByOffset(nodeOffsets, mapData.nodes);

          std::map<ObjectFileRef,LocationDescriptionService::ReverseLookupResult> reverseLookupMap;
          std::map<osmscout::FileOffset,osmscout::AdminRegionRef> regionMap;
          if (reverseLookupAddresses) {
            std::list<ObjectFileRef> objects;
            objects.insert(objects.begin(), entry.getReferences().cbegin(), entry.getReferences().cend());
            std::list<LocationDescriptionService::ReverseLookupResult> reverseLookupResult;
            db->GetLocationDescriptionService()->ReverseLookupObjects(objects, reverseLookupResult);
            for (const auto &res:reverseLookupResult){
              reverseLookupMap[res.object]=res;
            }
          }

          for (auto const &n:mapData.nodes){
            addObjectInfo(objectList,n,reverseLookupMap,db,regionMap);
          }

          //std::cout << "ways:  " << d.ways.size() << std::endl;
          for (auto const &w:mapData.ways){
            addObjectInfo(objectList,w,reverseLookupMap,db,regionMap);
          }

          //std::cout << "areas: " << d.areas.size() << std::endl;
          for (auto const &a:mapData.areas){
            addObjectInfo(objectList,a,reverseLookupMap,db,regionMap);
          }
        }
      }
    }
  );
  objectList.size();
  emit objectsLoaded(entry, objectList);
}

void LookupModule::filterObjectInView(const osmscout::MapData &mapData,
                                      QList<ObjectInfo> &objectList)
{
  osmscout::MercatorProjection projection;
  projection.Set(view.coord, /* angle */ 0, view.magnification, view.dpi, view.width, view.height);
  projection.SetLinearInterpolationUsage(view.magnification.GetLevel() >= 10);

  //std::cout << "object near " << this->screenX << " " << this->screenY << ":" << std::endl;

  // TODO: add reverse lookup for objects on the view
  std::map<ObjectFileRef,LocationDescriptionService::ReverseLookupResult> reverseLookupMap;
  std::map<osmscout::FileOffset,osmscout::AdminRegionRef> regionMap;
  DBInstanceRef db;

  double x;
  double y;
  double x2;
  double y2;

  //std::cout << "nodes: " << d.nodes.size() << std::endl;
  for (auto const &n:mapData.nodes){
    projection.GeoToPixel(n->GetCoords(),x,y);
    if (filterRectangle.contains(x,y)){
      addObjectInfo(objectList, n, reverseLookupMap, db, regionMap);
    }
  }

  //std::cout << "ways:  " << d.ways.size() << std::endl;
  for (auto const &w:mapData.ways){
    // TODO: better detection
    osmscout::GeoBox bbox=w->GetBoundingBox();
    projection.GeoToPixel(bbox.GetMinCoord(),x,y);
    projection.GeoToPixel(bbox.GetMaxCoord(),x2,y2);
    if (filterRectangle.intersects(QRectF(QPointF(x,y),QPointF(x2,y2)))){
      addObjectInfo(objectList, w, reverseLookupMap, db, regionMap);
    }
  }

  //std::cout << "areas: " << d.areas.size() << std::endl;
  for (auto const &a:mapData.areas){
    // TODO: better detection
    osmscout::GeoBox bbox=a->GetBoundingBox();
    projection.GeoToPixel(bbox.GetMinCoord(),x,y);
    projection.GeoToPixel(bbox.GetMaxCoord(),x2,y2);
    if (filterRectangle.intersects(QRectF(QPointF(x,y),QPointF(x2,y2)))){
      addObjectInfo(objectList, a, reverseLookupMap, db, regionMap);
    }
  }
}

void LookupModule::onDatabaseLoaded(QString dbPath,QList<osmscout::TileRef> tiles)
{
  osmscout::MapData data;
  QList<ObjectInfo> objectList;
  loadJob->AddTileDataToMapData(dbPath,tiles,data);
  filterObjectInView(data,objectList);
  emit viewObjectsLoaded(view, objectList);
}

void LookupModule::onLoadJobFinished(QMap<QString,QMap<osmscout::TileKey,osmscout::TileRef>> /*tiles*/)
{
  emit viewObjectsLoaded(view, QList<ObjectInfo>());
}

void LookupModule::requestLocationDescription(const osmscout::GeoCoord location)
{
  QMutexLocker locker(&mutex);
  OSMScoutQt::GetInstance().GetDBThread()->RunSynchronousJob(
    [this,location](const std::list<DBInstanceRef> &databases){
      int count = 0;
      for (auto db:databases){
        osmscout::LocationDescription description;
        osmscout::GeoBox dbBox=db->GetDBGeoBox();
        if (!dbBox.Includes(location)){
          continue;
        }

        std::map<osmscout::FileOffset,osmscout::AdminRegionRef> regionMap;
        if (!db->GetLocationDescriptionService()->DescribeLocationByAddress(location, description)) {
          osmscout::log.Error() << "Error during generation of location description";
          continue;
        }

        if (description.GetAtAddressDescription()){
          count++;

          auto place = description.GetAtAddressDescription()->GetPlace();
          emit locationDescription(location, db->path, description,
                                   BuildAdminRegionList(db, place.GetAdminRegion(), regionMap));
        }

        if (!db->GetLocationDescriptionService()->DescribeLocationByPOI(location, description)) {
          osmscout::log.Error() << "Error during generation of location description";
          continue;
        }

        if (description.GetAtPOIDescription()){
          count++;

          auto place = description.GetAtPOIDescription()->GetPlace();
          emit locationDescription(location, db->path, description,
                                   BuildAdminRegionList(db, place.GetAdminRegion(), regionMap));
        }
      }

      emit locationDescriptionFinished(location);
    }
  );
}

AdminRegionInfoRef LookupModule::buildAdminRegionInfo(const DBInstanceRef &db,
                                                      const osmscout::AdminRegionRef &region){

  AdminRegionInfoRef info=std::make_shared<AdminRegionInfo>();
  info->adminRegion=region;
  info->adminLevel=-1;

  if (!db) {
    return info;
  }

  info->database=db->path;

  // read admin region features
  auto database=db->GetDatabase();
  osmscout::TypeConfigRef typeConfig=database->GetTypeConfig();
  osmscout::FeatureValueReader<osmscout::AdminLevelFeature,osmscout::AdminLevelFeatureValue> adminLevelReader(*typeConfig);

  osmscout::FeatureValueBufferRef objectFeatureBuff;
  osmscout::NodeRef               node;
  osmscout::WayRef                way;
  osmscout::AreaRef               area;

  osmscout::AdminLevelFeatureValue *adminLevelValue=nullptr;

  switch (region->object.GetType()){
    case osmscout::refNode:
      if (database->GetNodeByOffset(region->object.GetFileOffset(), node)) {
        adminLevelValue=adminLevelReader.GetValue(node->GetFeatureValueBuffer());
        info->type=QString::fromStdString(node->GetType()->GetName());
      }
      break;
    case osmscout::refWay:
      if (database->GetWayByOffset(region->object.GetFileOffset(), way)) {
        adminLevelValue=adminLevelReader.GetValue(way->GetFeatureValueBuffer());
        info->type=QString::fromStdString(way->GetType()->GetName());
      }
      break;
    case osmscout::refArea:
      if (database->GetAreaByOffset(region->object.GetFileOffset(), area)) {
        adminLevelValue=adminLevelReader.GetValue(area->GetFeatureValueBuffer());
        info->type=QString::fromStdString(area->GetType()->GetName());
      }
      break;
    case osmscout::refNone:
    default:
      /* do nothing */
      break;
  }

  if (adminLevelValue!=nullptr) {
    info->adminLevel = (int) adminLevelValue->GetAdminLevel();
  }

  return info;
}

void LookupModule::requestRegionLookup(const osmscout::GeoCoord location) {
  QMutexLocker locker(&mutex);

  OSMScoutQt::GetInstance().GetDBThread()->RunSynchronousJob(
    [this,location](const std::list<DBInstanceRef> &databases) {
      for (auto db:databases) {
        osmscout::GeoBox dbBox=db->GetDBGeoBox();
        if (!dbBox.Includes(location)){
          continue;
        }

        std::list<osmscout::LocationDescriptionService::ReverseLookupResult> result;

        if (db->GetLocationDescriptionService()->ReverseLookupRegion(location,result)){
          std::map<osmscout::FileOffset,AdminRegionInfoRef> adminRegionMap=adminRegionCache[db->path];
          AdminRegionInfoRef bottomAdminRegion; // admin region with highest admin level

          for (const auto &entry:result) {
            if (entry.adminRegion) {
              AdminRegionInfoRef adminRegionInfo;
              auto it=adminRegionMap.find(entry.adminRegion->dataOffset);
              if (it!=adminRegionMap.end()){
                adminRegionInfo=it->second;
              }else {
                adminRegionInfo=buildAdminRegionInfo(db, entry.adminRegion);
                adminRegionMap[entry.adminRegion->regionOffset]=adminRegionInfo;
              }
              if (bottomAdminRegion){
                if (adminRegionInfo->adminLevel > bottomAdminRegion->adminLevel){
                  bottomAdminRegion=adminRegionInfo;
                }
              }else{
                bottomAdminRegion=adminRegionInfo;
              }
            }
          }

          if (bottomAdminRegion) {
            QList<AdminRegionInfoRef> adminRegionList=BuildAdminRegionInfoList(bottomAdminRegion,
                                                                               adminRegionMap);

            // std::cout << "Region list:" << std::endl;
            // for (const auto &region:adminRegionList) {
            //   std::cout << "  " << region->adminLevel << ": " << region->name.toStdString() << std::endl;
            // }
            emit locationAdminRegions(location,
                                      adminRegionList);
          }
          adminRegionCache[db->path]=adminRegionMap;
        }
      }
    }
  );

  emit locationAdminRegionFinished(location);
}

QList<AdminRegionInfoRef> LookupModule::BuildAdminRegionInfoList(AdminRegionInfoRef &bottom,
                                                                 std::map<osmscout::FileOffset,AdminRegionInfoRef> &regionInfoMap){
  QList<AdminRegionInfoRef> result;
  result << bottom;
  osmscout::AdminRegionRef top=bottom->adminRegion;
  while (top && top->parentRegionOffset!=0){
    auto it=regionInfoMap.find(top->parentRegionOffset);
    if (it!=regionInfoMap.end()){
      result << it->second;
      top=it->second->adminRegion;
    }else{
      // we are expecting that we have admin region chain in cache
      osmscout::log.Warn() << "Admin region " << top->parentRegionOffset << " is not in cache!";
      top.reset();
    }
  }
  return result;
}

QList<AdminRegionInfoRef> LookupModule::BuildAdminRegionList(const DBInstanceRef &db,
                                                             const osmscout::AdminRegionRef& adminRegion,
                                                             std::map<osmscout::FileOffset,osmscout::AdminRegionRef> &regionMap)
{
  if (!adminRegion){
    return QList<AdminRegionInfoRef>();
  }

  QList<AdminRegionInfoRef> list;
  if (db) {
    db->GetLocationService()->ResolveAdminRegionHierachie(adminRegion, regionMap);
  }

  list << buildAdminRegionInfo(db, adminRegion);
  osmscout::FileOffset parentOffset = adminRegion->parentRegionOffset;
  while (parentOffset != 0){
    const auto &it = regionMap.find(parentOffset);
    if (it==regionMap.end()) {
      break;
    }
    const osmscout::AdminRegionRef region=it->second;
    list << buildAdminRegionInfo(db, region);
    parentOffset = region->parentRegionOffset;
  }
  return list;
}

QStringList LookupModule::AdminRegionNames(const QList<AdminRegionInfoRef> &regionList, bool useAltNames)
{
  QStringList result;

  QString previousName;
  QString name;
  for (const auto &region: regionList) {
    name = (useAltNames && !region->altName().empty()) ? region->qStringAltName() : region->qStringName();
    if (previousName != name) {
      result << name;
      previousName = name;
    }
  }
  return result;
}

QStringList LookupModule::IndexedAdminRegionNames(const QList<AdminRegionInfoRef> &regionList, bool useAltNames)
{
  QStringList result;
  result.reserve(12);
  for (int i=0; i<=11; i++) {
    result << "";
  }
  for (const auto &region:regionList) {
    if (region->adminLevel >= 0 && region->adminLevel < result.size()) {
      result[region->adminLevel] = (useAltNames && !region->altName().empty()) ? region->qStringAltName() : region->qStringName();
    }
  }
  return result;
}

}
