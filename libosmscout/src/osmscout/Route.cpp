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

#include <osmscout/Route.h>

#include <cmath>
#include <sstream>

#include <osmscout/util/String.h>

namespace osmscout {

  /** Constant for a description of the start node (StartDescription) */
  const char* const RouteDescription::NODE_START_DESC       = "NodeStart";
  /** Constant for a description of the target node (TargetDescription) */
  const char* const RouteDescription::NODE_TARGET_DESC      = "NodeTarget";
  /** Constant for a description of name of the way (NameDescription) */
  const char* const RouteDescription::WAY_NAME_DESC         = "WayName";
  /** Constant for a description of a change of way name (NameChangedDescription) */
  const char* const RouteDescription::WAY_NAME_CHANGED_DESC = "WayChangedName";
  /** Constant for a description of list of way name crossing a node (CrossingWaysDescription) */
  const char* const RouteDescription::CROSSING_WAYS_DESC    = "CrossingWays";
  /** Constant for a description of a turn (TurnDescription) */
  const char* const RouteDescription::TURN_DESC             = "Turn";

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
    return "Start: "+description;
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
    return "Target: "+description;
  }

  std::string RouteDescription::TargetDescription::GetDescription() const
  {
    return description;
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
    return "Name: "+GetDescription();
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

  RouteDescription::NameChangedDescription::NameChangedDescription(NameDescription* originDescription,
                                                                   NameDescription* targetDescription)
  : originDescription(originDescription),
    targetDescription(targetDescription)
  {
    // no code
  }

  std::string RouteDescription::NameChangedDescription::GetDebugString() const
  {
    std::string result="Name Change: ";

    if (originDescription.Valid()) {
      result+=originDescription->GetDescription();
    }

    result+=" => ";

    if (targetDescription.Valid()) {
      result+=targetDescription->GetDescription();
    }

    return result;
  }

  RouteDescription::CrossingWaysDescription::CrossingWaysDescription(Type type,
                                                                     size_t exitCount,
                                                                     NameDescription* originDescription,
                                                                     NameDescription* targetDescription)
  : type(type),
    exitCount(exitCount),
    originDescription(originDescription),
    targetDescription(targetDescription)
  {
    // no code
  }

  void RouteDescription::CrossingWaysDescription::AddDescription(NameDescription* description)
  {
    descriptions.push_back(description);
  }

  std::string RouteDescription::CrossingWaysDescription::GetDebugString() const
  {
    std::string result;

    if (originDescription.Valid()) {
      result+="From "+originDescription->GetDescription();
    }

    if (targetDescription.Valid()) {
      if (!result.empty()) {
        result+=" ";
      }

      result+="to "+targetDescription->GetDescription();
    }

    if (!descriptions.empty()) {
      if (!result.empty()) {
        result+=" ";
      }

      result+="with";

      for (std::list<NameDescriptionRef>::const_iterator desc=descriptions.begin();
          desc!=descriptions.end();
          ++desc) {
        result+=" "+(*desc)->GetDescription();
      }
    }

    if (!descriptions.empty()) {
      if (!result.empty()) {
        result+=" ";
      }
      result+=NumberToString(exitCount)+ " exits";
    }

    return "Crossing: "+result;
  }

  RouteDescription::TurnDescription::Move RouteDescription::TurnDescription::ConvertAngleToMove(double angle) const
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

  std::string RouteDescription::TurnDescription::ConvertMoveToString(Move move) const
  {
    switch (move) {
    case osmscout::RouteDescription::TurnDescription::sharpLeft:
      return "Turn sharp left";
    case osmscout::RouteDescription::TurnDescription::left:
      return "Turn left";
    case osmscout::RouteDescription::TurnDescription::slightlyLeft:
      return "Turn slightly left";
    case osmscout::RouteDescription::TurnDescription::straightOn:
      return "Straight on";
    case osmscout::RouteDescription::TurnDescription::slightlyRight:
      return "Turn slightly right";
    case osmscout::RouteDescription::TurnDescription::right:
      return "Turn right";
    case osmscout::RouteDescription::TurnDescription::sharpRight:
      return "Turn sharp right";
    }

    assert(false);
  }

  RouteDescription::TurnDescription::TurnDescription(double turnAngle,
                                                     double curveAngle)
  : turnAngle(turnAngle),
    curveAngle(curveAngle)
  {
    turn=ConvertAngleToMove(turnAngle);
    curve=ConvertAngleToMove(curveAngle);
  }

  std::string RouteDescription::TurnDescription::GetDebugString() const
  {
    std::ostringstream stream;

    stream << "Turn: " << ConvertMoveToString(turn) << ", " << turnAngle << " degrees ";
    stream << "Curve: " << ConvertMoveToString(curve) << ", " << curveAngle << " degrees";

    return stream.str();
  }

  RouteDescription::Node::Node(Id currentNodeId,
                               const std::vector<Id>& ways,
                               const std::vector<Path>& paths,
                               Id pathWayId,
                               Id targetNodeId)
  : currentNodeId(currentNodeId),
    ways(ways),
    paths(paths),
    pathWayId(pathWayId),
    targetNodeId(targetNodeId),
    distance(0.0),
    time(0.0)
  {
    // no code
  }

  bool RouteDescription::Node::HasDescription(const char* name) const
  {
    OSMSCOUT_HASHMAP<std::string,DescriptionRef>::const_iterator entry;

    entry=descriptions.find(name);

    return entry!=descriptions.end() && entry->second.Valid();
  }

  RouteDescription::DescriptionRef RouteDescription::Node::GetDescription(const char* name) const
  {
    OSMSCOUT_HASHMAP<std::string,DescriptionRef>::const_iterator entry;

    entry=descriptions.find(name);

    if (entry!=descriptions.end()) {
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

  void RouteDescription::Node::SetTime(double time)
  {
    this->time=time;
  }

  void RouteDescription::Node::AddDescription(const char* name,
                                              Description* description)
  {
    descriptions[name]=description;
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

  void RouteDescription::AddNode(Id currentNodeId,
                                 const std::vector<Id>& ways,
                                 const std::vector<Path>& paths,
                                 Id pathWayId,
                                 Id targetNodeId)
  {
    nodes.push_back(Node(currentNodeId,
                         ways,
                         paths,
                         pathWayId,
                         targetNodeId));
  }
}

