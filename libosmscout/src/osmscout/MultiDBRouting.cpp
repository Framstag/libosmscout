/*
  This source is part of the libosmscout library
  Copyright (C) 2016  Tim Teulings
  Copyright (C) 2017  Lukas Karas

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

#include <osmscout/RoutingService.h>
#include <osmscout/MultiDBRouting.h>

#include <algorithm>

#include <osmscout/RoutingProfile.h>

#include <osmscout/system/Assert.h>

#include <osmscout/util/Geometry.h>
#include <osmscout/util/Logger.h>
#include <osmscout/util/StopClock.h>

//#define DEBUG_ROUTING

#include <iomanip>
#include <iostream>

namespace osmscout {

  MultiDBRouting::MultiDBRouting(std::vector<DatabaseRef> databases):
    isOpen(false)
  {
    for (auto &db:databases){
      this->databases[db->GetPath()]=db;
    }
  }

  MultiDBRouting::~MultiDBRouting()
  {
    Close();
  }

  bool MultiDBRouting::Open(RoutingProfileBuilder profileBuilder,
                            RouterParameter routerParameter)
  {
    if (databases.empty()){
      return false;
    }

    isOpen=true;
    for (auto &entry:databases){
      RoutingServiceRef router=std::make_shared<osmscout::RoutingService>(entry.second,
                                                      routerParameter,
                                                      osmscout::RoutingService::DEFAULT_FILENAME_BASE);
      if (!router->Open()){
        Close();
        return false;
      }
      services[entry.first]=router;
      profiles[entry.first]=profileBuilder(entry.second);
    }
    return true;
  }

  void MultiDBRouting::Close(){
    if (!isOpen){
      return;
    }
    for (auto &entry:services){
      entry.second->Close();
    }
    services.clear();
    profiles.clear();
    isOpen=false;
  }

  RoutePosition MultiDBRouting::GetClosestRoutableNode(const GeoCoord& coord,
                                                       double radius,
                                                       std::string databaseHint) const
  {
    RoutePosition position;

    // first try to find in hinted database
    auto it=services.find(databaseHint);
    if (it!=services.end()){
      position=it->second->GetClosestRoutableNode(coord,*(profiles.at(it->first)),radius);
      if (position.IsValid()){
        return position;
      }
    }

    // try all databases
    for (auto &entry:services){
      if (entry.first==databaseHint){
        continue;
      }

      position=entry.second->GetClosestRoutableNode(coord,*(profiles.at(entry.first)),radius);
      if (position.IsValid()){
        return position;
      }
    }
    return position;
  }
}
