
/*
 OSMScout - a Qt backend for libosmscout and libosmscout-map
 Copyright (C) 2016  Lukas Karas

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

#include <osmscoutclientqt/MapObjectInfoModel.h>
#include <osmscoutclientqt/OSMScoutQt.h>
#include <iostream>

namespace osmscout {

MapObjectInfoModel::MapObjectInfoModel():
ready(false), setup(false), view(), lookupModule(nullptr)
{

  lookupModule=OSMScoutQt::GetInstance().MakeLookupModule();
  settings=OSMScoutQt::GetInstance().GetSettings();
  this->mapDpi=settings->GetMapDPI();

  connect(lookupModule, &LookupModule::initialisationFinished,
          this, &MapObjectInfoModel::dbInitialized,
          Qt::QueuedConnection);

  connect(this, &MapObjectInfoModel::objectsOnViewRequested,
          lookupModule, &LookupModule::requestObjectsOnView,
          Qt::QueuedConnection);

  connect(lookupModule, &LookupModule::viewObjectsLoaded,
          this, &MapObjectInfoModel::onViewObjectsLoaded,
          Qt::QueuedConnection);

  connect(this, &MapObjectInfoModel::objectsRequested,
          lookupModule, &LookupModule::requestObjects,
          Qt::QueuedConnection);

  connect(lookupModule, &LookupModule::objectsLoaded,
          this, &MapObjectInfoModel::onObjectsLoaded,
          Qt::QueuedConnection);
}

MapObjectInfoModel::~MapObjectInfoModel()
{
  if (lookupModule!=nullptr){
    lookupModule->deleteLater();
    lookupModule=nullptr;
  }
}

void MapObjectInfoModel::dbInitialized(const DatabaseLoadedResponse&)
{
  if (setup){
    emit objectsOnViewRequested(view,filterRectangle);
  }
}

Qt::ItemFlags MapObjectInfoModel::flags(const QModelIndex &index) const
{
  if(!index.isValid()) {
      return Qt::ItemIsEnabled;
  }

  return QAbstractListModel::flags(index) | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QHash<int, QByteArray> MapObjectInfoModel::roleNames() const
{
  QHash<int, QByteArray> roles=QAbstractListModel::roleNames();

  roles[LabelRole]           = "label";
  roles[TypeRole]            = "type";
  roles[IdRole]              = "id";
  roles[NameRole]            = "name";
  roles[ObjectRole]          = "object";
  roles[PhoneRole]           = "phone";
  roles[WebsiteRole]         = "website";
  roles[AddressLocationRole] = "addressLocation";
  roles[AddressNumberRole]   = "addressNumber";
  roles[PostalCodeRole]      = "postalCode";
  roles[RegionRole]          = "region";
  roles[LatRole]             = "lat";
  roles[LonRole]             = "lon";
  roles[AltLangName]         = "altLangName";

  return roles;
}

QObject* MapObjectInfoModel::createOverlayObject(int row) const
{
  OverlayObject *o=nullptr;
  if(row < 0 || row >= model.size()) {
    qDebug() << "Undefined row" << row;
    return o;
  }
  const LookupModule::ObjectInfo &obj = model.at(row);
  if (!obj.points.empty()){
    if (obj.type=="node"){
      o=new OverlayNode();
    } else if (obj.type=="way"){
      o=new OverlayWay();
    } else if (obj.type=="area"){
      o=new OverlayArea();
    }
    if (o!=nullptr) {
      for (const auto& p:obj.points) {
        o->addPoint(p.GetLat(), p.GetLon());
      }
    }
  } else {
    qWarning() << "Object " << obj.name << " (" << obj.type << " / " << obj.objectType << ") has no points!";
  }

  return o;
}

QVariant MapObjectInfoModel::data(const QModelIndex &index, int role) const
{
  //qDebug() << "Get data" << index.row() << " role: " << role;

  if(index.row() < 0 || index.row() >= model.size()) {
    qDebug() << "Undefined row" << index.row();
    return QVariant();
  }
  const LookupModule::ObjectInfo &obj = model.at(index.row());

  if (role==LabelRole){
    return QVariant::fromValue(obj.type);
  }
  if (role==TypeRole){
    return QVariant::fromValue(obj.objectType);
  }
  if (role==IdRole){
    return QVariant::fromValue(obj.id);
  }
  if (role==NameRole){
    if (obj.name.isEmpty())
      return QVariant();
    return QVariant::fromValue(obj.name);
  }
  if (role==AltLangName) {
    if (obj.altLangName.isEmpty()) {
      return QVariant();
    }
    return QVariant::fromValue(obj.altLangName);
  }

  if (role==ObjectRole){
    return QVariant::fromValue(createOverlayObject(index.row()));
  }

  if (role==PhoneRole){
    return QVariant::fromValue(obj.phone);
  }
  if (role==WebsiteRole){
    return QVariant::fromValue(obj.website);
  }
  if (role==AddressNumberRole){
    return QVariant::fromValue(obj.addressNumber);
  }
  if (role==AddressLocationRole){
    if (!obj.reverseLookupRef || !obj.reverseLookupRef->location){
      return QVariant("");
    }
    return QVariant::fromValue(QString::fromStdString(obj.reverseLookupRef->location->name));
  }
  if (role==PostalCodeRole){
    if (!obj.reverseLookupRef || !obj.reverseLookupRef->postalArea){
      return QVariant("");
    }
    return QVariant::fromValue(QString::fromStdString(obj.reverseLookupRef->postalArea->name));
  }
  if (role==RegionRole){
    return QVariant::fromValue(LookupModule::AdminRegionNames(obj.adminRegionList, settings->GetShowAltLanguage()));
  }
  if (role==LatRole){
    return QVariant::fromValue(obj.center.GetLat());
  }
  if (role==LonRole){
    return QVariant::fromValue(obj.center.GetLon());
  }

  //qDebug() << "Undefined role" << role << "("<<LabelRole<<"..."<<NameRole<<")";
  return QVariant();
}

void MapObjectInfoModel::setPosition(QObject *o,
                                     const int width, const int height,
                                     const int screenX, const int screenY)
{
  MapView *mapView = dynamic_cast<MapView*>(o);
  if (mapView ==nullptr){
      qWarning() << "Failed to cast " << o << " to MapView*.";
      return;
  }
  MapViewStruct r;
  r.angle=mapView->angle;
  r.coord=mapView->center;
  r.width=width;
  r.height=height;
  r.magnification=mapView->magnification;
  r.dpi=mapDpi;

  double tolerance=mapDpi/4;
  QRectF rectangle(screenX-tolerance, screenY-tolerance, tolerance*2, tolerance*2);

  this->ready=false;
  emit readyChange(ready);

  if (this->view!=r || this->filterRectangle!=rectangle){
    this->view=r;
    this->filterRectangle=rectangle;
    beginResetModel();
    model.clear();
    mapData.clear();
    endResetModel();

    emit objectsOnViewRequested(view,rectangle);
  }else{
    // just emit signal that model is ready
    addToModel(QList<LookupModule::ObjectInfo>());
  }
  setup=true;
}

void MapObjectInfoModel::setLocationEntry(QObject *o)
{
  LocationEntry *location = dynamic_cast<LocationEntry*>(o);
  if (location ==nullptr){
    qWarning() << "Failed to cast " << o << " to LocationEntry*.";
    return;
  }
  locationEntry=*location;

  beginResetModel();
  model.clear();
  mapData.clear();
  endResetModel();

  this->ready=false;
  emit readyChange(ready);
  emit objectsRequested(*location, true);
}

void MapObjectInfoModel::onObjectsLoaded(const LocationEntry &entry,
                                         const QList<LookupModule::ObjectInfo> &objects)
{
  if (locationEntry.getDatabase()!=entry.getDatabase() ||
      locationEntry.getReferences()!=entry.getReferences()){
    return; // ignore
  }

  addToModel(objects);
}

void MapObjectInfoModel::onViewObjectsLoaded(const MapViewStruct &view,
                                             const QList<LookupModule::ObjectInfo> &objects)
{
  if (this->view!=view){
    return;
  }

  addToModel(objects);
}

void MapObjectInfoModel::addToModel(const QList<LookupModule::ObjectInfo> &objects)
{
  beginResetModel();
  //std::cout << "count: "<< model.size() << std::endl;
  model << objects;
  endResetModel();

  this->ready=true;
  emit readyChange(ready);
}
}
