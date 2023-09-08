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

POILookupModule::POILookupModule(QThread *thread,DBThreadRef dbThread):
  thread(thread), dbThread(dbThread)
{

}

POILookupModule::~POILookupModule()
{
  if (thread!=QThread::currentThread()){
    qWarning() << "Destroy" << this << "from non incorrect thread;" << thread << "!=" << QThread::currentThread();
  }
  if (thread!=nullptr){
    thread->quit();
  }
}

template<class T>
LocationEntry buildLocationEntry(T obj,
                                 QString dbPath,
                                 osmscout::GeoCoord coordinates,
                                 osmscout::GeoBox bbox)
{
  QString title;
  QString altName;
  QString objectType = QString::fromUtf8(obj->GetType()->GetName().c_str());
  const osmscout::FeatureValueBuffer &features=obj->GetFeatureValueBuffer();

  if (const osmscout::NameFeatureValue *name=features.findValue<osmscout::NameFeatureValue>();
      name!=nullptr){
    title=QString::fromStdString(name->GetLabel(osmscout::Locale(), 0));
    //std::cout << " \"" << name->GetLabel() << "\"";
  } else if (const osmscout::OperatorFeatureValue *operatorVal=features.findValue<osmscout::OperatorFeatureValue>();
             operatorVal!=nullptr) {
    title=QString::fromStdString(operatorVal->GetLabel(osmscout::Locale(), 0));
  } else if (const osmscout::RefFeatureValue *ref=features.findValue<osmscout::RefFeatureValue>();
             ref!=nullptr) {
    title=QString::fromStdString(ref->GetLabel(osmscout::Locale(), 0));
  }

  if (const osmscout::NameAltFeatureValue *name=features.findValue<osmscout::NameAltFeatureValue>();
    name!=nullptr) {
    altName = QString::fromStdString(name->GetLabel(osmscout::Locale(), 0));
  }

  LocationEntry location(LocationEntry::typeObject, title, altName, objectType, QList<AdminRegionInfoRef>(),
                         dbPath, coordinates, bbox);
  location.addReference(obj->GetObjectFileRef());
  return LocationEntry(location); // explicit copy. some older compilers (GCC 7.5.0) fails when tries to use deleted move constructor
}

QList<LocationEntry> POILookupModule::doPOIlookup(DBInstanceRef db,
                                                  osmscout::GeoBox searchBoundingBox,
                                                  osmscout::BreakerRef /*breaker*/,
                                                  QStringList types)
{
  QList<LocationEntry> result;

  osmscout::TypeInfoSet nodeTypes;
  std::vector<osmscout::NodeRef> nodes;
  osmscout::TypeInfoSet wayTypes;
  std::vector<osmscout::WayRef> ways;
  osmscout::TypeInfoSet areaTypes;
  std::vector<osmscout::AreaRef> areas;

  auto database=db->GetDatabase();
  if (!database){
    osmscout::log.Error() << "No db available";
    return result;
  }
  osmscout::TypeConfigRef typeConfig=database->GetTypeConfig();
  if (!typeConfig){
    osmscout::log.Error() << "No typeConfig available";
    return result;
  }

  // prepare type set
  for (const QString &typeName: types){
    osmscout::TypeInfoRef typeInfo=typeConfig->GetTypeInfo(typeName.toStdString());
    if (!typeInfo){
      osmscout::log.Warn() << "There is no type " << typeName.toStdString();
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
  osmscout::POIService poiService(database);
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
    osmscout::log.Error() << "Failed to load POIs in area: " << e.what();
    return result;
  }

  // build location entries
  for (osmscout::AreaRef &area:areas) {
    osmscout::GeoBox   bbox=area->GetBoundingBox();
    osmscout::GeoCoord coordinates=bbox.GetCenter();

    result << buildLocationEntry(area, QString::fromStdString(db->path), coordinates, bbox);
  }

  for (osmscout::WayRef &way:ways) {
    osmscout::GeoBox   bbox=way->GetBoundingBox();
    osmscout::GeoCoord coordinates=bbox.GetCenter();

    result << buildLocationEntry(way, QString::fromStdString(db->path), coordinates, bbox);
  }

  for (osmscout::NodeRef &node:nodes) {
    osmscout::GeoCoord coordinates=node->GetCoords();
    osmscout::GeoBox bbox;
    bbox.Include(osmscout::GeoBox::BoxByCenterAndRadius(node->GetCoords(), Distance::Of<Meter>(2.0)));

    result << buildLocationEntry(node, QString::fromStdString(db->path), coordinates, bbox);
  }

  return result;
}

void POILookupModule::lookupPOIRequest(int requestId,
                                       osmscout::BreakerRef breaker,
                                       osmscout::GeoCoord searchCenter,
                                       QStringList types,
                                       double maxDistance)
{
  osmscout::GeoBox searchBoundingBox=osmscout::GeoBox::BoxByCenterAndRadius(searchCenter, Distance::Of<Meter>(maxDistance));

  dbThread->RunSynchronousJob([&](const std::list<DBInstanceRef>& databases){

    for (auto &db : databases) {
      if (breaker && breaker->IsAborted()){
        emit lookupAborted(requestId);
        break;
      }
      emit lookupResult(requestId,
                        doPOIlookup(db, searchBoundingBox, breaker, types));
    }
  });

  emit lookupFinished(requestId);
}
}
