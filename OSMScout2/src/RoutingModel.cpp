/*
 OSMScout - a Qt backend for libosmscout and libosmscout-map
 Copyright (C) 2010  Tim Teulings

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "RoutingModel.h"

#include <cmath>
#include <iostream>
#include <iomanip>

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

  if (originDescription.Valid()) {
    std::string nameString=originDescription->GetDescription();

    if (!nameString.empty()) {
      names.insert(nameString);
    }
  }

  if (targetDescription.Valid()) {
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

    for (std::set<std::string>::const_iterator name=names.begin();
        name!=names.end();
        ++name) {
      if (name!=names.begin()) {
        stream << ", ";
      }
      stream << "'" << *name << "'";
    }

    return stream.str();
  }
  else {
    return "";
  }
}

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

RoutingListModel::RoutingListModel(QObject* parent)
: QAbstractListModel(parent)
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

RoutingListModel::~RoutingListModel()
{
    route.routeSteps.clear();
}

void RoutingListModel::GetCarSpeedTable(std::map<std::string,double>& map)
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
  map["highway_unclassified"]=50.0;
  map["highway_road"]=50.0;
  map["highway_residential"]=40.0;
  map["highway_roundabout"]=40.0;
  map["highway_living_street"]=10.0;
  map["highway_service"]=30.0;
}

void RoutingListModel::DumpStartDescription(const osmscout::RouteDescription::StartDescriptionRef& startDescription,
                                            const osmscout::RouteDescription::NameDescriptionRef& nameDescription)
{
  RouteStep startAt;

  startAt.description="Start at '"+QString::fromUtf8(startDescription->GetDescription().c_str())+"'";
  route.routeSteps.push_back(startAt);

  if (nameDescription.Valid() &&
      nameDescription->HasName()) {
    RouteStep driveAlong;

    driveAlong.description="Drive along '"+QString::fromUtf8(nameDescription->GetDescription().c_str())+"'";
    route.routeSteps.push_back(driveAlong);
  }
}

void RoutingListModel::DumpTargetDescription(const osmscout::RouteDescription::TargetDescriptionRef& targetDescription)
{
  RouteStep targetReached;

  targetReached.description="Target reached '"+QString::fromUtf8(targetDescription->GetDescription().c_str())+"'";
  route.routeSteps.push_back(targetReached);
}

void RoutingListModel::DumpTurnDescription(const osmscout::RouteDescription::TurnDescriptionRef& /*turnDescription*/,
                                           const osmscout::RouteDescription::CrossingWaysDescriptionRef& crossingWaysDescription,
                                           const osmscout::RouteDescription::DirectionDescriptionRef& directionDescription,
                                           const osmscout::RouteDescription::NameDescriptionRef& nameDescription)
{
  std::string crossingWaysString;

  if (crossingWaysDescription.Valid()) {
    crossingWaysString=CrossingWaysDescriptionToString(crossingWaysDescription);
  }

  if (!crossingWaysString.empty()) {
    RouteStep at;

    at.description="At crossing "+QString::fromUtf8(crossingWaysString.c_str());
    route.routeSteps.push_back(at);
  }

  RouteStep turn;

  if (directionDescription.Valid()) {
    turn.description=MoveToTurnCommand(directionDescription->GetCurve());
  }
  else {
    turn.description="Turn";
  }

  if (nameDescription.Valid() &&
      nameDescription->HasName()) {
    turn.description+=" into '"+QString::fromUtf8(nameDescription->GetDescription().c_str())+"'";
  }

  route.routeSteps.push_back(turn);
}

void RoutingListModel::DumpRoundaboutEnterDescription(const osmscout::RouteDescription::RoundaboutEnterDescriptionRef& /*roundaboutEnterDescription*/,
                                                      const osmscout::RouteDescription::CrossingWaysDescriptionRef& crossingWaysDescription)
{
  std::string crossingWaysString;

  if (crossingWaysDescription.Valid()) {
    crossingWaysString=CrossingWaysDescriptionToString(crossingWaysDescription);
  }

  if (!crossingWaysString.empty()) {
    RouteStep   at;

    at.description="At crossing ";
    at.description+=QString::fromUtf8(crossingWaysString.c_str());

    route.routeSteps.push_back(at);
  }

  RouteStep enter;

  enter.description="Enter roundabout";

  route.routeSteps.push_back(enter);
}

void RoutingListModel::DumpRoundaboutLeaveDescription(const osmscout::RouteDescription::RoundaboutLeaveDescriptionRef& roundaboutLeaveDescription,
                                                      const osmscout::RouteDescription::NameDescriptionRef& nameDescription)
{
  RouteStep leave;

  leave.description="Leave roundabout (";
  leave.description+=roundaboutLeaveDescription->GetExitCount();
  leave.description+=". exit)";

  if (nameDescription.Valid() &&
      nameDescription->HasName()) {
    leave.description+=" into street '";
    leave.description+=QString::fromUtf8(nameDescription->GetDescription().c_str());
    leave.description+="'";
  }

  route.routeSteps.push_back(leave);
}

void RoutingListModel::DumpMotorwayEnterDescription(const osmscout::RouteDescription::MotorwayEnterDescriptionRef& motorwayEnterDescription,
                                                    const osmscout::RouteDescription::CrossingWaysDescriptionRef& crossingWaysDescription)
{
  std::string crossingWaysString;

  if (crossingWaysDescription.Valid()) {
    crossingWaysString=CrossingWaysDescriptionToString(crossingWaysDescription);
  }

  if (!crossingWaysString.empty()) {
    RouteStep at;

    at.description="At crossing ";
    at.description+=QString::fromUtf8(crossingWaysString.c_str());
    route.routeSteps.push_back(at);
  }

  RouteStep enter;

  enter.description="Enter motorway";

  if (motorwayEnterDescription->GetToDescription().Valid() &&
      motorwayEnterDescription->GetToDescription()->HasName()) {
    enter.description+=" '";
    enter.description+=QString::fromUtf8(motorwayEnterDescription->GetToDescription()->GetDescription().c_str());
    enter.description+="'";
  }

  route.routeSteps.push_back(enter);
}

void RoutingListModel::DumpMotorwayChangeDescription(const osmscout::RouteDescription::MotorwayChangeDescriptionRef& motorwayChangeDescription)
{
  RouteStep change;

  change.description="Change motorway";

  if (motorwayChangeDescription->GetFromDescription().Valid() &&
      motorwayChangeDescription->GetFromDescription()->HasName()) {
    change.description+=" from '";
    change.description+=QString::fromUtf8(motorwayChangeDescription->GetFromDescription()->GetDescription().c_str());
    change.description+="'";
  }

  if (motorwayChangeDescription->GetToDescription().Valid() &&
      motorwayChangeDescription->GetToDescription()->HasName()) {
    change.description+=" to '";
    change.description+=QString::fromUtf8(motorwayChangeDescription->GetToDescription()->GetDescription().c_str());
    change.description+="'";
  }

  route.routeSteps.push_back(change);
}

void RoutingListModel::DumpMotorwayLeaveDescription(const osmscout::RouteDescription::MotorwayLeaveDescriptionRef& motorwayLeaveDescription,
                                                    const osmscout::RouteDescription::DirectionDescriptionRef& directionDescription,
                                                    const osmscout::RouteDescription::NameDescriptionRef& nameDescription)
{
  RouteStep leave;

  leave.description="Leave motorway";

  if (motorwayLeaveDescription->GetFromDescription().Valid() &&
      motorwayLeaveDescription->GetFromDescription()->HasName()) {
    leave.description+=" '";
    leave.description+=QString::fromUtf8(motorwayLeaveDescription->GetFromDescription()->GetDescription().c_str());
    leave.description+="'";
  }

  if (directionDescription.Valid() &&
      directionDescription->GetCurve()!=osmscout::RouteDescription::DirectionDescription::slightlyLeft &&
      directionDescription->GetCurve()!=osmscout::RouteDescription::DirectionDescription::straightOn &&
      directionDescription->GetCurve()!=osmscout::RouteDescription::DirectionDescription::slightlyRight) {
    leave.description+=MoveToTurnCommand(directionDescription->GetCurve());
  }

  if (nameDescription.Valid() &&
      nameDescription->HasName()) {
    leave.description+=" into '";
    leave.description+=QString::fromUtf8(nameDescription->GetDescription().c_str());
    leave.description+="'";
  }

  route.routeSteps.push_back(leave);
}

void RoutingListModel::DumpNameChangedDescription(const osmscout::RouteDescription::NameChangedDescriptionRef& nameChangedDescription)
{
  RouteStep changed;

  changed.description="Way changes name";

  if (nameChangedDescription->GetOriginDesccription().Valid()) {
    changed.description+=" from '";
    changed.description+=QString::fromUtf8(nameChangedDescription->GetOriginDesccription()->GetDescription().c_str());
    changed.description+="'";
  }

  changed.description+="' to '";
  changed.description+=QString::fromUtf8(nameChangedDescription->GetTargetDesccription()->GetDescription().c_str());
  changed.description+="'";

  route.routeSteps.push_back(changed);
}

void RoutingListModel::setStartAndTarget(Location* start,
                                         Location* target)
{
  beginResetModel();

  route.routeSteps.clear();

  std::cout << "Routing from '" << start->getName().toLocal8Bit().data() << "' to '" << target->getName().toLocal8Bit().data() << "'" << std::endl;

  osmscout::FastestPathRoutingProfile routingProfile;
  osmscout::Way                       routeWay;
  osmscout::Vehicle                   vehicle=osmscout::vehicleCar;//settings->GetRoutingVehicle();

  if (vehicle==osmscout::vehicleFoot) {
    routingProfile.ParametrizeForFoot(*DBThread::GetInstance()->GetTypeConfig(),
                                      5.0);
  }
  else if (vehicle==osmscout::vehicleBicycle) {
    routingProfile.ParametrizeForBicycle(*DBThread::GetInstance()->GetTypeConfig(),
                                         20.0);
  }
  else /* car */ {
    std::map<std::string,double> speedMap;

    GetCarSpeedTable(speedMap);

    routingProfile.ParametrizeForCar(*DBThread::GetInstance()->GetTypeConfig(),
                                     speedMap,
                                     160.0);
  }

  osmscout::ObjectFileRef startObject;
  size_t                  startNodeIndex;

  osmscout::ObjectFileRef targetObject;
  size_t                  targetNodeIndex;

  if (!DBThread::GetInstance()->GetClosestRoutableNode(start->getReferences().front(),
                                                       vehicle,
                                                       1000,
                                                       startObject,
                                                       startNodeIndex)) {
    std::cerr << "There was an error while routing!" << std::endl;
  }

  if (!startObject.Valid()) {
    std::cerr << "Cannot find a routing node close to the start location" << std::endl;
  }

  if (!DBThread::GetInstance()->GetClosestRoutableNode(target->getReferences().front(),
                                                       vehicle,
                                                       1000,
                                                       targetObject,
                                                       targetNodeIndex)) {
    std::cerr << "There was an error while routing!" << std::endl;
  }

  if (!targetObject.Valid()) {
    std::cerr << "Cannot find a routing node close to the target location" << std::endl;
  }

  if (!DBThread::GetInstance()->CalculateRoute(vehicle,
                                               routingProfile,
                                               startObject,
                                               startNodeIndex,
                                               targetObject,
                                               targetNodeIndex,
                                               route.routeData)) {
    std::cerr << "There was an error while routing!" << std::endl;
    return;
  }

  std::cout << "Route calculated" << std::endl;

  DBThread::GetInstance()->TransformRouteDataToRouteDescription(vehicle,
                                                                routingProfile,
                                                                route.routeData,
                                                                route.routeDescription,
                                                                start->getName().toUtf8().constData(),
                                                                target->getName().toUtf8().constData());

  std::cout << "Route transformed" << std::endl;

  size_t roundaboutCrossingCounter=0;
  int    lastStepIndex=-1;

  std::list<osmscout::RouteDescription::Node>::const_iterator prevNode=route.routeDescription.Nodes().end();
  for (std::list<osmscout::RouteDescription::Node>::const_iterator node=route.routeDescription.Nodes().begin();
       node!=route.routeDescription.Nodes().end();
       ++node) {
      std::cout << "Processing node..." << std::endl;
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
    if (desc.Valid()) {
      nameDescription=dynamic_cast<osmscout::RouteDescription::NameDescription*>(desc.Get());
    }

    desc=node->GetDescription(osmscout::RouteDescription::DIRECTION_DESC);
    if (desc.Valid()) {
      directionDescription=dynamic_cast<osmscout::RouteDescription::DirectionDescription*>(desc.Get());
    }

    desc=node->GetDescription(osmscout::RouteDescription::WAY_NAME_CHANGED_DESC);
    if (desc.Valid()) {
      nameChangedDescription=dynamic_cast<osmscout::RouteDescription::NameChangedDescription*>(desc.Get());
    }

    desc=node->GetDescription(osmscout::RouteDescription::CROSSING_WAYS_DESC);
    if (desc.Valid()) {
      crossingWaysDescription=dynamic_cast<osmscout::RouteDescription::CrossingWaysDescription*>(desc.Get());
    }

    desc=node->GetDescription(osmscout::RouteDescription::NODE_START_DESC);
    if (desc.Valid()) {
      startDescription=dynamic_cast<osmscout::RouteDescription::StartDescription*>(desc.Get());
    }

    desc=node->GetDescription(osmscout::RouteDescription::NODE_TARGET_DESC);
    if (desc.Valid()) {
      targetDescription=dynamic_cast<osmscout::RouteDescription::TargetDescription*>(desc.Get());
    }

    desc=node->GetDescription(osmscout::RouteDescription::TURN_DESC);
    if (desc.Valid()) {
      turnDescription=dynamic_cast<osmscout::RouteDescription::TurnDescription*>(desc.Get());
    }

    desc=node->GetDescription(osmscout::RouteDescription::ROUNDABOUT_ENTER_DESC);
    if (desc.Valid()) {
      roundaboutEnterDescription=dynamic_cast<osmscout::RouteDescription::RoundaboutEnterDescription*>(desc.Get());
    }

    desc=node->GetDescription(osmscout::RouteDescription::ROUNDABOUT_LEAVE_DESC);
    if (desc.Valid()) {
      roundaboutLeaveDescription=dynamic_cast<osmscout::RouteDescription::RoundaboutLeaveDescription*>(desc.Get());
    }

    desc=node->GetDescription(osmscout::RouteDescription::MOTORWAY_ENTER_DESC);
    if (desc.Valid()) {
      motorwayEnterDescription=dynamic_cast<osmscout::RouteDescription::MotorwayEnterDescription*>(desc.Get());
    }

    desc=node->GetDescription(osmscout::RouteDescription::MOTORWAY_CHANGE_DESC);
    if (desc.Valid()) {
      motorwayChangeDescription=dynamic_cast<osmscout::RouteDescription::MotorwayChangeDescription*>(desc.Get());
    }

    desc=node->GetDescription(osmscout::RouteDescription::MOTORWAY_LEAVE_DESC);
    if (desc.Valid()) {
      motorwayLeaveDescription=dynamic_cast<osmscout::RouteDescription::MotorwayLeaveDescription*>(desc.Get());
    }

    if (crossingWaysDescription.Valid() &&
        roundaboutCrossingCounter>0 &&
        crossingWaysDescription->GetExitCount()>1) {
      roundaboutCrossingCounter+=crossingWaysDescription->GetExitCount()-1;
    }

    if (startDescription.Valid()) {
      DumpStartDescription(startDescription,
                           nameDescription);
    }
    else if (targetDescription.Valid()) {
      DumpTargetDescription(targetDescription);
    }
    else if (turnDescription.Valid()) {
      DumpTurnDescription(turnDescription,
                          crossingWaysDescription,
                          directionDescription,
                          nameDescription);
    }
    else if (roundaboutEnterDescription.Valid()) {
      DumpRoundaboutEnterDescription(roundaboutEnterDescription,
                                     crossingWaysDescription);

      roundaboutCrossingCounter=1;
    }
    else if (roundaboutLeaveDescription.Valid()) {
      DumpRoundaboutLeaveDescription(roundaboutLeaveDescription,
                                     nameDescription);

      roundaboutCrossingCounter=0;
    }
    else if (motorwayEnterDescription.Valid()) {
      DumpMotorwayEnterDescription(motorwayEnterDescription,
                                   crossingWaysDescription);
    }
    else if (motorwayChangeDescription.Valid()) {
      DumpMotorwayChangeDescription(motorwayChangeDescription);
    }
    else if (motorwayLeaveDescription.Valid()) {
      DumpMotorwayLeaveDescription(motorwayLeaveDescription,
                                   directionDescription,
                                   nameDescription);
    }
    else if (nameChangedDescription.Valid()) {
      DumpNameChangedDescription(nameChangedDescription);
    }
    else {
      continue;
    }

    std::cout << "Dumped descriptions..." << std::endl;

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

  if (DBThread::GetInstance()->TransformRouteDataToWay(vehicle,
                                                       route.routeData,
                                                       routeWay)) {
    DBThread::GetInstance()->ClearRoute();
    DBThread::GetInstance()->AddRoute(routeWay);
  }
  else {
    std::cerr << "Error while transforming route" << std::endl;
  }

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
