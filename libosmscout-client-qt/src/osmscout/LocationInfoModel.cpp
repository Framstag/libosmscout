
/*
  This source is part of the libosmscout-map library
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

#include <QtCore/qabstractitemmodel.h>

#include <osmscout/LocationInfoModel.h>
#include <osmscout/OSMScoutQt.h>

LocationInfoModel::LocationInfoModel(): 
  ready(false), setup(false)
{
    lookupModule=OSMScoutQt::GetInstance().MakeLookupModule();

    connect(lookupModule, SIGNAL(initialisationFinished(const DatabaseLoadedResponse&)),
            this, SLOT(dbInitialized(const DatabaseLoadedResponse&)),
            Qt::QueuedConnection);
    
    connect(this, SIGNAL(locationDescriptionRequested(const osmscout::GeoCoord)), 
            lookupModule, SLOT(requestLocationDescription(const osmscout::GeoCoord)),
            Qt::QueuedConnection);
    
    connect(lookupModule, SIGNAL(locationDescription(const osmscout::GeoCoord, const QString, const osmscout::LocationDescription, const QStringList)),
            this, SLOT(onLocationDescription(const osmscout::GeoCoord, const QString, const osmscout::LocationDescription, const QStringList)),
            Qt::QueuedConnection);
    
    connect(lookupModule, SIGNAL(locationDescriptionFinished(const osmscout::GeoCoord)),
            this, SLOT(onLocationDescriptionFinished(const osmscout::GeoCoord)),
            Qt::QueuedConnection);    

}

void LocationInfoModel::setLocation(const double lat, const double lon)
{
    location = osmscout::GeoCoord(lat, lon);
    setup = true;
    ready = false;
    
    beginResetModel();
    objectSet.clear();
    model.clear();
    endResetModel();

    emit readyChange(ready);
    emit locationDescriptionRequested(location);
}

void LocationInfoModel::dbInitialized(const DatabaseLoadedResponse&)
{
    if (setup){
        emit locationDescriptionRequested(location);
    }
}

Qt::ItemFlags LocationInfoModel::flags(const QModelIndex &index) const
{
    if(!index.isValid()) {
        return Qt::ItemIsEnabled;
    }

    return QAbstractListModel::flags(index) | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QHash<int, QByteArray> LocationInfoModel::roleNames() const
{
    QHash<int, QByteArray> roles=QAbstractListModel::roleNames();

    roles[LabelRole]="label";
    roles[RegionRole]="region";
    roles[AddressRole]="address";
    roles[InPlaceRole]="inPlace";
    roles[DistanceRole]="distance";
    roles[BearingRole]="bearing";
    roles[PoiRole]="poi";
    roles[TypeRole] = "type";
    roles[PostalCodeRole] = "postalCode";
    roles[WebsiteRole] = "website";
    roles[PhoneRole] = "phone";

    return roles;
}

QVariant LocationInfoModel::data(const QModelIndex &index, int role) const
{
    //qDebug() << "Get data" << index.row() << " role: " << role << " (label: " << LabelRole<< ")";
    
    if(index.row() < 0 || index.row() >= model.size()) {
      return QVariant();
    }
    QMap<int, QVariant> obj = model.at(index.row());
    if (!obj.contains(role)){
      return QVariant();
    }
    return obj[role];
}

bool LocationInfoModel::distanceComparator(const QMap<int, QVariant> &obj1,
                                           const QMap<int, QVariant> &obj2)
{
  QVariant dist1 = obj1.contains(DistanceRole) ? obj1[DistanceRole] : QVariant();
  QVariant dist2 = obj2.contains(DistanceRole) ? obj2[DistanceRole] : QVariant();

  return dist1.toReal() < dist2.toReal();
}

bool operator==(const ObjectKey &k1, const ObjectKey &k2){
  return (k1.database == k2.database) && 
    (k1.ref.type == k2.ref.type) && 
    (k1.ref.offset == k2.ref.offset);
}

void LocationInfoModel::addToModel(const QString database,
                                   const osmscout::LocationAtPlaceDescriptionRef description,
                                   const QStringList regions)
{
  ObjectKey key = {database, description->GetPlace().GetObject()};
  if (objectSet.contains(key)){
    return;
  }
  beginResetModel();
  objectSet << key;
  QMap<int, QVariant> obj;
  
  osmscout::Place place = description->GetPlace();
  
  osmscout::POIRef poiRef = place.GetPOI();
  osmscout::LocationRef locRef = place.GetLocation();
  osmscout::AddressRef addrRef = place.GetAddress();

  double distance = description->GetDistance();
  bool inPlace = description->IsAtPlace() || (distance < 1);

  QStringList addressParts;
  if (locRef){
      addressParts << QString::fromStdString(locRef->name);
  }
  if (addrRef){
      addressParts << QString::fromStdString(addrRef->name);
  }
  QString address;
  for (int i = 0; i < addressParts.size(); i++){
      address += addressParts.at(i);
      if (i < addressParts.size() -1){
          address += " ";
      }
  }

  QString postalCode;
  QString website;
  QString phone;
  if (place.GetObjectFeatures()){
    for (auto featureInstance :place.GetObjectFeatures()->GetType()->GetFeatures()){
      if (place.GetObjectFeatures()->HasFeature(featureInstance.GetIndex())){
        osmscout::FeatureRef feature=featureInstance.GetFeature();
        if (feature->HasValue()){
          osmscout::FeatureValue *value=place.GetObjectFeatures()->GetValue(featureInstance.GetIndex());
          
          const osmscout::PostalCodeFeatureValue *postalCodeValue = dynamic_cast<const osmscout::PostalCodeFeatureValue*>(value);
          if (postalCodeValue!=NULL){
            postalCode = QString::fromStdString(postalCodeValue->GetPostalCode());
          }
          
          const osmscout::WebsiteFeatureValue *websiteValue = dynamic_cast<const osmscout::WebsiteFeatureValue*>(value);
          if (websiteValue!=NULL){
            website = QString::fromStdString(websiteValue->GetWebsite());
          }
          
          const osmscout::PhoneFeatureValue *phoneValue = dynamic_cast<const osmscout::PhoneFeatureValue*>(value);
          if (phoneValue!=NULL){
            phone = QString::fromStdString(phoneValue->GetPhone());
          }
        }
      }
    }
  }

  obj[LabelRole] = QString::fromStdString(place.GetDisplayString());
  obj[RegionRole] = regions;
  obj[AddressRole] = address;
  obj[InPlaceRole] = inPlace;
  obj[DistanceRole] = distance;
  obj[BearingRole] = QString::fromStdString(osmscout::BearingDisplayString(description->GetBearing()));
  obj[PoiRole] = (poiRef) ? QString::fromStdString(poiRef->name): "";
  obj[TypeRole] = QString::fromStdString(place.GetObjectFeatures()->GetType()->GetName());
  obj[PostalCodeRole] = postalCode;
  obj[WebsiteRole] = website;
  obj[PhoneRole] = phone;

  
  model << obj;
  qSort(model.begin(),model.end(),distanceComparator);
  endResetModel();
}

void LocationInfoModel::onLocationDescription(const osmscout::GeoCoord location, 
                                              const QString database,
                                              const osmscout::LocationDescription description,
                                              const QStringList regions)
{
    if (location != this->location){
        return; // not our request
    }
    
    if (description.GetAtAddressDescription()){
      addToModel(database, description.GetAtAddressDescription(), regions);
    }
    if (description.GetAtPOIDescription()){
      addToModel(database, description.GetAtPOIDescription(), regions);
    }
    
    
    // just debug
    osmscout::LocationAtPlaceDescriptionRef atAddressDescription=description.GetAtAddressDescription();
    if (atAddressDescription) {
        
        osmscout::Place place = atAddressDescription->GetPlace();
        
        if (atAddressDescription->IsAtPlace()){
            qDebug() << "Place " << QString::fromStdString(location.GetDisplayText()) << " description: " 
                     << QString::fromStdString(place.GetDisplayString()); 
        }else{
            qDebug() << "Place " << QString::fromStdString(location.GetDisplayText()) << " description: " 
                     << atAddressDescription->GetDistance() << " m " 
                     << QString::fromStdString(osmscout::BearingDisplayString(atAddressDescription->GetBearing())) << " from "
                     << QString::fromStdString(place.GetDisplayString());
        }
    }else{
        qWarning() << "No place description found for " << QString::fromStdString(location.GetDisplayText()) << "";
    }
    // end of debug    
}

void LocationInfoModel::onLocationDescriptionFinished(const osmscout::GeoCoord location)
{
    if (location != this->location){
        return; // not our request
    }
    
    ready = true;
    emit readyChange(ready);      
}

double LocationInfoModel::distance(double lat1, double lon1, 
                                    double lat2, double lon2)
{
    

    return osmscout::GetEllipsoidalDistance(
            osmscout::GeoCoord(lat1, lon1),
            osmscout::GeoCoord(lat2, lon2)) * 1000;
}

QString LocationInfoModel::bearing(double lat1, double lon1, 
                                   double lat2, double lon2)
{
    return QString::fromStdString(
            osmscout::BearingDisplayString(
                osmscout::GetSphericalBearingInitial(
                    osmscout::GeoCoord(lat1, lon1),
                    osmscout::GeoCoord(lat2, lon2))));
}
