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

  RouteDescription::Description::~Description()
  {
    // no code
  }

  RouteDescription::StartDescription::StartDescription(const std::string& description)
  : description(description)
  {
    // no code
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

  std::string RouteDescription::NameDescription::GetName() const
  {
    return name;
  }

  std::string RouteDescription::NameDescription::GetRef() const
  {
    return ref;
  }

  RouteDescription::NameChangedDescription::NameChangedDescription(NameDescription* originDescription,
                                                                   NameDescription* targetDescription)
  : originDescription(originDescription),
    targetDescription(targetDescription)
  {
    // no code
  }

  RouteDescription::CrossingWaysDescription::CrossingWaysDescription(Type type,
                                                                     NameDescription* originDescription,
                                                                     NameDescription* targetDescription)
  : type(type),
    originDescription(originDescription),
    targetDescription(targetDescription)
  {
    // no code
  }

  void RouteDescription::CrossingWaysDescription::AddDescription(NameDescription* description)
  {
    descriptions.push_back(description);
  }

  RouteDescription::Node::Node(Id currentNodeId,
                               const std::vector<Id>& ways,
                               Id pathWayId,
                               Id targetNodeId)
  : currentNodeId(currentNodeId),
    ways(ways),
    pathWayId(pathWayId),
    targetNodeId(targetNodeId),
    distance(0.0),
    time(0.0)
  {
    // no code
  }

  bool RouteDescription::Node::HasDescription(const char* name) const
  {
    std::map<std::string,DescriptionRef>::const_iterator entry;

    entry=descriptions.find(name);

    return entry!=descriptions.end() && entry->second.Valid();
  }

  RouteDescription::DescriptionRef RouteDescription::Node::GetDescription(const char* name) const
  {
    std::map<std::string,DescriptionRef>::const_iterator entry;

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

  void RouteDescription::Clear()
  {
    nodes.clear();
  }

  void RouteDescription::AddNode(Id currentNodeId,
                                 const std::vector<Id>& ways,
                                 Id pathWayId,
                                 Id targetNodeId)
  {
    nodes.push_back(Node(currentNodeId,
                         ways,
                         pathWayId,
                         targetNodeId));
  }
}

