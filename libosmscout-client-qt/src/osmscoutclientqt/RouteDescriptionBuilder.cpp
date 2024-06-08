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

#include <osmscoutclientqt/RouteDescriptionBuilder.h>

#include <set>
#include <sstream>
#include <string>

#include <QMetaType>
#include <QVariant>

#include <osmscout/routing/RouteDescription.h>
#include <osmscout/log/Logger.h>
#include <osmscout/util/String.h>

namespace osmscout {

RouteDescriptionBuilder::RouteDescriptionBuilder::Callback::Callback(QList<RouteStep> &routeSteps,
                                                                     const Distance &stopAfter,
                                                                     bool skipInformative):
  routeSteps(routeSteps), stopAfter(stopAfter), skipInformative(skipInformative)
{
}

static QString TurnCommandType(const RouteDescription::DirectionDescriptionRef& directionDescription)
{
  if (!directionDescription) {
    return "turn";
  }
  switch (directionDescription->GetCurve()) {
    case RouteDescription::DirectionDescription::sharpLeft:
      return "turn-sharp-left";
    case RouteDescription::DirectionDescription::left:
      return "turn-left";
    case RouteDescription::DirectionDescription::slightlyLeft:
      return "turn-slightly-left";
    case RouteDescription::DirectionDescription::straightOn:
      return "continue-straight-on";
    case RouteDescription::DirectionDescription::slightlyRight:
      return "turn-slightly-right";
    case RouteDescription::DirectionDescription::right:
      return "turn-right";
    case RouteDescription::DirectionDescription::sharpRight:
      return "turn-sharp-right";
  }

  assert(false);
  return "???";
}

static QString ShortTurnCommand(const RouteDescription::DirectionDescriptionRef& directionDescription)
{
  if (!directionDescription){
    return RouteDescriptionBuilder::tr("Turn");
  }
  switch (directionDescription->GetCurve()) {
    case RouteDescription::DirectionDescription::sharpLeft:
      return RouteDescriptionBuilder::tr("Turn sharp left");
    case RouteDescription::DirectionDescription::left:
      return RouteDescriptionBuilder::tr("Turn left");
    case RouteDescription::DirectionDescription::slightlyLeft:
      return RouteDescriptionBuilder::tr("Turn slightly left");
    case RouteDescription::DirectionDescription::straightOn:
      return RouteDescriptionBuilder::tr("Straight on");
    case RouteDescription::DirectionDescription::slightlyRight:
      return RouteDescriptionBuilder::tr("Turn slightly right");
    case RouteDescription::DirectionDescription::right:
      return RouteDescriptionBuilder::tr("Turn right");
    case RouteDescription::DirectionDescription::sharpRight:
      return RouteDescriptionBuilder::tr("Turn sharp right");
  }

  assert(false);
  return "???";
}

static QString FullTurnCommand(const RouteDescription::DirectionDescriptionRef& directionDescription)
{
  if (!directionDescription){
    return RouteDescriptionBuilder::tr("At crossing %1<strong>Turn</strong> into %2");
  }
  switch (directionDescription->GetCurve()) {
    case RouteDescription::DirectionDescription::sharpLeft:
      return RouteDescriptionBuilder::tr("At crossing %1<strong>Turn sharp left</strong> into %2");
    case RouteDescription::DirectionDescription::left:
      return RouteDescriptionBuilder::tr("At crossing %1<strong>Turn left</strong> into %2");
    case RouteDescription::DirectionDescription::slightlyLeft:
      return RouteDescriptionBuilder::tr("At crossing %1<strong>Turn slightly left</strong> into %2");
    case RouteDescription::DirectionDescription::straightOn:
      return RouteDescriptionBuilder::tr("At crossing %1<strong>Straight on</strong> into %2");
    case RouteDescription::DirectionDescription::slightlyRight:
      return RouteDescriptionBuilder::tr("At crossing %1<strong>Turn slightly right</strong> into %2");
    case RouteDescription::DirectionDescription::right:
      return RouteDescriptionBuilder::tr("At crossing %1<strong>Turn right</strong> into %2");
    case RouteDescription::DirectionDescription::sharpRight:
      return RouteDescriptionBuilder::tr("At crossing %1<strong>Turn sharp right</strong> into %2");
  }

  assert(false);
  return "???";
}

static QString TurnCommandWithList(const RouteDescription::DirectionDescriptionRef& directionDescription)
{
  if (!directionDescription){
    return RouteDescriptionBuilder::tr("At crossing %1<strong>Turn</strong>");
  }
  switch (directionDescription->GetCurve()) {
    case RouteDescription::DirectionDescription::sharpLeft:
      return RouteDescriptionBuilder::tr("At crossing %1<strong>Turn sharp left</strong>");
    case RouteDescription::DirectionDescription::left:
      return RouteDescriptionBuilder::tr("At crossing %1<strong>Turn left</strong>");
    case RouteDescription::DirectionDescription::slightlyLeft:
      return RouteDescriptionBuilder::tr("At crossing %1<strong>Turn slightly left</strong>");
    case RouteDescription::DirectionDescription::straightOn:
      return RouteDescriptionBuilder::tr("At crossing %1<strong>Straight on</strong>");
    case RouteDescription::DirectionDescription::slightlyRight:
      return RouteDescriptionBuilder::tr("At crossing %1<strong>Turn slightly right</strong>");
    case RouteDescription::DirectionDescription::right:
      return RouteDescriptionBuilder::tr("At crossing %1<strong>Turn right</strong>");
    case RouteDescription::DirectionDescription::sharpRight:
      return RouteDescriptionBuilder::tr("At crossing %1<strong>Turn sharp right</strong>");
  }

  assert(false);
  return "???";
}

static QString FormatName(const RouteDescription::NameDescription &nameDescription)
{
  std::string name=nameDescription.GetName();
  std::string ref=nameDescription.GetRef();
  if (name.empty() &&
      ref.empty()) {
    //: unknown road name
    return RouteDescriptionBuilder::tr("unnamed road");
  }
  if (name.empty()){
    //: road just with ref number
    return RouteDescriptionBuilder::tr("(%1)").arg(QString::fromStdString(ref));
  }
  if (ref.empty()){
    //: road just with name, without ref
    return RouteDescriptionBuilder::tr("\"%1\"").arg(QString::fromStdString(name));
  }
  //: road with name (%1) and ref (%2)
  return RouteDescriptionBuilder::tr("\"%1\" (%2)")
    .arg(QString::fromStdString(name))
    .arg(QString::fromStdString(ref));
}

static QString FormatMotorwayJunctionName(const RouteDescription::NameDescription &nameDescription)
{
  std::string name=nameDescription.GetName();
  std::string ref=nameDescription.GetRef();
  if (name.empty() &&
      ref.empty()) {
    //: unnamed motorway exit
    return RouteDescriptionBuilder::tr("On unnamed exit");
  }
  if (name.empty()){
    //: motorway exit just with ref
    return RouteDescriptionBuilder::tr("On exit %1").arg(QString::fromStdString(ref));
  }
  if (ref.empty()){
    //: motorway exit with name, without ref
    return RouteDescriptionBuilder::tr("On exit \"%1\"").arg(QString::fromStdString(name));
  }
  //: motorway exit with ref (%1) and name (%2)
  return RouteDescriptionBuilder::tr("On exit %1 \"%2\"")
    .arg(QString::fromStdString(ref))
    .arg(QString::fromStdString(name));
}

static QStringList SplitDestinations(const std::string &destinations)
{
  QStringList result;
  for (const std::string &destination: SplitString(destinations, ";")) {
    result << QString::fromStdString(destination);
  }
  return result;
}

static QString CrossingWaysDescriptionToString(const RouteDescription::CrossingWaysDescription& crossingWaysDescription)
{
  std::set<QString>                              names;
  RouteDescription::NameDescriptionRef originDescription=crossingWaysDescription.GetOriginDesccription();
  RouteDescription::NameDescriptionRef targetDescription=crossingWaysDescription.GetTargetDesccription();

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

  for (const auto & name : crossingWaysDescription.GetDescriptions()) {
    QString nameString=FormatName(*name);

    if (!nameString.isEmpty()) {
      names.insert(nameString);
    }
  }

  if (!names.empty()) {
    std::ostringstream stream;

    stream << "<ul>";
    for (const auto & name : names) {
      stream << "<li>" << name.toStdString() << "</li>";
    }
    stream << "</ul>";

    return QString::fromStdString(stream.str());
  }

  return "";
}


void RouteDescriptionBuilder::Callback::OnStart(const RouteDescription::StartDescriptionRef& startDescription,
                                                const RouteDescription::TypeNameDescriptionRef& /*typeNameDescription*/,
                                                const RouteDescription::NameDescriptionRef& nameDescription)
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
    RouteStep startAt = MkStep("start");
    startAt.description=RouteDescriptionBuilder::tr("<strong>Start</strong> at %1").arg(startDesc);
    startAt.shortDescription=RouteDescriptionBuilder::tr("Start");
    routeSteps.push_back(startAt);

    if (!driveAlongDesc.isEmpty()) {
      RouteStep driveAlong = MkStep("drive-along");
      driveAlong.description=RouteDescriptionBuilder::tr("<strong>Continue</strong> along %1").arg(driveAlongDesc);
      driveAlong.shortDescription=RouteDescriptionBuilder::tr("Continue");
      routeSteps.push_back(driveAlong);
    }
  } else if (!driveAlongDesc.isEmpty()) {
    RouteStep startAt = MkStep("start");
    startAt.description=RouteDescriptionBuilder::tr("<strong>Start</strong> along %1").arg(driveAlongDesc);
    startAt.shortDescription=RouteDescriptionBuilder::tr("Start");
    routeSteps.push_back(startAt);
  } else {
    RouteStep start = MkStep("start");
    start.description=RouteDescriptionBuilder::tr("<strong>Start</strong>");
    start.shortDescription=RouteDescriptionBuilder::tr("Start");
    routeSteps.push_back(start);
  }
}

void RouteDescriptionBuilder::Callback::OnTargetReached(const RouteDescription::TargetDescriptionRef& targetDescription)
{
  RouteStep targetReached = MkStep("target");

  QString targetDesc;
  if (targetDescription && !targetDescription->GetDescription().empty()){
    targetDesc = RouteDescriptionBuilder::tr("\"%1\"")
        .arg(QString::fromStdString(targetDescription->GetDescription()));
  }
  if (!targetDesc.isEmpty()){
    targetReached.description=RouteDescriptionBuilder::tr("<strong>Target reached</strong> at %1").arg(targetDesc);
  }else{
    targetReached.description=RouteDescriptionBuilder::tr("<strong>Target reached</strong>");
  }
  targetReached.shortDescription=RouteDescriptionBuilder::tr("Target reached");
  routeSteps.push_back(targetReached);
}

void RouteDescriptionBuilder::Callback::OnTurn(const RouteDescription::TurnDescriptionRef& /*turnDescription*/,
                                               const RouteDescription::CrossingWaysDescriptionRef& crossingWaysDescription,
                                               const RouteDescription::DirectionDescriptionRef& directionDescription,
                                               const RouteDescription::TypeNameDescriptionRef& /*typeNameDescription*/,
                                               const RouteDescription::NameDescriptionRef& nameDescription)
{
  RouteStep turn = MkStep(TurnCommandType(directionDescription));
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

void RouteDescriptionBuilder::Callback::OnRoundaboutEnter(const RouteDescription::RoundaboutEnterDescriptionRef& roundaboutEnterDescription,
                                                          const RouteDescription::CrossingWaysDescriptionRef& crossingWaysDescription)
{
  if (skipInformative){
    if (crossingWaysDescription) {
      PushStreetName(crossingWaysDescription->GetOriginDesccription());
      PushStreetName(crossingWaysDescription->GetTargetDesccription());
    }
    return;
  }

  RouteStep enter = MkStep("enter-roundabout");
  enter.roundaboutClockwise = roundaboutEnterDescription->IsClockwise();

  QString crossingWaysString;

  if (crossingWaysDescription) {
    crossingWaysString=CrossingWaysDescriptionToString(*crossingWaysDescription);
  }

  if (!crossingWaysString.isEmpty()) {
    enter.description=RouteDescriptionBuilder::tr("At crossing %1<strong>Enter roundabout</strong>")
        .arg(crossingWaysString);
  }else {
    enter.description=RouteDescriptionBuilder::tr("<strong>Enter roundabout</strong>");
  }
  enter.shortDescription=RouteDescriptionBuilder::tr("Enter roundabout");
  routeSteps.push_back(enter);
}

void RouteDescriptionBuilder::Callback::OnRoundaboutLeave(const RouteDescription::RoundaboutLeaveDescriptionRef& roundaboutLeaveDescription,
                                                          const RouteDescription::NameDescriptionRef& nameDescription)
{
  RouteStep leave = MkStep("leave-roundabout");
  leave.roundaboutExit = roundaboutLeaveDescription->GetExitCount();
  leave.roundaboutClockwise = roundaboutLeaveDescription->IsClockwise();

  switch (roundaboutLeaveDescription->GetExitCount()){
    case 1:
      leave.shortDescription=RouteDescriptionBuilder::tr("Take the first exit");
      break;
    case 2:
      leave.shortDescription=RouteDescriptionBuilder::tr("Take the second exit");
      break;
    case 3:
      leave.shortDescription=RouteDescriptionBuilder::tr("Take the third exit");
      break;
    default:
      leave.shortDescription=RouteDescriptionBuilder::tr("Take the %1th exit")
          .arg(roundaboutLeaveDescription->GetExitCount());
  }

  if (nameDescription &&
      nameDescription->HasName()) {

    leave.description=RouteDescriptionBuilder::tr("<strong>Leave roundabout</strong> on %1. exit into street %2")
        .arg(roundaboutLeaveDescription->GetExitCount())
        .arg(FormatName(*nameDescription));
  }else{
    leave.description=RouteDescriptionBuilder::tr("<strong>Leave roundabout</strong> on %1. exit")
        .arg(roundaboutLeaveDescription->GetExitCount());
  }

  routeSteps.push_back(leave);
}

void RouteDescriptionBuilder::Callback::OnMotorwayEnter(const RouteDescription::MotorwayEnterDescriptionRef& motorwayEnterDescription,
                                                        const RouteDescription::CrossingWaysDescriptionRef& crossingWaysDescription)
{
  RouteStep enter = MkStep("enter-motorway");

  enter.shortDescription=RouteDescriptionBuilder::tr("Enter motorway");

  QString crossingWaysString;

  if (crossingWaysDescription) {
    crossingWaysString=CrossingWaysDescriptionToString(*crossingWaysDescription);
  }

  if (motorwayEnterDescription->GetToDescription() &&
      motorwayEnterDescription->GetToDescription()->HasName()) {

    if (!crossingWaysString.isEmpty()){
      enter.description=RouteDescriptionBuilder::tr("At crossing %1<strong>Enter motorway</strong> %2")
          .arg(crossingWaysString)
          .arg(FormatName(*(motorwayEnterDescription->GetToDescription())));
    }else {
      enter.description=RouteDescriptionBuilder::tr("<strong>Enter motorway</strong> %1")
          .arg(FormatName(*(motorwayEnterDescription->GetToDescription())));
    }
  }else{
    if (!crossingWaysString.isEmpty()){
      enter.description=RouteDescriptionBuilder::tr("At crossing %1<strong>Enter motorway</strong>")
          .arg(crossingWaysString);
    }else {
      enter.description=RouteDescriptionBuilder::tr("<strong>Enter motorway</strong>");
    }
  }

  routeSteps.push_back(enter);
}

void RouteDescriptionBuilder::Callback::OnMotorwayChange(const RouteDescription::MotorwayChangeDescriptionRef& motorwayChangeDescription,
                                                         const RouteDescription::MotorwayJunctionDescriptionRef& motorwayJunctionDescription,
                                                         const RouteDescription::DirectionDescriptionRef& directionDescription,
                                                         const RouteDescription::DestinationDescriptionRef& crossingDestinationDescription)
{
  QString stepName="change-motorway";
  if (directionDescription) {
    if (directionDescription->GetCurve()==RouteDescription::DirectionDescription::sharpLeft ||
        directionDescription->GetCurve()==RouteDescription::DirectionDescription::left ||
        directionDescription->GetCurve()==RouteDescription::DirectionDescription::slightlyLeft) {
      stepName="change-motorway-left";
    } else if (directionDescription->GetCurve()==RouteDescription::DirectionDescription::sharpRight ||
               directionDescription->GetCurve()==RouteDescription::DirectionDescription::right ||
               directionDescription->GetCurve()==RouteDescription::DirectionDescription::slightlyRight) {
      stepName="change-motorway-right";
    }
  }
  RouteStep change = MkStep(stepName);

  change.shortDescription=RouteDescriptionBuilder::tr("Change motorway");

  QString exitDescription;
  if (motorwayJunctionDescription &&
      motorwayJunctionDescription->GetJunctionDescription() &&
      motorwayJunctionDescription->GetJunctionDescription()->HasName()) {
    exitDescription = FormatMotorwayJunctionName(*(motorwayJunctionDescription->GetJunctionDescription()));
  }

  if (motorwayChangeDescription->GetFromDescription() &&
      motorwayChangeDescription->GetFromDescription()->HasName() &&
      motorwayChangeDescription->GetToDescription() &&
      motorwayChangeDescription->GetToDescription()->HasName()) {

    if (exitDescription.isEmpty()) {
      change.description = RouteDescriptionBuilder::tr("<strong>Change motorway</strong> from %1 to %2")
          .arg(FormatName(*(motorwayChangeDescription->GetFromDescription())))
          .arg(FormatName(*(motorwayChangeDescription->GetToDescription())));
    } else {
      //: %1 is motorway exit description
      change.description = RouteDescriptionBuilder::tr("%1 <strong>Change motorway</strong> from %2 to %3")
          .arg(exitDescription)
          .arg(FormatName(*(motorwayChangeDescription->GetFromDescription())))
          .arg(FormatName(*(motorwayChangeDescription->GetToDescription())));
    }
  }else{
    if (exitDescription.isEmpty()) {
      change.description = RouteDescriptionBuilder::tr("<strong>Change motorway</strong>");
    } else {
      //: %1 is motorway exit description
      change.description = RouteDescriptionBuilder::tr("%1 <strong>Change motorway</strong>")
          .arg(exitDescription);
    }
  }

  if (crossingDestinationDescription) {
    change.destinations = SplitDestinations(crossingDestinationDescription->GetDescription());
  }

  routeSteps.push_back(change);
}

void RouteDescriptionBuilder::Callback::OnMotorwayLeave(const RouteDescription::MotorwayLeaveDescriptionRef& motorwayLeaveDescription,
                                                        const RouteDescription::MotorwayJunctionDescriptionRef& motorwayJunctionDescription,
                                                        const RouteDescription::DirectionDescriptionRef& directionDescription,
                                                        const RouteDescription::NameDescriptionRef& nameDescription,
                                                        const RouteDescription::DestinationDescriptionRef& destinationDescription)
{
  QString stepName="leave-motorway";
  if (directionDescription) {
    if (directionDescription->GetCurve()==RouteDescription::DirectionDescription::sharpLeft ||
        directionDescription->GetCurve()==RouteDescription::DirectionDescription::left ||
        directionDescription->GetCurve()==RouteDescription::DirectionDescription::slightlyLeft) {
      stepName="leave-motorway-left";
    } else if (directionDescription->GetCurve()==RouteDescription::DirectionDescription::sharpRight ||
               directionDescription->GetCurve()==RouteDescription::DirectionDescription::right ||
               directionDescription->GetCurve()==RouteDescription::DirectionDescription::slightlyRight) {
      stepName="leave-motorway-right";
    }
  }
  RouteStep leave = MkStep(stepName);

  leave.shortDescription=RouteDescriptionBuilder::tr("Leave motorway");

  QString exitDescription;
  if (motorwayJunctionDescription &&
      motorwayJunctionDescription->GetJunctionDescription() &&
      motorwayJunctionDescription->GetJunctionDescription()->HasName()) {
    exitDescription = FormatMotorwayJunctionName(*(motorwayJunctionDescription->GetJunctionDescription()));
  }

  if (motorwayLeaveDescription->GetFromDescription() &&
      motorwayLeaveDescription->GetFromDescription()->HasName()) {

    if (nameDescription &&
        nameDescription->HasName()) {

      if (exitDescription.isEmpty()) {
        leave.description = RouteDescriptionBuilder::tr("<strong>Leave motorway</strong> %1 into %2")
            .arg(FormatName(*(motorwayLeaveDescription->GetFromDescription())))
            .arg(FormatName(*nameDescription));
      } else {
        //: %1 is motorway exit description
        leave.description = RouteDescriptionBuilder::tr("%1 <strong>Leave motorway</strong> %2 into %3")
            .arg(exitDescription)
            .arg(FormatName(*(motorwayLeaveDescription->GetFromDescription())))
            .arg(FormatName(*nameDescription));
      }
    }else{
      if (exitDescription.isEmpty()) {
        leave.description = RouteDescriptionBuilder::tr("<strong>Leave motorway</strong> %1")
            .arg(FormatName(*(motorwayLeaveDescription->GetFromDescription())));
      } else {
        //: %1 is motorway exit description
        leave.description = RouteDescriptionBuilder::tr("%1 <strong>Leave motorway</strong> %2")
            .arg(exitDescription)
            .arg(FormatName(*(motorwayLeaveDescription->GetFromDescription())));
      }
    }
  }else{
    if (exitDescription.isEmpty()) {
      leave.description = RouteDescriptionBuilder::tr("<strong>Leave motorway</strong>");
    } else {
      //: %1 is motorway exit description
      leave.description=RouteDescriptionBuilder::tr("%1 <strong>Leave motorway</strong>")
          .arg(exitDescription);
    }
  }

  if (destinationDescription) {
    leave.destinations = SplitDestinations(destinationDescription->GetDescription());
  }

  routeSteps.push_back(leave);
}

void RouteDescriptionBuilder::Callback::PushStreetName(const RouteDescription::NameDescriptionRef &nameDescription)
{
  if (!nameDescription){
    return;
  }
  auto name=FormatName(*nameDescription);
  if (streetNames.empty() || streetNames.back() != name){
    streetNames << name;
  }
}

void RouteDescriptionBuilder::Callback::OnPathNameChange(const RouteDescription::NameChangedDescriptionRef& nameChangedDescription)
{
  assert(nameChangedDescription);
  if (skipInformative){
    PushStreetName(nameChangedDescription->GetOriginDescription());
    PushStreetName(nameChangedDescription->GetTargetDescription());
    return;
  }

  RouteStep changed = MkStep("name-change");

  changed.shortDescription=RouteDescriptionBuilder::tr("Way changes name");

  if (nameChangedDescription->GetOriginDescription()) {
    changed.description=RouteDescriptionBuilder::tr("<strong>Way changes name</strong> from %1 to %2")
        .arg(FormatName(*(nameChangedDescription->GetOriginDescription())))
        .arg(FormatName(*(nameChangedDescription->GetTargetDescription())));
  } else {
    changed.description=RouteDescriptionBuilder::tr("<strong>Way changes name</strong> to %1")
      .arg(FormatName(*(nameChangedDescription->GetTargetDescription())));
  }

  routeSteps.push_back(changed);
}

void RouteDescriptionBuilder::Callback::BeforeNode(const RouteDescription::Node& node)
{
  distance=node.GetDistance();
  coord=node.GetLocation();
  timestamp=node.GetTime();
}

RouteStep RouteDescriptionBuilder::Callback::MkStep(const QString &name)
{
  RouteStep step(name,
                 coord,
                 distance,
                 distance-distancePrevious,
                 timestamp,
                 timestamp-timestampPrevious,
                 streetNames);

  streetNames.clear();
  distancePrevious = distance;
  timestampPrevious = timestamp;
  return step;
}

bool RouteDescriptionBuilder::Callback::Continue() const
{
  return stopAfter < Distance::Zero() ||
         routeSteps.empty() ||
         routeSteps.constLast().distance <= stopAfter;
}

void RouteDescriptionBuilder::GenerateRouteSteps(const RouteDescription &routeDescription,
                                                 QList<RouteStep> &routeSteps) const
{
  RouteDescriptionPostprocessor postprocessor;
  Callback callback(routeSteps);
  postprocessor.GenerateDescription(routeDescription, callback);
}

std::list<RouteStep> RouteDescriptionBuilder::GenerateRouteInstructions(const RouteDescription::NodeIterator &first,
                                                                        const RouteDescription::NodeIterator &last) const
{
  QList<RouteStep> routeSteps;
  RouteDescriptionPostprocessor postprocessor;
  Callback callback(routeSteps);
  postprocessor.GenerateDescription(first, last, callback);

  std::list<RouteStep> result;
  for (auto &step:routeSteps){
    result.push_back(step);
  }
  return result;
}

RouteStep RouteDescriptionBuilder::GenerateNextRouteInstruction(const RouteDescription::NodeIterator &previous,
                                                                const RouteDescription::NodeIterator &last,
                                                                const GeoCoord &coord) const
{
  RouteStep result;
  if(previous==last){
    log.Warn() << "Can't generate route instruction without nodes";
    return result;
  }

  QList<RouteStep> routeSteps;
  RouteDescriptionPostprocessor postprocessor;
  Callback callback(routeSteps,
                    /*stop after*/ previous->GetDistance(),
                    /*skip informative*/ true);
  postprocessor.GenerateDescription(previous, last, callback);

  if (routeSteps.empty()){
    log.Warn() << "No route instruction generated";
    return result;
  }
  result=routeSteps.constLast();

  result.distanceTo = (result.distance - previous->GetDistance()) - GetEllipsoidalDistance(coord, previous->GetLocation());

  return result;
}

}
