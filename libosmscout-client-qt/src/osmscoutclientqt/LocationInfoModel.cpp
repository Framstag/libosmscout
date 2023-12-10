
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

#include <osmscoutclientqt/LocationInfoModel.h>

#include <algorithm>

#include <osmscout/feature/OpeningHoursFeature.h>
#include <osmscout/feature/PostalCodeFeature.h>

#include <osmscoutclientqt/OSMScoutQt.h>

#include <QtCore/qabstractitemmodel.h>

namespace osmscout {

LocationInfoModel::LocationInfoModel():
  ready(false), setup(false)
{
    lookupModule=OSMScoutQt::GetInstance().MakeLookupModule();
    settings=OSMScoutQt::GetInstance().GetSettings();

    connect(lookupModule, &LookupModule::initialisationFinished,
            this, &LocationInfoModel::dbInitialized,
            Qt::QueuedConnection);

    connect(this, &LocationInfoModel::locationDescriptionRequested,
            lookupModule, &LookupModule::requestLocationDescription,
            Qt::QueuedConnection);

    connect(lookupModule, &LookupModule::locationDescription,
            this, &LocationInfoModel::onLocationDescription,
            Qt::QueuedConnection);

    connect(lookupModule, &LookupModule::locationDescriptionFinished,
            this, &LocationInfoModel::onLocationDescriptionFinished,
            Qt::QueuedConnection);

    connect(this, &LocationInfoModel::regionLookupRequested,
            lookupModule, &LookupModule::requestRegionLookup,
            Qt::QueuedConnection);

    connect(lookupModule, &LookupModule::locationAdminRegions,
            this, &LocationInfoModel::onLocationAdminRegions,
            Qt::QueuedConnection);

    connect(lookupModule, &LookupModule::locationAdminRegionFinished,
            this, &LocationInfoModel::onLocationAdminRegionFinished,
            Qt::QueuedConnection);
}

LocationInfoModel::~LocationInfoModel()
{
    if (lookupModule!=nullptr){
        lookupModule->deleteLater();
        lookupModule=nullptr;
    }
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

void LocationInfoModel::dbInitialized(const osmscout::GeoBox&)
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
    roles[AddressLocationRole] = "addressLocation";
    roles[AddressNumberRole] = "addressNumber";
    roles[IndexedAdminRegionRole] = "indexedAdminRegion";
    roles[AltLangName]="altLangName";
    roles[OpeningHours]="openingHours";

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

bool LocationInfoModel::adminRegionComparator(const AdminRegionInfoRef& reg1,
                                              const AdminRegionInfoRef& reg2)
{
  return reg1->adminLevel > reg2->adminLevel;
}

bool operator==(const ObjectKey &k1, const ObjectKey &k2){
  return (k1.database == k2.database) &&
    (k1.ref.type == k2.ref.type) &&
    (k1.ref.offset == k2.ref.offset);
}

void LocationInfoModel::addToModel(const QString database,
                                   const osmscout::LocationAtPlaceDescriptionRef description,
                                   const QList<AdminRegionInfoRef> regions)
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

  Distance distance = description->GetDistance();
  bool inPlace = description->IsAtPlace() || (distance.AsMeter() < 1);

  QStringList addressParts;
  QString addressLocation;
  QString addressNumber;
  if (locRef){
    addressLocation = QString::fromStdString(locRef->name);
    addressParts << addressLocation;
  }
  if (addrRef){
    addressNumber = QString::fromStdString(addrRef->name);
    addressParts << addressNumber;
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
  QString altName;
  QString openingHours;
  if (place.GetObjectFeatures()){
    for (const auto& featureInstance :place.GetObjectFeatures()->GetType()->GetFeatures()){
      if (place.GetObjectFeatures()->HasFeature(featureInstance.GetIndex())){
        osmscout::FeatureRef feature=featureInstance.GetFeature();
        if (feature->HasValue()){
          osmscout::FeatureValue *value=place.GetObjectFeatures()->GetValue(featureInstance.GetIndex());


          if (const osmscout::PostalCodeFeatureValue *postalCodeValue = dynamic_cast<const osmscout::PostalCodeFeatureValue*>(value);
              postalCodeValue!=nullptr){
            postalCode = QString::fromStdString(postalCodeValue->GetPostalCode());
          } else if (const osmscout::WebsiteFeatureValue *websiteValue = dynamic_cast<const osmscout::WebsiteFeatureValue*>(value);
                     websiteValue!=nullptr){
            website = QString::fromStdString(websiteValue->GetWebsite());
          } else if (const osmscout::PhoneFeatureValue *phoneValue = dynamic_cast<const osmscout::PhoneFeatureValue*>(value);
                     phoneValue!=nullptr){
            phone = QString::fromStdString(phoneValue->GetPhone());
          } else if (const osmscout::NameAltFeatureValue *altNameValue = dynamic_cast<const osmscout::NameAltFeatureValue*>(value);
                     altNameValue != nullptr){
            altName = QString::fromStdString(altNameValue->GetNameAlt());
          } else if (const osmscout::OpeningHoursFeatureValue *openingHoursValue = dynamic_cast<const osmscout::OpeningHoursFeatureValue*>(value);
                     openingHoursValue != nullptr) {
            openingHours = QString::fromStdString(openingHoursValue->GetValue());
          }
        }
      }
    }
  }

  if (postalCode.isEmpty() && place.GetPostalArea()){
    // postal code is not part of the object (address), but we resolved postal region
    postalCode = QString::fromStdString(place.GetPostalArea()->name);
  }

  obj[LabelRole] = QString::fromStdString(place.GetDisplayString());
  obj[RegionRole] = LookupModule::AdminRegionNames(regions, settings->GetShowAltLanguage());
  obj[AddressRole] = address;
  obj[InPlaceRole] = inPlace;
  obj[DistanceRole] = distance.AsMeter();
  obj[BearingRole] = QString::fromStdString(description->GetBearing().LongDisplayString());
  obj[PoiRole] = (poiRef) ? QString::fromStdString(poiRef->name): "";
  obj[TypeRole] = QString::fromStdString(place.GetObjectFeatures()->GetType()->GetName());
  obj[PostalCodeRole] = postalCode;
  obj[WebsiteRole] = website;
  obj[PhoneRole] = phone;
  obj[AddressLocationRole] = addressLocation;
  obj[AddressNumberRole] = addressNumber;
  obj[IndexedAdminRegionRole] = LookupModule::IndexedAdminRegionNames(regions, settings->GetShowAltLanguage());
  obj[AltLangName] = altName;
  obj[OpeningHours] = openingHours;

  model << obj;

  std::sort(model.begin(),model.end(),distanceComparator);
  endResetModel();
}

void LocationInfoModel::onLocationDescription(const osmscout::GeoCoord location,
                                              const QString database,
                                              const osmscout::LocationDescription description,
                                              const QList<AdminRegionInfoRef> regions)
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
                     << atAddressDescription->GetDistance().AsMeter() << " m "
                     << QString::fromStdString(atAddressDescription->GetBearing().LongDisplayString()) << " from "
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
    if (model.isEmpty()){
      // if model is empty, request for location region
      emit regionLookupRequested(location);
    } else {
      ready = true;
      emit readyChange(ready);
    }
}

void LocationInfoModel::onLocationAdminRegions(const osmscout::GeoCoord location,
                                               QList<AdminRegionInfoRef> regions)
{
  if (location != this->location || regions.empty()){
    return; // not our request
  }
  beginResetModel();

  QMap<int, QVariant> obj;

  std::sort(regions.begin(),regions.end(),adminRegionComparator);

  const AdminRegionInfoRef bottom=regions.first();
  QStringList regionNames=LookupModule::AdminRegionNames(regions, settings->GetShowAltLanguage());

  obj[LabelRole] = QString::fromStdString(bottom->name());
  obj[RegionRole] = regionNames;
  obj[AddressRole] = QString::fromStdString(bottom->name());
  obj[InPlaceRole] = true;
  obj[DistanceRole] = 0;
  obj[BearingRole] = "";
  obj[PoiRole] = "";
  obj[TypeRole] = QString::fromStdString(bottom->type);
  obj[PostalCodeRole] = "";
  obj[WebsiteRole] = "";
  obj[PhoneRole] = "";
  obj[AddressLocationRole] = "";
  obj[AddressNumberRole] = "";
  obj[IndexedAdminRegionRole] = QStringList();
  obj[AltLangName] = QString::fromStdString(bottom->altName());
  obj[OpeningHours] = "";

  model << obj;

  std::sort(model.begin(),model.end(),distanceComparator);
  endResetModel();
}

void LocationInfoModel::onLocationAdminRegionFinished(const osmscout::GeoCoord)
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
            osmscout::GeoCoord(lat2, lon2)).AsMeter();
}

QString LocationInfoModel::bearing(double lat1, double lon1,
                                   double lat2, double lon2)
{
    return QString::fromStdString(
                osmscout::GetSphericalBearingInitial(
                    osmscout::GeoCoord(lat1, lon1),
                    osmscout::GeoCoord(lat2, lon2)).LongDisplayString());
}
}
