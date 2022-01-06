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

#include <osmscoutclientqt/NavigationModel.h>
#include <osmscout/GeoCoord.h>
#include <osmscoutclientqt/OSMScoutQt.h>

namespace osmscout {

NavigationModel::NavigationModel():
    vehicle(osmscout::Vehicle::vehicleCar)
{
  navigationModule=OSMScoutQt::GetInstance().MakeNavigation();

  connect(this, &NavigationModel::routeChanged,
          navigationModule, &NavigationModule::setupRoute,
          Qt::QueuedConnection);
  connect(this, &NavigationModel::positionChange,
          navigationModule, &NavigationModule::locationChanged,
          Qt::QueuedConnection);

  connect(navigationModule, &NavigationModule::update,
          this, &NavigationModel::onUpdate,
          Qt::QueuedConnection);
  connect(navigationModule, &NavigationModule::updateNext,
          this, &NavigationModel::onUpdateNext,
          Qt::QueuedConnection);
  connect(navigationModule, &NavigationModule::positionEstimate,
          this, &NavigationModel::onPositionEstimate,
          Qt::QueuedConnection);
  connect(navigationModule, &NavigationModule::targetReached,
          this, &NavigationModel::onTargetReached,
          Qt::QueuedConnection);
  connect(navigationModule, &NavigationModule::rerouteRequest,
          this, &NavigationModel::onRerouteRequest,
          Qt::QueuedConnection);
  connect(navigationModule, &NavigationModule::arrivalEstimate,
          this, &NavigationModel::onArrivalEstimate,
          Qt::QueuedConnection);

  connect(navigationModule, &NavigationModule::currentSpeed,
          this, &NavigationModel::onCurrentSpeed,
          Qt::QueuedConnection);
  connect(navigationModule, &NavigationModule::maxAllowedSpeed,
          this, &NavigationModel::onMaxAllowedSpeed,
          Qt::QueuedConnection);

  connect(navigationModule, &NavigationModule::laneUpdate,
          this, &NavigationModel::onLaneUpdate,
          Qt::QueuedConnection);

  connect(this, &NavigationModel::routeChanged,
          this, &NavigationModel::routeAheadChanged);
  connect(this, &NavigationModel::arrivalUpdate,
          this, &NavigationModel::routeAheadChanged);
}

NavigationModel::~NavigationModel(){
  if (navigationModule){
    navigationModule->deleteLater();
    navigationModule=nullptr;
  }
}

OverlayWay* NavigationModel::getRouteWayAhead() const
{
  if (!route){
    return nullptr;
  }

  const auto &nodes=route.routeDescription().Nodes();
  if (nodes.empty()){
    return nullptr;
  }
  if (!remainingDistance.has_value()){
    return new OverlayWay(route.routeWay().nodes);
  }
  std::vector<osmscout::Point> pointsAhead;
  pointsAhead.reserve(nodes.size());
  Distance traveled = nodes.back().GetDistance() - remainingDistance.value();
  for (auto const &node : nodes){
    if (node.GetDistance() > traveled) {
      pointsAhead.emplace_back(0, node.GetLocation());
    }
  }
  return new OverlayWay(pointsAhead);
}

OverlayWay* NavigationModel::getRouteWayPassed() const
{
  if (!route){
    return nullptr;
  }

  const auto &nodes=route.routeDescription().Nodes();
  if (nodes.empty()){
    return nullptr;
  }
  if (!remainingDistance.has_value()){
    return nullptr;
  }
  std::vector<osmscout::Point> pointsPassed;
  pointsPassed.reserve(nodes.size());
  Distance traveled = nodes.back().GetDistance() - remainingDistance.value();
  for (auto const &node : nodes){
    if (node.GetDistance() > traveled) {
      break;
    }
    pointsPassed.emplace_back(0, node.GetLocation());
  }
  return new OverlayWay(pointsPassed);
}


void NavigationModel::locationChanged(bool /*locationValid*/,
                                      double lat, double lon,
                                      bool horizontalAccuracyValid, double horizontalAccuracy)
{
  emit positionChange(osmscout::GeoCoord(lat, lon), horizontalAccuracyValid, horizontalAccuracy);
}

QObject *NavigationModel::getRoute() const
{
  assert(route.parent()==nullptr); // Ownership is copied. To transfer ownership to QML, parent have to be null.
  return new QtRouteData(route);
}

void NavigationModel::onUpdate(std::list<RouteStep> instructions)
{
  beginResetModel();
  routeSteps.clear();
  routeSteps.reserve(instructions.size());
  routeSteps.insert(routeSteps.begin(), instructions.begin(), instructions.end());
  endResetModel();
}

void NavigationModel::onUpdateNext(RouteStep routeStep)
{
  // qDebug() << routeStep.getDistanceTo() << "m :" << routeStep.getShortDescription();
  nextRouteStep=routeStep;
  emit update();
  emit vehiclePositionChanged();
}

void NavigationModel::onPositionEstimate(const PositionAgent::PositionState state,
                                         const GeoCoord coord,
                                         const std::optional<Bearing> bearing)
{
  PositionAgent::PositionState oldState=this->vehicleState;
  this->vehicleState=state;
  this->vehicleCoord=coord;
  this->vehicleBearing=bearing;
  emit positionEstimate(state,
                        coord.GetLat(), coord.GetLon(),
                        bearing ? QString::fromStdString(bearing->LongDisplayString()) : "");
  emit vehiclePositionChanged();
  if (oldState!=state &&
      (oldState==PositionAgent::EstimateInTunnel || state==PositionAgent::EstimateInTunnel)) {
    emit positionEstimateInTunnelChanged();
  }
}

void NavigationModel::onTargetReached(const osmscout::Bearing targetBearing,
                                      const Distance targetDistance)
{
  emit targetReached(QString::fromStdString(targetBearing.LongDisplayString()),
                     targetDistance.AsMeter());
}

void NavigationModel::onRerouteRequest(const GeoCoord from,
                                       const std::optional<Bearing> initialBearing,
                                       const GeoCoord to)
{
  emit rerouteRequest(from.GetLat(), from.GetLon(),
                      initialBearing ? QString::fromStdString(initialBearing->LongDisplayString()) : "",
                      initialBearing ? initialBearing->AsRadians() : -1,
                      to.GetLat(), to.GetLon());
}

void NavigationModel::onArrivalEstimate(QDateTime arrivalEstimate, osmscout::Distance remainingDistance)
{
  this->arrivalEstimate = arrivalEstimate;
  this->remainingDistance = remainingDistance;
  emit arrivalUpdate();
}

void NavigationModel::onCurrentSpeed(double currentSpeed)
{
  this->currentSpeed=currentSpeed;
  emit currentSpeedUpdate(currentSpeed);
}

void NavigationModel::onMaxAllowedSpeed(double maxAllowedSpeed)
{
  this->maxAllowedSpeed=maxAllowedSpeed;
  emit maxAllowedSpeedUpdate(maxAllowedSpeed);
}

void NavigationModel::onLaneUpdate(osmscout::LaneAgent::Lane lane)
{
  this->lane = lane;
  emit laneUpdate();
}

QObject *NavigationModel::getNextRoutStep()
{
  return new RouteStep(nextRouteStep);
}

void NavigationModel::setRoute(QObject *o)
{
  QtRouteData *rd = dynamic_cast<QtRouteData *>(o);
  if (o != nullptr && rd == nullptr) {
    qWarning() << "Failed to cast " << o << " to QtRouteData*.";
    return;
  }

  if (rd==nullptr){
    route.clear();
  } else {
    route=*rd;
  }

  beginResetModel();
  routeSteps.clear();
  nextRouteStep=RouteStep();
  vehicleState=PositionAgent::Uninitialised;
  if (route) {
    auto steps = route.routeSteps();
    routeSteps.reserve(steps.size());
    routeSteps.insert(routeSteps.begin(), steps.begin(), steps.end());
  }
  arrivalEstimate=QDateTime();
  remainingDistance=std::nullopt;
  endResetModel();

  emit arrivalUpdate();
  emit routeChanged(this->route, vehicle);
  emit vehiclePositionChanged();
  emit positionEstimateInTunnelChanged();
  emit update();
  emit currentSpeedUpdate(0);
  emit maxAllowedSpeedUpdate(0);
}

QVariant NavigationModel::data(const QModelIndex &index, int role) const
{
  if(index.row() < 0 || index.row() >= (int)routeSteps.size()) {
    return QVariant();
  }

  RouteStep step=routeSteps[index.row()];
  return step.data(role);
}

int NavigationModel::rowCount(const QModelIndex &/*parent*/) const
{
  if (!route){
    return 0;
  }
  return routeSteps.size();
}

Qt::ItemFlags NavigationModel::flags(const QModelIndex &index) const
{
  if(!index.isValid()) {
    return Qt::ItemIsEnabled;
  }

  return QAbstractListModel::flags(index) | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QHash<int, QByteArray> NavigationModel::roleNames() const
{
  return RouteStep::roleNames(QAbstractListModel::roleNames());
}


}
