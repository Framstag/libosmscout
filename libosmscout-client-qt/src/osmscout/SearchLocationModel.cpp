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

#include <osmscout/DBThread.h>
#include <osmscout/SearchLocationModel.h>
#include <osmscout/OSMScoutQt.h>

#include <QtQml>

#include <iostream>

#define INVALID_COORD -1000.0

LocationListModel::LocationListModel(QObject* parent)
: QAbstractListModel(parent), searching(false), searchCenter(INVALID_COORD,INVALID_COORD), resultLimit(50)
{
  searchModule=OSMScoutQt::GetInstance().MakeSearchModule();
  lookupModule=OSMScoutQt::GetInstance().MakeLookupModule();

  connect(this, SIGNAL(SearchRequested(const QString, int, osmscout::GeoCoord, AdminRegionInfoRef, osmscout::BreakerRef)),
          searchModule, SLOT(SearchForLocations(const QString, int, osmscout::GeoCoord, AdminRegionInfoRef, osmscout::BreakerRef)),
          Qt::QueuedConnection);

  connect(searchModule, SIGNAL(searchResult(const QString, const QList<LocationEntry>)),
          this, SLOT(onSearchResult(const QString, const QList<LocationEntry>)),
          Qt::QueuedConnection);

  connect(searchModule, SIGNAL(searchFinished(const QString, bool)),
          this, SLOT(onSearchFinished(const QString, bool)),
          Qt::QueuedConnection);

  connect(this, SIGNAL(regionLookupRequested(osmscout::GeoCoord)),
          lookupModule, SLOT(requestRegionLookup(osmscout::GeoCoord)),
          Qt::QueuedConnection);

  connect(lookupModule, SIGNAL(locationAdminRegions(const osmscout::GeoCoord,QList<AdminRegionInfoRef>)),
          this, SLOT(onLocationAdminRegions(const osmscout::GeoCoord,QList<AdminRegionInfoRef>)),
          Qt::QueuedConnection);

  connect(lookupModule, SIGNAL(locationAdminRegionFinished(const osmscout::GeoCoord)),
          this, SLOT(onLocationAdminRegionFinished(const osmscout::GeoCoord)),
          Qt::QueuedConnection);
}

LocationListModel::~LocationListModel()
{
  for (QList<LocationEntry*>::iterator location=locations.begin();
       location!=locations.end();
       ++location) {
      delete *location;
  }

  locations.clear();

  if (breaker){
    breaker->Break();
    breaker.reset();
  }
  if (searchModule!=NULL){
    searchModule->deleteLater();
    searchModule=NULL;
  }
  if (lookupModule!=NULL){
    lookupModule->deleteLater();
    lookupModule=NULL;
  }
}

void LocationListModel::onSearchResult(const QString searchPattern, 
                                       const QList<LocationEntry> foundLocations)
{
  if (this->pattern.isEmpty()) {
    return; //No search requested
  }
  if (!searchPattern.contains(this->pattern, Qt::CaseInsensitive)){
    return; // result is not for us
  }

  QTime timer;
  timer.start();

  QQmlEngine *engine = qmlEngine(this);
  for (auto &location : foundLocations) {

    if (equalsFn.isCallable()){
      // try to merge locations that are equals
      bool merge=false;
      for (LocationEntry* secondLocation:locations) {
        if (secondLocation->getLabel()==location.getLabel() &&
            secondLocation->getDatabase()==location.getDatabase() &&
            secondLocation->getType()==LocationEntry::typeObject &&
            location.getType()==LocationEntry::typeObject){

          QJSValueList args;
          args << engine->newQObject(new LocationEntry(location));
          args << engine->newQObject(new LocationEntry(*secondLocation));
          QJSValue result = equalsFn.call(args);
          if (result.isBool() && result.toBool()){

            // qDebug() << "Merge " << location.getLabel() << " to location " << secondLocation->getLabel();
            secondLocation->mergeWith(location);

            // emit data change
            int index=locations.indexOf(secondLocation);
            if (index>=0) {
              QVector<int> roles;
              roles << LatRole << LonRole << DistanceRole << BearingRole;
              emit dataChanged(createIndex(index, 0), createIndex(index, 0), roles);
            }

            merge=true;
            break;
          }
        }
      }
      if (merge){
        continue;
      }
    }

    // evaluate final position of location
    int position=locations.size();
    if (compareFn.isCallable()) {
      position=0;
      for (LocationEntry* secondLocation:locations){
        QJSValueList args;
        args << engine->newQObject(new LocationEntry(location));
        args << engine->newQObject(new LocationEntry(*secondLocation));
        QJSValue result = compareFn.call(args);
        if (result.isNumber() && result.toNumber() <= 0){
          break;
        }
        position++;
      }
    }

    emit beginInsertRows(QModelIndex(), position, position);
    locations.insert(position, new LocationEntry(location));

    // qDebug() << "Put " << location.getLabel() << " to position: " << position;

    emit endInsertRows();
  }  
  emit countChanged(locations.size());
  qDebug() << "added " << foundLocations.size() << " (in " << timer.elapsed() << " ms), model size" << locations.size();
}

void LocationListModel::onLocationAdminRegions(const osmscout::GeoCoord location,
                                               QList<AdminRegionInfoRef> adminRegionList)
{
  if (location==searchCenter){
    bool first=true;
    for (const auto &info:adminRegionList){
      // adminRegionList is sorted by decreasing admin level
      if (first || (info->adminLevel >= LookupModule::Town && defaultRegion!=info)){
        defaultRegion=info;
        first=false;
      }
    }
  }
}

void LocationListModel::onLocationAdminRegionFinished(const osmscout::GeoCoord location)
{
  if (location==searchCenter &&
      !pattern.isEmpty() &&
      defaultRegion &&
      lastRequestDefaultRegion!=defaultRegion){
    osmscout::log.Debug() << "Search again with new default region: " << defaultRegion->name.toStdString();
    setPattern(pattern);
  }
}

void LocationListModel::onSearchFinished(const QString searchPattern, bool /*error*/)
{
  if (this->lastRequestPattern!=searchPattern){
    return; // result is not actual
  }
  if (lastRequestPattern!=pattern || lastRequestDefaultRegion!=defaultRegion){
    qDebug() << "Search postponed" << pattern;
    lastRequestPattern=pattern;
    lastRequestDefaultRegion=defaultRegion;
    breaker=std::make_shared<osmscout::ThreadedBreaker>();
    emit SearchRequested(pattern,resultLimit,searchCenter,defaultRegion,breaker);
  }else{
    searching = false;
    emit SearchingChanged(false);
  }
}

void LocationListModel::setPattern(const QString& pattern)
{
  if (this->pattern==pattern){
    return;
  }
  this->pattern = pattern;
  
  // remove old items
  // TODO: remove only invalid items that don't match to new pattern
  beginRemoveRows(QModelIndex(), 0, locations.size()-1);
  for (QList<LocationEntry*>::iterator location=locations.begin();
       location!=locations.end();
       ++location) {
      delete *location;
  }

  locations.clear();
  endRemoveRows();
  emit countChanged(locations.size());

  std::string stdPattern=pattern.toUtf8().constData();

  osmscout::GeoCoord coord;

  if (osmscout::GeoCoord::Parse(stdPattern, coord)) {
      QString name=QString::fromLocal8Bit(coord.GetDisplayText().c_str());
      QString label=name;

      LocationEntry *location=new LocationEntry(label, coord);
      beginInsertRows(QModelIndex(), locations.size(), locations.size());
      locations.insert(0, location);
      endInsertRows();
  }
  emit countChanged(locations.size());

  if (breaker){
    breaker->Break();
    breaker.reset();
  }
  if (searching){
    // we are still waiting for previous request, postpone current
    qDebug() << "Clear (" << locations.size() << ") postpone search" << pattern << "(default region:" << (defaultRegion?defaultRegion->name:"NULL") << ")";
    return;
  }
  
  qDebug() << "Clear (" << locations.size() << ") search" << pattern << "(default region:" << (defaultRegion?defaultRegion->name:"NULL") << ")";
  searching = true;
  lastRequestPattern = pattern;
  lastRequestDefaultRegion=defaultRegion;
  emit SearchingChanged(true);
  breaker=std::make_shared<osmscout::ThreadedBreaker>();
  emit SearchRequested(pattern,resultLimit,searchCenter,defaultRegion,breaker);
}

int LocationListModel::rowCount(const QModelIndex& ) const
{
  return locations.size();
}

QVariant LocationListModel::data(const QModelIndex &index, int role) const
{
  if(index.row() < 0 || index.row() >= locations.size()) {
    return QVariant();
  }

  LocationEntry* location=locations.at(index.row());

  switch (role) {
  case Qt::DisplayRole:
  case LabelRole:
    return location->getLabel();
  case TypeRole:
    if (location->getType()==LocationEntry::typeCoordinate)
      return "coordinate";
    else
      return location->getObjectType();
  case RegionRole:
    return location->getAdminRegionList();
  case LatRole:
    return QVariant::fromValue(location->getCoord().GetLat());
  case LonRole:
    return QVariant::fromValue(location->getCoord().GetLon());
  case DistanceRole:
    if (searchCenter.GetLat()!=INVALID_COORD && searchCenter.GetLon()!=INVALID_COORD) {
      return osmscout::GetSphericalDistance(location->getCoord(), searchCenter)*1000;
    }else{
      return 0;
    }
  case BearingRole:
    if (searchCenter.GetLat()!=INVALID_COORD && searchCenter.GetLon()!=INVALID_COORD) {
      return QString::fromStdString(
          osmscout::BearingDisplayString(
              osmscout::GetSphericalBearingInitial(searchCenter, location->getCoord())));
    }else{
      return "";
    }
  case LocationObjectRole:
    // QML will take ownerhip
    return QVariant::fromValue(new LocationEntry(*location));
  default:
    break;
  }

  return QVariant();
}

Qt::ItemFlags LocationListModel::flags(const QModelIndex &index) const
{
  if(!index.isValid()) {
    return Qt::ItemIsEnabled;
  }

  return QAbstractListModel::flags(index) | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QHash<int, QByteArray> LocationListModel::roleNames() const
{
  QHash<int, QByteArray> roles=QAbstractListModel::roleNames();

  roles[LabelRole]="label";
  roles[TypeRole]="type";
  roles[RegionRole]="region";
  roles[LatRole]="lat";
  roles[LonRole]="lon";
  roles[DistanceRole]="distance";
  roles[BearingRole]="bearing";
  roles[LocationObjectRole]="locationObject";

  return roles;
}

LocationEntry* LocationListModel::get(int row) const
{
    if(row < 0 || row >= locations.size()) {
        return NULL;
    }

    LocationEntry* location=locations.at(row);
    // QML will take ownerhip
    return new LocationEntry(*location);
}

void LocationListModel::lookupRegion()
{
  if (searchCenter.GetLat()!=INVALID_COORD && searchCenter.GetLon()!=INVALID_COORD){
    emit regionLookupRequested(searchCenter);
  }
}
