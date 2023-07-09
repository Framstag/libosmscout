/*
  This source is part of the libosmscout-map library
  Copyright (C) 2016  Lukáš Karas

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

#include <osmscoutclient/OnlineTileProvider.h>

#include <osmscoutclient/json/json.hpp>

namespace osmscout {

OnlineTileProvider OnlineTileProvider::fromJson(const nlohmann::json &val)
{
  if (!val.is_object()) {
    return OnlineTileProvider();
  }

  auto id = val["id"];
  auto name = val["name"];
  auto servers = val["servers"];
  auto maximumZoomLevel = val["maximumZoomLevel"];
  auto copyright = val["copyright"];
  
  if (!(id.is_string() && name.is_string() && servers.is_array() &&
          maximumZoomLevel.is_number() && copyright.is_string())){
    return OnlineTileProvider();      
  }

  std::vector<std::string> serverList;
  for (auto serverVal: servers){
      if (serverVal.is_string()){
          serverList.push_back(serverVal.get<std::string>());
      }
  }
  if (serverList.empty()){
    return OnlineTileProvider();
  }
  
  return OnlineTileProvider(id.get<std::string>(),
                            name.get<std::string>(),
                            serverList,
                            maximumZoomLevel.get<int>(),
                            copyright.get<std::string>());
}
}
