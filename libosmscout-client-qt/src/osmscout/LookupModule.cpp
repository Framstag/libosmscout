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

#include <osmscout/LookupModule.h>
#include <osmscout/OSMScoutQt.h>
#include <iostream>

LookupModule::LookupModule(QThread *thread,DBThreadRef dbThread):
  QObject(),
  thread(thread),
  dbThread(dbThread),
  loadJob(NULL)
{

  connect(dbThread.get(), SIGNAL(initialisationFinished(const DatabaseLoadedResponse&)),
          this, SIGNAL(initialisationFinished(const DatabaseLoadedResponse&)));
}

LookupModule::~LookupModule()
{
  if (thread!=QThread::currentThread()){
    qWarning() << "Destroy" << this << "from non incorrect thread;" << thread << "!=" << QThread::currentThread();
  }
  if (thread!=NULL){
    thread->quit();
  }
}

void LookupModule::requestObjectsOnView(const MapViewStruct &view)
{
  double mapDpi=dbThread->GetMapDpi();

  // setup projection for data lookup
  osmscout::MercatorProjection lookupProjection;
  lookupProjection.Set(view.coord,  0, view.magnification, mapDpi, view.width*1.5, view.height*1.5);
  lookupProjection.SetLinearInterpolationUsage(view.magnification.GetLevel() >= 10);

  if (loadJob!=NULL){
    delete loadJob;
  }

  unsigned long maximumAreaLevel=4;
  if (view.magnification.GetLevel() >= 15) {
    maximumAreaLevel=6;
  }

  loadJob=new DBLoadJob(lookupProjection,maximumAreaLevel,/* lowZoomOptimization */ true);
  this->view=view;

  connect(loadJob, SIGNAL(databaseLoaded(QString,QList<osmscout::TileRef>)),
          this, SLOT(onDatabaseLoaded(QString,QList<osmscout::TileRef>)));
  connect(loadJob, SIGNAL(finished(QMap<QString,QMap<osmscout::TileId,osmscout::TileRef>>)),
          this, SLOT(onLoadJobFinished(QMap<QString,QMap<osmscout::TileId,osmscout::TileRef>>)));

  dbThread->RunJob(loadJob);
}

void LookupModule::requestObjects(const LocationEntry &entry)
{
  osmscout::MapData mapData;

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

          db->database->GetAreasByOffset(areaOffsets, mapData.areas);
          db->database->GetWaysByOffset(wayOffsets, mapData.ways);
          db->database->GetNodesByOffset(nodeOffsets, mapData.nodes);
        }
      }
    }
  );

  emit objectsLoaded(entry, mapData);
}

void LookupModule::onDatabaseLoaded(QString dbPath,QList<osmscout::TileRef> tiles)
{
  osmscout::MapData data;
  loadJob->AddTileDataToMapData(dbPath,tiles,data);
  emit viewObjectsLoaded(view, data);
}

void LookupModule::onLoadJobFinished(QMap<QString,QMap<osmscout::TileId,osmscout::TileRef>> /*tiles*/)
{
  emit viewObjectsLoaded(view, osmscout::MapData());
}

void LookupModule::requestLocationDescription(const osmscout::GeoCoord location)
{
  QMutexLocker locker(&mutex);
  OSMScoutQt::GetInstance().GetDBThread()->RunSynchronousJob(
    [this,location](const std::list<DBInstanceRef> &databases){
      int count = 0;
      for (auto db:databases){
        osmscout::LocationDescription description;
        osmscout::GeoBox dbBox;
        if (!db->database->GetBoundingBox(dbBox)){
          continue;
        }
        if (!dbBox.Includes(location)){
          continue;
        }

        std::map<osmscout::FileOffset,osmscout::AdminRegionRef> regionMap;
        if (!db->locationDescriptionService->DescribeLocationByAddress(location, description)) {
          osmscout::log.Error() << "Error during generation of location description";
          continue;
        }

        if (description.GetAtAddressDescription()){
          count++;

          auto place = description.GetAtAddressDescription()->GetPlace();
          emit locationDescription(location, db->path, description,
                                   BuildAdminRegionList(db->locationService, place.GetAdminRegion(), regionMap));
        }

        if (!db->locationDescriptionService->DescribeLocationByPOI(location, description)) {
          osmscout::log.Error() << "Error during generation of location description";
          continue;
        }

        if (description.GetAtPOIDescription()){
          count++;

          auto place = description.GetAtPOIDescription()->GetPlace();
          emit locationDescription(location, db->path, description,
                                   BuildAdminRegionList(db->locationService, place.GetAdminRegion(), regionMap));
        }
      }

      emit locationDescriptionFinished(location);
    }
  );
}

AdminRegionInfoRef LookupModule::buildAdminRegionInfo(DBInstanceRef &db,
                                                      const osmscout::AdminRegionRef &region){

  AdminRegionInfoRef info=std::make_shared<AdminRegionInfo>();
  info->database=db->path;
  info->adminRegion=region;
  info->name=QString::fromStdString(region->name);
  info->adminLevel=-1;

  // read admin region features
  osmscout::TypeConfigRef typeConfig=db->database->GetTypeConfig();
  osmscout::FeatureValueReader<osmscout::NameAltFeature,osmscout::NameAltFeatureValue> altNameReader(*typeConfig);
  osmscout::FeatureValueReader<osmscout::AdminLevelFeature,osmscout::AdminLevelFeatureValue> adminLevelReader(*typeConfig);

  osmscout::FeatureValueBufferRef objectFeatureBuff;
  osmscout::NodeRef               node;
  osmscout::WayRef                way;
  osmscout::AreaRef               area;

  osmscout::NameAltFeatureValue    *altNameValue=NULL;
  osmscout::AdminLevelFeatureValue *adminLevelValue=NULL;

  switch (region->object.GetType()){
    case osmscout::refNode:
      if (db->database->GetNodeByOffset(region->object.GetFileOffset(), node)) {
        altNameValue=altNameReader.GetValue(node->GetFeatureValueBuffer());
        adminLevelValue=adminLevelReader.GetValue(node->GetFeatureValueBuffer());
        info->type=QString::fromStdString(node->GetType()->GetName());
      }
      break;
    case osmscout::refWay:
      if (db->database->GetWayByOffset(region->object.GetFileOffset(), way)) {
        altNameValue=altNameReader.GetValue(way->GetFeatureValueBuffer());
        adminLevelValue=adminLevelReader.GetValue(way->GetFeatureValueBuffer());
        info->type=QString::fromStdString(way->GetType()->GetName());
      }
      break;
    case osmscout::refArea:
      if (db->database->GetAreaByOffset(region->object.GetFileOffset(), area)) {
        altNameValue=altNameReader.GetValue(area->GetFeatureValueBuffer());
        adminLevelValue=adminLevelReader.GetValue(area->GetFeatureValueBuffer());
        info->type=QString::fromStdString(area->GetType()->GetName());
      }
      break;
    case osmscout::refNone:
    default:
      /* do nothing */
      break;
  }

  if (altNameValue!=NULL)
    info->altName=QString::fromStdString(altNameValue->GetNameAlt());
  if (adminLevelValue!=NULL)
    info->adminLevel=(int)adminLevelValue->GetAdminLevel();

  return info;
}

void LookupModule::requestRegionLookup(const osmscout::GeoCoord location) {
  QMutexLocker locker(&mutex);

  OSMScoutQt::GetInstance().GetDBThread()->RunSynchronousJob(
    [this,location](const std::list<DBInstanceRef> &databases) {
      for (auto db:databases) {
        osmscout::GeoBox dbBox;
        if (!db->database->GetBoundingBox(dbBox)){
          continue;
        }
        if (!dbBox.Includes(location)){
          continue;
        }

        std::list<osmscout::LocationDescriptionService::ReverseLookupResult> result;

        if (db->locationDescriptionService->ReverseLookupRegion(location,result)){
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

QStringList LookupModule::BuildAdminRegionList(const osmscout::AdminRegionRef& adminRegion,
                                               std::map<osmscout::FileOffset,osmscout::AdminRegionRef> regionMap)
{
  return BuildAdminRegionList(osmscout::LocationServiceRef(), adminRegion, regionMap);
}

QStringList LookupModule::BuildAdminRegionList(const osmscout::LocationServiceRef& locationService,
                                               const osmscout::AdminRegionRef& adminRegion,
                                               std::map<osmscout::FileOffset,osmscout::AdminRegionRef> &regionMap)
{
  if (!adminRegion){
    return QStringList();
  }

  QStringList list;
  if (locationService){
    locationService->ResolveAdminRegionHierachie(adminRegion, regionMap);
  }
  QString name = QString::fromStdString(adminRegion->name);
  list << name;
  QString last = name;
  osmscout::FileOffset parentOffset = adminRegion->parentRegionOffset;
  while (parentOffset != 0){
    const auto &it = regionMap.find(parentOffset);
    if (it==regionMap.end())
      break;
    const osmscout::AdminRegionRef region=it->second;
    name = QString::fromStdString(region->name);
    if (last != name){ // skip duplicates in admin region names
      list << name;
    }
    last = name;
    parentOffset = region->parentRegionOffset;
  }
  return list;
}
