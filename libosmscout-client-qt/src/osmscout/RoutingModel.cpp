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

  connect(this,SIGNAL(routeRequest(LocationEntry*,LocationEntry*,osmscout::Vehicle,int)),
          router,SLOT(onRouteRequest(LocationEntry*,LocationEntry*,osmscout::Vehicle,int)),
          Qt::QueuedConnection);

  connect(router,SIGNAL(routeComputed(RouteSelection,int)),
          this,SLOT(onRouteComputed(RouteSelection,int)),
          Qt::QueuedConnection);
  connect(router,SIGNAL(routeFailed(QString,int)),
          this,SLOT(onRouteFailed(QString,int)),
          Qt::QueuedConnection);
}

RoutingListModel::~RoutingListModel()
{
  route.routeSteps.clear();
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
  emit computingChanged();
  emit routeRequest(start,target,vehicle,++requestId);
}

void RoutingListModel::onRouteComputed(RouteSelection route,
                                       int requestId)
{
  if (requestId!=this->requestId){
    return;
  }
  beginResetModel();
  this->route=route;
  endResetModel();

  computing=false;
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
  emit computingChanged();
  emit routeFailed(reason);
  osmscout::log.Warn() << "Route computation failed: " << reason.toStdString();
}

void RoutingListModel::clear()
{
  beginResetModel();

  ++requestId;
  route.routeSteps.clear();

  endResetModel();
}

int RoutingListModel::rowCount(const QModelIndex& ) const
{
    return route.routeSteps.size();
}

QVariant RoutingListModel::data(const QModelIndex &index, int role) const
{
    if(index.row() < 0 || index.row() >= route.routeSteps.size()) {
        return QVariant();
    }

    RouteStep step=route.routeSteps.at(index.row());

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
    if(row < 0 || row >= route.routeSteps.size()) {
        return NULL;
    }

    RouteStep step=route.routeSteps.at(row);

    return new RouteStep(step);
}
