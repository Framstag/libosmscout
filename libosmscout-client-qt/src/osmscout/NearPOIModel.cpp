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

#include <osmscout/NearPOIModel.h>
#include <osmscout/OSMScoutQt.h>

namespace osmscout {

NearPOIModel::NearPOIModel()
{
  poiModule=OSMScoutQt::GetInstance().MakePOILookupModule();

  connect(this, &NearPOIModel::lookupPOIRequest,
          poiModule, &POILookupModule::lookupPOIRequest,
          Qt::QueuedConnection);

  connect(poiModule, &POILookupModule::lookupAborted,
          this, &NearPOIModel::onLookupFinished,
          Qt::QueuedConnection);
  connect(poiModule, &POILookupModule::lookupFinished,
          this, &NearPOIModel::onLookupFinished,
          Qt::QueuedConnection);
  connect(poiModule, &POILookupModule::lookupResult,
          this, &NearPOIModel::onLookupResult,
          Qt::QueuedConnection);
}

NearPOIModel::~NearPOIModel()
{
  locations.clear();

  if (breaker){
    breaker->Break();
    breaker.reset();
  }
  if (poiModule!=nullptr){
    poiModule->deleteLater();
    poiModule=nullptr;
  }
}


QVariant NearPOIModel::data(const QModelIndex &index, int role) const
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
      return location->getAdminRegionList();
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
            osmscout::GetSphericalBearingInitial(searchCenter, location->getCoord()).LongDisplayString());
      }else{
        return "";
      }
    case LocationObjectRole:
      // QML will take ownership
      return QVariant::fromValue(new LocationEntry(*location));
    default:
      break;
  }

  return QVariant();
}

Qt::ItemFlags NearPOIModel::flags(const QModelIndex &index) const
{
  if(!index.isValid()) {
    return Qt::ItemIsEnabled;
  }

  return QAbstractListModel::flags(index) | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QHash<int, QByteArray> NearPOIModel::roleNames() const
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

QObject* NearPOIModel::get(int row) const
{
  if(row < 0 || row >= locations.size()) {
    return nullptr;
  }

  LocationEntryRef location=locations.at(row);
  // QML will take ownership
  return new LocationEntry(*location);
}

int NearPOIModel::rowCount(const QModelIndex &/*parent*/) const
{
  return locations.size();
}

void NearPOIModel::onLookupFinished(int requestId)
{
  if (requestId!=currentRequest){
    return;
  }
  searching=false;
  emit SearchingChanged(searching);
}

void NearPOIModel::onLookupResult(int requestId, QList<LocationEntry> newLocations)
{
  if (requestId!=currentRequest){
    return;
  }
  for (LocationEntry &location:newLocations){
    int position=0;
    Distance distance=osmscout::GetSphericalDistance(location.getCoord(), searchCenter);
    if (distance > maxDistance){
      continue;
    }
    for (const LocationEntryRef& secondLocation:locations) {
      if (distance < osmscout::GetSphericalDistance(secondLocation->getCoord(), searchCenter)){
        break;
      }
      position++;
    }

    // do not care of result outside the limit
    if (position > resultLimit) {
      continue;
    }

    emit beginInsertRows(QModelIndex(), position, position);
    locations.insert(position, std::make_shared<LocationEntry>(location));
    qDebug() << "Put " << location.getObjectType() << location.getLabel() << " to position: " << position << "(distance" << distance.AsMeter() << " m)";
    emit endInsertRows();
  }
}

void NearPOIModel::lookupPOI()
{
  if (breaker){
    breaker->Break();
  }

  if (!locations.isEmpty()){
    beginResetModel();
    locations.clear();
    endResetModel();
    emit countChanged(locations.size());
  }

  if (searchCenter.GetLat()!=INVALID_COORD &&
      searchCenter.GetLon()!=INVALID_COORD &&
      !types.isEmpty() &&
      maxDistance.AsMeter()>0 &&
      resultLimit>0){

    breaker=std::make_shared<osmscout::ThreadedBreaker>();
    searching=true;
    // TODO: use resultLimit
    emit lookupPOIRequest(++currentRequest, breaker, searchCenter, types, maxDistance.AsMeter());
  }else{
    searching=false;
  }
  emit SearchingChanged(searching);
}

}
