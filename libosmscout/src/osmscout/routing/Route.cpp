/*
  This source is part of the libosmscout library
  Copyright (C) 2009  Tim Teulings

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

#include <sstream>

#include <osmscout/system/Assert.h>
#include <osmscout/system/Math.h>

namespace osmscout {

  /** Constant for a description of the start node (StartDescription) */
  const char* const RouteDescription::NODE_START_DESC        = "NodeStart";
  /** Constant for a description of the target node (TargetDescription) */
  const char* const RouteDescription::NODE_TARGET_DESC       = "NodeTarget";
  /** Constant for a description of name of the way (NameDescription) */
  const char* const RouteDescription::WAY_NAME_DESC          = "WayName";
  /** Constant for a description of a change of way name (NameChangedDescription) */
  const char* const RouteDescription::WAY_NAME_CHANGED_DESC  = "WayChangedName";
  /** Constant for a description of list of way name crossing a node (CrossingWaysDescription) */
  const char* const RouteDescription::CROSSING_WAYS_DESC     = "CrossingWays";
  /** Constant for a description of a turn (TurnDescription) */
  const char* const RouteDescription::DIRECTION_DESC         = "Direction";
  /** Constant for doing description of an explicit turn (TurnDescription) */
  const char* const RouteDescription::TURN_DESC              = "Turn";
  /** Constant for a description of entering a roundabout (RoundaboutEnterDescription) */
  const char* const RouteDescription::ROUNDABOUT_ENTER_DESC  = "RountaboutEnter";
  /** Constant for a description of entering a roundabout (RoundaboutLeaveDescription) */
  const char* const RouteDescription::ROUNDABOUT_LEAVE_DESC  = "RountaboutLeave";
  /** Constant for a description of entering a motorway (MotorwayEnterDescription) */
  const char* const RouteDescription::MOTORWAY_ENTER_DESC    = "MotorwayEnter";
  /** Constant for a description of changing a motorway (MotorwayChangeDescription) */
  const char* const RouteDescription::MOTORWAY_CHANGE_DESC   = "MotorwayChange";
  /** Constant for a description of leaving a motorway (MotorwayLeaveDescription) */
  const char* const RouteDescription::MOTORWAY_LEAVE_DESC    = "MotorwayLeave";
  /** Constant for a description of motorway junction (MotorwayJunctionDescription) */
  const char* const RouteDescription::MOTORWAY_JUNCTION_DESC = "MotorwayJunction";
  /** Constant for a description of a destination to choose at a junction */
  const char* const RouteDescription::CROSSING_DESTINATION_DESC = "CrossingDestination";
  /** Constant for a description of the maximum speed for the given way */
  const char* const RouteDescription::WAY_MAXSPEED_DESC      = "MaxSpeed";
  /** Constant for a description of type name of the way (TypeNameDescription) */
  const char* const RouteDescription::WAY_TYPE_NAME_DESC     = "TypeName";

  RouteDescription::Description::~Description()
  {
    // no code
  }

  RouteDescription::StartDescription::StartDescription(const std::string& description)
  : description(description)
  {
    // no code
  }

  std::string RouteDescription::StartDescription::GetDebugString() const
  {
    return "Start: '"+description+"'";
  }

  std::string RouteDescription::StartDescription::GetDescription() const
  {
    return description;
  }

  RouteDescription::TargetDescription::TargetDescription(const std::string& description)
  : description(description)
  {
    // no code
  }

  std::string RouteDescription::TargetDescription::GetDebugString() const
  {
    return "Target: '"+description+"'";
  }

  std::string RouteDescription::TargetDescription::GetDescription() const
  {
    return description;
  }

  RouteDescription::NameDescription::NameDescription(const std::string& name)
  : name(name)
  {
    // no code
  }

  RouteDescription::NameDescription::NameDescription(const std::string& name,
                                                     const std::string& ref)
  : name(name),
    ref(ref)
  {
    // no code
  }

  std::string RouteDescription::NameDescription::GetDebugString() const
  {
    return "Name: '"+GetDescription()+"'";
  }

  bool RouteDescription::NameDescription::HasName() const
  {
    return !name.empty() || !ref.empty();
  }

  std::string RouteDescription::NameDescription::GetName() const
  {
    return name;
  }

  std::string RouteDescription::NameDescription::GetRef() const
  {
    return ref;
  }

  std::string RouteDescription::NameDescription::GetDescription() const
  {
    std::ostringstream stream;

    if (name.empty() &&
        ref.empty()) {
      return "unnamed road";
    }

    if (!name.empty()) {
      stream << name;
    }

    if (!name.empty() &&
        !ref.empty()) {
      stream << " (";
    }

    if (!ref.empty()) {
      stream << ref;
    }

    if (!name.empty() &&
        !ref.empty()) {
      stream << ")";
    }

    return stream.str();
  }

  RouteDescription::NameChangedDescription::NameChangedDescription(const NameDescriptionRef& originDescription,
                                                                   const NameDescriptionRef& targetDescription)
  : originDescription(originDescription),
    targetDescription(targetDescription)
  {
    // no code
  }

  std::string RouteDescription::NameChangedDescription::GetDebugString() const
  {
    std::string result="Name Change: ";

    if (originDescription) {
      result+="'"+originDescription->GetDescription()+"'";
    }

    result+=" => ";

    if (targetDescription) {
      result+="'"+targetDescription->GetDescription()+"'";
    }

    return result;
  }

  RouteDescription::CrossingWaysDescription::CrossingWaysDescription(size_t exitCount,
                                                                     const NameDescriptionRef& originDescription,
                                                                     const NameDescriptionRef& targetDescription)
  : exitCount(exitCount),
    originDescription(originDescription),
    targetDescription(targetDescription)
  {
    // no code
  }

  void RouteDescription::CrossingWaysDescription::AddDescription(const NameDescriptionRef& description)
  {
    descriptions.push_back(description);
  }

  std::string RouteDescription::CrossingWaysDescription::GetDebugString() const
  {
    std::string result;

    result+="Crossing";

    if (originDescription) {
      if (!result.empty()) {
        result+=" ";
      }

      result+="from '"+originDescription->GetDescription()+"'";
    }

    if (targetDescription) {
      if (!result.empty()) {
        result+=" ";
      }

      result+="to '"+targetDescription->GetDescription()+"'";
    }

    if (!descriptions.empty()) {
      if (!result.empty()) {
        result+=" ";
      }

      result+="with";

      for (std::list<NameDescriptionRef>::const_iterator desc=descriptions.begin();
          desc!=descriptions.end();
          ++desc) {
        result+=" '"+(*desc)->GetDescription()+"'";
      }
    }

    if (!descriptions.empty()) {
      if (!result.empty()) {
        result+=" ";
      }
      result+=std::to_string(exitCount)+ " exits";
    }

    return "Crossing: "+result;
  }

  RouteDescription::DirectionDescription::Move RouteDescription::DirectionDescription::ConvertAngleToMove(double angle) const
  {
    if (fabs(angle)<=10.0) {
      return straightOn;
    }
    else if (fabs(angle)<=45.0) {
      return angle<0 ? slightlyLeft : slightlyRight;
    }
    else if (fabs(angle)<=120.0) {
      return angle<0 ? left : right;
    }
    else {
      return angle<0 ? sharpLeft : sharpRight;
    }
  }

  std::string RouteDescription::DirectionDescription::ConvertMoveToString(Move move) const
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

  RouteDescription::DirectionDescription::DirectionDescription(double turnAngle,
                                                               double curveAngle)
  : turnAngle(turnAngle),
    curveAngle(curveAngle)
  {
    turn=ConvertAngleToMove(turnAngle);
    curve=ConvertAngleToMove(curveAngle);
  }

  std::string RouteDescription::DirectionDescription::GetDebugString() const
  {
    std::ostringstream stream;

    stream << "Direction: ";
    stream << "Turn: " << ConvertMoveToString(turn) << ", " << turnAngle << " degrees ";
    stream << "Curve: " << ConvertMoveToString(curve) << ", " << curveAngle << " degrees";

    return stream.str();
  }

  RouteDescription::TurnDescription::TurnDescription()
  {
    // no code
  }

  std::string RouteDescription::TurnDescription::GetDebugString() const
  {
    return "Turn";
  }

  RouteDescription::RoundaboutEnterDescription::RoundaboutEnterDescription()
  {
    // no code
  }

  std::string RouteDescription::RoundaboutEnterDescription::GetDebugString() const
  {
    return "Enter roundabout";
  }

  RouteDescription::RoundaboutLeaveDescription::RoundaboutLeaveDescription(size_t exitCount)
  : exitCount(exitCount)
  {
    // no code
  }

  std::string RouteDescription::RoundaboutLeaveDescription::GetDebugString() const
  {
    return "Leave roundabout";
  }

  RouteDescription::MotorwayEnterDescription::MotorwayEnterDescription(const NameDescriptionRef& targetDescription)
  : toDescription(targetDescription)
  {
    // no code
  }

  std::string RouteDescription::MotorwayEnterDescription::GetDebugString() const
  {
    std::string result="Enter motorway";

    if (toDescription &&
        toDescription->HasName()) {
      result+=" '"+toDescription->GetDescription()+"'";
    }

    return result;
  }

  RouteDescription::MotorwayChangeDescription::MotorwayChangeDescription(const NameDescriptionRef& fromDescription,
                                                                         const NameDescriptionRef& toDescription)
  : fromDescription(fromDescription),
    toDescription(toDescription)
  {
    // no code
  }

  std::string RouteDescription::MotorwayChangeDescription::GetDebugString() const
  {
    return "Change motorway";
  }

  RouteDescription::MotorwayLeaveDescription::MotorwayLeaveDescription(const NameDescriptionRef& fromDescription)
  : fromDescription(fromDescription)
  {
    // no code
  }

  std::string RouteDescription::MotorwayLeaveDescription::GetDebugString() const
  {
    return "Leave motorway";
  }

  RouteDescription::MotorwayJunctionDescription::MotorwayJunctionDescription(const NameDescriptionRef& junctionDescription)
  : junctionDescription(junctionDescription)
  {
    // no code
  }

  std::string RouteDescription::MotorwayJunctionDescription::GetDebugString() const
  {
    return "motorway junction";
  }

  RouteDescription::DestinationDescription::DestinationDescription(const std::string& description)
    : description(description)
  {
    // no code
  }

  std::string RouteDescription::DestinationDescription::GetDebugString() const
  {
    return "Start: '"+description+"'";
  }

  std::string RouteDescription::DestinationDescription::GetDescription() const
  {
    return description;
  }

  RouteDescription::MaxSpeedDescription::MaxSpeedDescription(uint8_t speed)
  : maxSpeed(speed)
  {

  }

  std::string RouteDescription::MaxSpeedDescription::GetDebugString() const
  {
    return std::string("Max. speed ")+std::to_string(maxSpeed)+"km/h";
  }

  RouteDescription::Node::Node(DatabaseId database,
                               size_t currentNodeIndex,
                               const std::vector<ObjectFileRef>& objects,
                               const ObjectFileRef& pathObject,
                               size_t targetNodeIndex)
  : database(database),
    currentNodeIndex(currentNodeIndex),
    objects(objects),
    pathObject(pathObject),
    targetNodeIndex(targetNodeIndex),
    distance(0.0),
    time(0.0),
    location(GeoCoord(NAN, NAN))
  {
    // no code
  }

  RouteDescription::TypeNameDescription::TypeNameDescription(const std::string& name)
    : name(name)
  {
    // no code
  }

  std::string RouteDescription::TypeNameDescription::GetDebugString() const
  {
    return "Name: '"+GetDescription()+"'";
  }

  bool RouteDescription::TypeNameDescription::HasName() const
  {
    return !name.empty();
  }

  std::string RouteDescription::TypeNameDescription::GetName() const
  {
    return name;
  }

  std::string RouteDescription::TypeNameDescription::GetDescription() const
  {
    std::ostringstream stream;

    stream << name;

    return stream.str();
  }

  bool RouteDescription::Node::HasDescription(const char* name) const
  {
    std::unordered_map<std::string,DescriptionRef>::const_iterator entry;

    entry=descriptionMap.find(name);

    return entry!=descriptionMap.end() && entry->second;
  }

  RouteDescription::DescriptionRef RouteDescription::Node::GetDescription(const char* name) const
  {
    std::unordered_map<std::string,DescriptionRef>::const_iterator entry;

    entry=descriptionMap.find(name);

    if (entry!=descriptionMap.end()) {
      return entry->second;
    }
    else {
      return NULL;
    }
  }

  void RouteDescription::Node::SetDistance(double distance)
  {
    this->distance=distance;
  }

  void RouteDescription::Node::SetLocation(const GeoCoord &coord)
  {
    this->location=coord;
  }


  void RouteDescription::Node::SetTime(double time)
  {
    this->time=time;
  }

  void RouteDescription::Node::AddDescription(const char* name,
                                              const DescriptionRef& description)
  {
    descriptions.push_back(description);
    descriptionMap[name]=description;
  }

  RouteDescription::RouteDescription()
  {
    // no code
  }

  RouteDescription::~RouteDescription()
  {
    // no code
  }

  void RouteDescription::Clear()
  {
    nodes.clear();
  }

  void RouteDescription::AddNode(DatabaseId database,
                                 size_t currentNodeIndex,
                                 const std::vector<ObjectFileRef>& objects,
                                 const ObjectFileRef& pathObject,
                                 size_t targetNodeIndex)
  {
    nodes.push_back(Node(database,
                         currentNodeIndex,
                         objects,
                         pathObject,
                         targetNodeIndex));
  }
}

