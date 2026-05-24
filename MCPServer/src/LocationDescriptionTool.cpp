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

#include <osmscout/log/Logger.h>

#include "LocationDescriptionTool.h"
#include "LocationDescriptionMapper.h"

namespace osmscout::mcp {

  ToolResult HandleLocationDescription(unsigned int id,
                                       const nlohmann::json& arguments,
                                       const LocationDescriptionServiceRef& locationDescriptionService)
  {
    // Validate required parameters
    if (!arguments.contains("latitude") || !arguments["latitude"].is_number()) {
      ToolResult result;
      result.status = 400;
      result.body["jsonrpc"] = "2.0";
      result.body["id"] = id;
      result.body["error"]["message"] = "Missing or invalid required parameter: latitude";
      return result;
    }

    if (!arguments.contains("longitude") || !arguments["longitude"].is_number()) {
      ToolResult result;
      result.status = 400;
      result.body["jsonrpc"] = "2.0";
      result.body["id"] = id;
      result.body["error"]["message"] = "Missing or invalid required parameter: longitude";
      return result;
    }

    double latitude = arguments["latitude"];
    double longitude = arguments["longitude"];

    // Validate coordinate range
    if (latitude < -90.0 || latitude > 90.0) {
      ToolResult result;
      result.status = 400;
      result.body["jsonrpc"] = "2.0";
      result.body["id"] = id;
      result.body["error"]["message"] = "Latitude out of range (-90 to 90)";
      return result;
    }

    if (longitude < -180.0 || longitude > 180.0) {
      ToolResult result;
      result.status = 400;
      result.body["jsonrpc"] = "2.0";
      result.body["id"] = id;
      result.body["error"]["message"] = "Longitude out of range (-180 to 180)";
      return result;
    }

    osmscout::GeoCoord location(latitude, longitude);
    osmscout::LocationDescription description;

    if (!locationDescriptionService->DescribeLocation(location, description)) {
      ToolResult result;
      result.status = 500;
      result.body["jsonrpc"] = "2.0";
      result.body["id"] = id;
      result.body["error"]["message"] = "Failed to describe location";
      return result;
    }

    // Map description to JSON using the three-layer mapper
    nlohmann::json content = nlohmann::json::array();
    nlohmann::json structured;

    ToJson(description, content, structured);

    ToolResult result;
    result.status = 200;
    result.body["jsonrpc"] = "2.0";
    result.body["id"] = id;
    result.body["result"]["content"] = content;
    result.body["result"]["structuredContent"] = structured;

    return result;
  }

} // namespace osmscout::mcp