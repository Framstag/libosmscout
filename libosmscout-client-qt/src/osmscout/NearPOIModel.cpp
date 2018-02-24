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

NearPOIModel::NearPOIModel()
{
  poiModule=OSMScoutQt::GetInstance().MakePOILookupModule();

  connect(this, SIGNAL(lookupPOI(int, osmscout::BreakerRef, osmscout::GeoCoord, QStringList, double)),
          poiModule, SLOT(lookupPOIRequest(int, osmscout::BreakerRef, osmscout::GeoCoord, QStringList, double)),
          Qt::QueuedConnection);

  connect(poiModule, SIGNAL(lookupAborted(int)),
          this, SLOT(onLookupFinished(int)),
          Qt::QueuedConnection);
  connect(poiModule, SIGNAL(lookupFinished(int)),
          this, SLOT(onLookupFinished(int)),
          Qt::QueuedConnection);
  connect(poiModule, SIGNAL(lookupResult(int, QList<LocationEntry>)),
          this, SLOT(onLookupResult(int, QList<LocationEntry>)),
          Qt::QueuedConnection);
}

NearPOIModel::~NearPOIModel()
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

LocationEntry* NearPOIModel::get(int row) const
{
  if(row < 0 || row >= locations.size()) {
    return NULL;
  }

  LocationEntry* location=locations.at(row);
  // QML will take ownerhip
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
    double distanceKm=osmscout::GetSphericalDistance(location.getCoord(), searchCenter);
    if ((distanceKm*1000)>maxDistance){
      continue;
    }
    for (LocationEntry* secondLocation:locations) {
      if (distanceKm < osmscout::GetSphericalDistance(secondLocation->getCoord(), searchCenter)){
        break;
      }
      position++;
    }

    emit beginInsertRows(QModelIndex(), position, position);
    locations.insert(position, new LocationEntry(location));
    qDebug() << "Put " << location.getObjectType() << location.getLabel() << " to position: " << position << "(distance" << distanceKm << ")";
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
    for (QList<LocationEntry*>::iterator location=locations.begin();
         location!=locations.end();
         ++location) {
      delete *location;
    }

    locations.clear();
    endResetModel();
    emit countChanged(locations.size());
  }

  if (searchCenter.GetLat()!=INVALID_COORD &&
      searchCenter.GetLon()!=INVALID_COORD &&
      !types.isEmpty() &&
      maxDistance>0 &&
      resultLimit>0){

    breaker=std::make_shared<osmscout::ThreadedBreaker>();
    searching=true;
    // TODO: use resultLimit
    emit lookupPOI(++currentRequest, breaker, searchCenter, types, maxDistance);
  }else{
    searching=false;
  }
  emit SearchingChanged(searching);
}

