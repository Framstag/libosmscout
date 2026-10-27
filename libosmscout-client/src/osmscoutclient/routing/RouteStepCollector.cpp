/*
  This source is part of the libosmscout library
  Copyright (C) 2026  Tim Teulings

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

#include <osmscoutclient/routing/RouteStepCollector.h>

#include <osmscout/routing/RouteDescription.h>

#include <map>
#include <optional>
#include <string>
#include <utility>

namespace osmscout::client {

  namespace {

    /**
     * Fill the name of a name description, when the node carries one.
     */
    std::optional<RouteStep::Name> ToName(const RouteDescription::NameDescriptionRef& description)
    {
      if (!description) {
        return std::nullopt;
      }

      return RouteStep::Name{description->GetName(),
                             description->GetRef()};
    }

    /**
     * Whether a description of a node is one the model represents.
     */
    bool IsHandledDescription(const RouteDescription::Description& description)
    {
      return dynamic_cast<const RouteDescription::StartDescription*>(&description)!=nullptr ||
             dynamic_cast<const RouteDescription::TargetDescription*>(&description)!=nullptr ||
             dynamic_cast<const RouteDescription::NameDescription*>(&description)!=nullptr ||
             dynamic_cast<const RouteDescription::NameChangedDescription*>(&description)!=nullptr ||
             dynamic_cast<const RouteDescription::CrossingWaysDescription*>(&description)!=nullptr ||
             dynamic_cast<const RouteDescription::DirectionDescription*>(&description)!=nullptr ||
             dynamic_cast<const RouteDescription::TurnDescription*>(&description)!=nullptr ||
             dynamic_cast<const RouteDescription::RoundaboutEnterDescription*>(&description)!=nullptr ||
             dynamic_cast<const RouteDescription::RoundaboutLeaveDescription*>(&description)!=nullptr ||
             dynamic_cast<const RouteDescription::MotorwayEnterDescription*>(&description)!=nullptr ||
             dynamic_cast<const RouteDescription::MotorwayChangeDescription*>(&description)!=nullptr ||
             dynamic_cast<const RouteDescription::MotorwayLeaveDescription*>(&description)!=nullptr ||
             dynamic_cast<const RouteDescription::MotorwayJunctionDescription*>(&description)!=nullptr ||
             dynamic_cast<const RouteDescription::DestinationDescription*>(&description)!=nullptr ||
             dynamic_cast<const RouteDescription::MaxSpeedDescription*>(&description)!=nullptr ||
             dynamic_cast<const RouteDescription::TypeNameDescription*>(&description)!=nullptr ||
             dynamic_cast<const RouteDescription::POIAtRouteDescription*>(&description)!=nullptr ||
             dynamic_cast<const RouteDescription::LaneDescription*>(&description)!=nullptr ||
             dynamic_cast<const RouteDescription::SuggestedLaneDescription*>(&description)!=nullptr ||
             dynamic_cast<const RouteDescription::ViaDescription*>(&description)!=nullptr;
    }

    /**
     * The kind of a node, derived from the descriptions it carries. Only used when no hook
     * reported a manoeuvre for the node; the hooks are the authority on what a manoeuvre is.
     */
    RouteStep::Kind DeriveKind(const RouteDescription::Node& node)
    {
      if (node.HasDescription<RouteDescription::StartDescription>()) {
        return RouteStep::Kind::start;
      }

      if (node.HasDescription<RouteDescription::TargetDescription>()) {
        return RouteStep::Kind::target;
      }

      if (node.HasDescription<RouteDescription::ViaDescription>()) {
        return RouteStep::Kind::informative;
      }

      if (node.HasDescription<RouteDescription::RoundaboutEnterDescription>()) {
        return RouteStep::Kind::roundaboutEnter;
      }

      if (node.HasDescription<RouteDescription::RoundaboutLeaveDescription>()) {
        return RouteStep::Kind::roundaboutLeave;
      }

      if (node.HasDescription<RouteDescription::TurnDescription>()) {
        return RouteStep::Kind::turn;
      }

      if (node.HasDescription<RouteDescription::MotorwayEnterDescription>()) {
        return RouteStep::Kind::motorwayEnter;
      }

      if (node.HasDescription<RouteDescription::MotorwayChangeDescription>()) {
        return RouteStep::Kind::motorwayChange;
      }

      if (node.HasDescription<RouteDescription::MotorwayLeaveDescription>()) {
        return RouteStep::Kind::motorwayLeave;
      }

      if (node.HasDescription<RouteDescription::NameChangedDescription>()) {
        return RouteStep::Kind::nameChange;
      }

      return RouteStep::Kind::informative;
    }
  }

  void RouteStepCollector::Reset()
  {
    result=RouteSteps();
    unhandledDescriptions.clear();
    pendingKindSet=false;
    pendingKind=RouteStep::Kind::unknown;
    firstStep=true;
    previousDistance=Distance::Zero();
    previousTime=Duration::zero();
    stopAfter=Distance::Lowest();
  }

  void RouteStepCollector::NoteKind(RouteStep::Kind kind)
  {
    if (!pendingKindSet) {
      pendingKindSet=true;
      pendingKind=kind;
    }
  }

  RouteSteps RouteStepCollector::Collect(const RouteDescription& description)
  {
    RouteSteps steps=Collect(description.Nodes().cbegin(),
                             description.Nodes().cend());

    steps.databaseMapping=description.GetDatabaseMapping();

    return steps;
  }

  RouteSteps RouteStepCollector::Collect(const RouteDescription::NodeIterator& first,
                                         const RouteDescription::NodeIterator& last)
  {
    return Collect(first, last, Distance::Lowest());
  }

  RouteSteps RouteStepCollector::Collect(const RouteDescription::NodeIterator& first,
                                         const RouteDescription::NodeIterator& last,
                                         const Distance& stopAfter)
  {
    Reset();

    this->stopAfter=stopAfter;

    RouteDescriptionPostprocessor postprocessor;

    postprocessor.GenerateDescription(first, last, *this);

    return std::move(result);
  }

  const std::set<std::string>& RouteStepCollector::HandledDescriptionKeys()
  {
    static const std::set<std::string> keys{
      std::string(RouteDescription::StartDescription::Key),
      std::string(RouteDescription::TargetDescription::Key),
      std::string(RouteDescription::NameDescription::Key),
      std::string(RouteDescription::NameChangedDescription::Key),
      std::string(RouteDescription::CrossingWaysDescription::Key),
      std::string(RouteDescription::DirectionDescription::Key),
      std::string(RouteDescription::TurnDescription::Key),
      std::string(RouteDescription::RoundaboutEnterDescription::Key),
      std::string(RouteDescription::RoundaboutLeaveDescription::Key),
      std::string(RouteDescription::MotorwayEnterDescription::Key),
      std::string(RouteDescription::MotorwayChangeDescription::Key),
      std::string(RouteDescription::MotorwayLeaveDescription::Key),
      std::string(RouteDescription::MotorwayJunctionDescription::Key),
      std::string(RouteDescription::DestinationDescription::Key),
      std::string(RouteDescription::MaxSpeedDescription::Key),
      std::string(RouteDescription::TypeNameDescription::Key),
      std::string(RouteDescription::POIAtRouteDescription::Key),
      std::string(RouteDescription::LaneDescription::Key),
      std::string(RouteDescription::SuggestedLaneDescription::Key),
      std::string(RouteDescription::ViaDescription::Key)
    };

    return keys;
  }

  std::optional<RouteStep> RouteStepCollector::BuildNodeStep(const RouteDescription::Node& node)
  {
    if (node.GetDescriptions().empty()) {
      return std::nullopt;
    }

    RouteStep step;

    step.kind=DeriveKind(node);
    step.databaseId=node.GetDatabaseId();
    step.currentNodeIndex=node.GetCurrentNodeIndex();
    step.pathObject=node.GetPathObject();
    step.targetNodeIndex=node.GetTargetNodeIndex();
    step.coord=node.GetLocation();
    step.distanceFromStart=node.GetDistance();
    step.timeFromStart=node.GetTime();
    step.objects=node.GetObjects();

    if (auto description=node.GetDescription<RouteDescription::NameDescription>()) {
      step.name=ToName(description);
    }

    if (auto description=node.GetDescription<RouteDescription::NameChangedDescription>()) {
      RouteStep::NameChanged nameChanged;

      if (auto from=ToName(description->GetOriginDescription())) {
        nameChanged.from=*from;
      }

      if (auto to=ToName(description->GetTargetDescription())) {
        nameChanged.to=*to;
      }

      step.nameChanged=nameChanged;
    }

    if (auto description=node.GetDescription<RouteDescription::TypeNameDescription>()) {
      step.typeName=description->GetName();
    }

    if (auto description=node.GetDescription<RouteDescription::CrossingWaysDescription>()) {
      RouteStep::CrossingWays crossingWays;

      crossingWays.exitCount=description->GetExitCount();
      crossingWays.origin=ToName(description->GetOriginDesccription());
      crossingWays.target=ToName(description->GetTargetDesccription());

      for (const auto& name : description->GetDescriptions()) {
        if (auto other=ToName(name)) {
          crossingWays.others.push_back(*other);
        }
      }

      step.crossingWays=crossingWays;
    }

    if (auto description=node.GetDescription<RouteDescription::DirectionDescription>()) {
      step.direction=RouteStep::Direction{description->GetTurnAngle(),
                                          description->GetCurveAngle(),
                                          description->GetTurn(),
                                          description->GetCurve()};
    }

    if (auto description=node.GetDescription<RouteDescription::TurnDescription>()) {
      step.turn=description->GetDirection();
    }

    if (auto description=node.GetDescription<RouteDescription::RoundaboutEnterDescription>()) {
      step.roundaboutEnter=RouteStep::RoundaboutEnter{description->IsClockwise()};
    }

    if (auto description=node.GetDescription<RouteDescription::RoundaboutLeaveDescription>()) {
      step.roundaboutLeave=RouteStep::RoundaboutLeave{description->GetExitCount(),
                                                      description->IsClockwise()};
    }

    if (auto description=node.GetDescription<RouteDescription::MotorwayEnterDescription>()) {
      RouteStep::MotorwayEnter motorwayEnter;

      if (auto to=ToName(description->GetToDescription())) {
        motorwayEnter.to=*to;
      }

      step.motorwayEnter=motorwayEnter;
    }

    if (auto description=node.GetDescription<RouteDescription::MotorwayChangeDescription>()) {
      RouteStep::MotorwayChange motorwayChange;

      if (auto from=ToName(description->GetFromDescription())) {
        motorwayChange.from=*from;
      }

      if (auto to=ToName(description->GetToDescription())) {
        motorwayChange.to=*to;
      }

      motorwayChange.move=description->GetDirection();

      step.motorwayChange=motorwayChange;
    }

    if (auto description=node.GetDescription<RouteDescription::MotorwayLeaveDescription>()) {
      RouteStep::MotorwayLeave motorwayLeave;

      if (auto from=ToName(description->GetFromDescription())) {
        motorwayLeave.from=*from;
      }

      motorwayLeave.move=description->GetDirection();

      step.motorwayLeave=motorwayLeave;
    }

    if (auto description=node.GetDescription<RouteDescription::MotorwayJunctionDescription>()) {
      step.junction=ToName(description->GetJunctionDescription());
    }

    if (auto description=node.GetDescription<RouteDescription::DestinationDescription>()) {
      step.destination=description->GetDescription();
    }

    if (auto description=node.GetDescription<RouteDescription::MaxSpeedDescription>()) {
      step.maxSpeed=description->GetMaxSpeed();
    }

    if (auto description=node.GetDescription<RouteDescription::LaneDescription>()) {
      step.lanes=RouteStep::Lane{description->IsOneway(),
                                 description->GetLaneCount(),
                                 description->GetLaneTurns()};
    }

    if (auto description=node.GetDescription<RouteDescription::SuggestedLaneDescription>()) {
      step.suggestedLanes=RouteStep::SuggestedLane{description->GetFrom(),
                                                   description->GetTo(),
                                                   description->GetTurn()};
    }

    if (auto description=node.GetDescription<RouteDescription::POIAtRouteDescription>()) {
      RouteStep::Poi poi;

      poi.databaseId=description->GetDatabaseId();
      poi.object=description->GetObject();
      poi.name=ToName(description->GetName());
      poi.distance=description->GetDistance();

      step.poi=poi;
    }

    if (auto description=node.GetDescription<RouteDescription::ViaDescription>()) {
      step.via=RouteStep::Via{description->GetSectionNumber(),
                              description->GetNodeCount()};
    }

    if (auto description=node.GetDescription<RouteDescription::StartDescription>()) {
      step.start=description->GetDescription();
    }

    if (auto description=node.GetDescription<RouteDescription::TargetDescription>()) {
      step.target=description->GetDescription();
    }

    return step;
  }

  void RouteStepCollector::BeforeRoute()
  {
    // no code
  }

  void RouteStepCollector::OnStart(const RouteDescription::StartDescriptionRef& /*startDescription*/,
                                   const RouteDescription::TypeNameDescriptionRef& /*typeNameDescription*/,
                                   const RouteDescription::NameDescriptionRef& /*nameDescription*/)
  {
    NoteKind(RouteStep::Kind::start);
  }

  void RouteStepCollector::OnTargetReached(const RouteDescription::TargetDescriptionRef& /*targetDescription*/)
  {
    NoteKind(RouteStep::Kind::target);
  }

  void RouteStepCollector::OnTurn(const RouteDescription::TurnDescriptionRef& /*turnDescription*/,
                                  const RouteDescription::CrossingWaysDescriptionRef& /*crossingWaysDescription*/,
                                  const RouteDescription::DirectionDescriptionRef& /*directionDescription*/,
                                  const RouteDescription::TypeNameDescriptionRef& /*typeNameDescription*/,
                                  const RouteDescription::NameDescriptionRef& /*nameDescription*/)
  {
    NoteKind(RouteStep::Kind::turn);
  }

  void RouteStepCollector::OnRoundaboutEnter(
    const RouteDescription::RoundaboutEnterDescriptionRef& /*roundaboutEnterDescription*/,
    const RouteDescription::CrossingWaysDescriptionRef& /*crossingWaysDescription*/)
  {
    NoteKind(RouteStep::Kind::roundaboutEnter);
  }

  void RouteStepCollector::OnRoundaboutLeave(
    const RouteDescription::RoundaboutLeaveDescriptionRef& /*roundaboutLeaveDescription*/,
    const RouteDescription::NameDescriptionRef& /*nameDescription*/)
  {
    NoteKind(RouteStep::Kind::roundaboutLeave);
  }

  void RouteStepCollector::OnMotorwayEnter(
    const RouteDescription::MotorwayEnterDescriptionRef& /*motorwayEnterDescription*/,
    const RouteDescription::CrossingWaysDescriptionRef& /*crossingWaysDescription*/)
  {
    NoteKind(RouteStep::Kind::motorwayEnter);
  }

  void RouteStepCollector::OnMotorwayChange(
    const RouteDescription::MotorwayChangeDescriptionRef& /*motorwayChangeDescription*/,
    const RouteDescription::MotorwayJunctionDescriptionRef& /*motorwayJunctionDescription*/,
    const RouteDescription::DirectionDescriptionRef& /*directionDescription*/,
    const RouteDescription::DestinationDescriptionRef& /*crossingDestinationDescription*/)
  {
    NoteKind(RouteStep::Kind::motorwayChange);
  }

  void RouteStepCollector::OnMotorwayLeave(
    const RouteDescription::MotorwayLeaveDescriptionRef& /*motorwayLeaveDescription*/,
    const RouteDescription::MotorwayJunctionDescriptionRef& /*motorwayJunctionDescription*/,
    const RouteDescription::DirectionDescriptionRef& /*directionDescription*/,
    const RouteDescription::NameDescriptionRef& /*nameDescription*/,
    const RouteDescription::DestinationDescriptionRef& /*destinationDescription*/)
  {
    NoteKind(RouteStep::Kind::motorwayLeave);
  }

  void RouteStepCollector::OnPathNameChange(
    const RouteDescription::NameChangedDescriptionRef& /*nameChangedDescription*/)
  {
    NoteKind(RouteStep::Kind::nameChange);
  }

  void RouteStepCollector::OnMaxSpeed(const RouteDescription::MaxSpeedDescriptionRef& /*maxSpeedDescription*/)
  {
    // The max speed is a fact of the node, not a manoeuvre of its own.
  }

  void RouteStepCollector::OnPOIAtRoute(const RouteDescription::POIAtRouteDescriptionRef& /*poiAtRouteDescription*/)
  {
    // The POI is a fact of the node, not a manoeuvre of its own.
  }

  void RouteStepCollector::OnViaAtRoute(const RouteDescription::ViaDescriptionRef& /*viaDescription*/)
  {
    NoteKind(RouteStep::Kind::informative);
  }

  void RouteStepCollector::BeforeNode(const RouteDescription::Node& /*node*/)
  {
    pendingKindSet=false;
    pendingKind=RouteStep::Kind::unknown;
  }

  void RouteStepCollector::AfterNode(const RouteDescription::Node& node)
  {
    for (const auto& description : node.GetDescriptions()) {
      if (!IsHandledDescription(*description)) {
        unhandledDescriptions.push_back(description->GetDebugString());
      }
    }

    auto step=BuildNodeStep(node);

    if (!step) {
      return;
    }

    if (pendingKindSet) {
      step->kind=pendingKind;
    }

    if (firstStep) {
      step->legDistance=node.GetDistance();
      step->legTime=node.GetTime();
      firstStep=false;
    }
    else {
      step->legDistance=node.GetDistance()-previousDistance;
      step->legTime=node.GetTime()-previousTime;
    }

    previousDistance=node.GetDistance();
    previousTime=node.GetTime();

    result.steps.push_back(std::move(*step));
  }

  bool RouteStepCollector::Continue() const
  {
    return stopAfter<Distance::Zero() ||
           result.steps.empty() ||
           result.steps.back().distanceFromStart<=stopAfter;
  }
} // namespace osmscout::client
