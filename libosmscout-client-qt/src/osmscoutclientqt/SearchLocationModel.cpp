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

#include <osmscoutclient/DBThread.h>

#include <osmscoutclientqt/SearchLocationModel.h>
#include <osmscoutclientqt/OSMScoutQt.h>

#include <QtQml>
#include <QElapsedTimer>

#include <iostream>

namespace osmscout {

#define INVALID_COORD -1000.0

LocationListModel::LocationListModel(QObject* parent)
: QAbstractListModel(parent), searchCenter(INVALID_COORD,INVALID_COORD)
{
  searchModule=OSMScoutQt::GetInstance().MakeSearchModule();
  lookupModule=OSMScoutQt::GetInstance().MakeLookupModule();
  settings=OSMScoutQt::GetInstance().GetSettings();

  connect(this, &LocationListModel::SearchRequested,
          searchModule, &SearchModule::SearchForLocations,
          Qt::QueuedConnection);

  connect(searchModule, &SearchModule::searchResult,
          this, &LocationListModel::onSearchResult,
          Qt::QueuedConnection);

  connect(searchModule, &SearchModule::searchFinished,
          this, &LocationListModel::onSearchFinished,
          Qt::QueuedConnection);

  connect(this, &LocationListModel::regionLookupRequested,
          lookupModule, &LookupModule::requestRegionLookup,
          Qt::QueuedConnection);

  connect(lookupModule, &LookupModule::locationAdminRegions,
          this, &LocationListModel::onLocationAdminRegions,
          Qt::QueuedConnection);

  connect(lookupModule, &LookupModule::locationAdminRegionFinished,
          this, &LocationListModel::onLocationAdminRegionFinished,
          Qt::QueuedConnection);

  connect(&postponeTimer, &QTimer::timeout,
          this, &LocationListModel::postponeAdd,
          Qt::QueuedConnection);
}

LocationListModel::~LocationListModel()
{
  locations.clear();

  if (breaker){
    breaker->Break();
    breaker.reset();
  }
  if (searchModule!=nullptr){
    searchModule->deleteLater();
    searchModule=nullptr;
  }
  if (lookupModule!=nullptr){
    lookupModule->deleteLater();
    lookupModule=nullptr;
  }
}

void LocationListModel::onSearchResult(const QString searchPattern, 
                                       const QList<LocationEntry> foundLocationsConst) {
  if (this->pattern.isEmpty()) {
    return; //No search requested
  }
  if (!searchPattern.contains(this->pattern, Qt::CaseInsensitive)) {
    return; // result is not for us
  }

  /**
   * Adding huge batch of entries to model (more that 100) may take longer than ~16 ms
   * and UI may become choppy. So, we enqueue all found entries to postponedEntries
   * and start postponeTimer that adds entries by small chunks. Whole operation is splitted
   * to multiple animation frames.
   */
  for (const LocationEntry &e: foundLocationsConst) {
    postponedEntries.push_back(std::make_shared<LocationEntry>(e));
  }

  postponeTimer.setInterval(1);
  postponeTimer.start();
}

void LocationListModel::postponeAdd() {
  constexpr int batchSize = 50;

  if (postponedEntries.size() <= batchSize) {
    addBatch(postponedEntries);
    postponedEntries.clear();
    postponeTimer.stop();
    emit SearchingChanged(isSearching());
  } else {
    QList<LocationEntryRef> batch;
    batch.reserve(batchSize);
    std::copy(postponedEntries.begin(), postponedEntries.begin() + batchSize, std::back_inserter(batch));
    addBatch(batch);
    postponedEntries.erase(postponedEntries.begin(), postponedEntries.begin() + batchSize);
  }
}

void LocationListModel::addBatch(QList<LocationEntryRef> foundLocations) {
  QElapsedTimer timer;
  timer.start();

  int equalityCall = 0;
  int comparisonsCall = 0;

  QQmlEngine *engine = qmlEngine(this);
  if (equalsFn.isCallable()) {
    // try to merge locations that are equals
    auto Equal = [&](const LocationEntryRef& a, const LocationEntryRef& b) -> bool {
      assert(a != nullptr && b != nullptr);
      if (a->getLabel()==b->getLabel() &&
          a->getDatabase()==b->getDatabase() &&
          a->getType()==LocationEntry::typeObject &&
          b->getType()==LocationEntry::typeObject){

        QJSValueList args;
        // to transfer ownership to QML, parent have to be null. copy constructor copy ownership
        assert(a->parent() == nullptr && b->parent() == nullptr);
        args << engine->newQObject(new LocationEntry(*a));
        args << engine->newQObject(new LocationEntry(*b));
        QJSValue result = equalsFn.call(args);
        equalityCall++;
        if (result.isError()){
          qWarning() << "Equals failed: " << result.toString();
          return false;
        }
        if (result.isBool()){
          return result.toBool();
        }
      }
      return false;
    };

    // merge equal locations in new entries
    for (auto locA = foundLocations.begin(); locA != foundLocations.end();) {
      bool merged=false;
      for (auto locB = locA+1; locB != foundLocations.end(); ++locB) {
        if (Equal(*locA, *locB)){
          // qDebug() << "merge" << (*locA)->getLabel() << "with" << (*locB)->getLabel();
          (*locB)->mergeWith(**locA);
          merged=true;
          break;
        }
      }
      if (merged) {
        locA = foundLocations.erase(locA);
      } else {
        ++locA;
      }
    }
    // merge new locations to existing one in model
    for (auto newLocation = foundLocations.begin(); newLocation != foundLocations.end(); ) {
      int index=0;
      bool merged=false;
      for (LocationEntryRef& modelLocation:locations) {
        if (Equal(modelLocation, *newLocation)){
          // qDebug() << "Merge " << modelLocation->getLabel() << " to location " << (*newLocation)->getLabel();
          modelLocation->mergeWith(**newLocation);
          merged=true;

          // emit data change
          QVector<int> roles;
          roles << LatRole << LonRole << DistanceRole << BearingRole;
          emit dataChanged(createIndex(index, 0), createIndex(index, 0), roles);
          break;
        }
        index++;
      }
      if (merged) {
        newLocation = foundLocations.erase(newLocation);
      } else {
        ++newLocation;
      }
    }
  }

  if (compareFn.isCallable()){
    auto Compare = [&](const LocationEntryRef& a, const LocationEntryRef& b) -> bool {
      assert(a != nullptr && b != nullptr);
      if (a.get() == b.get()) {
        return false; // comp(a,a)==false
      }
      QJSValueList args;
      // to transfer ownership to QML, parent have to be null. copy constructor copy ownership
      assert(a->parent() == nullptr && b->parent() == nullptr);
      args << engine->newQObject(new LocationEntry(*a));
      args << engine->newQObject(new LocationEntry(*b));
      QJSValue result = compareFn.call(args);
      comparisonsCall++;
      if (result.isError()){
        qWarning() << "Compare failed: " << result.toString();
        return false;
      }
      if (result.isNumber()){
        return result.toNumber() < 0; // true if the first argument is less than (i.e. is ordered before) the second
      }
      return false;
    };

    // sort new location entries
    std::sort(foundLocations.begin(), foundLocations.end(), Compare);

    // use merge sort with existing location entries, they are sorted already
    auto position = 0;
    auto positionIt = locations.begin();
    for (auto &location : foundLocations) {
      for (bool inserted = false; !inserted; ) {
        if (positionIt == locations.end() || Compare(location, *positionIt)){
          emit beginInsertRows(QModelIndex(), position, position);
          positionIt = locations.insert(positionIt, location);
          // qDebug() << "Put " << location->getLabel() << " to position: " << position;
          emit endInsertRows();
          inserted = true;
        }
        ++positionIt;
        ++position;
      }
    }
  } else {
    // no sorting, just append new entries to the end
    if (!foundLocations.empty()) {
      emit beginInsertRows(QModelIndex(), locations.size(), locations.size() + foundLocations.size() - 1);
      for (auto &location : foundLocations) {
        locations.push_back(location);
      }
      emit endInsertRows();
    }
  }

  if (locations.size()>displayLimit) {
    emit beginRemoveRows(QModelIndex(), displayLimit, locations.size() -1);
    locations.erase(locations.begin()+displayLimit, locations.end());
    emit endRemoveRows();
  }

  emit countChanged(locations.size());
  qDebug() << "added" << foundLocations.size()
           << "(in" << timer.elapsed() << "ms,"
           << "equal" << equalityCall << "x, compare" << comparisonsCall << "x),"
           << "model size" << locations.size();
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
    osmscout::log.Debug() << "Search again with new default region: " << defaultRegion->name();
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
    emit SearchingChanged(isSearching());
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
  locations.clear();
  endRemoveRows();
  emit countChanged(locations.size());
  postponedEntries.clear();
  postponeTimer.stop();

  std::string stdPattern=pattern.toUtf8().constData();

  osmscout::GeoCoord coord;

  if (osmscout::GeoCoord::Parse(stdPattern, coord)) {
      QString name=QString::fromLocal8Bit(coord.GetDisplayText().c_str());
      QString label=name;

      LocationEntryRef location=std::make_shared<LocationEntry>(label, coord);
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
    qDebug() << "Clear (" << locations.size() << ") postpone search" << pattern << "(default region:" << (defaultRegion?defaultRegion->qStringName():"NULL") << ")";
    return;
  }
  
  qDebug() << "Clear (" << locations.size() << ") search" << pattern << "(default region:" << (defaultRegion?defaultRegion->qStringName():"NULL") << ")";
  searching = true;
  lastRequestPattern = pattern;
  lastRequestDefaultRegion=defaultRegion;
  emit SearchingChanged(isSearching());
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

  LocationEntryRef location=locations.at(index.row());

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
    return LookupModule::AdminRegionNames(location->getAdminRegionList(), settings->GetShowAltLanguage());
  case LatRole:
    return QVariant::fromValue(location->getCoord().GetLat());
  case LonRole:
    return QVariant::fromValue(location->getCoord().GetLon());
  case DistanceRole:
    if (searchCenter.GetLat()!=INVALID_COORD && searchCenter.GetLon()!=INVALID_COORD) {
      return osmscout::GetSphericalDistance(location->getCoord(), searchCenter).AsMeter();
    }else{
      return 0;
    }
  case BearingRole:
    if (searchCenter.GetLat()!=INVALID_COORD && searchCenter.GetLon()!=INVALID_COORD) {
      return QString::fromStdString(
              osmscout::GetSphericalBearingInitial(searchCenter, location->getCoord())
                .LongDisplayString());
    }else{
      return "";
    }
  case LocationObjectRole:
    // QML will take ownership
    return QVariant::fromValue(new LocationEntry(*location));
  case IndexedAdminRegionRole:
      return LookupModule::IndexedAdminRegionNames(location->getAdminRegionList(), settings->GetShowAltLanguage());;
  case AltLangName:
    return location->getAltName();
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
  roles[IndexedAdminRegionRole]="indexedAdminRegion";
  roles[AltLangName]="altLangName";

  return roles;
}

QObject* LocationListModel::get(int row) const
{
    if(row < 0 || row >= locations.size()) {
        return nullptr;
    }

    LocationEntryRef location=locations.at(row);
    // QML will take ownership
    return new LocationEntry(*location);
}

void LocationListModel::lookupRegion()
{
  if (searchCenter.GetLat()!=INVALID_COORD && searchCenter.GetLon()!=INVALID_COORD){
    emit regionLookupRequested(searchCenter);
  }
}
}
