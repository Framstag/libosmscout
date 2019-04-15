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
#include <osmscout/util/Logger.h>

#include <set>
#include <sstream>
#include <iomanip>


#include <QMetaType>
#include <QVariant>


namespace osmscout {

RouteDescriptionBuilder::RouteDescriptionBuilder::Callback::Callback(QList<RouteStep> &routeSteps,
                                                                     const Distance &stopAfter,
                                                                     bool skipInformative):
  routeSteps(routeSteps), stopAfter(stopAfter), skipInformative(skipInformative)
{
}

RouteDescriptionBuilder::RouteDescriptionBuilder::Callback::~Callback()
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
    return osmscout::RouteDescriptionBuilder::tr("Turn");
  }
  switch (directionDescription->GetCurve()) {
    case osmscout::RouteDescription::DirectionDescription::sharpLeft:
      return osmscout::RouteDescriptionBuilder::tr("Turn sharp left");
    case osmscout::RouteDescription::DirectionDescription::left:
      return osmscout::RouteDescriptionBuilder::tr("Turn left");
    case osmscout::RouteDescription::DirectionDescription::slightlyLeft:
      return osmscout::RouteDescriptionBuilder::tr("Turn slightly left");
    case osmscout::RouteDescription::DirectionDescription::straightOn:
      return osmscout::RouteDescriptionBuilder::tr("Straight on");
    case osmscout::RouteDescription::DirectionDescription::slightlyRight:
      return osmscout::RouteDescriptionBuilder::tr("Turn slightly right");
    case osmscout::RouteDescription::DirectionDescription::right:
      return osmscout::RouteDescriptionBuilder::tr("Turn right");
    case osmscout::RouteDescription::DirectionDescription::sharpRight:
      return osmscout::RouteDescriptionBuilder::tr("Turn sharp right");
  }

  assert(false);

  return "???";
}

static QString FullTurnCommand(const osmscout::RouteDescription::DirectionDescriptionRef& directionDescription)
{
  if (!directionDescription){
    return osmscout::RouteDescriptionBuilder::tr("At crossing %1<strong>Turn</strong> into %2");
  }
  switch (directionDescription->GetCurve()) {
    case osmscout::RouteDescription::DirectionDescription::sharpLeft:
      return osmscout::RouteDescriptionBuilder::tr("At crossing %1<strong>Turn sharp left</strong> into %2");
    case osmscout::RouteDescription::DirectionDescription::left:
      return osmscout::RouteDescriptionBuilder::tr("At crossing %1<strong>Turn left</strong> into %2");
    case osmscout::RouteDescription::DirectionDescription::slightlyLeft:
      return osmscout::RouteDescriptionBuilder::tr("At crossing %1<strong>Turn slightly left</strong> into %2");
    case osmscout::RouteDescription::DirectionDescription::straightOn:
      return osmscout::RouteDescriptionBuilder::tr("At crossing %1<strong>Straight on</strong> into %2");
    case osmscout::RouteDescription::DirectionDescription::slightlyRight:
      return osmscout::RouteDescriptionBuilder::tr("At crossing %1<strong>Turn slightly right</strong> into %2");
    case osmscout::RouteDescription::DirectionDescription::right:
      return osmscout::RouteDescriptionBuilder::tr("At crossing %1<strong>Turn right</strong> into %2");
    case osmscout::RouteDescription::DirectionDescription::sharpRight:
      return osmscout::RouteDescriptionBuilder::tr("At crossing %1<strong>Turn sharp right</strong> into %2");
  }

  assert(false);

  return "???";
}

static QString TurnCommandWithList(const osmscout::RouteDescription::DirectionDescriptionRef& directionDescription)
{
  if (!directionDescription){
    return osmscout::RouteDescriptionBuilder::tr("At crossing %1<strong>Turn</strong>");
  }
  switch (directionDescription->GetCurve()) {
    case osmscout::RouteDescription::DirectionDescription::sharpLeft:
      return osmscout::RouteDescriptionBuilder::tr("At crossing %1<strong>Turn sharp left</strong>");
    case osmscout::RouteDescription::DirectionDescription::left:
      return osmscout::RouteDescriptionBuilder::tr("At crossing %1<strong>Turn left</strong>");
    case osmscout::RouteDescription::DirectionDescription::slightlyLeft:
      return osmscout::RouteDescriptionBuilder::tr("At crossing %1<strong>Turn slightly left</strong>");
    case osmscout::RouteDescription::DirectionDescription::straightOn:
      return osmscout::RouteDescriptionBuilder::tr("At crossing %1<strong>Straight on</strong>");
    case osmscout::RouteDescription::DirectionDescription::slightlyRight:
      return osmscout::RouteDescriptionBuilder::tr("At crossing %1<strong>Turn slightly right</strong>");
    case osmscout::RouteDescription::DirectionDescription::right:
      return osmscout::RouteDescriptionBuilder::tr("At crossing %1<strong>Turn right</strong>");
    case osmscout::RouteDescription::DirectionDescription::sharpRight:
      return osmscout::RouteDescriptionBuilder::tr("At crossing %1<strong>Turn sharp right</strong>");
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
    return osmscout::RouteDescriptionBuilder::tr("unnamed road");
  }
  if (name.empty()){
    return osmscout::RouteDescriptionBuilder::tr("(%1)").arg(QString::fromStdString(ref));
  }
  if (ref.empty()){
    return osmscout::RouteDescriptionBuilder::tr("\"%1\"").arg(QString::fromStdString(name));
  }
  return osmscout::RouteDescriptionBuilder::tr("\"%1\" (%2)").arg(QString::fromStdString(name)).arg(QString::fromStdString(ref));
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


void RouteDescriptionBuilder::Callback::OnStart(const RouteDescription::StartDescriptionRef& startDescription,
                                                const RouteDescription::TypeNameDescriptionRef& /*typeNameDescription*/,
                                                const RouteDescription::NameDescriptionRef& nameDescription)
{
  QString startDesc;
  QString driveAlongDesc;
  if (startDescription && !startDescription->GetDescription().empty()) {
    startDesc = osmscout::RouteDescriptionBuilder::tr("\"%1\"")
        .arg(QString::fromStdString(startDescription->GetDescription()));
  }
  if (nameDescription && nameDescription->HasName()) {
    driveAlongDesc = FormatName(*nameDescription);
  }

  if (!startDesc.isEmpty()){
    RouteStep startAt = MkStep("start");
    startAt.description=osmscout::RouteDescriptionBuilder::tr("<strong>Start</strong> at %1").arg(startDesc);
    startAt.shortDescription=osmscout::RouteDescriptionBuilder::tr("Start");
    routeSteps.push_back(startAt);

    if (!driveAlongDesc.isEmpty()) {
      RouteStep driveAlong = MkStep("drive-along");
      driveAlong.description=osmscout::RouteDescriptionBuilder::tr("<strong>Continue</strong> along %1").arg(driveAlongDesc);
      driveAlong.shortDescription=osmscout::RouteDescriptionBuilder::tr("Continue");
      routeSteps.push_back(driveAlong);
    }
  } else if (!driveAlongDesc.isEmpty()) {
    RouteStep startAt = MkStep("start");
    startAt.description=osmscout::RouteDescriptionBuilder::tr("<strong>Start</strong> along %1").arg(driveAlongDesc);
    startAt.shortDescription=osmscout::RouteDescriptionBuilder::tr("Start");
    routeSteps.push_back(startAt);
  } else {
    RouteStep start = MkStep("start");
    start.description=osmscout::RouteDescriptionBuilder::tr("<strong>Start</strong>");
    start.shortDescription=osmscout::RouteDescriptionBuilder::tr("Start");
    routeSteps.push_back(start);
  }
}

void RouteDescriptionBuilder::Callback::OnTargetReached(const osmscout::RouteDescription::TargetDescriptionRef& targetDescription)
{
  RouteStep targetReached = MkStep("target");

  QString targetDesc;
  if (targetDescription && !targetDescription->GetDescription().empty()){
    targetDesc = osmscout::RouteDescriptionBuilder::tr("\"%1\"")
        .arg(QString::fromStdString(targetDescription->GetDescription()));
  }
  if (!targetDesc.isEmpty()){
    targetReached.description=osmscout::RouteDescriptionBuilder::tr("<strong>Target reached</strong> at %1").arg(targetDesc);
  }else{
    targetReached.description=osmscout::RouteDescriptionBuilder::tr("<strong>Target reached</strong>");
  }
  targetReached.shortDescription=osmscout::RouteDescriptionBuilder::tr("Target reached");
  routeSteps.push_back(targetReached);
}

void RouteDescriptionBuilder::Callback::OnTurn(const osmscout::RouteDescription::TurnDescriptionRef& /*turnDescription*/,
                                               const osmscout::RouteDescription::CrossingWaysDescriptionRef& crossingWaysDescription,
                                               const osmscout::RouteDescription::DirectionDescriptionRef& directionDescription,
                                               const RouteDescription::TypeNameDescriptionRef& /*typeNameDescription*/,
                                               const osmscout::RouteDescription::NameDescriptionRef& nameDescription)
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

void RouteDescriptionBuilder::Callback::OnRoundaboutEnter(const osmscout::RouteDescription::RoundaboutEnterDescriptionRef& /*roundaboutEnterDescription*/,
                                                          const osmscout::RouteDescription::CrossingWaysDescriptionRef& crossingWaysDescription)
{
  if (skipInformative){
    if (crossingWaysDescription) {
      PushStreetName(crossingWaysDescription->GetOriginDesccription());
      PushStreetName(crossingWaysDescription->GetTargetDesccription());
    }
    return;
  }

  RouteStep enter = MkStep("enter-roundabout");
  QString crossingWaysString;

  if (crossingWaysDescription) {
    crossingWaysString=CrossingWaysDescriptionToString(*crossingWaysDescription);
  }

  if (!crossingWaysString.isEmpty()) {
    enter.description=osmscout::RouteDescriptionBuilder::tr("At crossing %1<strong>Enter roundabout</strong>")
        .arg(crossingWaysString);
  }else {
    enter.description=osmscout::RouteDescriptionBuilder::tr("<strong>Enter roundabout</strong>");
  }
  enter.shortDescription=osmscout::RouteDescriptionBuilder::tr("Enter roundabout");
  routeSteps.push_back(enter);
}

void RouteDescriptionBuilder::Callback::OnRoundaboutLeave(const osmscout::RouteDescription::RoundaboutLeaveDescriptionRef& roundaboutLeaveDescription,
                                                          const osmscout::RouteDescription::NameDescriptionRef& nameDescription)
{
  RouteStep leave = MkStep("leave-roundabout");
  leave.roundaboutExit = roundaboutLeaveDescription->GetExitCount();

  switch (roundaboutLeaveDescription->GetExitCount()){
    case 1:
      leave.shortDescription=osmscout::RouteDescriptionBuilder::tr("Take the first exit");
      break;
    case 2:
      leave.shortDescription=osmscout::RouteDescriptionBuilder::tr("Take the second exit");
      break;
    case 3:
      leave.shortDescription=osmscout::RouteDescriptionBuilder::tr("Take the third exit");
      break;
    default:
      leave.shortDescription=osmscout::RouteDescriptionBuilder::tr("Take the %1th exit")
          .arg(roundaboutLeaveDescription->GetExitCount());
  }

  if (nameDescription &&
      nameDescription->HasName()) {

    leave.description=osmscout::RouteDescriptionBuilder::tr("<strong>Leave roundabout</strong> on %1. exit into street %2")
        .arg(roundaboutLeaveDescription->GetExitCount())
        .arg(FormatName(*nameDescription));
  }else{
    leave.description=osmscout::RouteDescriptionBuilder::tr("<strong>Leave roundabout</strong> on %1. exit")
        .arg(roundaboutLeaveDescription->GetExitCount());
  }

  routeSteps.push_back(leave);
}

void RouteDescriptionBuilder::Callback::OnMotorwayEnter(const osmscout::RouteDescription::MotorwayEnterDescriptionRef& motorwayEnterDescription,
                                                        const osmscout::RouteDescription::CrossingWaysDescriptionRef& crossingWaysDescription)
{
  RouteStep enter = MkStep("enter-motorway");

  enter.shortDescription=osmscout::RouteDescriptionBuilder::tr("Enter motorway");

  QString crossingWaysString;

  if (crossingWaysDescription) {
    crossingWaysString=CrossingWaysDescriptionToString(*crossingWaysDescription);
  }

  if (motorwayEnterDescription->GetToDescription() &&
      motorwayEnterDescription->GetToDescription()->HasName()) {

    if (!crossingWaysString.isEmpty()){
      enter.description=osmscout::RouteDescriptionBuilder::tr("At crossing %1<strong>Enter motorway</strong> %2")
          .arg(crossingWaysString)
          .arg(FormatName(*(motorwayEnterDescription->GetToDescription())));
    }else {
      enter.description=osmscout::RouteDescriptionBuilder::tr("<strong>Enter motorway</strong> %1")
          .arg(FormatName(*(motorwayEnterDescription->GetToDescription())));
    }
  }else{
    if (!crossingWaysString.isEmpty()){
      enter.description=osmscout::RouteDescriptionBuilder::tr("At crossing %1<strong>Enter motorway</strong>")
          .arg(crossingWaysString);
    }else {
      enter.description=osmscout::RouteDescriptionBuilder::tr("<strong>Enter motorway</strong>");
    }
  }

  routeSteps.push_back(enter);
}

void RouteDescriptionBuilder::Callback::OnMotorwayChange(const osmscout::RouteDescription::MotorwayChangeDescriptionRef& motorwayChangeDescription,
                                                         const RouteDescription::MotorwayJunctionDescriptionRef& /*motorwayJunctionDescription*/,
                                                         const RouteDescription::DestinationDescriptionRef& /*crossingDestinationDescription*/)
{
  RouteStep change = MkStep("change-motorway");

  change.shortDescription=osmscout::RouteDescriptionBuilder::tr("Change motorway");

  if (motorwayChangeDescription->GetFromDescription() &&
      motorwayChangeDescription->GetFromDescription()->HasName() &&
      motorwayChangeDescription->GetToDescription() &&
      motorwayChangeDescription->GetToDescription()->HasName()) {

    change.description=osmscout::RouteDescriptionBuilder::tr("<strong>Change motorway</strong> from %1 to %2")
        .arg(FormatName(*(motorwayChangeDescription->GetFromDescription())))
        .arg(FormatName(*(motorwayChangeDescription->GetToDescription())));
  }else{
    change.description=osmscout::RouteDescriptionBuilder::tr("<strong>Change motorway</strong>");
  }

  routeSteps.push_back(change);
}

void RouteDescriptionBuilder::Callback::OnMotorwayLeave(const RouteDescription::MotorwayLeaveDescriptionRef& motorwayLeaveDescription,
                                                        const RouteDescription::MotorwayJunctionDescriptionRef& /*motorwayJunctionDescription*/,
                                                        const RouteDescription::DirectionDescriptionRef& /*directionDescription*/,
                                                        const RouteDescription::NameDescriptionRef& nameDescription)
{
  RouteStep leave = MkStep("leave-motorway");

  leave.shortDescription=osmscout::RouteDescriptionBuilder::tr("Leave motorway");

  // TODO: should we add leave direction to phrase? directionDescription->GetCurve()
  if (motorwayLeaveDescription->GetFromDescription() &&
      motorwayLeaveDescription->GetFromDescription()->HasName()) {

    if (nameDescription &&
        nameDescription->HasName()) {

      leave.description = osmscout::RouteDescriptionBuilder::tr("<strong>Leave motorway</strong> %1 into %2")
          .arg(FormatName(*(motorwayLeaveDescription->GetFromDescription())))
          .arg(FormatName(*nameDescription));
    }else{
      leave.description = osmscout::RouteDescriptionBuilder::tr("<strong>Leave motorway</strong> %1")
          .arg(FormatName(*(motorwayLeaveDescription->GetFromDescription())));
    }
  }else{
    leave.description=osmscout::RouteDescriptionBuilder::tr("<strong>Leave motorway</strong>");
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

void RouteDescriptionBuilder::Callback::OnPathNameChange(const osmscout::RouteDescription::NameChangedDescriptionRef& nameChangedDescription)
{
  assert(nameChangedDescription);
  if (skipInformative){
    PushStreetName(nameChangedDescription->GetOriginDescription());
    PushStreetName(nameChangedDescription->GetTargetDescription());
    return;
  }

  RouteStep changed = MkStep("name-change");

  changed.shortDescription=osmscout::RouteDescriptionBuilder::tr("Way changes name");

  if (nameChangedDescription->GetOriginDescription()) {
    changed.description=osmscout::RouteDescriptionBuilder::tr("<strong>Way changes name</strong> from %1 to %2")
        .arg(FormatName(*(nameChangedDescription->GetOriginDescription())))
        .arg(FormatName(*(nameChangedDescription->GetTargetDescription())));
  } else {
    changed.description=osmscout::RouteDescriptionBuilder::tr("<strong>Way changes name</strong> to %1")
      .arg(FormatName(*(nameChangedDescription->GetTargetDescription())));
  }

  routeSteps.push_back(changed);
}

void RouteDescriptionBuilder::Callback::BeforeNode(const RouteDescription::Node& node)
{
  distance=node.GetDistance();
  coord=node.GetLocation();
  time=node.GetTime();
}

RouteStep RouteDescriptionBuilder::Callback::MkStep(const QString &name)
{
  RouteStep step(name,
                 coord,
                 distance,
                 distance-distancePrevious,
                 time,
                 time-timePrevious,
                 streetNames);

  streetNames.clear();
  distancePrevious = distance;
  timePrevious = time;
  return step;
}

bool RouteDescriptionBuilder::Callback::Continue() const
{
  return stopAfter < Distance::Zero() ||
         routeSteps.empty() ||
         routeSteps.constLast().distance <= stopAfter;
}

RouteDescriptionBuilder::RouteDescriptionBuilder()
{}

RouteDescriptionBuilder::~RouteDescriptionBuilder()
{}

void RouteDescriptionBuilder::GenerateRouteSteps(const osmscout::RouteDescription &routeDescription,
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
