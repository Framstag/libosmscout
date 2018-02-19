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

#include <osmscout/RouteDescriptionBuilder.h>
#include <osmscout/routing/Route.h>

#include <QMetaType>
#include <QVariant>

#include <sstream>
#include <iomanip>

RouteDescriptionBuilder::RouteDescriptionBuilder()
{
}

RouteDescriptionBuilder::~RouteDescriptionBuilder()
{
}

static QString TurnCommandType(const osmscout::RouteDescription::DirectionDescriptionRef& directionDescription)
{
  if (!directionDescription) {
    return "turn";
  }
  switch (directionDescription->GetCurve()) {
    case osmscout::RouteDescription::DirectionDescription::sharpLeft:
      return "turn-sharp-left";
    case osmscout::RouteDescription::DirectionDescription::left:
      return "turn-left";
    case osmscout::RouteDescription::DirectionDescription::slightlyLeft:
      return "turn-slightly-left";
    case osmscout::RouteDescription::DirectionDescription::straightOn:
      return "continue-straight-on";
    case osmscout::RouteDescription::DirectionDescription::slightlyRight:
      return "turn-slightly-right";
    case osmscout::RouteDescription::DirectionDescription::right:
      return "turn-right";
    case osmscout::RouteDescription::DirectionDescription::sharpRight:
      return "turn-sharp-right";
  }

  assert(false);

  return "???";
}

static QString ShortTurnCommand(const osmscout::RouteDescription::DirectionDescriptionRef& directionDescription)
{
  if (!directionDescription){
    return RouteDescriptionBuilder::tr("Turn");
  }
  switch (directionDescription->GetCurve()) {
    case osmscout::RouteDescription::DirectionDescription::sharpLeft:
      return RouteDescriptionBuilder::tr("Turn sharp left");
    case osmscout::RouteDescription::DirectionDescription::left:
      return RouteDescriptionBuilder::tr("Turn left");
    case osmscout::RouteDescription::DirectionDescription::slightlyLeft:
      return RouteDescriptionBuilder::tr("Turn slightly left");
    case osmscout::RouteDescription::DirectionDescription::straightOn:
      return RouteDescriptionBuilder::tr("Straight on");
    case osmscout::RouteDescription::DirectionDescription::slightlyRight:
      return RouteDescriptionBuilder::tr("Turn slightly right");
    case osmscout::RouteDescription::DirectionDescription::right:
      return RouteDescriptionBuilder::tr("Turn right");
    case osmscout::RouteDescription::DirectionDescription::sharpRight:
      return RouteDescriptionBuilder::tr("Turn sharp right");
  }

  assert(false);

  return "???";
}

static QString FullTurnCommand(const osmscout::RouteDescription::DirectionDescriptionRef& directionDescription)
{
  if (!directionDescription){
    return RouteDescriptionBuilder::tr("At crossing %1<strong>Turn</strong> into %2");
  }
  switch (directionDescription->GetCurve()) {
    case osmscout::RouteDescription::DirectionDescription::sharpLeft:
      return RouteDescriptionBuilder::tr("At crossing %1<strong>Turn sharp left</strong> into %2");
    case osmscout::RouteDescription::DirectionDescription::left:
      return RouteDescriptionBuilder::tr("At crossing %1<strong>Turn left</strong> into %2");
    case osmscout::RouteDescription::DirectionDescription::slightlyLeft:
      return RouteDescriptionBuilder::tr("At crossing %1<strong>Turn slightly left</strong> into %2");
    case osmscout::RouteDescription::DirectionDescription::straightOn:
      return RouteDescriptionBuilder::tr("At crossing %1<strong>Straight on</strong> into %2");
    case osmscout::RouteDescription::DirectionDescription::slightlyRight:
      return RouteDescriptionBuilder::tr("At crossing %1<strong>Turn slightly right</strong> into %2");
    case osmscout::RouteDescription::DirectionDescription::right:
      return RouteDescriptionBuilder::tr("At crossing %1<strong>Turn right</strong> into %2");
    case osmscout::RouteDescription::DirectionDescription::sharpRight:
      return RouteDescriptionBuilder::tr("At crossing %1<strong>Turn sharp right</strong> into %2");
  }

  assert(false);

  return "???";
}

static QString TurnCommandWithList(const osmscout::RouteDescription::DirectionDescriptionRef& directionDescription)
{
  if (!directionDescription){
    return RouteDescriptionBuilder::tr("At crossing %1<strong>Turn</strong>");
  }
  switch (directionDescription->GetCurve()) {
    case osmscout::RouteDescription::DirectionDescription::sharpLeft:
      return RouteDescriptionBuilder::tr("At crossing %1<strong>Turn sharp left</strong>");
    case osmscout::RouteDescription::DirectionDescription::left:
      return RouteDescriptionBuilder::tr("At crossing %1<strong>Turn left</strong>");
    case osmscout::RouteDescription::DirectionDescription::slightlyLeft:
      return RouteDescriptionBuilder::tr("At crossing %1<strong>Turn slightly left</strong>");
    case osmscout::RouteDescription::DirectionDescription::straightOn:
      return RouteDescriptionBuilder::tr("At crossing %1<strong>Straight on</strong>");
    case osmscout::RouteDescription::DirectionDescription::slightlyRight:
      return RouteDescriptionBuilder::tr("At crossing %1<strong>Turn slightly right</strong>");
    case osmscout::RouteDescription::DirectionDescription::right:
      return RouteDescriptionBuilder::tr("At crossing %1<strong>Turn right</strong>");
    case osmscout::RouteDescription::DirectionDescription::sharpRight:
      return RouteDescriptionBuilder::tr("At crossing %1<strong>Turn sharp right</strong>");
  }

  assert(false);

  return "???";
}

static QString FormatName(const osmscout::RouteDescription::NameDescription &nameDescription)
{
  std::string name=nameDescription.GetName();
  std::string ref=nameDescription.GetRef();
  if (name.empty() &&
      ref.empty()) {
    return RouteDescriptionBuilder::tr("unnamed road");
  }
  if (name.empty()){
    return RouteDescriptionBuilder::tr("(%1)").arg(QString::fromStdString(ref));
  }
  if (ref.empty()){
    return RouteDescriptionBuilder::tr("\"%1\"").arg(QString::fromStdString(name));
  }
  return RouteDescriptionBuilder::tr("\"%1\" (%2)").arg(QString::fromStdString(name)).arg(QString::fromStdString(ref));
}

static QString CrossingWaysDescriptionToString(const osmscout::RouteDescription::CrossingWaysDescription& crossingWaysDescription)
{
  std::set<QString>                              names;
  osmscout::RouteDescription::NameDescriptionRef originDescription=crossingWaysDescription.GetOriginDesccription();
  osmscout::RouteDescription::NameDescriptionRef targetDescription=crossingWaysDescription.GetTargetDesccription();

  if (originDescription) {
    QString nameString=FormatName(*originDescription);

    if (!nameString.isEmpty()) {
      names.insert(nameString);
    }
  }

  if (targetDescription) {
    QString nameString=FormatName(*targetDescription);

    if (!nameString.isEmpty()) {
      names.insert(nameString);
    }
  }

  for (std::list<osmscout::RouteDescription::NameDescriptionRef>::const_iterator name=crossingWaysDescription.GetDescriptions().begin();
       name!=crossingWaysDescription.GetDescriptions().end();
       ++name) {
    QString nameString=FormatName(*(*name));

    if (!nameString.isEmpty()) {
      names.insert(nameString);
    }
  }

  if (names.size()>0) {
    std::ostringstream stream;

    stream << "<ul>";
    for (std::set<QString>::const_iterator name=names.begin();
         name!=names.end();
         ++name) {
      stream << "<li>" << name->toStdString() << "</li>";
    }
    stream << "</ul>";

    return QString::fromStdString(stream.str());
  }
  else {
    return "";
  }
}


void RouteDescriptionBuilder::DumpStartDescription(QList<RouteStep> &routeSteps,
                                                   const osmscout::RouteDescription::StartDescriptionRef& startDescription,
                                                   const osmscout::RouteDescription::NameDescriptionRef& nameDescription)
{
  QString startDesc;
  QString driveAlongDesc;
  if (startDescription && !startDescription->GetDescription().empty()) {
    startDesc = RouteDescriptionBuilder::tr("\"%1\"")
        .arg(QString::fromStdString(startDescription->GetDescription()));
  }
  if (nameDescription && nameDescription->HasName()) {
    driveAlongDesc = FormatName(*nameDescription);
  }

  if (!startDesc.isEmpty()){
    RouteStep startAt("start");
    startAt.description=::RouteDescriptionBuilder::tr("<strong>Start</strong> at %1").arg(startDesc);
    startAt.shortDescription=::RouteDescriptionBuilder::tr("Start");
    routeSteps.push_back(startAt);

    if (!driveAlongDesc.isEmpty()) {
      RouteStep driveAlong("drive-along");
      driveAlong.description=::RouteDescriptionBuilder::tr("<strong>Continue</strong> along %1").arg(driveAlongDesc);
      driveAlong.shortDescription=::RouteDescriptionBuilder::tr("Continue");
      routeSteps.push_back(driveAlong);
    }
  } else if (!driveAlongDesc.isEmpty()) {
    RouteStep startAt("start");
    startAt.description=::RouteDescriptionBuilder::tr("<strong>Start</strong> along %1").arg(driveAlongDesc);
    startAt.shortDescription=::RouteDescriptionBuilder::tr("Start");
    routeSteps.push_back(startAt);
  } else {
    RouteStep start("start");
    start.description=::RouteDescriptionBuilder::tr("<strong>Start</strong>");
    start.shortDescription=::RouteDescriptionBuilder::tr("Start");
    routeSteps.push_back(start);
  }
}

void RouteDescriptionBuilder::DumpTargetDescription(QList<RouteStep> &routeSteps,
                                                    const osmscout::RouteDescription::TargetDescriptionRef& targetDescription)
{
  RouteStep targetReached("target");

  QString targetDesc;
  if (targetDescription && !targetDescription->GetDescription().empty()){
    targetDesc = RouteDescriptionBuilder::tr("\"%1\"")
        .arg(QString::fromStdString(targetDescription->GetDescription()));
  }
  if (!targetDesc.isEmpty()){
    targetReached.description=::RouteDescriptionBuilder::tr("<strong>Target reached</strong> at %1").arg(targetDesc);
  }else{
    targetReached.description=::RouteDescriptionBuilder::tr("<strong>Target reached</strong>");
  }
  targetReached.shortDescription=::RouteDescriptionBuilder::tr("Target reached");
  routeSteps.push_back(targetReached);
}

void RouteDescriptionBuilder::DumpTurnDescription(QList<RouteStep> &routeSteps,
                                                  const osmscout::RouteDescription::TurnDescriptionRef& /*turnDescription*/,
                                                  const osmscout::RouteDescription::CrossingWaysDescriptionRef& crossingWaysDescription,
                                                  const osmscout::RouteDescription::DirectionDescriptionRef& directionDescription,
                                                  const osmscout::RouteDescription::NameDescriptionRef& nameDescription)
{
  RouteStep   turn(TurnCommandType(directionDescription));
  turn.shortDescription=ShortTurnCommand(directionDescription);

  QString crossingWaysString;
  QString targetName;

  if (crossingWaysDescription) {
    crossingWaysString=CrossingWaysDescriptionToString(*crossingWaysDescription);
  }
  if (nameDescription && nameDescription->HasName()) {
    targetName=FormatName(*nameDescription);
  }

  if (!crossingWaysString.isEmpty() && !targetName.isEmpty()) {
    turn.description=FullTurnCommand(directionDescription).arg(crossingWaysString).arg(targetName);
  } else if (!crossingWaysString.isEmpty()) {
    turn.description=TurnCommandWithList(directionDescription).arg(crossingWaysString);
  } else {
    turn.description=QString("<strong>%1</strong>").arg(turn.shortDescription);
  }

  routeSteps.push_back(turn);
}

void RouteDescriptionBuilder::DumpRoundaboutEnterDescription(QList<RouteStep> &routeSteps,
                                                             const osmscout::RouteDescription::RoundaboutEnterDescriptionRef& /*roundaboutEnterDescription*/,
                                                             const osmscout::RouteDescription::CrossingWaysDescriptionRef& crossingWaysDescription)
{
  RouteStep   enter("enter-roundabout");
  QString crossingWaysString;

  if (crossingWaysDescription) {
    crossingWaysString=CrossingWaysDescriptionToString(*crossingWaysDescription);
  }

  if (!crossingWaysString.isEmpty()) {
    enter.description=::RouteDescriptionBuilder::tr("At crossing %1<strong>Enter roundabout</strong>")
        .arg(crossingWaysString);
  }else {
    enter.description=::RouteDescriptionBuilder::tr("<strong>Enter roundabout</strong>");
  }
  enter.shortDescription=::RouteDescriptionBuilder::tr("Enter roundabout");
  routeSteps.push_back(enter);
}

void RouteDescriptionBuilder::DumpRoundaboutLeaveDescription(QList<RouteStep> &routeSteps,
                                                             const osmscout::RouteDescription::RoundaboutLeaveDescriptionRef& roundaboutLeaveDescription,
                                                             const osmscout::RouteDescription::NameDescriptionRef& nameDescription)
{
  RouteStep leave("leave-roundabout");

  leave.shortDescription=::RouteDescriptionBuilder::tr("Leave roundabout");

  if (nameDescription &&
      nameDescription->HasName()) {

    leave.description=::RouteDescriptionBuilder::tr("<strong>Leave roundabout</strong> on %1. exit into street %2")
        .arg(roundaboutLeaveDescription->GetExitCount())
        .arg(FormatName(*nameDescription));
  }else{
    leave.description=::RouteDescriptionBuilder::tr("<strong>Leave roundabout</strong> on %1. exit")
        .arg(roundaboutLeaveDescription->GetExitCount());
  }

  routeSteps.push_back(leave);
}

void RouteDescriptionBuilder::DumpMotorwayEnterDescription(QList<RouteStep> &routeSteps,
                                                           const osmscout::RouteDescription::MotorwayEnterDescriptionRef& motorwayEnterDescription,
                                                           const osmscout::RouteDescription::CrossingWaysDescriptionRef& crossingWaysDescription)
{
  RouteStep   enter("enter-motorway");

  enter.shortDescription=::RouteDescriptionBuilder::tr("Enter motorway");

  QString crossingWaysString;

  if (crossingWaysDescription) {
    crossingWaysString=CrossingWaysDescriptionToString(*crossingWaysDescription);
  }

  if (motorwayEnterDescription->GetToDescription() &&
      motorwayEnterDescription->GetToDescription()->HasName()) {

    if (!crossingWaysString.isEmpty()){
      enter.description=::RouteDescriptionBuilder::tr("At crossing %1<strong>Enter motorway</strong> %2")
          .arg(crossingWaysString)
          .arg(FormatName(*(motorwayEnterDescription->GetToDescription())));
    }else {
      enter.description=::RouteDescriptionBuilder::tr("<strong>Enter motorway</strong> %1")
          .arg(FormatName(*(motorwayEnterDescription->GetToDescription())));
    }
  }else{
    if (!crossingWaysString.isEmpty()){
      enter.description=::RouteDescriptionBuilder::tr("At crossing %1<strong>Enter motorway</strong>")
          .arg(crossingWaysString);
    }else {
      enter.description=::RouteDescriptionBuilder::tr("<strong>Enter motorway</strong>");
    }
  }

  routeSteps.push_back(enter);
}

void RouteDescriptionBuilder::DumpMotorwayChangeDescription(QList<RouteStep> &routeSteps,
                                                            const osmscout::RouteDescription::MotorwayChangeDescriptionRef& motorwayChangeDescription)
{
  RouteStep change("change-motorway");

  change.shortDescription=::RouteDescriptionBuilder::tr("Change motorway");

  if (motorwayChangeDescription->GetFromDescription() &&
      motorwayChangeDescription->GetFromDescription()->HasName() &&
      motorwayChangeDescription->GetToDescription() &&
      motorwayChangeDescription->GetToDescription()->HasName()) {

    change.description=::RouteDescriptionBuilder::tr("<strong>Change motorway</strong> from %1 to %2")
        .arg(FormatName(*(motorwayChangeDescription->GetFromDescription())))
        .arg(FormatName(*(motorwayChangeDescription->GetToDescription())));
  }else{
    change.description=::RouteDescriptionBuilder::tr("<strong>Change motorway</strong>");
  }

  routeSteps.push_back(change);
}

void RouteDescriptionBuilder::DumpMotorwayLeaveDescription(QList<RouteStep> &routeSteps,
                                                           const osmscout::RouteDescription::MotorwayLeaveDescriptionRef& motorwayLeaveDescription,
                                                           const osmscout::RouteDescription::DirectionDescriptionRef& /*directionDescription*/,
                                                           const osmscout::RouteDescription::NameDescriptionRef& nameDescription)
{
  RouteStep leave("leave-motorway");

  leave.shortDescription=::RouteDescriptionBuilder::tr("Leave motorway");

  // TODO: should we add leave direction to phrase? directionDescription->GetCurve()
  if (motorwayLeaveDescription->GetFromDescription() &&
      motorwayLeaveDescription->GetFromDescription()->HasName()) {

    if (nameDescription &&
        nameDescription->HasName()) {

      leave.description = ::RouteDescriptionBuilder::tr("<strong>Leave motorway</strong> %1 into %2")
          .arg(FormatName(*(motorwayLeaveDescription->GetFromDescription())))
          .arg(FormatName(*nameDescription));
    }else{
      leave.description = ::RouteDescriptionBuilder::tr("<strong>Leave motorway</strong> %1")
          .arg(FormatName(*(motorwayLeaveDescription->GetFromDescription())));
    }
  }else{
    leave.description=::RouteDescriptionBuilder::tr("<strong>Leave motorway</strong>");
  }

  routeSteps.push_back(leave);
}

void RouteDescriptionBuilder::DumpNameChangedDescription(QList<RouteStep> &routeSteps,
                                                         const osmscout::RouteDescription::NameChangedDescriptionRef& nameChangedDescription)
{
  RouteStep changed("name-change");

  changed.shortDescription=::RouteDescriptionBuilder::tr("Way changes name");

  if (nameChangedDescription->GetOriginDesccription()) {
    changed.description=::RouteDescriptionBuilder::tr("<strong>Way changes name</strong> from %1 to %2")
        .arg(FormatName(*(nameChangedDescription->GetOriginDesccription())))
        .arg(FormatName(*(nameChangedDescription->GetTargetDesccription())));
  } else {
    changed.description=::RouteDescriptionBuilder::tr("<strong>Way changes name</strong> to %1")
      .arg(FormatName(*(nameChangedDescription->GetTargetDesccription())));
  }

  routeSteps.push_back(changed);
}

bool RouteDescriptionBuilder::GenerateRouteStep(const osmscout::RouteDescription::Node &node,
                                                QList<RouteStep> &routeSteps,
                                                size_t &roundaboutCrossingCounter)
{
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

  desc=node.GetDescription(osmscout::RouteDescription::WAY_NAME_DESC);
  if (desc) {
    nameDescription=std::dynamic_pointer_cast<osmscout::RouteDescription::NameDescription>(desc);
  }

  desc=node.GetDescription(osmscout::RouteDescription::DIRECTION_DESC);
  if (desc) {
    directionDescription=std::dynamic_pointer_cast<osmscout::RouteDescription::DirectionDescription>(desc);
  }

  desc=node.GetDescription(osmscout::RouteDescription::WAY_NAME_CHANGED_DESC);
  if (desc) {
    nameChangedDescription=std::dynamic_pointer_cast<osmscout::RouteDescription::NameChangedDescription>(desc);
  }

  desc=node.GetDescription(osmscout::RouteDescription::CROSSING_WAYS_DESC);
  if (desc) {
    crossingWaysDescription=std::dynamic_pointer_cast<osmscout::RouteDescription::CrossingWaysDescription>(desc);
  }

  desc=node.GetDescription(osmscout::RouteDescription::NODE_START_DESC);
  if (desc) {
    startDescription=std::dynamic_pointer_cast<osmscout::RouteDescription::StartDescription>(desc);
  }

  desc=node.GetDescription(osmscout::RouteDescription::NODE_TARGET_DESC);
  if (desc) {
    targetDescription=std::dynamic_pointer_cast<osmscout::RouteDescription::TargetDescription>(desc);
  }

  desc=node.GetDescription(osmscout::RouteDescription::TURN_DESC);
  if (desc) {
    turnDescription=std::dynamic_pointer_cast<osmscout::RouteDescription::TurnDescription>(desc);
  }

  desc=node.GetDescription(osmscout::RouteDescription::ROUNDABOUT_ENTER_DESC);
  if (desc) {
    roundaboutEnterDescription=std::dynamic_pointer_cast<osmscout::RouteDescription::RoundaboutEnterDescription>(desc);
  }

  desc=node.GetDescription(osmscout::RouteDescription::ROUNDABOUT_LEAVE_DESC);
  if (desc) {
    roundaboutLeaveDescription=std::dynamic_pointer_cast<osmscout::RouteDescription::RoundaboutLeaveDescription>(desc);
  }

  desc=node.GetDescription(osmscout::RouteDescription::MOTORWAY_ENTER_DESC);
  if (desc) {
    motorwayEnterDescription=std::dynamic_pointer_cast<osmscout::RouteDescription::MotorwayEnterDescription>(desc);
  }

  desc=node.GetDescription(osmscout::RouteDescription::MOTORWAY_CHANGE_DESC);
  if (desc) {
    motorwayChangeDescription=std::dynamic_pointer_cast<osmscout::RouteDescription::MotorwayChangeDescription>(desc);
  }

  desc=node.GetDescription(osmscout::RouteDescription::MOTORWAY_LEAVE_DESC);
  if (desc) {
    motorwayLeaveDescription=std::dynamic_pointer_cast<osmscout::RouteDescription::MotorwayLeaveDescription>(desc);
  }

  if (crossingWaysDescription &&
      roundaboutCrossingCounter>0 &&
      crossingWaysDescription->GetExitCount()>1) {
    roundaboutCrossingCounter+=crossingWaysDescription->GetExitCount()-1;
  }

  if (startDescription) {
    DumpStartDescription(routeSteps,
                         startDescription,
                         nameDescription);
  }
  else if (targetDescription) {
    DumpTargetDescription(routeSteps,
                          targetDescription);
  }
  else if (turnDescription) {
    DumpTurnDescription(routeSteps,
                        turnDescription,
                        crossingWaysDescription,
                        directionDescription,
                        nameDescription);
  }
  else if (roundaboutEnterDescription) {
    DumpRoundaboutEnterDescription(routeSteps,
                                   roundaboutEnterDescription,
                                   crossingWaysDescription);

    roundaboutCrossingCounter=1;
  }
  else if (roundaboutLeaveDescription) {
    DumpRoundaboutLeaveDescription(routeSteps,
                                   roundaboutLeaveDescription,
                                   nameDescription);

    roundaboutCrossingCounter=0;
  }
  else if (motorwayEnterDescription) {
    DumpMotorwayEnterDescription(routeSteps,
                                 motorwayEnterDescription,
                                 crossingWaysDescription);
  }
  else if (motorwayChangeDescription) {
    DumpMotorwayChangeDescription(routeSteps,
                                  motorwayChangeDescription);
  }
  else if (motorwayLeaveDescription) {
    DumpMotorwayLeaveDescription(routeSteps,
                                 motorwayLeaveDescription,
                                 directionDescription,
                                 nameDescription);
  }
  else if (nameChangedDescription) {
    DumpNameChangedDescription(routeSteps,
                               nameChangedDescription);
  }
  else {
    return false;
  }
  return true;
}

void RouteDescriptionBuilder::GenerateRouteSteps(const osmscout::RouteDescription &routeDescription,
                                                 QList<RouteStep> &routeSteps)
{
  size_t roundaboutCrossingCounter=0;
  int    lastStepIndex=-1;

  std::list<osmscout::RouteDescription::Node>::const_iterator prevNode=routeDescription.Nodes().end();
  for (std::list<osmscout::RouteDescription::Node>::const_iterator node=routeDescription.Nodes().begin();
       node!=routeDescription.Nodes().end();
       ++node) {

    if (!GenerateRouteStep(*node, routeSteps, roundaboutCrossingCounter)){
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
      routeSteps[currentStepIndex].distance=node->GetDistance() * 1000;
      routeSteps[currentStepIndex].time=node->GetTime() * 3600;

      if (prevNode!=routeDescription.Nodes().end() &&
          node->GetDistance()-prevNode->GetDistance()!=0.0) {
        routeSteps[currentStepIndex].distanceDelta=node->GetDistance()-prevNode->GetDistance() * 1000;
      }

      if (prevNode!=routeDescription.Nodes().end() &&
          node->GetTime()-prevNode->GetTime()!=0.0) {
        routeSteps[currentStepIndex].timeDelta=node->GetTime()-prevNode->GetTime() * 3600;
      }
    }

    lastStepIndex=routeSteps.size()-1;

    prevNode=node;
  }
}
