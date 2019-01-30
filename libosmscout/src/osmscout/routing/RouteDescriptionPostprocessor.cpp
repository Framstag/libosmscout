/*
  This source is part of the libosmscout library
  Copyright (C) 2009-2018  Tim Teulings

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

#include <osmscout/routing/Route.h>
#include <osmscout/routing/RouteDescriptionPostprocessor.h>

#include <osmscout/system/Assert.h>
#include <osmscout/system/Math.h>

#include <sstream>


namespace osmscout {
  RouteDescriptionPostprocessor::Callback::~Callback()
  {
    // no code
  }

  void RouteDescriptionPostprocessor::Callback::BeforeRoute()
  {
    // no code
  }

  void RouteDescriptionPostprocessor::Callback::OnStart(const RouteDescription::StartDescriptionRef& /*startDescription*/,
                                                        const RouteDescription::TypeNameDescriptionRef& /*typeNameDescription*/,
                                                        const RouteDescription::NameDescriptionRef& /*nameDescription*/)
  {
    // no code
  }

  void RouteDescriptionPostprocessor::Callback::OnTargetReached(const RouteDescription::TargetDescriptionRef& /*targetDescription*/)
  {
    // no code
  }

  void RouteDescriptionPostprocessor::Callback::OnTurn(const RouteDescription::TurnDescriptionRef& /*turnDescription*/,
                                                       const RouteDescription::CrossingWaysDescriptionRef& /*crossingWaysDescription*/,
                                                       const RouteDescription::DirectionDescriptionRef& /*directionDescription*/,
                                                       const RouteDescription::TypeNameDescriptionRef& /*typeNameDescription*/,
                                                       const RouteDescription::NameDescriptionRef& /*nameDescription*/)
  {
    // no code
  }

  void RouteDescriptionPostprocessor::Callback::OnRoundaboutEnter(const RouteDescription::RoundaboutEnterDescriptionRef& /*roundaboutEnterDescription*/,
                                                                  const RouteDescription::CrossingWaysDescriptionRef& /*crossingWaysDescription*/)
  {
    // no code
  }

  void RouteDescriptionPostprocessor::Callback::OnRoundaboutLeave(const RouteDescription::RoundaboutLeaveDescriptionRef& /*roundaboutLeaveDescription*/,
                                                                  const RouteDescription::NameDescriptionRef& /*nameDescription*/)
  {
    // no code
  }

  void RouteDescriptionPostprocessor::Callback::OnMotorwayEnter(const RouteDescription::MotorwayEnterDescriptionRef& /*motorwayEnterDescription*/,
                                                                const RouteDescription::CrossingWaysDescriptionRef& /*crossingWaysDescription*/)
  {
    // no code
  }

  void RouteDescriptionPostprocessor::Callback::OnMotorwayChange(const RouteDescription::MotorwayChangeDescriptionRef& /*motorwayChangeDescription*/,
                                                                 const RouteDescription::MotorwayJunctionDescriptionRef& /*motorwayJunctionDescription*/,
                                                                 const RouteDescription::DestinationDescriptionRef& /*crossingDestinationDescription*/)
  {
    // no code
  }

  void RouteDescriptionPostprocessor::Callback::OnMotorwayLeave(const RouteDescription::MotorwayLeaveDescriptionRef& /*motorwayLeaveDescription*/,
                                                                const RouteDescription::MotorwayJunctionDescriptionRef& /*motorwayJunctionDescription*/,
                                                                const RouteDescription::DirectionDescriptionRef& /*directionDescription*/,
                                                                const RouteDescription::NameDescriptionRef& /*nameDescription*/)
  {
    // no code
  }

  void RouteDescriptionPostprocessor::Callback::OnPathNameChange(const RouteDescription::NameChangedDescriptionRef& /*nameChangedDescription*/)
  {
    // no code
  }

  void RouteDescriptionPostprocessor::Callback::OnMaxSpeed(const RouteDescription::MaxSpeedDescriptionRef& /*maxSpeedDescription*/)
  {
    // no code
  }

  void RouteDescriptionPostprocessor::Callback::OnPOIAtRoute(const RouteDescription::POIAtRouteDescriptionRef& /*poiAtRouteDescription*/)
  {
    // no code
  }

  void RouteDescriptionPostprocessor::Callback::BeforeNode(const RouteDescription::Node& /*node*/)
  {
    // no code
  }

  void RouteDescriptionPostprocessor::Callback::AfterNode(const RouteDescription::Node& /*node*/)
  {
    // no code
  }


  /**
   * Evaluate the already postprocessed RouteDescription and call the given callback for
   * node segments where something important happens or changes.
   *
   * @param description
   * @param callback
   */
  void RouteDescriptionPostprocessor::GenerateDescription(const RouteDescription& description,
                                                          RouteDescriptionPostprocessor::Callback& callback)
  {
    callback.BeforeRoute();

    auto prevNode=description.Nodes().cend();
    for (auto node=description.Nodes().cbegin();
         node!=description.Nodes().cend();
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
      osmscout::RouteDescription::MotorwayJunctionDescriptionRef motorwayJunctionDescription;
      osmscout::RouteDescription::DestinationDescriptionRef      crossingDestinationDescription;
      osmscout::RouteDescription::MaxSpeedDescriptionRef         maxSpeedDescription;
      osmscout::RouteDescription::TypeNameDescriptionRef         typeNameDescription;
      osmscout::RouteDescription::POIAtRouteDescriptionRef       poiAtRouteDescription;

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

      desc=node->GetDescription(osmscout::RouteDescription::MOTORWAY_JUNCTION_DESC);
      if (desc) {
        motorwayJunctionDescription=std::dynamic_pointer_cast<osmscout::RouteDescription::MotorwayJunctionDescription>(desc);
      }

      desc=node->GetDescription(osmscout::RouteDescription::CROSSING_DESTINATION_DESC);
      if (desc) {
        crossingDestinationDescription=std::dynamic_pointer_cast<osmscout::RouteDescription::DestinationDescription>(desc);
      }

      desc=node->GetDescription(osmscout::RouteDescription::WAY_MAXSPEED_DESC);
      if (desc) {
        maxSpeedDescription=std::dynamic_pointer_cast<osmscout::RouteDescription::MaxSpeedDescription>(desc);
      }

      desc=node->GetDescription(osmscout::RouteDescription::WAY_TYPE_NAME_DESC);
      if (desc) {
        typeNameDescription=std::dynamic_pointer_cast<osmscout::RouteDescription::TypeNameDescription>(desc);
      }

      desc=node->GetDescription(osmscout::RouteDescription::POI_AT_ROUTE_DESC);
      if (desc) {
        poiAtRouteDescription=std::dynamic_pointer_cast<osmscout::RouteDescription::POIAtRouteDescription>(desc);
      }

      callback.BeforeNode(*node);

      if (startDescription) {
        callback.OnStart(startDescription,
                         typeNameDescription,
                         nameDescription);
      }

      if (targetDescription) {
        callback.OnTargetReached(targetDescription);
      }

      if (turnDescription) {
        callback.OnTurn(turnDescription,
                        crossingWaysDescription,
                        directionDescription,
                        typeNameDescription,
                        nameDescription);
      }
      else if (roundaboutEnterDescription) {
        callback.OnRoundaboutEnter(roundaboutEnterDescription,
                                   crossingWaysDescription);
      }
      else if (roundaboutLeaveDescription) {
        callback.OnRoundaboutLeave(roundaboutLeaveDescription,
                                   nameDescription);
      }

      if (motorwayEnterDescription) {
        callback.OnMotorwayEnter(motorwayEnterDescription,
                                 crossingWaysDescription);
      }
      else if (motorwayChangeDescription) {
        callback.OnMotorwayChange(motorwayChangeDescription,
                                  motorwayJunctionDescription,
                                  crossingDestinationDescription);
      }
      else if (motorwayLeaveDescription) {
        callback.OnMotorwayLeave(motorwayLeaveDescription,
                                 motorwayJunctionDescription,
                                 directionDescription,
                                 nameDescription);
      }
      else if (nameChangedDescription) {
        callback.OnPathNameChange(nameChangedDescription);
      }

      if (maxSpeedDescription) {
        callback.OnMaxSpeed(maxSpeedDescription);
      }

      if (poiAtRouteDescription) {
        callback.OnPOIAtRoute(poiAtRouteDescription);
      }

      callback.AfterNode(*node);

      prevNode=node;
    }
  }
}
