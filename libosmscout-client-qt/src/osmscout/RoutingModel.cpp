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

namespace osmscout {

RoutingListModel::RoutingListModel(QObject* parent)
: QAbstractListModel(parent), requestId(0)
{
  router=OSMScoutQt::GetInstance().MakeRouter();

  connect(this, &RoutingListModel::routeRequest,
          router, &Router::onRouteRequest,
          Qt::QueuedConnection);

  connect(router, &Router::routeComputed,
          this, &RoutingListModel::onRouteComputed,
          Qt::QueuedConnection);
  connect(router, &Router::routeFailed,
          this, &RoutingListModel::onRouteFailed,
          Qt::QueuedConnection);
  connect(router, &Router::routingProgress,
          this, &RoutingListModel::onRoutingProgress,
          Qt::QueuedConnection);
}

RoutingListModel::~RoutingListModel()
{
  if (router!=nullptr){
    router->deleteLater();
    router=nullptr;
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
  QmlRoutingProfile profile(vehicle);
  setStartAndTarget(start, target, &profile);
}

void RoutingListModel::setStartAndTarget(LocationEntry* start,
                                         LocationEntry* target,
                                         QmlRoutingProfile *routingProfile)
{
  cancel(); // cancel current computation
  clear(); // clear model

  if (start==nullptr || target==nullptr || routingProfile==nullptr) {
    if (routingProfile == nullptr) {
      qWarning() << "Routing profile is null";
    } else if (start == nullptr) {
      qWarning() << "Start is null";
    } else {
      qWarning() << "Target is null";
    }
    computing=false;
    emit computingChanged();
    return;
  }

  computing=true;
  breaker=std::make_shared<osmscout::ThreadedBreaker>();
  emit computingChanged();

  // make copy to shared ptr, remove ownership
  LocationEntryRef startRef=std::make_shared<LocationEntry>(*start);
  startRef->setParent(Q_NULLPTR);
  LocationEntryRef targetRef=std::make_shared<LocationEntry>(*target);
  targetRef->setParent(Q_NULLPTR);
  QmlRoutingProfileRef profileRef=std::make_shared<QmlRoutingProfile>(*routingProfile);
  profileRef->setParent(Q_NULLPTR);

  emit routeRequest(startRef,targetRef,profileRef,++requestId,breaker);
}

void RoutingListModel::onRouteComputed(QtRouteData route,
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
  route.clear();

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
  return route.routeSteps().size();
}

double RoutingListModel::getRouteLength() const
{
  if (!route || route.routeDescription().Nodes().empty()){
    return 0;
  }
  return route.routeDescription().Nodes().back().GetDistance().AsMeter();
}

double RoutingListModel::getRouteDuration() const
{
  if (!route || route.routeDescription().Nodes().empty()){
    return 0;
  }
  return DurationAsSeconds(route.routeDescription().Nodes().back().GetTime());
}

QVariant RoutingListModel::data(const QModelIndex &index, int role) const
{
  if(!route || index.row() < 0 || index.row() >= route.routeSteps().size()) {
    return QVariant();
  }

  RouteStep step=route.routeSteps().at(index.row());
  return step.data(role);
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
  return RouteStep::roleNames(QAbstractListModel::roleNames());
}

QObject* RoutingListModel::get(int row) const
{
  if(!route || row < 0 || row >= route.routeSteps().size()) {
    return nullptr;
  }

  RouteStep step=route.routeSteps().at(row);

  return new RouteStep(step);
}
}
