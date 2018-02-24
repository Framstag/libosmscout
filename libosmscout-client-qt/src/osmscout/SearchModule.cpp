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

#include <osmscout/SearchModule.h>
#include <osmscout/OSMScoutQt.h>
#include <osmscout/LookupModule.h>

#include <osmscout/util/Logger.h>
#include <iostream>

SearchModule::SearchModule(QThread *thread,DBThreadRef dbThread,LookupModule *lookupModule):
  thread(thread),dbThread(dbThread),lookupModule(lookupModule)
{
}

SearchModule::~SearchModule()
{
  lookupModule->deleteLater();
  lookupModule=NULL;

  if (thread!=QThread::currentThread()){
    qWarning() << "Destroy" << this << "from non incorrect thread;" << thread << "!=" << QThread::currentThread();
  }
  if (thread!=NULL){
    thread->quit();
  }
}

void SearchModule::SearchLocations(DBInstanceRef &db,
                                   const QString searchPattern,
                                   const osmscout::AdminRegionRef defaultRegion,
                                   int limit,
                                   osmscout::BreakerRef &breaker,
                                   std::map<osmscout::FileOffset,osmscout::AdminRegionRef> &adminRegionMap)
{
  QList<LocationEntry> locations;
  std::string stdSearchPattern=searchPattern.toUtf8().constData();

  osmscout::LocationStringSearchParameter searchParameter(stdSearchPattern);

  // searchParameter.SetSearchForLocation(args.searchForLocation);
  // searchParameter.SetSearchForPOI(args.searchForPOI);
  // searchParameter.SetAdminRegionOnlyMatch(args.adminRegionOnlyMatch);
  // searchParameter.SetPOIOnlyMatch(args.poiOnlyMatch);
  // searchParameter.SetLocationOnlyMatch(args.locationOnlyMatch);
  // searchParameter.SetAddressOnlyMatch(args.addressOnlyMatch);
  // searchParameter.SetStringMatcherFactory(matcherFactory);
  if (defaultRegion)
    searchParameter.SetDefaultAdminRegion(defaultRegion);

  searchParameter.SetLimit(limit);
  searchParameter.SetBreaker(breaker);

  osmscout::LocationSearchResult result;

  if (!db->locationService->SearchForLocationByString(searchParameter, result)){
    emit searchFinished(searchPattern, /*error*/ true);
    return;
  }

  for (auto &entry: result.results){
    if (!BuildLocationEntry(entry, db, adminRegionMap, locations)){
      emit searchFinished(searchPattern, /*error*/ true);
      return;
    }
  }

  emit searchResult(searchPattern, locations);
}

void SearchModule::FreeTextSearch(DBInstanceRef &db,
                                  const QString searchPattern,
                                  int limit,
                                  std::map<osmscout::FileOffset,osmscout::AdminRegionRef> &adminRegionMap)
{
#ifdef OSMSCOUT_HAVE_LIB_MARISA
  // Search by free text
  QList<LocationEntry> locations;
  QList<osmscout::ObjectFileRef> objectSet;
  osmscout::TextSearchIndex textSearch;
  if(!textSearch.Load(db->path.toStdString())){
      qWarning("Failed to load text index files, search was for locations only");
      return; // silently continue, text indexes are optional in database
  }
  osmscout::TextSearchIndex::ResultsMap resultsTxt;
  textSearch.Search(searchPattern.toStdString(),
                    /*searchPOIs*/ true, /*searchLocations*/ true,
                    /*searchRegions*/ true, /*searchOther*/ true,
                    resultsTxt);
  osmscout::TextSearchIndex::ResultsMap::iterator it;
  int count = 0;
  for(it=resultsTxt.begin();
    it != resultsTxt.end() && count < limit;
    ++it, ++count)
  {
    std::vector<osmscout::ObjectFileRef> &refs=it->second;

    std::size_t maxPrintedOffsets=5;
    std::size_t minRefCount=std::min(refs.size(),maxPrintedOffsets);

    for(size_t r=0; r < minRefCount; r++){
      if (locations.size()>=limit)
          break;

      osmscout::ObjectFileRef fref = refs[r];

      if (objectSet.contains(fref))
          continue;

      objectSet << fref;
      BuildLocationEntry(fref, QString::fromStdString(it->first),
                         db, adminRegionMap, locations);
    }
  }
  // TODO: merge locations with same label database type
  // bus stations can have more points for example...
  emit searchResult(searchPattern, locations);
#endif
}

void SearchModule::SearchForLocations(const QString searchPattern,
                                      int limit,
                                      osmscout::GeoCoord searchCenter,
                                      AdminRegionInfoRef defaultRegionInfo,
                                      osmscout::BreakerRef breaker)
{
  QMutexLocker locker(&mutex);

  osmscout::log.Debug() << "Searching for " << searchPattern.toStdString();
  QTime timer;
  timer.start();

  OSMScoutQt::GetInstance().GetDBThread()->RunSynchronousJob(
    [this,&searchPattern,&limit,&searchCenter,&breaker,&defaultRegionInfo](const std::list<DBInstanceRef>& databases) {

      // sort databases by distance from search center
      // to provide nearest results first
      std::list<DBInstanceRef> sortedDbs=databases;
      sortedDbs.sort(
          [&searchCenter](const DBInstanceRef &a,const DBInstanceRef &b){
            osmscout::GeoBox abox;
            osmscout::GeoBox bbox;
            a->database->GetBoundingBox(abox);
            b->database->GetBoundingBox(bbox);
            bool ain=abox.Includes(searchCenter);
            bool bin=bbox.Includes(searchCenter);
            //std::cout << "  " << a->path.toStdString() << " ? " << b->path.toStdString() << std::endl;
            if (bin && !ain){
              return false;
            }
            if (ain && !bin){
              return true;
            }
            // (ain==bin)
            double adist=osmscout::DistanceSquare(searchCenter,abox.GetCenter());
            double bdist=osmscout::DistanceSquare(searchCenter,bbox.GetCenter());
            return adist<bdist;
          });

      //std::cout << "Sorted databases:" << std::endl;

      for (auto db:sortedDbs){
        //std::cout << "  " << db->path.toStdString() << std::endl;
        std::map<osmscout::FileOffset,osmscout::AdminRegionRef> adminRegionMap;

        if (breaker && breaker->IsAborted()){
          emit searchFinished(searchPattern, /*error*/ false);
          break;
        }
        osmscout::AdminRegionRef defaultRegion;
        if (defaultRegionInfo && defaultRegionInfo->database==db->path){
          defaultRegion=defaultRegionInfo->adminRegion;
        }
        SearchLocations(db,searchPattern,defaultRegion,limit,breaker,adminRegionMap);

        if (breaker && breaker->IsAborted()){
          emit searchFinished(searchPattern, /*error*/ false);
          break;
        }
        FreeTextSearch(db,searchPattern,limit,adminRegionMap);
      }
    }
  );

  osmscout::log.Debug() << "Retrieve result tooks " << timer.elapsed() << " ms";
  emit searchFinished(searchPattern, /*error*/ false);
}

bool SearchModule::BuildLocationEntry(const osmscout::ObjectFileRef& object,
                                      const QString title,
                                      DBInstanceRef db,
                                      std::map<osmscout::FileOffset,osmscout::AdminRegionRef> &/*adminRegionMap*/,
                                      QList<LocationEntry> &locations
                                      )
{
    QStringList adminRegionList;
    QString objectType;
    osmscout::GeoCoord coordinates;
    osmscout::GeoBox bbox;

    if (!GetObjectDetails(db, object, objectType, coordinates, bbox)){
      return false;
    }
    osmscout::log.Debug() << "obj:    " << title.toStdString() << " (" << objectType.toStdString() << ")";

    // Reverse lookup is slow for all search entries
    // TODO: move it to SearchModel and make it asynchronous
    /*
    std::list<osmscout::LocationService::ReverseLookupResult> result;
    if (db->locationService->ReverseLookupObject(object, result)){
        for (const osmscout::LocationService::ReverseLookupResult& entry : result){
            if (entry.adminRegion){
              adminRegionList=LookupModule::BuildAdminRegionList(db->locationService,
                                                                 entry.adminRegion,
                                                                 adminRegionMap);
              break;
            }
        }
    }
    */

    LocationEntry location(LocationEntry::typeObject, title, objectType, adminRegionList,
                           db->path, coordinates, bbox);
    location.addReference(object);
    locations.append(location);

    return true;
}

bool SearchModule::BuildLocationEntry(const osmscout::LocationSearchResult::Entry &entry,
                                      DBInstanceRef db,
                                      std::map<osmscout::FileOffset,osmscout::AdminRegionRef> &adminRegionMap,
                                      QList<LocationEntry> &locations
                                      )
{
    if (entry.adminRegion){
      db->locationService->ResolveAdminRegionHierachie(entry.adminRegion, adminRegionMap);
    }

    QStringList adminRegionList=LookupModule::BuildAdminRegionList(entry.adminRegion, adminRegionMap);
    QString objectType;
    osmscout::GeoCoord coordinates;
    osmscout::GeoBox bbox;

    if (entry.adminRegion &&
        entry.location &&
        entry.address) {

      QString loc=QString::fromUtf8(entry.location->name.c_str());
      QString address=QString::fromUtf8(entry.address->name.c_str());

      QString label;
      label+=loc;
      label+=" ";
      label+=address;

      if (!GetObjectDetails(db, entry.address->object, objectType, coordinates, bbox)){
        return false;
      }

      osmscout::log.Debug() << "address:  " << label.toStdString();

      LocationEntry location(LocationEntry::typeObject, label, objectType, adminRegionList,
                             db->path, coordinates, bbox);
      location.addReference(entry.address->object);
      locations.append(location);
    }
    else if (entry.adminRegion &&
             entry.location) {

      QString loc=QString::fromUtf8(entry.location->name.c_str());
      if (!GetObjectDetails(db, entry.location->objects, objectType, coordinates, bbox)){
        return false;
      }

      osmscout::log.Debug() << "loc:      " << loc.toStdString();

      LocationEntry location(LocationEntry::typeObject, loc, objectType, adminRegionList,
                             db->path, coordinates, bbox);

      for (std::vector<osmscout::ObjectFileRef>::const_iterator object=entry.location->objects.begin();
          object!=entry.location->objects.end();
          ++object) {
          location.addReference(*object);
      }
      locations.append(location);
    }
    else if (entry.adminRegion &&
             entry.poi) {

      QString poi=QString::fromUtf8(entry.poi->name.c_str());
      if (!GetObjectDetails(db, entry.poi->object, objectType, coordinates, bbox)){
        return false;
      }
      osmscout::log.Debug() << "poi:      " << poi.toStdString();

      LocationEntry location(LocationEntry::typeObject, poi, objectType, adminRegionList,
                             db->path, coordinates, bbox);
      location.addReference(entry.poi->object);
      locations.append(location);
    }
    else if (entry.adminRegion) {
      QString objectType;
      if (entry.adminRegion->aliasObject.Valid()) {
          if (!GetObjectDetails(db, entry.adminRegion->aliasObject, objectType, coordinates, bbox)){
            return false;
          }
      }
      else {
          if (!GetObjectDetails(db, entry.adminRegion->object, objectType, coordinates, bbox)){
            return false;
          }
      }
      QString name;
      if (!entry.adminRegion->aliasName.empty()) {
        name=QString::fromUtf8(entry.adminRegion->aliasName.c_str());
      }
      else {
        name=QString::fromUtf8(entry.adminRegion->name.c_str());
      }

      //=QString::fromUtf8(entry.adminRegion->name.c_str());
      LocationEntry location(LocationEntry::typeObject, name, objectType, adminRegionList,
                             db->path, coordinates, bbox);

      osmscout::log.Debug() << "region: " << name.toStdString();

      if (entry.adminRegion->aliasObject.Valid()) {
          location.addReference(entry.adminRegion->aliasObject);
      }
      else {
          location.addReference(entry.adminRegion->object);
      }
      locations.append(location);
    }
    return true;
}

bool SearchModule::GetObjectDetails(DBInstanceRef db,
                                    const osmscout::ObjectFileRef& object,
                                    QString &typeName,
                                    osmscout::GeoCoord& coordinates,
                                    osmscout::GeoBox& bbox) {

  std::vector<osmscout::ObjectFileRef> objects;
  objects.push_back(object);
  return GetObjectDetails(db,objects,typeName,coordinates,bbox);
}

bool SearchModule::GetObjectDetails(DBInstanceRef db,
                                    const std::vector<osmscout::ObjectFileRef>& objects,
                                    QString &typeName,
                                    osmscout::GeoCoord& coordinates,
                                    osmscout::GeoBox& bbox)
{
  osmscout::GeoBox tmpBox;
  for (const osmscout::ObjectFileRef& object:objects) {
    if (!object.Valid()){
      continue;
    }
    if (object.GetType() == osmscout::RefType::refNode) {
      osmscout::NodeRef node;

      if (!db->database->GetNodeByOffset(object.GetFileOffset(), node)) {
        return false;
      }
      if (typeName.isEmpty()) {
        typeName = QString::fromUtf8(node->GetType()->GetName().c_str());
      }

      bbox.Include(osmscout::GeoBox::BoxByCenterAndRadius(node->GetCoords(), 2.0));
    } else if (object.GetType() == osmscout::RefType::refArea) {
      osmscout::AreaRef area;

      if (!db->database->GetAreaByOffset(object.GetFileOffset(), area)) {
        return false;
      }
      if (typeName.isEmpty()) {
        typeName = QString::fromUtf8(area->GetType()->GetName().c_str());
      }
      area->GetBoundingBox(tmpBox);
      bbox.Include(tmpBox);
    } else if (object.GetType() == osmscout::RefType::refWay) {
      osmscout::WayRef way;
      if (!db->database->GetWayByOffset(object.GetFileOffset(), way)) {
        return false;
      }
      if (typeName.isEmpty()) {
        typeName = QString::fromUtf8(way->GetType()->GetName().c_str());
      }
      way->GetBoundingBox(tmpBox);
      bbox.Include(tmpBox);
    }
  }
  coordinates=bbox.GetCenter();
  return true;
}
