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

#include <osmscout/routing/RouteDescription.h>
#include <osmscout/routing/RouteDescriptionPostprocessor.h>

#include <osmscout/system/Assert.h>
#include <osmscout/system/Math.h>

#include <sstream>


namespace osmscout {

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
                                                                 const RouteDescription::DirectionDescriptionRef& /*directionDescription*/,
                                                                 const RouteDescription::DestinationDescriptionRef& /*crossingDestinationDescription*/)
  {
    // no code
  }

  void RouteDescriptionPostprocessor::Callback::OnMotorwayLeave(const RouteDescription::MotorwayLeaveDescriptionRef& /*motorwayLeaveDescription*/,
                                                                const RouteDescription::MotorwayJunctionDescriptionRef& /*motorwayJunctionDescription*/,
                                                                const RouteDescription::DirectionDescriptionRef& /*directionDescription*/,
                                                                const RouteDescription::NameDescriptionRef& /*nameDescription*/,
                                                                const RouteDescription::DestinationDescriptionRef& /*destinationDescription*/)
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

  void RouteDescriptionPostprocessor::Callback::OnViaAtRoute(const RouteDescription::ViaDescriptionRef& /*viaDescription*/)
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

  bool RouteDescriptionPostprocessor::Callback::Continue() const
  {
    return true;
  }


  /**
   * Evaluate the already postprocessed RouteDescription and call the given callback for
   * node segments where something important happens or changes.
   *
   * @param description
   * @param callback
   */
  void RouteDescriptionPostprocessor::GenerateDescription(const RouteDescription& description,
                                                          RouteDescriptionPostprocessor::Callback& callback) const
  {
    GenerateDescription(description.Nodes().cbegin(),
                        description.Nodes().cend(),
                        callback);
  }

  void RouteDescriptionPostprocessor::GenerateDescription(const RouteDescription::NodeIterator &first,
                                                          const RouteDescription::NodeIterator &last,
                                                          RouteDescriptionPostprocessor::Callback& callback) const
  {
    callback.BeforeRoute();

    for (auto node=first;
         node!=last && callback.Continue();
         ++node) {

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
      osmscout::RouteDescription::ViaDescriptionRef              viaDescription;

      nameDescription = node->GetDescription<osmscout::RouteDescription::NameDescription>();
      directionDescription = node->GetDescription<osmscout::RouteDescription::DirectionDescription>();
      nameChangedDescription = node->GetDescription<osmscout::RouteDescription::NameChangedDescription>();
      crossingWaysDescription = node->GetDescription<osmscout::RouteDescription::CrossingWaysDescription>();
      startDescription = node->GetDescription<osmscout::RouteDescription::StartDescription>();
      targetDescription = node->GetDescription<osmscout::RouteDescription::TargetDescription>();
      turnDescription = node->GetDescription<osmscout::RouteDescription::TurnDescription>();
      roundaboutEnterDescription = node->GetDescription<osmscout::RouteDescription::RoundaboutEnterDescription>();
      roundaboutLeaveDescription = node->GetDescription<osmscout::RouteDescription::RoundaboutLeaveDescription>();
      motorwayEnterDescription = node->GetDescription<osmscout::RouteDescription::MotorwayEnterDescription>();
      motorwayChangeDescription = node->GetDescription<osmscout::RouteDescription::MotorwayChangeDescription>();
      motorwayLeaveDescription = node->GetDescription<osmscout::RouteDescription::MotorwayLeaveDescription>();
      motorwayJunctionDescription = node->GetDescription<osmscout::RouteDescription::MotorwayJunctionDescription>();
      crossingDestinationDescription = node->GetDescription<osmscout::RouteDescription::DestinationDescription>();
      maxSpeedDescription = node->GetDescription<osmscout::RouteDescription::MaxSpeedDescription>();
      typeNameDescription = node->GetDescription<osmscout::RouteDescription::TypeNameDescription>();
      poiAtRouteDescription = node->GetDescription<osmscout::RouteDescription::POIAtRouteDescription>();
      viaDescription = node->GetDescription<osmscout::RouteDescription::ViaDescription>();
        
      callback.BeforeNode(*node);

      if (startDescription) {
        callback.OnStart(startDescription,
                         typeNameDescription,
                         nameDescription);
      }

      if (targetDescription) {
        callback.OnTargetReached(targetDescription);
      }
        
      if (viaDescription) {
        callback.OnViaAtRoute(viaDescription);
      }

      if (roundaboutEnterDescription || roundaboutLeaveDescription) {
        if (roundaboutEnterDescription) {
          callback.OnRoundaboutEnter(roundaboutEnterDescription,
                                     crossingWaysDescription);
        }
        if (roundaboutLeaveDescription) {
          callback.OnRoundaboutLeave(roundaboutLeaveDescription,
                                     nameDescription);
        }
      }
      else if (turnDescription) {
        callback.OnTurn(turnDescription,
                        crossingWaysDescription,
                        directionDescription,
                        typeNameDescription,
                        nameDescription);
      }

      if (motorwayEnterDescription) {
        callback.OnMotorwayEnter(motorwayEnterDescription,
                                 crossingWaysDescription);
      }
      else if (motorwayChangeDescription) {
        callback.OnMotorwayChange(motorwayChangeDescription,
                                  motorwayJunctionDescription,
                                  directionDescription,
                                  crossingDestinationDescription);
      }
      else if (motorwayLeaveDescription) {
        callback.OnMotorwayLeave(motorwayLeaveDescription,
                                 motorwayJunctionDescription,
                                 directionDescription,
                                 nameDescription,
                                 crossingDestinationDescription);
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
    }
  }
}
