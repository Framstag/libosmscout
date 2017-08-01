/*
 OSMScout - a Qt backend for libosmscout and libosmscout-map
 Copyright (C) 2010  Tim Teulings

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

#include <osmscout/util/Logger.h>
#include <osmscout/RoutingModel.h>
#include <osmscout/OSMScoutQt.h>

RoutingListModel::RoutingListModel(QObject* parent)
: QAbstractListModel(parent), requestId(0)
{
  router=OSMScoutQt::GetInstance().MakeRouter();

  connect(this,SIGNAL(routeRequest(LocationEntryRef,LocationEntryRef,osmscout::Vehicle,int,osmscout::BreakerRef)),
          router,SLOT(onRouteRequest(LocationEntryRef,LocationEntryRef,osmscout::Vehicle,int,osmscout::BreakerRef)),
          Qt::QueuedConnection);

  connect(router,SIGNAL(routeComputed(RouteSelectionRef,int)),
          this,SLOT(onRouteComputed(RouteSelectionRef,int)),
          Qt::QueuedConnection);
  connect(router,SIGNAL(routeFailed(QString,int)),
          this,SLOT(onRouteFailed(QString,int)),
          Qt::QueuedConnection);
  connect(router,SIGNAL(routingProgress(int,int)),
          this,SLOT(onRoutingProgress(int,int)),
          Qt::QueuedConnection);
}

RoutingListModel::~RoutingListModel()
{
  if (route){
    route->routeSteps.clear();
  }
  if (router!=NULL){
    router->deleteLater();
    router=NULL;
  }
}

void RoutingListModel::setStartAndTarget(LocationEntry* start,
                                         LocationEntry* target,
                                         QString vehicleStr)
{
  osmscout::Vehicle vehicle=osmscout::Vehicle::vehicleCar;
  if (vehicleStr=="bicycle"){
    vehicle=osmscout::Vehicle::vehicleBicycle;
  } else if (vehicleStr=="foot"){
    vehicle=osmscout::Vehicle::vehicleFoot;
  }
  clear();
  computing=true;
  breaker=std::make_shared<osmscout::ThreadedBreaker>();
  emit computingChanged();

  // make copy to shared ptr, remove owhership
  LocationEntryRef startRef=std::make_shared<LocationEntry>(*start);
  startRef->setParent(Q_NULLPTR);
  LocationEntryRef targetRef=std::make_shared<LocationEntry>(*target);
  targetRef->setParent(Q_NULLPTR);

  emit routeRequest(startRef,targetRef,vehicle,++requestId,breaker);
}

void RoutingListModel::onRouteComputed(RouteSelectionRef route,
                                       int requestId)
{
  if (!route || requestId!=this->requestId){
    return;
  }
  beginResetModel();
  this->route=route;
  endResetModel();

  computing=false;
  breaker.reset();
  emit computingChanged();
}

void RoutingListModel::onRouteFailed(QString reason,
                                     int requestId)
{
  if (requestId!=this->requestId){
    return;
  }

  clear();

  computing=false;
  breaker.reset();
  emit computingChanged();
  emit routeFailed(reason);
  osmscout::log.Warn() << "Route computation failed: " << reason.toStdString();
}

void RoutingListModel::onRoutingProgress(int percent,
                                         int requestId)
{
  if (requestId!=this->requestId){
    return;
  }
  emit routingProgress(percent);
  osmscout::log.Debug() << "Route progress: " << percent;
}

void RoutingListModel::clear()
{
  if (!route){
    return;
  }
  beginResetModel();

  ++requestId;
  route->routeSteps.clear();

  endResetModel();
}

void RoutingListModel::cancel()
{
  if (breaker){
    breaker->Break();
  }
  computing=false;
  ++requestId; // ignore future callbacks
}

int RoutingListModel::rowCount(const QModelIndex& ) const
{
  if (!route){
    return 0;
  }
  return route->routeSteps.size();
}

double RoutingListModel::getRouteLength() const
{
  if (!route || route->routeDescription.Nodes().empty()){
    return 0;
  }
  return route->routeDescription.Nodes().back().GetDistance() * 1000;
}

double RoutingListModel::getRouteDuration() const
{
  if (!route || route->routeDescription.Nodes().empty()){
    return 0;
  }
  return route->routeDescription.Nodes().back().GetTime() * 3600;
}

QVariant RoutingListModel::data(const QModelIndex &index, int role) const
{
    if(!route || index.row() < 0 || index.row() >= route->routeSteps.size()) {
        return QVariant();
    }

    RouteStep step=route->routeSteps.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
    case LabelRole:
        return step.getDescription();
    default:
        break;
    }

    return QVariant();
}

Qt::ItemFlags RoutingListModel::flags(const QModelIndex &index) const
{
    if(!index.isValid()) {
        return Qt::ItemIsEnabled;
    }

    return QAbstractListModel::flags(index) | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QHash<int, QByteArray> RoutingListModel::roleNames() const
{
    QHash<int, QByteArray> roles=QAbstractListModel::roleNames();

    roles[LabelRole]="label";

    return roles;
}

RouteStep* RoutingListModel::get(int row) const
{
    if(!route || row < 0 || row >= route->routeSteps.size()) {
        return NULL;
    }

    RouteStep step=route->routeSteps.at(row);

    return new RouteStep(step);
}
