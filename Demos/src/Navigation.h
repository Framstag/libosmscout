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

#include <osmscout/Navigation.h>
#include <osmscout/GeoCoord.h>

namespace osmscout {
    
  typedef struct {
    int roundaboutExitNumber;
    int index;
    std::string instructions;
    double distance;
    double time;
    GeoCoord location;
  } NodeDescription;
    
  bool HasRelevantDescriptions(const RouteDescription::Node& node);
    
  std::string DumpStartDescription(const RouteDescription::StartDescriptionRef& startDescription,
				   const RouteDescription::NameDescriptionRef& nameDescription);
  std::string DumpTargetDescription(const RouteDescription::TargetDescriptionRef& targetDescription);
  NodeDescription DumpTurnDescription(const RouteDescription::TurnDescriptionRef& turnDescription,
				      const RouteDescription::CrossingWaysDescriptionRef& crossingWaysDescription,
				      const RouteDescription::DirectionDescriptionRef& directionDescription,
				      const RouteDescription::NameDescriptionRef& nameDescription);
  std::string DumpRoundaboutEnterDescription(const RouteDescription::RoundaboutEnterDescriptionRef& roundaboutEnterDescription,
					     const RouteDescription::CrossingWaysDescriptionRef& crossingWaysDescription);
  std::string DumpRoundaboutLeaveDescription(const RouteDescription::RoundaboutLeaveDescriptionRef& roundaboutLeaveDescription,
					     const RouteDescription::NameDescriptionRef& nameDescription, int roundaboutCrossingCounter);
  NodeDescription DumpMotorwayEnterDescription(const RouteDescription::MotorwayEnterDescriptionRef& motorwayEnterDescription,
					       const RouteDescription::CrossingWaysDescriptionRef& crossingWaysDescription);
  NodeDescription DumpMotorwayChangeDescription(const RouteDescription::MotorwayChangeDescriptionRef& motorwayChangeDescription);
  NodeDescription DumpMotorwayLeaveDescription(const RouteDescription::MotorwayLeaveDescriptionRef& motorwayLeaveDescription,
					       const RouteDescription::DirectionDescriptionRef& directionDescription,
					       const RouteDescription::NameDescriptionRef& nameDescription,
					       const RouteDescription::MotorwayJunctionDescriptionRef& motorwayJunction);
  NodeDescription DumpNameChangedDescription(const RouteDescription::NameChangedDescriptionRef& nameChangedDescription);
    
  bool advanceToNextWaypoint(std::list<RouteDescription::Node>::const_iterator &waypoint,
			     std::list<RouteDescription::Node>::const_iterator end);
    
    template <class NodeDescription>
      class NavigationDescription : public OutputDescription<NodeDescription> {
  public:
    NavigationDescription(): roundaboutCrossingCounter(0), index(0), previousDistance(0.0) {};
        
    void NextDescription(double distance,
			 std::list<RouteDescription::Node>::const_iterator &waypoint,
			 std::list<RouteDescription::Node>::const_iterator end) {
            
      if (waypoint == end || (distance >= 0 && previousDistance>distance)){
	return;
      }
            
      do {
                
	description.roundaboutExitNumber = -1;
	description.instructions = "";
                
	do {
                    
	  RouteDescription::DescriptionRef                 desc;
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
                    
	  RouteDescription::MotorwayJunctionDescriptionRef                   motorwayJunctionDescription;
                    
	  desc=waypoint->GetDescription(RouteDescription::WAY_NAME_DESC);
	  if (desc) {
	    nameDescription=std::dynamic_pointer_cast<RouteDescription::NameDescription>(desc);
	  }
                    
	  desc=waypoint->GetDescription(RouteDescription::DIRECTION_DESC);
	  if (desc) {
	    directionDescription=std::dynamic_pointer_cast<RouteDescription::DirectionDescription>(desc);
	  }
                    
	  desc=waypoint->GetDescription(RouteDescription::WAY_NAME_CHANGED_DESC);
	  if (desc) {
	    nameChangedDescription=std::dynamic_pointer_cast<RouteDescription::NameChangedDescription>(desc);
	  }
                    
	  desc=waypoint->GetDescription(RouteDescription::CROSSING_WAYS_DESC);
	  if (desc) {
	    crossingWaysDescription=std::dynamic_pointer_cast<RouteDescription::CrossingWaysDescription>(desc);
	  }
                    
	  desc=waypoint->GetDescription(RouteDescription::NODE_START_DESC);
	  if (desc) {
	    startDescription=std::dynamic_pointer_cast<RouteDescription::StartDescription>(desc);
	  }
                    
	  desc=waypoint->GetDescription(RouteDescription::NODE_TARGET_DESC);
	  if (desc) {
	    targetDescription=std::dynamic_pointer_cast<RouteDescription::TargetDescription>(desc);
	  }
                    
	  desc=waypoint->GetDescription(RouteDescription::TURN_DESC);
	  if (desc) {
	    turnDescription=std::dynamic_pointer_cast<RouteDescription::TurnDescription>(desc);
	  }
                    
	  desc=waypoint->GetDescription(RouteDescription::ROUNDABOUT_ENTER_DESC);
	  if (desc) {
	    roundaboutEnterDescription=std::dynamic_pointer_cast<RouteDescription::RoundaboutEnterDescription>(desc);
	  }
                    
	  desc=waypoint->GetDescription(RouteDescription::ROUNDABOUT_LEAVE_DESC);
	  if (desc) {
	    roundaboutLeaveDescription=std::dynamic_pointer_cast<RouteDescription::RoundaboutLeaveDescription>(desc);
	  }
                    
	  desc=waypoint->GetDescription(RouteDescription::MOTORWAY_ENTER_DESC);
	  if (desc) {
	    motorwayEnterDescription=std::dynamic_pointer_cast<RouteDescription::MotorwayEnterDescription>(desc);
	  }
                    
	  desc=waypoint->GetDescription(RouteDescription::MOTORWAY_CHANGE_DESC);
	  if (desc) {
	    motorwayChangeDescription=std::dynamic_pointer_cast<RouteDescription::MotorwayChangeDescription>(desc);
	  }
                    
	  desc=waypoint->GetDescription(RouteDescription::MOTORWAY_LEAVE_DESC);
	  if (desc) {
	    motorwayLeaveDescription=std::dynamic_pointer_cast<RouteDescription::MotorwayLeaveDescription>(desc);
	  }
                    
	  desc=waypoint->GetDescription(RouteDescription::MOTORWAY_JUNCTION_DESC);
	  if (desc) {
	    motorwayJunctionDescription=std::dynamic_pointer_cast<RouteDescription::MotorwayJunctionDescription>(desc);
	  }
                    
	  if (crossingWaysDescription &&
                        roundaboutCrossingCounter>0 &&
	      crossingWaysDescription->GetExitCount()>1) {
	    roundaboutCrossingCounter+=crossingWaysDescription->GetExitCount()-1;
	  }
                    
	  if (!HasRelevantDescriptions(*waypoint)) {
	    continue;
	  }
                    
	  if (startDescription) {
	    description.instructions = DumpStartDescription(startDescription,
							    nameDescription);
	  }
	  else if (targetDescription) {
	    description.instructions = DumpTargetDescription(targetDescription);
	  }
	  else if (turnDescription) {
	    description = DumpTurnDescription(turnDescription,
					      crossingWaysDescription,
					      directionDescription,
					      nameDescription);
	  }
	  else if (roundaboutEnterDescription) {
	    description.instructions = DumpRoundaboutEnterDescription(roundaboutEnterDescription,
								      crossingWaysDescription);
	    description.roundaboutExitNumber = 1;
	    roundaboutCrossingCounter=1;
	  }
	  else if (roundaboutLeaveDescription) {
	    description.instructions += DumpRoundaboutLeaveDescription(roundaboutLeaveDescription,
								       nameDescription, roundaboutCrossingCounter);
	    description.roundaboutExitNumber = (int)roundaboutLeaveDescription->GetExitCount();
	    roundaboutCrossingCounter=0;
	  }
	  else if (motorwayEnterDescription) {
	    description = DumpMotorwayEnterDescription(motorwayEnterDescription,
						       crossingWaysDescription);
	  }
	  else if (motorwayChangeDescription) {
	    description = DumpMotorwayChangeDescription(motorwayChangeDescription);
	  }
	  else if (motorwayLeaveDescription) {
	    description = DumpMotorwayLeaveDescription(motorwayLeaveDescription,
						       directionDescription,
						       nameDescription,
						       motorwayJunctionDescription);
	  }
	  else if (nameChangedDescription) {
	    description = DumpNameChangedDescription(nameChangedDescription);
	  } else {
	    description.instructions = "";
	  }
                    
	} while((description.instructions.empty() || roundaboutCrossingCounter>0 ) && advanceToNextWaypoint(waypoint, end));
                
	description.index = index++;
      } while (distance>waypoint->GetDistance() && advanceToNextWaypoint(waypoint, end));
      description.distance = waypoint->GetDistance();
      description.time = waypoint->GetTime();
      description.location = waypoint->GetLocation();
      previousDistance = description.distance;
      waypoint++;
    }
        
    NodeDescription GetDescription() {
      return description;
    }
        
    void Clear() {
      previousDistance = 0.0;
      roundaboutCrossingCounter = 0;
      index = 0;
    }
        
  private:
    int             roundaboutCrossingCounter;
    int             index;
    double          previousDistance;
    NodeDescription description;
  };
}

