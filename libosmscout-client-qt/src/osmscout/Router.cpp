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

#include <osmscout/util/Logger.h>
#include <osmscout/system/Math.h>
#include <osmscout/OSMScoutQt.h>
#include <osmscout/Router.h>


Router::Router(QThread *thread,
               SettingsRef settings,
               DBThreadRef dbThread):
  thread(thread),
  settings(settings),
  dbThread(dbThread)
{
  connect(thread, SIGNAL(started()),
          this, SLOT(Initialize()));
}

Router::~Router()
{
  if (thread!=QThread::currentThread()){
    qWarning() << "Destroy" << this << "from non incorrect thread;" << thread << "!=" << QThread::currentThread();
  }
  qDebug() << "~Router";
  if (thread!=NULL){
    thread->quit();
  }
}

void Router::Initialize()
{

}

void Router::GetCarSpeedTable(std::map<std::string,double>& map)
{
  map["highway_motorway"]=110.0;
  map["highway_motorway_trunk"]=100.0;
  map["highway_motorway_primary"]=70.0;
  map["highway_motorway_link"]=60.0;
  map["highway_motorway_junction"]=60.0;
  map["highway_trunk"]=100.0;
  map["highway_trunk_link"]=60.0;
  map["highway_primary"]=70.0;
  map["highway_primary_link"]=60.0;
  map["highway_secondary"]=60.0;
  map["highway_secondary_link"]=50.0;
  map["highway_tertiary"]=55.0;
  map["highway_tertiary_link"]=55.0;
  map["highway_unclassified"]=50.0;
  map["highway_road"]=50.0;
  map["highway_residential"]=40.0;
  map["highway_roundabout"]=40.0;
  map["highway_living_street"]=10.0;
  map["highway_service"]=30.0;
}

osmscout::MultiDBRoutingServiceRef Router::MakeRoutingService(const std::list<DBInstanceRef>& databases,
                                                              const osmscout::Vehicle vehicle)
{
  osmscout::MultiDBRoutingService::RoutingProfileBuilder profileBuilder=
      [this,vehicle](const osmscout::DatabaseRef &database){
        osmscout::TypeConfigRef typeConfig=database->GetTypeConfig();
        osmscout::FastestPathRoutingProfileRef routingProfile=
          std::make_shared<osmscout::FastestPathRoutingProfile>(typeConfig);

        if (vehicle==osmscout::vehicleFoot) {
          routingProfile->ParametrizeForFoot(*typeConfig,
                                             5.0);
        }
        else if (vehicle==osmscout::vehicleBicycle) {
          routingProfile->ParametrizeForBicycle(*typeConfig,
                                                20.0);
        }
        else /* car */ {
          std::map<std::string,double> speedMap;

          GetCarSpeedTable(speedMap);

          routingProfile->ParametrizeForCar(*typeConfig,
                                            speedMap,
                                            160.0);
        }

        return routingProfile;
      };

  std::vector<osmscout::DatabaseRef> dbs;
  dbs.reserve(databases.size());
  for (const auto instance:databases){
    dbs.push_back(instance->database);
  }
  osmscout::MultiDBRoutingServiceRef routingService=std::make_shared<osmscout::MultiDBRoutingService>(routerParameter,dbs);
  if (!routingService->Open(profileBuilder)){
    routingService=NULL;
  }
  return routingService;
}

bool Router::CalculateRoute(osmscout::MultiDBRoutingServiceRef &routingService,
                            const osmscout::RoutePosition& start,
                            const osmscout::RoutePosition& target,
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
                                        parameter);

  if (!result.Success()){
    return false;
  }

  route=std::move(result.GetRoute());

  return true;
}

bool Router::TransformRouteDataToRouteDescription(osmscout::MultiDBRoutingServiceRef &routingService,
                                                  const osmscout::RouteData& data,
                                                  osmscout::RouteDescription& description,
                                                  const std::string& start,
                                                  const std::string& target)
{
  if (!routingService->TransformRouteDataToRouteDescription(data,description)) {
    return false;
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
  postprocessors.push_back(std::make_shared<osmscout::RoutePostprocessor::InstructionPostprocessor>());

  if (!routingService->PostProcessRouteDescription(description,
                                                   postprocessors)){
    return false;
  }

  return true;
}

std::string vehicleStr(osmscout::Vehicle vehicle){
  switch (vehicle){
    case osmscout::vehicleCar : return "Car";
    case osmscout::vehicleBicycle : return "Bicycle";
    case osmscout::vehicleFoot : return "Foot";
    default : return "Unknown";
  }
}

void Router::ProcessRouteRequest(osmscout::MultiDBRoutingServiceRef &routingService,
                                 const LocationEntryRef &start,
                                 const LocationEntryRef &target,
                                 osmscout::Vehicle /*vehicle*/,
                                 int requestId,
                                 const osmscout::BreakerRef &breaker)
{
  osmscout::RoutePosition startNode=routingService->GetClosestRoutableNode(
                                start->getCoord(),
                                /*radius*/1000);
  if (!startNode.IsValid()){
    osmscout::log.Warn() << "Can't found route node near start coord " << start->getCoord().GetDisplayText();
    emit routeFailed(QString("Can't found route node near start coord %1").arg(QString::fromStdString(start->getCoord().GetDisplayText())),
                     requestId);
    return;
  }

  osmscout::RoutePosition targetNode=routingService->GetClosestRoutableNode(
                                target->getCoord(),
                                /*radius*/1000);
  if (!targetNode.IsValid()){
    osmscout::log.Warn() << "Can't found route node near target coord " << target->getCoord().GetDisplayText();
    emit routeFailed(QString("Can't found route node near target coord %1").arg(QString::fromStdString(target->getCoord().GetDisplayText())),
                     requestId);
    return;
  }

  osmscout::RouteData routeData;
  if (!CalculateRoute(routingService,
                      startNode,
                      targetNode,
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

  osmscout::RouteDescription routeDescription;
  TransformRouteDataToRouteDescription(routingService,
                                       routeData,
                                       routeDescription,
                                       start->getLabel().toUtf8().constData(),
                                       target->getLabel().toUtf8().constData());

  osmscout::log.Debug() << "Route transformed";

  RouteDescriptionBuilder builder;
  QList<RouteStep>        routeSteps;
  builder.GenerateRouteSteps(routeDescription, routeSteps);

  osmscout::Way routeWay;
  if (!routingService->TransformRouteDataToWay(routeData,routeWay)) {
    emit routeFailed("Error while transforming route",requestId);
    return;
  }

  emit routeComputed(QtRouteData(std::move(routeDescription),
                                 std::move(routeSteps),
                                 std::move(routeWay)),
                     requestId);
}

void Router::onRouteRequest(LocationEntryRef start,
                            LocationEntryRef target,
                            osmscout::Vehicle vehicle,
                            int requestId,
                            osmscout::BreakerRef breaker)
{
  osmscout::log.Debug() << "Routing from '" << start->getLabel().toLocal8Bit().data() << 
    "' to '" << target->getLabel().toLocal8Bit().data() << "'" <<
    " by '" << vehicleStr(vehicle) << "'";

  dbThread->RunSynchronousJob(
    [this,vehicle,requestId,start,target,breaker](const std::list<DBInstanceRef>& databases) {
      osmscout::MultiDBRoutingServiceRef routingService=MakeRoutingService(databases,vehicle);
      if (!routingService){
        osmscout::log.Warn() << "Can't open routing service";
        emit routeFailed("Can't open routing service",requestId);
        return;
      }
      ProcessRouteRequest(routingService,start,target,vehicle,requestId,breaker);
      routingService->Close();
    }
  );
}
