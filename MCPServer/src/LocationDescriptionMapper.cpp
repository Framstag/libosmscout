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

#include <osmscout/location/Location.h>

#include "LocationDescriptionMapper.h"

namespace osmscout::mcp {

  // === Layer 1: Attribute mappers ===

  nlohmann::json ToJson(const GeoCoord& coord)
  {
    nlohmann::json obj;
    obj["latitude"] = coord.GetLat();
    obj["longitude"] = coord.GetLon();
    obj["displayText"] = coord.GetDisplayText();
    return obj;
  }

  nlohmann::json ToJson(const Distance& distance)
  {
    nlohmann::json obj;
    obj["value"] = distance.AsMeter();
    obj["unit"] = "m";
    return obj;
  }

  nlohmann::json ToJson(const Bearing& bearing)
  {
    nlohmann::json obj;
    obj["degrees"] = bearing.AsDegrees();
    obj["display"] = bearing.DisplayString();
    return obj;
  }

  nlohmann::json ToJson(const Place& place)
  {
    nlohmann::json obj;

    if (auto adminRegion = place.GetAdminRegion()) {
      nlohmann::json regionObj;
      regionObj["name"] = adminRegion->name;
      if (!adminRegion->altName.empty()) {
        regionObj["altName"] = adminRegion->altName;
      }
      if (!adminRegion->postalAreas.empty()) {
        nlohmann::json postalArray = nlohmann::json::array();
        for (const auto& postal : adminRegion->postalAreas) {
          postalArray.push_back(postal.name);
        }
        regionObj["postalAreas"] = postalArray;
      }
      obj["adminRegion"] = regionObj;
    }

    if (auto postalArea = place.GetPostalArea()) {
      obj["postalArea"] = postalArea->name;
    }

    if (auto poi = place.GetPOI()) {
      obj["poi"] = poi->name;
    }

    if (auto location = place.GetLocation()) {
      obj["street"] = location->name;
    }

    if (auto address = place.GetAddress()) {
      obj["houseNumber"] = address->name;
    }

    return obj;
  }

  // === Layer 2: Struct mappers ===

  nlohmann::json ToJson(const LocationCoordDescription& desc)
  {
    return ToJson(desc.GetLocation());
  }

  nlohmann::json ToJson(const LocationAtPlaceDescription& desc)
  {
    nlohmann::json obj;

    obj["atPlace"] = desc.IsAtPlace();
    obj["distanceInMeter"] = desc.GetDistance().AsMeter();
    obj["bearing"] = desc.GetBearing().DisplayString();

    // Map the Place fields individually (no whole-struct display string)
    const Place& place = desc.GetPlace();
    if (auto adminRegion = place.GetAdminRegion()) {
      nlohmann::json regionObj;
      regionObj["name"] = adminRegion->name;
      if (!adminRegion->altName.empty()) {
        regionObj["altName"] = adminRegion->altName;
      }
      if (!adminRegion->postalAreas.empty()) {
        nlohmann::json postalArray = nlohmann::json::array();
        for (const auto& postal : adminRegion->postalAreas) {
          postalArray.push_back(postal.name);
        }
        regionObj["postalAreas"] = postalArray;
      }
      obj["adminRegion"] = regionObj;
    }

    if (auto postalArea = place.GetPostalArea()) {
      obj["postalArea"] = postalArea->name;
    }

    if (auto poi = place.GetPOI()) {
      obj["poi"] = poi->name;
    }

    if (auto location = place.GetLocation()) {
      obj["street"] = location->name;
    }

    if (auto address = place.GetAddress()) {
      obj["houseNumber"] = address->name;
    }

    return obj;
  }

  nlohmann::json ToJson(const LocationWayDescription& desc)
  {
    nlohmann::json obj;
    obj["way"] = desc.GetWay().GetDisplayString();
    obj["distanceInMeter"] = desc.GetDistance().AsMeter();
    return obj;
  }

  nlohmann::json ToJson(const LocationCrossingDescription& desc)
  {
    nlohmann::json obj;
    obj["crossingLatitude"] = desc.GetCrossing().GetLat();
    obj["crossingLongitude"] = desc.GetCrossing().GetLon();
    obj["atPlace"] = desc.IsAtPlace();
    obj["distanceInMeter"] = desc.GetDistance().AsMeter();
    obj["bearing"] = desc.GetBearing().DisplayString();

    nlohmann::json waysArray = nlohmann::json::array();
    for (const auto& way : desc.GetWays()) {
      waysArray.push_back(way.GetDisplayString());
    }
    obj["ways"] = waysArray;

    return obj;
  }

  nlohmann::json ToJson(const LocationHighwayMilestoneDescription& desc)
  {
    nlohmann::json obj;
    obj["atPlace"] = desc.IsAtPlace();
    obj["distanceInMeter"] = desc.GetDistance().AsMeter();
    obj["bearing"] = desc.GetBearing().DisplayString();
    obj["milestoneDistance"] = desc.GetMilestoneDistance();
    obj["milestoneRef"] = desc.GetMilestoneRef();
    obj["carriagewayRef"] = desc.GetMilestoneCarriagewayRef();
    return obj;
  }

  // === Layer 3: Payload mapper ===

  void ToJson(const LocationDescription& desc,
              nlohmann::json& content,
              nlohmann::json& structured)
  {
    size_t contentIdx = 0;

    if (auto coordDesc = desc.GetCoordDescription()) {
      content[contentIdx]["type"] = "text";
      content[contentIdx]["text"] = coordDesc->GetLocation().GetDisplayText();
      structured["coordinateDescription"] = ToJson(*coordDesc);
      contentIdx++;
    }

    if (auto atNameDesc = desc.GetAtNameDescription()) {
      content[contentIdx]["type"] = "text";
      content[contentIdx]["text"] = atNameDesc->GetPlace().GetDisplayString();
      structured["atNameDescription"] = ToJson(*atNameDesc);
      contentIdx++;
    }

    if (auto atAddressDesc = desc.GetAtAddressDescription()) {
      content[contentIdx]["type"] = "text";
      content[contentIdx]["text"] = atAddressDesc->GetPlace().GetDisplayString();
      structured["atAddressDescription"] = ToJson(*atAddressDesc);
      contentIdx++;
    }

    if (auto atPOIDesc = desc.GetAtPOIDescription()) {
      content[contentIdx]["type"] = "text";
      content[contentIdx]["text"] = atPOIDesc->GetPlace().GetDisplayString();
      structured["atPOIDescription"] = ToJson(*atPOIDesc);
      contentIdx++;
    }

    if (auto wayDesc = desc.GetWayDescription()) {
      content[contentIdx]["type"] = "text";
      content[contentIdx]["text"] = wayDesc->GetWay().GetDisplayString();
      structured["wayDescription"] = ToJson(*wayDesc);
      contentIdx++;
    }

    if (auto crossingDesc = desc.GetCrossingDescription()) {
      content[contentIdx]["type"] = "text";
      // Build human-readable text from individual fields
      std::string crossingText = "At crossing of";
      bool first = true;
      for (const auto& way : crossingDesc->GetWays()) {
        if (!first) {
          crossingText += " and";
        }
        crossingText += " " + way.GetDisplayString();
        first = false;
      }
      content[contentIdx]["text"] = crossingText;
      structured["crossingDescription"] = ToJson(*crossingDesc);
      contentIdx++;
    }

    if (auto hmDesc = desc.GetHighwayMilestoneDescription()) {
      content[contentIdx]["type"] = "text";
      content[contentIdx]["text"] = "Highway milestone " + hmDesc->GetMilestoneRef();
      structured["highwayMilestoneDescription"] = ToJson(*hmDesc);
      contentIdx++;
    }
  }

} // namespace osmscout::mcp