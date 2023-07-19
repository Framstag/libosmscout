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

#include <osmscoutclient/MapProvider.h>

#include <osmscoutclient/json/json.hpp>

namespace osmscout {

MapProvider MapProvider::fromJson(const nlohmann::json &obj)
{
  if (!obj.is_object()) {
    return MapProvider();
  }

  auto name = obj["name"];
  auto uri = obj["uri"];
  auto listUri = obj["listUri"];

  if (!(name.is_string() && uri.is_string() && listUri.is_string())){
    return MapProvider();
  }
  return MapProvider(name.get<std::string>(),
                     uri.get<std::string>(),
                     listUri.get<std::string>());
}
}
