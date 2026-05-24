/*
 Navigation - a demo program for libosmscout
 Copyright (C) 2009  Tim Teulings

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

#include <osmscout/navigation/Navigation.h>
#include <osmscout/GeoCoord.h>

namespace osmscout {

  struct NodeDescription
  {
    int         roundaboutExitNumber{-1};
    int         index{0};
    std::string instructions;
    Distance    distance;
    Duration    time;
    GeoCoord    location;
  };

  bool HasRelevantDescriptions(const RouteDescription::Node& node);

  std::string DumpStartDescription(const RouteDescription::StartDescriptionRef& startDescription,
                                   const RouteDescription::NameDescriptionRef& nameDescription);

  std::string DumpTargetDescription(const RouteDescription::TargetDescriptionRef& targetDescription);

  NodeDescription DumpTurnDescription(const RouteDescription::TurnDescriptionRef& turnDescription,
                                      const RouteDescription::CrossingWaysDescriptionRef& crossingWaysDescription,
                                      const RouteDescription::DirectionDescriptionRef& directionDescription,
                                      const RouteDescription::NameDescriptionRef& nameDescription);

  std::string
  DumpRoundaboutEnterDescription(const RouteDescription::RoundaboutEnterDescriptionRef& roundaboutEnterDescription,
                                 const RouteDescription::CrossingWaysDescriptionRef& crossingWaysDescription);

  std::string
  DumpRoundaboutLeaveDescription(const RouteDescription::RoundaboutLeaveDescriptionRef& roundaboutLeaveDescription,
                                 const RouteDescription::NameDescriptionRef& nameDescription,
                                 size_t roundaboutCrossingCounter);

  NodeDescription
  DumpMotorwayEnterDescription(const RouteDescription::MotorwayEnterDescriptionRef& motorwayEnterDescription,
                               const RouteDescription::CrossingWaysDescriptionRef& crossingWaysDescription);

  NodeDescription
  DumpMotorwayChangeDescription(const RouteDescription::MotorwayChangeDescriptionRef& motorwayChangeDescription);

  NodeDescription
  DumpMotorwayLeaveDescription(const RouteDescription::MotorwayLeaveDescriptionRef& motorwayLeaveDescription,
                               const RouteDescription::DirectionDescriptionRef& directionDescription,
                               const RouteDescription::NameDescriptionRef& nameDescription,
                               const RouteDescription::MotorwayJunctionDescriptionRef& motorwayJunction);

  NodeDescription DumpNameChangedDescription(const RouteDescription::NameChangedDescriptionRef& nameChangedDescription);

  bool advanceToNextWaypoint(std::list<RouteDescription::Node>::const_iterator& waypoint,
                             std::list<RouteDescription::Node>::const_iterator end);

  template<class NodeDescription>
  class NavigationDescription : public OutputDescription<NodeDescription>
  {
  public:
    NavigationDescription() = default;

    void NextDescription(const Distance &distance,
                         std::list<RouteDescription::Node>::const_iterator& waypoint,
                         std::list<RouteDescription::Node>::const_iterator end)
    {

      if (waypoint==end || (distance.AsMeter()>=0 && previousDistance>distance)) {
        return;
      }

      do {

        description.roundaboutExitNumber=-1;
        description.instructions.clear();

        do {
          RouteDescription::NameDescriptionRef             nameDescription;
          RouteDescription::DirectionDescriptionRef        directionDescription;
          RouteDescription::NameChangedDescriptionRef      nameChangedDescription;
          RouteDescription::CrossingWaysDescriptionRef     crossingWaysDescription;
          RouteDescription::StartDescriptionRef            startDescription;
          RouteDescription::TargetDescriptionRef           targetDescription;
          RouteDescription::TurnDescriptionRef             turnDescription;
          RouteDescription::RoundaboutEnterDescriptionRef  roundaboutEnterDescription;
          RouteDescription::RoundaboutLeaveDescriptionRef  roundaboutLeaveDescription;
          RouteDescription::MotorwayEnterDescriptionRef    motorwayEnterDescription;
          RouteDescription::MotorwayChangeDescriptionRef   motorwayChangeDescription;
          RouteDescription::MotorwayLeaveDescriptionRef    motorwayLeaveDescription;
          RouteDescription::MotorwayJunctionDescriptionRef motorwayJunctionDescription;

          nameDescription = waypoint->GetDescription<osmscout::RouteDescription::NameDescription>();
          directionDescription = waypoint->GetDescription<osmscout::RouteDescription::DirectionDescription>();
          nameChangedDescription = waypoint->GetDescription<osmscout::RouteDescription::NameChangedDescription>();
          crossingWaysDescription = waypoint->GetDescription<osmscout::RouteDescription::CrossingWaysDescription>();
          startDescription = waypoint->GetDescription<osmscout::RouteDescription::StartDescription>();
          targetDescription = waypoint->GetDescription<osmscout::RouteDescription::TargetDescription>();
          turnDescription = waypoint->GetDescription<osmscout::RouteDescription::TurnDescription>();
          roundaboutEnterDescription = waypoint->GetDescription<osmscout::RouteDescription::RoundaboutEnterDescription>();
          roundaboutLeaveDescription = waypoint->GetDescription<osmscout::RouteDescription::RoundaboutLeaveDescription>();
          motorwayEnterDescription = waypoint->GetDescription<osmscout::RouteDescription::MotorwayEnterDescription>();
          motorwayChangeDescription = waypoint->GetDescription<osmscout::RouteDescription::MotorwayChangeDescription>();
          motorwayLeaveDescription = waypoint->GetDescription<osmscout::RouteDescription::MotorwayLeaveDescription>();
          motorwayJunctionDescription = waypoint->GetDescription<osmscout::RouteDescription::MotorwayJunctionDescription>();

          if (crossingWaysDescription &&
              roundaboutCrossingCounter>0 &&
              crossingWaysDescription->GetExitCount()>1) {
            roundaboutCrossingCounter+=crossingWaysDescription->GetExitCount()-1;
          }

          if (!HasRelevantDescriptions(*waypoint)) {
            continue;
          }

          if (startDescription) {
            description.instructions=DumpStartDescription(startDescription,
                                                          nameDescription);
          }
          else if (targetDescription) {
            description.instructions=DumpTargetDescription(targetDescription);
          }
          else if (turnDescription) {
            description=DumpTurnDescription(turnDescription,
                                            crossingWaysDescription,
                                            directionDescription,
                                            nameDescription);
          }
          else if (roundaboutEnterDescription) {
            description.instructions        =DumpRoundaboutEnterDescription(roundaboutEnterDescription,
                                                                            crossingWaysDescription);
            description.roundaboutExitNumber=1;
            roundaboutCrossingCounter=1;
          }
          else if (roundaboutLeaveDescription) {
            description.instructions+=DumpRoundaboutLeaveDescription(roundaboutLeaveDescription,
                                                                     nameDescription,
                                                                     roundaboutCrossingCounter);
            description.roundaboutExitNumber=(int) roundaboutLeaveDescription->GetExitCount();
            roundaboutCrossingCounter=0;
          }
          else if (motorwayEnterDescription) {
            description=DumpMotorwayEnterDescription(motorwayEnterDescription,
                                                     crossingWaysDescription);
          }
          else if (motorwayChangeDescription) {
            description=DumpMotorwayChangeDescription(motorwayChangeDescription);
          }
          else if (motorwayLeaveDescription) {
            description=DumpMotorwayLeaveDescription(motorwayLeaveDescription,
                                                     directionDescription,
                                                     nameDescription,
                                                     motorwayJunctionDescription);
          }
          else if (nameChangedDescription) {
            description=DumpNameChangedDescription(nameChangedDescription);
          }
          else {
            description.instructions.clear();
          }
        }
        while ((description.instructions.empty() || roundaboutCrossingCounter>0) && advanceToNextWaypoint(waypoint,
                                                                                                          end));

        description.index=index++;
      }
      while (distance>waypoint->GetDistance() && advanceToNextWaypoint(waypoint,
                                                                       end));
      description.distance=waypoint->GetDistance();
      description.time    =waypoint->GetTime();
      description.location=waypoint->GetLocation();
      previousDistance=description.distance;
      waypoint++;
    }

    NodeDescription GetDescription()
    {
      return description;
    }

    void Clear()
    {
      previousDistance         =Distance::Of<Meter>(0.0);
      roundaboutCrossingCounter=0;
      index                    =0;
    }

  private:
    size_t          roundaboutCrossingCounter=0;
    size_t          index=0;
    Distance        previousDistance;
    NodeDescription description;
  };
}

