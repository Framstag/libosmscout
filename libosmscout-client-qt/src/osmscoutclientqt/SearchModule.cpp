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

#include <osmscoutclientqt/SearchModule.h>
#include <osmscoutclientqt/OSMScoutQt.h>
#include <osmscoutclientqt/LookupModule.h>

#include <osmscout/util/Logger.h>
#include <iostream>

namespace osmscout {

SearchRunnable::SearchRunnable(SearchModule *searchModule,
                               DBInstanceRef &db,
                               const QString &searchPattern,
                               int limit,
                               osmscout::BreakerRef &breaker):
    searchModule(searchModule),
    db(db),
    searchPattern(searchPattern),
    limit(limit),
    breaker(breaker)
{
}

std::future<bool> SearchRunnable::getFuture()
{
  return promise.get_future();
}

SearchLocationsRunnable::SearchLocationsRunnable(SearchModule *searchModule,
                                                 DBInstanceRef &db,
                                                 const QString &searchPattern,
                                                 int limit,
                                                 osmscout::BreakerRef &breaker,
                                                 AdminRegionInfoRef &defaultRegionInfo):
    SearchRunnable(searchModule, db, searchPattern, limit, breaker),
    defaultRegionInfo(defaultRegionInfo)
{}

void SearchLocationsRunnable::run()
{
  if (breaker && breaker->IsAborted()){
    promise.set_value(true);
    return;
  }

  osmscout::AdminRegionRef defaultRegion;
  if (defaultRegionInfo && defaultRegionInfo->database==db->path){
    defaultRegion=defaultRegionInfo->adminRegion;
  }

  promise.set_value( SearchLocations(db,searchPattern,defaultRegion,limit,breaker,adminRegionMap));
}

FreeTextSearchRunnable::FreeTextSearchRunnable(SearchModule *searchModule,
                                               DBInstanceRef &db,
                                               const QString &searchPattern,
                                               int limit,
                                               osmscout::BreakerRef &breaker):
    SearchRunnable(searchModule, db, searchPattern, limit, breaker)
{}

void FreeTextSearchRunnable::run()
{
  if (breaker && breaker->IsAborted()){
    promise.set_value(true);
    return;
  }

  promise.set_value(FreeTextSearch(db,searchPattern, limit,adminRegionMap));
}

SearchModule::SearchModule(QThread *thread,DBThreadRef dbThread,LookupModule *lookupModule):
  thread(thread),dbThread(dbThread),lookupModule(lookupModule)
{
}

SearchModule::~SearchModule()
{
  lookupModule->deleteLater();
  lookupModule=nullptr;

  if (thread!=QThread::currentThread()){
    qWarning() << "Destroy" << this << "from non incorrect thread;" << thread << "!=" << QThread::currentThread();
  }
  if (thread!=nullptr){
    thread->quit();
  }
}

bool SearchLocationsRunnable::SearchLocations(DBInstanceRef &db,
                                              const QString &searchPattern,
                                              const osmscout::AdminRegionRef &defaultRegion,
                                              int limit,
                                              osmscout::BreakerRef &breaker,
                                              std::map<osmscout::FileOffset,osmscout::AdminRegionRef> &adminRegionMap)
{
  QList<LocationEntry> locations;
  std::string stdSearchPattern=searchPattern.toUtf8().constData();

  osmscout::LocationStringSearchParameter searchParameter(stdSearchPattern);

  searchParameter.SetStringMatcherFactory(std::make_shared<osmscout::StringMatcherTransliterateFactory>());
  if (defaultRegion) {
    searchParameter.SetDefaultAdminRegion(defaultRegion);
  }

  searchParameter.SetLimit(limit);
  searchParameter.SetBreaker(breaker);

  osmscout::LocationSearchResult result;
  LocationIndex::ScopeCacheCleaner cacheCleaner(db->GetDatabase()->GetLocationIndex());

  if (!db->GetLocationService()->SearchForLocationByString(searchParameter, result)){
    if (breaker){
      breaker->Break();
    }
    return false;
  }

  for (auto &entry: result.results){
    if (!BuildLocationEntry(entry, db, adminRegionMap, locations)){
      if (breaker) {
        breaker->Break();
      }
      return false;
    }
  }

  searchModule->searchResult(searchPattern, locations);
  return true;
}

#ifdef OSMSCOUT_HAVE_LIB_MARISA
  bool FreeTextSearchRunnable::FreeTextSearch(DBInstanceRef &db,
                                              const QString &searchPattern,
                                              int limit,
                                              std::map<osmscout::FileOffset,osmscout::AdminRegionRef> &adminRegionMap)
  {
    // Search by free text
    QList<LocationEntry> locations;
    QList<osmscout::ObjectFileRef> objectSet;
    osmscout::TextSearchIndex textSearch;
    if(!textSearch.Load(db->path.toStdString())){
      osmscout::log.Warn() << "Failed to load text index files, search only for locations with database " << db->path.toStdString();
      return true; // silently continue, text indexes are optional in database
    }
    osmscout::TextSearchIndex::ResultsMap resultsTxt;
    textSearch.Search(searchPattern.toStdString(),
                      /*searchPOIs*/ true, /*searchLocations*/ true,
                      /*searchRegions*/ true, /*searchOther*/ true,
                      resultsTxt);

    for(const auto &e: resultsTxt) {
      if (locations.size()>=limit) {
        break;
      }
      QString title=QString::fromStdString(e.first);
      const std::vector<osmscout::ObjectFileRef> &refs=e.second;

      for (const auto &fref:refs){
        if (locations.size()>=limit) {
          break;
        }

        if (objectSet.contains(fref)) {
          continue;
        }

        objectSet << fref;
        BuildLocationEntry(fref, title, db, adminRegionMap, locations);
      }
    }

    searchModule->searchResult(searchPattern, locations);
    return true;
  }
#else
  bool FreeTextSearchRunnable::FreeTextSearch(DBInstanceRef &/*db*/,
                                              const QString& /*1:1searchPattern*/,
                                              int /*limit*/,
                                              std::map<osmscout::FileOffset,osmscout::AdminRegionRef> &/*adminRegionMap*/)
  {
    // no code
    return true;
  }
#endif

void SearchModule::SearchForLocations(const QString searchPattern,
                                      int limit,
                                      osmscout::GeoCoord searchCenter,
                                      AdminRegionInfoRef defaultRegionInfo,
                                      osmscout::BreakerRef breaker)
{
  if (thread!=QThread::currentThread()){
    qWarning() << "Destroy" << this << "from non incorrect thread;" << thread << "!=" << QThread::currentThread();
  }

  osmscout::log.Debug() << "Searching for " << searchPattern.toStdString();
  QElapsedTimer timer;
  timer.start();

  OSMScoutQt::GetInstance().GetDBThread()->RunSynchronousJob(
    [this,&searchPattern,&limit,&searchCenter,&breaker,&defaultRegionInfo](const std::list<DBInstanceRef>& databases) {

      // sort databases by distance from search center
      // to provide nearest results first
      std::list<DBInstanceRef> sortedDbs=databases;
      sortedDbs.sort(
          [&searchCenter](const DBInstanceRef &a,const DBInstanceRef &b){
            osmscout::GeoBox abox=a->GetDBGeoBox();
            osmscout::GeoBox bbox=b->GetDBGeoBox();
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

      std::vector<std::future<bool>> results;
      results.reserve(2*sortedDbs.size());

      auto StartRunnable = [&results](SearchRunnable *runnable){
        // thread pool takes ownership
        assert(QThreadPool::globalInstance());
        results.push_back(runnable->getFuture());
        QThreadPool::globalInstance()->start(runnable);
      };

      for (auto& db:sortedDbs){
        //std::cout << "  " << db->path.toStdString() << std::endl;
        StartRunnable(new SearchLocationsRunnable(this, db, searchPattern, limit, breaker, defaultRegionInfo));
        StartRunnable(new FreeTextSearchRunnable(this, db, searchPattern, limit, breaker));
      }

      // wait for all runnables
      bool success=true;
      for (auto &resultFuture : results){
        success = success && resultFuture.get();
      }
      emit searchFinished(searchPattern, /*error*/ !success);
    }
  );

  osmscout::log.Debug() << "Retrieve result tooks " << timer.elapsed() << " ms";
  emit searchFinished(searchPattern, /*error*/ false);
}

bool SearchRunnable::BuildLocationEntry(const osmscout::ObjectFileRef& object,
                                        const QString &title,
                                        DBInstanceRef &db,
                                        std::map<osmscout::FileOffset,osmscout::AdminRegionRef> &/*adminRegionMap*/,
                                        QList<LocationEntry> &locations
                                        )
{
    QList<AdminRegionInfoRef> adminRegionList;
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

bool SearchRunnable::BuildLocationEntry(const osmscout::LocationSearchResult::Entry &entry,
                                        DBInstanceRef &db,
                                        std::map<osmscout::FileOffset,osmscout::AdminRegionRef> &adminRegionMap,
                                        QList<LocationEntry> &locations
                                        )
{
    QList<AdminRegionInfoRef> adminRegionList=LookupModule::BuildAdminRegionList(db, entry.adminRegion, adminRegionMap);
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

      for (auto object : entry.location->objects) {
          location.addReference(object);
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
      if (!GetObjectDetails(db, entry.adminRegion->object, objectType, coordinates, bbox)){
        return false;
      }
      QString name=QString::fromUtf8(entry.adminRegion->name.c_str());

      //=QString::fromUtf8(entry.adminRegion->name.c_str());
      LocationEntry location(LocationEntry::typeObject, name, objectType, adminRegionList,
                             db->path, coordinates, bbox);

      osmscout::log.Debug() << "region: " << name.toStdString();

      location.addReference(entry.adminRegion->object);
      locations.append(location);
    }
    return true;
}

bool SearchRunnable::GetObjectDetails(DBInstanceRef &db,
                                      const osmscout::ObjectFileRef& object,
                                      QString &typeName,
                                      osmscout::GeoCoord& coordinates,
                                      osmscout::GeoBox& bbox) {

  std::vector<osmscout::ObjectFileRef> objects;
  objects.push_back(object);
  return GetObjectDetails(db,objects,typeName,coordinates,bbox);
}

bool SearchRunnable::GetObjectDetails(DBInstanceRef &db,
                                      const std::vector<osmscout::ObjectFileRef>& objects,
                                      QString &typeName,
                                      osmscout::GeoCoord& coordinates,
                                      osmscout::GeoBox& bbox)
{
  auto database=db->GetDatabase();
  for (const osmscout::ObjectFileRef& object:objects) {
    if (!object.Valid()){
      continue;
    }
    if (object.GetType() == osmscout::RefType::refNode) {
      osmscout::NodeRef node;

      if (!database->GetNodeByOffset(object.GetFileOffset(), node)) {
        return false;
      }
      if (typeName.isEmpty()) {
        typeName = QString::fromUtf8(node->GetType()->GetName().c_str());
      }

      bbox.Include(osmscout::GeoBox::BoxByCenterAndRadius(node->GetCoords(), Distance::Of<Meter>(2.0)));
    } else if (object.GetType() == osmscout::RefType::refArea) {
      osmscout::AreaRef area;

      if (!database->GetAreaByOffset(object.GetFileOffset(), area)) {
        return false;
      }
      if (typeName.isEmpty()) {
        typeName = QString::fromUtf8(area->GetType()->GetName().c_str());
      }
      bbox.Include(area->GetBoundingBox());
    } else if (object.GetType() == osmscout::RefType::refWay) {
      osmscout::WayRef way;
      if (!database->GetWayByOffset(object.GetFileOffset(), way)) {
        return false;
      }
      if (typeName.isEmpty()) {
        typeName = QString::fromUtf8(way->GetType()->GetName().c_str());
      }
      bbox.Include(way->GetBoundingBox());
    }
  }
  coordinates=bbox.GetCenter();
  return true;
}
}
