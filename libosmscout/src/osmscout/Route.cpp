/*
  TravelJinni - Openstreetmap offline viewer
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

#include <osmscout/Route.h>

namespace osmscout {

  RouteData::RouteEntry::RouteEntry(Id wayId, Id nodeId)
   : wayId(wayId),
     nodeId(nodeId)
  {
    // no code
  }

  RouteData::RouteData()
  {
    // no code
  }

  void RouteData::Clear()
  {
    entries.clear();
  }

  void RouteData::AddEntry(Id wayId, Id nodeId)
  {
    entries.push_back(RouteEntry(wayId,nodeId));
  }

  RouteDescription::RouteStep::RouteStep(double distance,
                                         Action action,
                                         const std::string& name,
                                         const std::string& refName)
   : distance(distance),
     action(action),
     name(name),
     refName(refName)
  {
    // no code
  }

  RouteDescription::RouteDescription()
  {
    // no code
  }

  void RouteDescription::Clear()
  {
    steps.clear();
  }

  void RouteDescription::AddStep(double distance,
                                 Action action,
                                 const std::string& name,
                                 const std::string& refName)
  {
    steps.push_back(RouteStep(distance,action,name,refName));
  }
}

