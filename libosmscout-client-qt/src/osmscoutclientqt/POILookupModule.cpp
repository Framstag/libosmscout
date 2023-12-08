/*
 OSMScout - a Qt backend for libosmscout and libosmscout-map
 Copyright (C) 2018 Lukas Karas

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

#include <osmscoutclientqt/POILookupModule.h>

#include <osmscout/feature/OperatorFeature.h>

#include <osmscout/poi/POIService.h>

namespace osmscout {

POILookupModule::POILookupModule(DBThreadRef dbThread):
  AsyncWorker("POILookupModule"), dbThread(dbThread)
{
  // no code
}

POILookupModule::~POILookupModule()
{
  ThreadAssert();
}

template<class T>
LocationEntry buildLocationEntry(T obj,
                                 QString dbPath,
                                 GeoCoord coordinates,
                                 GeoBox bbox)
{
  QString title;
  QString altName;
  QString objectType = QString::fromUtf8(obj->GetType()->GetName().c_str());
  const FeatureValueBuffer &features=obj->GetFeatureValueBuffer();

  if (const NameFeatureValue *name=features.findValue<NameFeatureValue>();
      name!=nullptr){
    title=QString::fromStdString(name->GetLabel(Locale(), 0));
    //std::cout << " \"" << name->GetLabel() << "\"";
  } else if (const OperatorFeatureValue *operatorVal=features.findValue<OperatorFeatureValue>();
             operatorVal!=nullptr) {
    title=QString::fromStdString(operatorVal->GetLabel(Locale(), 0));
  } else if (const RefFeatureValue *ref=features.findValue<RefFeatureValue>();
             ref!=nullptr) {
    title=QString::fromStdString(ref->GetLabel(Locale(), 0));
  }

  if (const NameAltFeatureValue *name=features.findValue<NameAltFeatureValue>();
    name!=nullptr) {
    altName = QString::fromStdString(name->GetLabel(Locale(), 0));
  }

  LocationEntry location(LocationEntry::typeObject, title, altName, objectType, QList<AdminRegionInfoRef>(),
                         dbPath, coordinates, bbox);
  location.addReference(obj->GetObjectFileRef());
  return LocationEntry(location); // explicit copy. some older compilers (GCC 7.5.0) fails when tries to use deleted move constructor
}

std::vector<LocationEntry> POILookupModule::doPOIlookup(DBInstanceRef db,
                                                        const GeoBox &searchBoundingBox,
                                                        const std::vector<std::string> &types)
{
  std::vector<LocationEntry> result;

  TypeInfoSet nodeTypes;
  std::vector<NodeRef> nodes;
  TypeInfoSet wayTypes;
  std::vector<WayRef> ways;
  TypeInfoSet areaTypes;
  std::vector<AreaRef> areas;

  auto database=db->GetDatabase();
  if (!database){
    log.Error() << "No db available";
    return result;
  }
  TypeConfigRef typeConfig=database->GetTypeConfig();
  if (!typeConfig){
    log.Error() << "No typeConfig available";
    return result;
  }

  // prepare type set
  for (const auto &typeName: types){
    TypeInfoRef typeInfo=typeConfig->GetTypeInfo(typeName);
    if (!typeInfo){
      log.Warn() << "There is no type " << typeName;
      continue;
    }
    if (typeInfo->CanBeArea()){
      areaTypes.Set(typeInfo);
    }
    if (typeInfo->CanBeWay()){
      wayTypes.Set(typeInfo);
    }
    if (typeInfo->CanBeNode()){
      nodeTypes.Set(typeInfo);
    }
  }

  // lookup objects
  POIService poiService(database);
  try {
    poiService.GetPOIsInArea(searchBoundingBox,
                             nodeTypes,
                             nodes,
                             wayTypes,
                             ways,
                             areaTypes,
                             areas);
  }
  catch (const std::exception& e) {
    log.Error() << "Failed to load POIs in area: " << e.what();
    return result;
  }

  // build location entries
  for (AreaRef &area:areas) {
    GeoBox   bbox=area->GetBoundingBox();
    GeoCoord coordinates=bbox.GetCenter();

    result.push_back(buildLocationEntry(area, QString::fromStdString(db->path), coordinates, bbox));
  }

  for (WayRef &way:ways) {
    GeoBox   bbox=way->GetBoundingBox();
    GeoCoord coordinates=bbox.GetCenter();

    result.push_back(buildLocationEntry(way, QString::fromStdString(db->path), coordinates, bbox));
  }

  for (NodeRef &node:nodes) {
    GeoCoord coordinates=node->GetCoords();
    GeoBox bbox;
    bbox.Include(GeoBox::BoxByCenterAndRadius(node->GetCoords(), Distance::Of<Meter>(2.0)));

    result.push_back(buildLocationEntry(node, QString::fromStdString(db->path), coordinates, bbox));
  }

  return result;
}

POILookupModule::LookupFuture POILookupModule::lookupPOIRequest(int requestId,
                                                                const GeoCoord &searchCenter,
                                                                const std::vector<std::string> &types,
                                                                const Distance &maxDistance)
{
  return Async<std::vector<LocationEntry>>([=](Breaker &breaker) -> std::vector<LocationEntry> {
    std::vector<LocationEntry> result;
    GeoBox searchBoundingBox=GeoBox::BoxByCenterAndRadius(searchCenter, maxDistance);

    dbThread->RunSynchronousJob([&](const std::list<DBInstanceRef>& databases){
      for (auto &db : databases) {
        if (breaker.IsAborted()){
          lookupAborted.Emit(requestId);
          break;
        }
        auto partialResult=doPOIlookup(db, searchBoundingBox, types);
        lookupResult.Emit(requestId,partialResult);
        std::copy(partialResult.begin(), partialResult.end(), std::back_inserter(result));
      }
    });

    return result;
  });
}
}
