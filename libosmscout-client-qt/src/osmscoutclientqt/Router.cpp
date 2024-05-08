/*
 OSMScout - a Qt backend for libosmscout and libosmscout-map
 Copyright (C) 2010 Tim Teulings
 Copyright (C) 2017 Lukas Karas

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

#include <iostream>
#include <iomanip>
#include <sstream>

#include <osmscout/log/Logger.h>
#include <osmscout/system/Math.h>
#include <osmscoutclientqt/OSMScoutQt.h>
#include <osmscoutclientqt/Router.h>

namespace osmscout {

Router::Router(QThread *thread,
               SettingsRef settings,
               DBThreadRef dbThread):
  thread(thread),
  settings(settings),
  dbThread(dbThread)
{
  connect(thread, &QThread::started,
          this, &Router::Initialize);
}

Router::~Router()
{
  if (thread!=QThread::currentThread()){
    qWarning() << "Destroy" << this << "from non incorrect thread;" << thread << "!=" << QThread::currentThread();
  }
  qDebug() << "~Router";
  if (thread!=nullptr){
    thread->quit();
  }
}

void Router::Initialize()
{

}

osmscout::MultiDBRoutingServiceRef Router::MakeRoutingService(const std::list<DBInstanceRef>& databases,
                                                              const QmlRoutingProfileRef &profile)
{
  osmscout::MultiDBRoutingService::RoutingProfileBuilder profileBuilder=
      [profile](const osmscout::DatabaseRef &database){
        return profile->MakeInstance(database->GetTypeConfig());
      };

  std::vector<osmscout::DatabaseRef> dbs;
  dbs.reserve(databases.size());
  for (auto& instance:databases){
    dbs.push_back(instance->GetDatabase());
  }
  osmscout::MultiDBRoutingServiceRef routingService=std::make_shared<osmscout::MultiDBRoutingService>(routerParameter,dbs);
  if (!routingService->Open(profileBuilder)){
    routingService=nullptr;
  }
  return routingService;
}

bool Router::CalculateRoute(osmscout::MultiDBRoutingServiceRef &routingService,
                            const osmscout::RoutePosition& start,
                            const osmscout::RoutePosition& target,
                            const std::optional<osmscout::Bearing> &bearing,
                            osmscout::RouteData& route,
                            int requestId,
                            const osmscout::BreakerRef &breaker)
{
  osmscout::RoutingResult    result;
  osmscout::RoutingParameter parameter;

  parameter.SetProgress(std::make_shared<QtRoutingProgress>(
    [this,requestId](size_t percent){
      emit routingProgress(percent,requestId);
    }
  ));

  parameter.SetBreaker(breaker);

  result=routingService->CalculateRoute(start,
                                        target,
                                        bearing,
                                        parameter);

  if (!result.Success()){
    return false;
  }

  route=std::move(result.GetRoute());

  return true;
}

RouteDescriptionResult Router::TransformRouteDataToRouteDescription(osmscout::MultiDBRoutingServiceRef &routingService,
                                                  const osmscout::RouteData& data,
                                                  const std::string& start,
                                                  const std::string& target)
{
  auto result=routingService->TransformRouteDataToRouteDescription(data);

  if (!result.Success()) {
    return result;
  }

  std::list<osmscout::RoutePostprocessor::PostprocessorRef> postprocessors;

  postprocessors.push_back(std::make_shared<osmscout::RoutePostprocessor::DistanceAndTimePostprocessor>());
  postprocessors.push_back(std::make_shared<osmscout::RoutePostprocessor::StartPostprocessor>(start));
  postprocessors.push_back(std::make_shared<osmscout::RoutePostprocessor::TargetPostprocessor>(target));
  postprocessors.push_back(std::make_shared<osmscout::RoutePostprocessor::WayNamePostprocessor>());
  postprocessors.push_back(std::make_shared<osmscout::RoutePostprocessor::WayTypePostprocessor>());
  postprocessors.push_back(std::make_shared<osmscout::RoutePostprocessor::CrossingWaysPostprocessor>());
  postprocessors.push_back(std::make_shared<osmscout::RoutePostprocessor::DirectionPostprocessor>());
  postprocessors.push_back(std::make_shared<osmscout::RoutePostprocessor::MotorwayJunctionPostprocessor>());
  postprocessors.push_back(std::make_shared<osmscout::RoutePostprocessor::DestinationPostprocessor>());
  postprocessors.push_back(std::make_shared<osmscout::RoutePostprocessor::MaxSpeedPostprocessor>());
  postprocessors.push_back(std::make_shared<osmscout::RoutePostprocessor::LanesPostprocessor>());
  postprocessors.push_back(std::make_shared<osmscout::RoutePostprocessor::SuggestedLanesPostprocessor>());
  postprocessors.push_back(std::make_shared<osmscout::RoutePostprocessor::InstructionPostprocessor>());

  if (!routingService->PostProcessRouteDescription(*result.GetDescription(),
                                                   postprocessors)){
    return {};
  }

  return result;
}

std::string vehicleStr(osmscout::Vehicle vehicle){
  switch (vehicle){
    case osmscout::vehicleCar : return "Car";
    case osmscout::vehicleBicycle : return "Bicycle";
    case osmscout::vehicleFoot : return "Foot";
    default : return "Unknown";
  }
}

std::optional<RoutePosition> Router::LocationToRoutePosition(osmscout::MultiDBRoutingServiceRef &routingService,
                                                             const LocationEntryRef &location)
{
  RoutePositionResult routePositionResult;
  if (auto dbId=routingService->GetDatabaseId(location->getDatabase().toStdString()); dbId) {
    std::vector<ObjectFileRef> refs;
    for (const auto &ref:location->getReferences()){
      refs.emplace_back(ref);
    }
    routePositionResult=routingService->GetRoutableNode(*dbId, refs);
    if (!routePositionResult.IsValid()) {
      log.Debug() << location->getLabel().toStdString() << " doesn't have routable node";
    }
  }

  if (!routePositionResult.IsValid()) {
    routePositionResult = routingService->GetClosestRoutableNode(location->getCoord(),
                                                                 /*radius*/ Kilometers(1));
  }
  if (!routePositionResult.IsValid()) {
    return std::nullopt;
  }

  return routePositionResult.GetRoutePosition();
}

void Router::ProcessRouteRequest(osmscout::MultiDBRoutingServiceRef &routingService,
                                 const LocationEntryRef &start,
                                 const LocationEntryRef &target,
                                 int requestId,
                                 const osmscout::BreakerRef &breaker,
                                 const std::optional<osmscout::Bearing> &bearing)
{
  std::optional<RoutePosition> startNodeOpt=LocationToRoutePosition(routingService, start);
  if (!startNodeOpt) {
    osmscout::log.Warn() << "Can't found route node near start coord " << start->getCoord().GetDisplayText();
    emit routeFailed(QString("Can't found route node near start coord %1").arg(
                       QString::fromStdString(start->getCoord().GetDisplayText())),
                     requestId);
    return;
  }
  RoutePosition startNode=*startNodeOpt;

  std::optional<RoutePosition> targetNodeOpt=LocationToRoutePosition(routingService, target);
  if (!targetNodeOpt) {
    osmscout::log.Warn() << "Can't found route node near target coord " << start->getCoord().GetDisplayText();
    emit routeFailed(QString("Can't found route node near target coord %1").arg(
                       QString::fromStdString(start->getCoord().GetDisplayText())),
                     requestId);
    return;
  }
  RoutePosition targetNode=*targetNodeOpt;

  osmscout::RouteData routeData;
  if (!CalculateRoute(routingService,
                      startNode,
                      targetNode,
                      bearing,
                      routeData,
                      requestId,
                      breaker)) {

    if (breaker->IsAborted()){
      osmscout::log.Debug() << "Routing was canceled by user";
      emit routeCanceled(requestId);
    }else{
      emit routeFailed("There was an error while routing!",requestId);
    }
    return;
  }

  osmscout::log.Debug() << "Route calculated";

  auto routeDescriptionResult=TransformRouteDataToRouteDescription(routingService,
                                                                   routeData,
                                                                   start->getLabel().toUtf8().constData(),
                                                                   target->getLabel().toUtf8().constData());
  if (!routeDescriptionResult.Success()){
    osmscout::log.Warn() << "Route postprocessing failed!";
    emit routeFailed("Route postprocessing failed!",requestId);
    return;
  }
  assert(routeDescriptionResult.GetDescription()); // should be setup when success==true
  osmscout::log.Debug() << "Route transformed";

  RouteDescriptionBuilder builder;
  QList<RouteStep>        routeSteps;
  builder.GenerateRouteSteps(*routeDescriptionResult.GetDescription(), routeSteps);

  auto routeWayResult=routingService->TransformRouteDataToWay(routeData);

  if (!routeWayResult.Success()) {
    emit routeFailed("Error while transforming route",requestId);
    return;
  }

  emit routeComputed(QtRouteData(std::move(*routeDescriptionResult.GetDescription()),
                                 std::move(routeSteps),
                                 std::move(*routeWayResult.GetWay())),
                     requestId);
}

void Router::onRouteRequest(LocationEntryRef start,
                            LocationEntryRef target,
                            QmlRoutingProfileRef profile,
                            int requestId,
                            BreakerRef breaker,
                            std::optional<osmscout::Bearing> bearing)
{
  osmscout::log.Debug() << "Routing from '" << start->getDebugString().toStdString() <<
    "' to '" << target->getDebugString().toStdString() << "'" <<
    " by '" << vehicleStr(profile->getVehicle()) << "' " <<
    (bearing ? "bearing " + bearing->LongDisplayString() : "no bearing");

  dbThread->RunSynchronousJob(
    [this,profile,requestId,start,target,breaker,bearing](const std::list<DBInstanceRef>& databases) {
      osmscout::MultiDBRoutingServiceRef routingService=MakeRoutingService(databases,profile);
      if (!routingService){
        osmscout::log.Warn() << "Can't open routing service";
        emit routeFailed("Can't open routing service",requestId);
        return;
      }
      ProcessRouteRequest(routingService,start,target,requestId,breaker,bearing);
      routingService->Close();
    }
  );
}
}
