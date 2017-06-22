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

RouteStep::RouteStep()
{

}

RouteStep::RouteStep(const RouteStep& other)
    : QObject(other.parent()),
      distance(other.distance),
      distanceDelta(other.distanceDelta),
      time(other.time),
      timeDelta(other.timeDelta),
      description(other.description)
{
    // no code
}

RouteStep& RouteStep::operator=(const RouteStep& other)
{
  if (this!=&other) {
    setParent(other.parent());
    distance=other.distance;
    distanceDelta=other.distanceDelta;
    time=other.time;
    timeDelta=other.timeDelta;
    description=other.description;
  }

  return *this;
}

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

static QString DistanceToString(double distance)
{
  std::ostringstream stream;

  stream.setf(std::ios::fixed);
  stream.precision(1);
  stream << distance << "km";

  return QString::fromStdString(stream.str());
}

static QString TimeToString(double time)
{
  std::ostringstream stream;

  stream << std::setfill(' ') << std::setw(2) << (int)std::floor(time) << ":";

  time-=std::floor(time);

  stream << std::setfill('0') << std::setw(2) << (int)floor(60*time+0.5);

  stream << "h";

  return QString::fromStdString(stream.str());
}

static QString MoveToTurnCommand(osmscout::RouteDescription::DirectionDescription::Move move)
{
  switch (move) {
  case osmscout::RouteDescription::DirectionDescription::sharpLeft:
    return "Turn sharp left";
  case osmscout::RouteDescription::DirectionDescription::left:
    return "Turn left";
  case osmscout::RouteDescription::DirectionDescription::slightlyLeft:
    return "Turn slightly left";
  case osmscout::RouteDescription::DirectionDescription::straightOn:
    return "Straight on";
  case osmscout::RouteDescription::DirectionDescription::slightlyRight:
    return "Turn slightly right";
  case osmscout::RouteDescription::DirectionDescription::right:
    return "Turn right";
  case osmscout::RouteDescription::DirectionDescription::sharpRight:
    return "Turn sharp right";
  }

  assert(false);

  return "???";
}

static std::string CrossingWaysDescriptionToString(const osmscout::RouteDescription::CrossingWaysDescription& crossingWaysDescription)
{
  std::set<std::string>                          names;
  osmscout::RouteDescription::NameDescriptionRef originDescription=crossingWaysDescription.GetOriginDesccription();
  osmscout::RouteDescription::NameDescriptionRef targetDescription=crossingWaysDescription.GetTargetDesccription();

  if (originDescription) {
    std::string nameString=originDescription->GetDescription();

    if (!nameString.empty()) {
      names.insert(nameString);
    }
  }

  if (targetDescription) {
    std::string nameString=targetDescription->GetDescription();

    if (!nameString.empty()) {
      names.insert(nameString);
    }
  }

  for (std::list<osmscout::RouteDescription::NameDescriptionRef>::const_iterator name=crossingWaysDescription.GetDescriptions().begin();
      name!=crossingWaysDescription.GetDescriptions().end();
      ++name) {
    std::string nameString=(*name)->GetDescription();

    if (!nameString.empty()) {
      names.insert(nameString);
    }
  }

  if (names.size()>0) {
    std::ostringstream stream;

    stream << "<ul>";
    for (std::set<std::string>::const_iterator name=names.begin();
        name!=names.end();
        ++name) {
        /*
      if (name!=names.begin()) {
        stream << ", ";
      }*/
      stream << "<li>'" << *name << "'</li>";
    }
    stream << "</ul>";

    return stream.str();
  }
  else {
    return "";
  }
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

void Router::DumpStartDescription(RouteSelection &route,
                                  const osmscout::RouteDescription::StartDescriptionRef& startDescription,
                                  const osmscout::RouteDescription::NameDescriptionRef& nameDescription)
{
  RouteStep startAt;

  startAt.description="Start at '"+QString::fromUtf8(startDescription->GetDescription().c_str())+"'";
  route.routeSteps.push_back(startAt);

  if (nameDescription &&
      nameDescription->HasName()) {
    RouteStep driveAlong;

    driveAlong.description="Drive along '"+QString::fromUtf8(nameDescription->GetDescription().c_str())+"'";
    route.routeSteps.push_back(driveAlong);
  }
}

void Router::DumpTargetDescription(RouteSelection &route,
                                   const osmscout::RouteDescription::TargetDescriptionRef& targetDescription)
{
  RouteStep targetReached;

  targetReached.description="Target reached '"+QString::fromUtf8(targetDescription->GetDescription().c_str())+"'";
  route.routeSteps.push_back(targetReached);
}

void Router::DumpTurnDescription(RouteSelection &route,
                                 const osmscout::RouteDescription::TurnDescriptionRef& /*turnDescription*/,
                                 const osmscout::RouteDescription::CrossingWaysDescriptionRef& crossingWaysDescription,
                                 const osmscout::RouteDescription::DirectionDescriptionRef& directionDescription,
                                 const osmscout::RouteDescription::NameDescriptionRef& nameDescription)
{
  RouteStep   turn;
  std::string crossingWaysString;

  if (crossingWaysDescription) {
    crossingWaysString=CrossingWaysDescriptionToString(*crossingWaysDescription);
  }

  if (!crossingWaysString.empty()) {
    turn.description="At crossing "+QString::fromUtf8(crossingWaysString.c_str())+"";
  }

  if (directionDescription) {
    turn.description+=MoveToTurnCommand(directionDescription->GetCurve());
  }
  else {
    turn.description=+"Turn";
  }

  if (nameDescription &&
      nameDescription->HasName()) {
    turn.description+=" into '"+QString::fromUtf8(nameDescription->GetDescription().c_str())+"'";
  }

  route.routeSteps.push_back(turn);
}

void Router::DumpRoundaboutEnterDescription(RouteSelection &route,
                                            const osmscout::RouteDescription::RoundaboutEnterDescriptionRef& /*roundaboutEnterDescription*/,
                                            const osmscout::RouteDescription::CrossingWaysDescriptionRef& crossingWaysDescription)
{
  RouteStep   enter;
  std::string crossingWaysString;

  if (crossingWaysDescription) {
    crossingWaysString=CrossingWaysDescriptionToString(*crossingWaysDescription);
  }

  if (!crossingWaysString.empty()) {
    enter.description="At crossing "+QString::fromUtf8(crossingWaysString.c_str())+"";
  }


  enter.description+="Enter roundabout";

  route.routeSteps.push_back(enter);
}

void Router::DumpRoundaboutLeaveDescription(RouteSelection &route,
                                            const osmscout::RouteDescription::RoundaboutLeaveDescriptionRef& roundaboutLeaveDescription,
                                            const osmscout::RouteDescription::NameDescriptionRef& nameDescription)
{
  RouteStep leave;

  leave.description="Leave roundabout (";
  leave.description+=QString::number(roundaboutLeaveDescription->GetExitCount());
  leave.description+=". exit)";

  if (nameDescription &&
      nameDescription->HasName()) {
    leave.description+=" into street '";
    leave.description+=QString::fromUtf8(nameDescription->GetDescription().c_str());
    leave.description+="'";
  }

  route.routeSteps.push_back(leave);
}

void Router::DumpMotorwayEnterDescription(RouteSelection &route,
                                          const osmscout::RouteDescription::MotorwayEnterDescriptionRef& motorwayEnterDescription,
                                          const osmscout::RouteDescription::CrossingWaysDescriptionRef& crossingWaysDescription)
{
  RouteStep   enter;
  std::string crossingWaysString;

  if (crossingWaysDescription) {
    crossingWaysString=CrossingWaysDescriptionToString(*crossingWaysDescription);
  }

  if (!crossingWaysString.empty()) {
    enter.description="At crossing "+QString::fromUtf8(crossingWaysString.c_str());
  }

  enter.description+="Enter motorway";

  if (motorwayEnterDescription->GetToDescription() &&
      motorwayEnterDescription->GetToDescription()->HasName()) {
    enter.description+=" '";
    enter.description+=QString::fromUtf8(motorwayEnterDescription->GetToDescription()->GetDescription().c_str());
    enter.description+="'";
  }

  route.routeSteps.push_back(enter);
}

void Router::DumpMotorwayChangeDescription(RouteSelection &route,
                                           const osmscout::RouteDescription::MotorwayChangeDescriptionRef& motorwayChangeDescription)
{
  RouteStep change;

  change.description="Change motorway";

  if (motorwayChangeDescription->GetFromDescription() &&
      motorwayChangeDescription->GetFromDescription()->HasName()) {
    change.description+=" from '";
    change.description+=QString::fromUtf8(motorwayChangeDescription->GetFromDescription()->GetDescription().c_str());
    change.description+="'";
  }

  if (motorwayChangeDescription->GetToDescription() &&
      motorwayChangeDescription->GetToDescription()->HasName()) {
    change.description+=" to '";
    change.description+=QString::fromUtf8(motorwayChangeDescription->GetToDescription()->GetDescription().c_str());
    change.description+="'";
  }

  route.routeSteps.push_back(change);
}

void Router::DumpMotorwayLeaveDescription(RouteSelection &route,
                                          const osmscout::RouteDescription::MotorwayLeaveDescriptionRef& motorwayLeaveDescription,
                                          const osmscout::RouteDescription::DirectionDescriptionRef& directionDescription,
                                          const osmscout::RouteDescription::NameDescriptionRef& nameDescription)
{
  RouteStep leave;

  leave.description="Leave motorway";

  if (motorwayLeaveDescription->GetFromDescription() &&
      motorwayLeaveDescription->GetFromDescription()->HasName()) {
    leave.description+=" '";
    leave.description+=QString::fromUtf8(motorwayLeaveDescription->GetFromDescription()->GetDescription().c_str());
    leave.description+="'";
  }

  if (directionDescription &&
      directionDescription->GetCurve()!=osmscout::RouteDescription::DirectionDescription::slightlyLeft &&
      directionDescription->GetCurve()!=osmscout::RouteDescription::DirectionDescription::straightOn &&
      directionDescription->GetCurve()!=osmscout::RouteDescription::DirectionDescription::slightlyRight) {
    leave.description+=MoveToTurnCommand(directionDescription->GetCurve());
  }

  if (nameDescription &&
      nameDescription->HasName()) {
    leave.description+=" into '";
    leave.description+=QString::fromUtf8(nameDescription->GetDescription().c_str());
    leave.description+="'";
  }

  route.routeSteps.push_back(leave);
}

void Router::DumpNameChangedDescription(RouteSelection &route,
                                        const osmscout::RouteDescription::NameChangedDescriptionRef& nameChangedDescription)
{
  RouteStep changed;

  changed.description="";

  if (nameChangedDescription->GetOriginDesccription()) {
    changed.description+="Way changes name<br/>";
    changed.description+="from '";
    changed.description+=QString::fromUtf8(nameChangedDescription->GetOriginDesccription()->GetDescription().c_str());
    changed.description+="'<br/>";
    changed.description+=" to '";
    changed.description+=QString::fromUtf8(nameChangedDescription->GetTargetDesccription()->GetDescription().c_str());
    changed.description+="'";
  }
  else {
    changed.description+="Way changes name<br/>";
    changed.description+="to '";
    changed.description+=QString::fromUtf8(nameChangedDescription->GetTargetDesccription()->GetDescription().c_str());
    changed.description+="'";
  }

  route.routeSteps.push_back(changed);
}

void Router::onRouteRequest(LocationEntry* start,
                            LocationEntry* target,
                            osmscout::Vehicle vehicle,
                            int requestId)
{
  RouteSelection route;
  osmscout::log.Debug() << "Routing from '" << start->getLabel().toLocal8Bit().data() << "' to '" << target->getLabel().toLocal8Bit().data() << "'";

  // TODO:
  //  - handle location entries with typeCoordinate that don't have database
  //  - implement routing with multiple databases
  //
  // as first step, we can try to manipulate with locations from different
  // databases like with type coordinates...
  if (start->getDatabase() != target->getDatabase()){
    osmscout::log.Warn() << "Start and target are from different databases! It is not supported yet.";
    osmscout::log.Warn() << "Start database:" << start->getDatabase().toStdString();
    osmscout::log.Warn() << "Target database:" << target->getDatabase().toStdString();
    emit routeFailed("Start and target are from different databases!",requestId);
    return;
  }

  route.routeSteps.clear();

  QString                             databasePath=start->getDatabase();
  osmscout::TypeConfigRef             typeConfig=OSMScoutQt::GetInstance().GetDBThread()->GetTypeConfig(databasePath);
  if (!typeConfig){
    osmscout::log.Warn() << "Can't find database: " << databasePath.toStdString();
    emit routeFailed(QString("Can't find database: %1").arg(databasePath),requestId);
    return;
  }
  osmscout::FastestPathRoutingProfile routingProfile(typeConfig);
  osmscout::Way                       routeWay;
  //osmscout::Vehicle                   vehicle=osmscout::vehicleCar;//settings->GetRoutingVehicle();

  if (vehicle==osmscout::vehicleFoot) {
    routingProfile.ParametrizeForFoot(*typeConfig,
                                      5.0);
  }
  else if (vehicle==osmscout::vehicleBicycle) {
    routingProfile.ParametrizeForBicycle(*typeConfig,
                                         20.0);
  }
  else /* car */ {
    std::map<std::string,double> speedMap;

    GetCarSpeedTable(speedMap);

    routingProfile.ParametrizeForCar(*typeConfig,
                                     speedMap,
                                     160.0);
  }

  osmscout::RoutePosition startPosition=OSMScoutQt::GetInstance().GetDBThread()
    ->GetClosestRoutableNode(databasePath,
                             start->getReferences().front(),
                             routingProfile,
                             1000);

  if (!startPosition.IsValid()) {
    //std::cerr << "Cannot find a routing node close to the start location" << std::endl;
    emit routeFailed("Cannot find a routing node close to the start location",requestId);
    return;
  }

  osmscout::RoutePosition targetPosition=OSMScoutQt::GetInstance().GetDBThread()
    ->GetClosestRoutableNode(databasePath,
                             target->getReferences().front(),
                             routingProfile,
                             1000);

  if (!targetPosition.IsValid()) {
    //std::cerr << "Cannot find a routing node close to the target location" << std::endl;
    emit routeFailed("Cannot find a routing node close to the target location",requestId);
    return;
  }

  if (!OSMScoutQt::GetInstance().GetDBThread()->CalculateRoute(databasePath,
                                                               routingProfile,
                                                               startPosition,
                                                               targetPosition,
                                                               route.routeData)) {
    //std::cerr << "There was an error while routing!" << std::endl;
    emit routeFailed("There was an error while routing!",requestId);
    return;
  }

  osmscout::log.Debug() << "Route calculated";

  OSMScoutQt::GetInstance().GetDBThread()
      ->TransformRouteDataToRouteDescription(databasePath,
                                             routingProfile,
                                             route.routeData,
                                             route.routeDescription,
                                             start->getLabel().toUtf8().constData(),
                                             target->getLabel().toUtf8().constData());

  osmscout::log.Debug() << "Route transformed";

  size_t roundaboutCrossingCounter=0;
  int    lastStepIndex=-1;

  std::list<osmscout::RouteDescription::Node>::const_iterator prevNode=route.routeDescription.Nodes().end();
  for (std::list<osmscout::RouteDescription::Node>::const_iterator node=route.routeDescription.Nodes().begin();
       node!=route.routeDescription.Nodes().end();
       ++node) {
    osmscout::RouteDescription::DescriptionRef                 desc;
    osmscout::RouteDescription::NameDescriptionRef             nameDescription;
    osmscout::RouteDescription::DirectionDescriptionRef        directionDescription;
    osmscout::RouteDescription::NameChangedDescriptionRef      nameChangedDescription;
    osmscout::RouteDescription::CrossingWaysDescriptionRef     crossingWaysDescription;

    osmscout::RouteDescription::StartDescriptionRef            startDescription;
    osmscout::RouteDescription::TargetDescriptionRef           targetDescription;
    osmscout::RouteDescription::TurnDescriptionRef             turnDescription;
    osmscout::RouteDescription::RoundaboutEnterDescriptionRef  roundaboutEnterDescription;
    osmscout::RouteDescription::RoundaboutLeaveDescriptionRef  roundaboutLeaveDescription;
    osmscout::RouteDescription::MotorwayEnterDescriptionRef    motorwayEnterDescription;
    osmscout::RouteDescription::MotorwayChangeDescriptionRef   motorwayChangeDescription;
    osmscout::RouteDescription::MotorwayLeaveDescriptionRef    motorwayLeaveDescription;

    desc=node->GetDescription(osmscout::RouteDescription::WAY_NAME_DESC);
    if (desc) {
      nameDescription=std::dynamic_pointer_cast<osmscout::RouteDescription::NameDescription>(desc);
    }

    desc=node->GetDescription(osmscout::RouteDescription::DIRECTION_DESC);
    if (desc) {
      directionDescription=std::dynamic_pointer_cast<osmscout::RouteDescription::DirectionDescription>(desc);
    }

    desc=node->GetDescription(osmscout::RouteDescription::WAY_NAME_CHANGED_DESC);
    if (desc) {
      nameChangedDescription=std::dynamic_pointer_cast<osmscout::RouteDescription::NameChangedDescription>(desc);
    }

    desc=node->GetDescription(osmscout::RouteDescription::CROSSING_WAYS_DESC);
    if (desc) {
      crossingWaysDescription=std::dynamic_pointer_cast<osmscout::RouteDescription::CrossingWaysDescription>(desc);
    }

    desc=node->GetDescription(osmscout::RouteDescription::NODE_START_DESC);
    if (desc) {
      startDescription=std::dynamic_pointer_cast<osmscout::RouteDescription::StartDescription>(desc);
    }

    desc=node->GetDescription(osmscout::RouteDescription::NODE_TARGET_DESC);
    if (desc) {
      targetDescription=std::dynamic_pointer_cast<osmscout::RouteDescription::TargetDescription>(desc);
    }

    desc=node->GetDescription(osmscout::RouteDescription::TURN_DESC);
    if (desc) {
      turnDescription=std::dynamic_pointer_cast<osmscout::RouteDescription::TurnDescription>(desc);
    }

    desc=node->GetDescription(osmscout::RouteDescription::ROUNDABOUT_ENTER_DESC);
    if (desc) {
      roundaboutEnterDescription=std::dynamic_pointer_cast<osmscout::RouteDescription::RoundaboutEnterDescription>(desc);
    }

    desc=node->GetDescription(osmscout::RouteDescription::ROUNDABOUT_LEAVE_DESC);
    if (desc) {
      roundaboutLeaveDescription=std::dynamic_pointer_cast<osmscout::RouteDescription::RoundaboutLeaveDescription>(desc);
    }

    desc=node->GetDescription(osmscout::RouteDescription::MOTORWAY_ENTER_DESC);
    if (desc) {
      motorwayEnterDescription=std::dynamic_pointer_cast<osmscout::RouteDescription::MotorwayEnterDescription>(desc);
    }

    desc=node->GetDescription(osmscout::RouteDescription::MOTORWAY_CHANGE_DESC);
    if (desc) {
      motorwayChangeDescription=std::dynamic_pointer_cast<osmscout::RouteDescription::MotorwayChangeDescription>(desc);
    }

    desc=node->GetDescription(osmscout::RouteDescription::MOTORWAY_LEAVE_DESC);
    if (desc) {
      motorwayLeaveDescription=std::dynamic_pointer_cast<osmscout::RouteDescription::MotorwayLeaveDescription>(desc);
    }

    if (crossingWaysDescription &&
        roundaboutCrossingCounter>0 &&
        crossingWaysDescription->GetExitCount()>1) {
      roundaboutCrossingCounter+=crossingWaysDescription->GetExitCount()-1;
    }

    if (startDescription) {
      DumpStartDescription(route,
                           startDescription,
                           nameDescription);
    }
    else if (targetDescription) {
      DumpTargetDescription(route,
                            targetDescription);
    }
    else if (turnDescription) {
      DumpTurnDescription(route,
                          turnDescription,
                          crossingWaysDescription,
                          directionDescription,
                          nameDescription);
    }
    else if (roundaboutEnterDescription) {
      DumpRoundaboutEnterDescription(route,
                                     roundaboutEnterDescription,
                                     crossingWaysDescription);

      roundaboutCrossingCounter=1;
    }
    else if (roundaboutLeaveDescription) {
      DumpRoundaboutLeaveDescription(route,
                                     roundaboutLeaveDescription,
                                     nameDescription);

      roundaboutCrossingCounter=0;
    }
    else if (motorwayEnterDescription) {
      DumpMotorwayEnterDescription(route,
                                   motorwayEnterDescription,
                                   crossingWaysDescription);
    }
    else if (motorwayChangeDescription) {
      DumpMotorwayChangeDescription(route,
                                    motorwayChangeDescription);
    }
    else if (motorwayLeaveDescription) {
      DumpMotorwayLeaveDescription(route,
                                   motorwayLeaveDescription,
                                   directionDescription,
                                   nameDescription);
    }
    else if (nameChangedDescription) {
      DumpNameChangedDescription(route,
                                 nameChangedDescription);
    }
    else {
      continue;
    }

    int currentStepIndex;

    if (lastStepIndex>=0) {
        currentStepIndex=lastStepIndex+1;
    }
    else {
        currentStepIndex=0;
    }

    if (currentStepIndex>=0) {
      route.routeSteps[currentStepIndex].distance=DistanceToString(node->GetDistance());
      route.routeSteps[currentStepIndex].time=TimeToString(node->GetTime());

      if (prevNode!=route.routeDescription.Nodes().end() &&
          node->GetDistance()-prevNode->GetDistance()!=0.0) {
        route.routeSteps[currentStepIndex].distanceDelta=DistanceToString(node->GetDistance()-prevNode->GetDistance());
      }

      if (prevNode!=route.routeDescription.Nodes().end() &&
          node->GetTime()-prevNode->GetTime()!=0.0) {
        route.routeSteps[currentStepIndex].timeDelta=TimeToString(node->GetTime()-prevNode->GetTime());
      }
    }

    lastStepIndex=route.routeSteps.size()-1;

    prevNode=node;
  }

  DBThreadRef dbThread=OSMScoutQt::GetInstance().GetDBThread();
  if (dbThread->TransformRouteDataToWay(databasePath,
                                        vehicle,
                                        route.routeData,
                                        routeWay)) {
    dbThread->ClearRoute();
    dbThread->AddRoute(routeWay);
  }
  else {
    // std::cerr << "Error while transforming route" << std::endl;
    emit routeFailed("Error while transforming route",requestId);
    return;
  }

  emit routeComputed(route,requestId);
}
