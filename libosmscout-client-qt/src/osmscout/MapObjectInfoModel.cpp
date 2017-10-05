
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

#include <osmscout/MapObjectInfoModel.h>
#include <osmscout/OSMScoutQt.h>

MapObjectInfoModel::MapObjectInfoModel():
ready(false), setup(false), view(), lookupModule(NULL)
{

  lookupModule=OSMScoutQt::GetInstance().MakeLookupModule();
  this->mapDpi=OSMScoutQt::GetInstance().GetSettings()->GetMapDPI();

  connect(lookupModule, SIGNAL(initialisationFinished(const DatabaseLoadedResponse&)),
          this, SLOT(dbInitialized(const DatabaseLoadedResponse&)),
          Qt::QueuedConnection);

  connect(this, SIGNAL(objectsRequested(const MapViewStruct &)),
          lookupModule, SLOT(requestObjectsOnView(const MapViewStruct&)),
          Qt::QueuedConnection);

  connect(lookupModule, SIGNAL(viewObjectsLoaded(const MapViewStruct&, const osmscout::MapData&)),
          this, SLOT(onViewObjectsLoaded(const MapViewStruct&, const osmscout::MapData&)),
          Qt::QueuedConnection);
}

MapObjectInfoModel::~MapObjectInfoModel()
{
  if (lookupModule!=NULL){
    lookupModule->deleteLater();
    lookupModule=NULL;
  }
}

void MapObjectInfoModel::dbInitialized(const DatabaseLoadedResponse&)
{
  if (setup){
    emit objectsRequested(view);
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

  roles[LabelRole]="label";
  roles[TypeRole] ="type";
  roles[IdRole]   ="id";
  roles[NameRole] ="name";
  
  return roles;
}

QVariant MapObjectInfoModel::data(const QModelIndex &index, int role) const
{
  //qDebug() << "Get data" << index.row() << " role: " << role;

  if(index.row() < 0 || index.row() >= model.size()) {
    qDebug() << "Undefined row" << index.row();
    return QVariant();
  }
  QMap<int, QVariant> obj = model.at(index.row());
  if (!obj.contains(role)){
    //qDebug() << "Undefined role" << role << "("<<LabelRole<<"..."<<NameRole<<")";
    return QVariant();
  }
  return obj[role];
}

void MapObjectInfoModel::setPosition(QObject *o,
                                     const int width, const int height,
                                     const int screenX, const int screenY)
{
  MapView *mapView = dynamic_cast<MapView*>(o);
  if (mapView == NULL){
      qWarning() << "Failed to cast " << o << " to MapView*.";
      return;
  }
  MapViewStruct r;
  r.angle=mapView->angle;
  r.coord=mapView->center;
  r.width=width;
  r.height=height;
  r.magnification=mapView->magnification;

  this->screenX=screenX;
  this->screenY=screenY;
  this->ready=false;
  emit readyChange(ready);

  if (this->view!=r){
    this->view=r;
    beginResetModel();
    model.clear();
    mapData.clear();
    endResetModel();
    emit objectsRequested(view);
  }else{
    update();
  }
  setup=true;
}

void MapObjectInfoModel::onViewObjectsLoaded(const MapViewStruct &view,
                                             const osmscout::MapData &data)
{
  if (this->view!=view){
    return;
  }
  mapData << data;
  update();
}

void MapObjectInfoModel::update()
{
  osmscout::MercatorProjection projection;
  projection.Set(view.coord, /* angle */ 0, view.magnification, mapDpi, view.width, view.height);
  projection.SetLinearInterpolationUsage(view.magnification.GetLevel() >= 10);

  beginResetModel();
  model.clear();
  //std::cout << "object near " << this->screenX << " " << this->screenY << ":" << std::endl;

  double x;
  double y;
  double x2;
  double y2;
  double tolerance=mapDpi/4;
  QRectF rectangle(this->screenX-tolerance, this->screenY-tolerance, tolerance*2, tolerance*2);
  for (auto const &d:mapData){

    //std::cout << "nodes: " << d.nodes.size() << std::endl;
    for (auto const &n:d.nodes){
      projection.GeoToPixel(n->GetCoords(),x,y);
      if (rectangle.contains(x,y)){
        fillObjectInfo("node",n);
      }
    }

    //std::cout << "ways:  " << d.ways.size() << std::endl;
    for (auto const &w:d.ways){
      // TODO: better detection
      osmscout::GeoBox bbox;
      w->GetBoundingBox(bbox);
      projection.GeoToPixel(bbox.GetMinCoord(),x,y);
      projection.GeoToPixel(bbox.GetMaxCoord(),x2,y2);
      if (rectangle.intersects(QRectF(QPointF(x,y),QPointF(x2,y2)))){
        fillObjectInfo("way",w);
      }
    }

    //std::cout << "areas: " << d.areas.size() << std::endl;
    for (auto const &a:d.areas){
      // TODO: better detection
      osmscout::GeoBox bbox;
      a->GetBoundingBox(bbox);
      projection.GeoToPixel(bbox.GetMinCoord(),x,y);
      projection.GeoToPixel(bbox.GetMaxCoord(),x2,y2);
      if (rectangle.intersects(QRectF(QPointF(x,y),QPointF(x2,y2)))){
        fillObjectInfo("area",a);
      }
    }
  }
  //std::cout << "count: "<< model.size() << std::endl;
  endResetModel();

  this->ready=true;
  emit readyChange(ready);
}
