#ifndef OSMSCOUT_MCP_LOCATIONDESCRIPTIONMAPPER_H
#define OSMSCOUT_MCP_LOCATIONDESCRIPTIONMAPPER_H

/*
  MCPServer - a demo program for libosmscout
  Copyright (C) 2025  Tim Teulings

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

#include <osmscout/location/LocationDescriptionService.h>

#include <nlohmann/json.hpp>

namespace osmscout::mcp {

  // === Layer 1: Attribute mappers (reusable across types) ===

  nlohmann::json ToJson(const GeoCoord& coord);
  nlohmann::json ToJson(const Distance& distance);
  nlohmann::json ToJson(const Bearing& bearing);
  nlohmann::json ToJson(const Place& place);

  // === Layer 2: Struct mappers (compose attribute mappers) ===

  nlohmann::json ToJson(const LocationCoordDescription& desc);
  nlohmann::json ToJson(const LocationAtPlaceDescription& desc);
  nlohmann::json ToJson(const LocationWayDescription& desc);
  nlohmann::json ToJson(const LocationCrossingDescription& desc);
  nlohmann::json ToJson(const LocationHighwayMilestoneDescription& desc);

  // === Layer 3: Payload mapper (assembles final result) ===

  void ToJson(const LocationDescription& desc,
              nlohmann::json& content,
              nlohmann::json& structured);

} // namespace osmscout::mcp

#endif // OSMSCOUT_MCP_LOCATIONDESCRIPTIONMAPPER_H