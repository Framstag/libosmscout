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

#include <osmscout/NavigationModel.h>
#include <osmscout/GeoCoord.h>
#include <osmscout/OSMScoutQt.h>

NavigationModel::NavigationModel():
    vehicle(osmscout::Vehicle::vehicleCar), onRoute(false)
{
  navigationModule=OSMScoutQt::GetInstance().MakeNavigation();

  connect(this, SIGNAL(routeChanged(LocationEntryRef, QtRouteData, osmscout::Vehicle)),
          navigationModule, SLOT(setupRoute(LocationEntryRef, QtRouteData, osmscout::Vehicle)),
          Qt::QueuedConnection);
  connect(this, SIGNAL(positionChange(osmscout::GeoCoord, bool, double)),
          navigationModule, SLOT(locationChanged(osmscout::GeoCoord, bool, double)),
          Qt::QueuedConnection);
  connect(navigationModule, SIGNAL(update(bool, RouteStep)),
          this, SLOT(onUpdated(bool, RouteStep)),
          Qt::QueuedConnection);
}

NavigationModel::~NavigationModel(){
  if (navigationModule){
    navigationModule->deleteLater();
    navigationModule=nullptr;
  }
}

bool NavigationModel::isPositionOnRoute()
{
  return onRoute;
}

void NavigationModel::locationChanged(bool /*locationValid*/,
                                      double lat, double lon,
                                      bool horizontalAccuracyValid, double horizontalAccuracy)
{
  emit positionChange(osmscout::GeoCoord(lat, lon), horizontalAccuracyValid, horizontalAccuracy);
}

QObject *NavigationModel::getRoute() const
{
  return new QtRouteData(route);
}

void NavigationModel::onUpdated(bool onRoute, RouteStep routeStep)
{
  // qDebug() << onRoute << routeStep.getDistanceTo() << "m :" << routeStep.getShortDescription();
  if (this->onRoute != onRoute) {
    this->onRoute = onRoute;
    emit positionOnRouteChanged();
  }
  nextRouteStep=routeStep;

  emit update();
}

QObject *NavigationModel::getNextRoutStep()
{
  return new RouteStep(nextRouteStep);
}

void NavigationModel::setRoute(QObject *o)
{
  QtRouteData* route=dynamic_cast<QtRouteData*>(o);
  if (route == nullptr){
    qWarning() << "Failed to cast " << o << " to QtRouteData*.";
    return;
  }
  this->route=*route;
  emit routeChanged(target, this->route, vehicle);
}
