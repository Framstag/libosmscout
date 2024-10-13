/*
  This source is part of the libosmscout library
  Copyright (C) 2024  Tim Teulings

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

#include <ranges>
#include <osmscout/description/DescriptionService.h>
#include <osmscout/feature/AccessFeature.h>
#include <osmscout/feature/AccessRestrictedFeature.h>
#include <osmscout/feature/AddressFeature.h>
#include <osmscout/feature/AdminLevelFeature.h>
#include <osmscout/feature/BrandFeature.h>
#include <osmscout/feature/BridgeFeature.h>
#include <osmscout/feature/ChargingStationFeature.h>
#include <osmscout/feature/ClockwiseDirectionFeature.h>
#include <osmscout/feature/ConstructionYearFeature.h>
#include <osmscout/feature/DestinationFeature.h>
#include <osmscout/feature/EmbankmentFeature.h>
#include <osmscout/feature/FeeFeature.h>
#include <osmscout/feature/FromToFeature.h>
#include <osmscout/feature/GradeFeature.h>
#include <osmscout/feature/IsInFeature.h>
#include <osmscout/feature/LanesFeature.h>
#include <osmscout/feature/LayerFeature.h>
#include <osmscout/feature/LocationFeature.h>
#include <osmscout/feature/MaxSpeedFeature.h>
#include <osmscout/feature/MaxStayFeature.h>
#include <osmscout/feature/NameAltFeature.h>
#include <osmscout/feature/NameFeature.h>
#include <osmscout/feature/NameShortFeature.h>
#include <osmscout/feature/NetworkFeature.h>
#include <osmscout/feature/OpeningHoursFeature.h>
#include <osmscout/feature/OperatorFeature.h>
#include <osmscout/feature/PhoneFeature.h>
#include <osmscout/feature/PostalCodeFeature.h>
#include <osmscout/feature/RefFeature.h>
#include <osmscout/feature/RoundaboutFeature.h>
#include <osmscout/feature/SidewayFeature.h>
#include <osmscout/feature/TunnelFeature.h>
#include <osmscout/feature/WebsiteFeature.h>
#include <osmscout/feature/WidthFeature.h>

namespace osmscout {
  static uint32_t CalculateCellLevel(const osmscout::GeoBox& boundingBox)
  {
    uint32_t level=25;
    while (true) {
      if (boundingBox.GetWidth()<=osmscout::cellDimension[level].width &&
          boundingBox.GetHeight()<=osmscout::cellDimension[level].height) {
        break;
          }

      if (level==0) {
        break;
      }

      level--;
    }

    return level;
  }

  DescriptionEntry::DescriptionEntry(const std::string& sectionKey,
                                     const std::string& labelKey,
                                     const std::string& value)
  : sectionKey(sectionKey),
    hasIndex(false),
    index(0),
    labelKey(labelKey),
    value(value)
  {
    // no code
  }

  DescriptionEntry::DescriptionEntry(const std::string& sectionKey,
                                     const std::string& subsectionKey,
                                     const std::string& labelKey,
                                     const std::string& value)
  : sectionKey(sectionKey),
    subsectionKey(subsectionKey),
    hasIndex(false),
    index(0),
    labelKey(labelKey),
    value(value)
  {
    // no code
  }

  DescriptionEntry::DescriptionEntry(const std::string& sectionKey,
                                     const std::string& subsectionKey,
                                     size_t index,
                                     const std::string& labelKey,
                                     const std::string& value)
: sectionKey(sectionKey),
  subsectionKey(subsectionKey),
  hasIndex(true),
  index(index),
  labelKey(labelKey),
  value(value)
  {
    // no code
  }

  ObjectDescription::ObjectDescription()
  {
    // no code
  }

  FeatureToDescriptionProcessor::~FeatureToDescriptionProcessor()
  {
    // no code
  }

  FeatureValue* FeatureToDescriptionProcessor::GetFeatureValue(const FeatureValueBuffer& buffer,
                                                               const std::string &featureName) const
  {
    if (size_t featureIdx=0;
        buffer.GetType()->GetFeature(featureName,featureIdx) &&
        buffer.HasFeature(featureIdx)) {
      return buffer.GetValue(featureIdx);
    }

    return nullptr;
  }


  const std::string GeneralDescriptionProcessor::SECTION_NAME_GENERAL = "General";

  const std::string GeneralDescriptionProcessor::LABEL_KEY_NAME_TYPE = "Type";
  const std::string GeneralDescriptionProcessor::LABEL_KEY_NAME_NAME = "Name";
  const std::string GeneralDescriptionProcessor::LABEL_KEY_NAME_NAME_ALT = "NameAlt";
  const std::string GeneralDescriptionProcessor::LABEL_KEY_NAME_NAME_SHORT = "NameShort";
  const std::string GeneralDescriptionProcessor::LABEL_KEY_NAME_NAME_REF = "NameRef";
  const std::string GeneralDescriptionProcessor::LABEL_KEY_NAME_NAME_CONSTRUCTIONYEAR = "ConstructionYear";

  void GeneralDescriptionProcessor::Process(const FeatureValueBuffer &buffer, ObjectDescription &description)
  {
    description.AddEntry(DescriptionEntry(SECTION_NAME_GENERAL,
                                      LABEL_KEY_NAME_TYPE,
                                      buffer.GetType()->GetName()));

    if (const auto* value=dynamic_cast<NameFeatureValue*>(GetFeatureValue(buffer,NameFeature::NAME));
        value!=nullptr) {
      description.AddEntry(DescriptionEntry(SECTION_NAME_GENERAL,
                                            LABEL_KEY_NAME_NAME,
                                            value->GetName()));
    }

    if (const auto* value=dynamic_cast<NameAltFeatureValue*>(GetFeatureValue(buffer,NameAltFeature::NAME));
        value!=nullptr) {
      description.AddEntry(DescriptionEntry(SECTION_NAME_GENERAL,
                                            LABEL_KEY_NAME_NAME_ALT,
                                            value->GetNameAlt()));
    }

    if (const auto* value=dynamic_cast<NameShortFeatureValue*>(GetFeatureValue(buffer,NameShortFeature::NAME));
        value!=nullptr) {
      description.AddEntry(DescriptionEntry(SECTION_NAME_GENERAL,
                                            LABEL_KEY_NAME_NAME_SHORT,
                                            value->GetNameShort()));
    }

    if (const auto* value=dynamic_cast<RefFeatureValue*>(GetFeatureValue(buffer,RefFeature::NAME));
        value!=nullptr) {
      description.AddEntry(DescriptionEntry(SECTION_NAME_GENERAL,
                                            LABEL_KEY_NAME_NAME_REF,
                                            value->GetRef()));
    }

    if (const auto* value=dynamic_cast<ConstructionYearFeatureValue*>(GetFeatureValue(buffer,ConstructionYearFeature::NAME));
        value!=nullptr) {
      std::string text;

      if (value->GetStartYear()==value->GetEndYear()) {
        text=std::to_string(value->GetStartYear());
      }
      else {
        text=std::to_string(value->GetStartYear())+"-"+std::to_string(value->GetEndYear());
      }

      description.AddEntry(DescriptionEntry(SECTION_NAME_GENERAL,
                                            LABEL_KEY_NAME_NAME_CONSTRUCTIONYEAR,
                                            text));
    }
  }

  const std::string GeometryDescriptionProcessor::SECTION_NAME_GEOMETRY = "Geometry";

  const std::string GeometryDescriptionProcessor::LABEL_KEY_GEOMETRY_COORDINATE = "Coordinate";
  const std::string GeometryDescriptionProcessor::LABEL_KEY_GEOMETRY_BOUNDINGBOX = "BoundingBox";
  const std::string GeometryDescriptionProcessor::LABEL_KEY_GEOMETRY_CENTER = "Center";
  const std::string GeometryDescriptionProcessor::LABEL_KEY_GEOMETRY_CELLLEVEL = "CellLevel";
  const std::string GeometryDescriptionProcessor::LABEL_KEY_GEOMETRY_LAYER = "Layer";
  const std::string GeometryDescriptionProcessor::LABEL_KEY_GEOMETRY_ISIN = "IsIn";

  void GeometryDescriptionProcessor::Process(const FeatureValueBuffer &buffer, ObjectDescription &description) {
    if (const auto *value = dynamic_cast<LayerFeatureValue *>(GetFeatureValue(buffer, BrandFeature::NAME));
      value != nullptr) {
      description.AddEntry(DescriptionEntry(SECTION_NAME_GEOMETRY,
                                            LABEL_KEY_GEOMETRY_LAYER,
                                            std::to_string(value->GetLayer())));
    }

    if (const auto *value = dynamic_cast<IsInFeatureValue*>(GetFeatureValue(buffer, IsInFeature::NAME));
      value != nullptr) {
      description.AddEntry(DescriptionEntry(SECTION_NAME_GEOMETRY,
                                            LABEL_KEY_GEOMETRY_ISIN,
                                           value->GetIsIn()));
    }
  }

  const std::string WayDescriptionProcessor::SECTION_NAME_WAY = "Way";

  const std::string WayDescriptionProcessor::SUBSECTION_NAME_WAY_LANES = "Lanes";
  const std::string WayDescriptionProcessor::SUBSECTION_NAME_WAY_SIDEWAYS = "Sideways";
  const std::string WayDescriptionProcessor::SUBSECTION_NAME_WAY_ACCESS = "Access";
  const std::string WayDescriptionProcessor::SUBSECTION_NAME_WAY_ACCESSRESTRICTED = "AccessRestricted";

  const std::string WayDescriptionProcessor::LABEL_KEY_WAY_BRIDGE = "Bridge";
  const std::string WayDescriptionProcessor::LABEL_KEY_WAY_TUNNEL = "Tunnel";
  const std::string WayDescriptionProcessor::LABEL_KEY_WAY_ROUNDABOUT = "Roundabout";
  const std::string WayDescriptionProcessor::LABEL_KEY_WAY_EMBANKMENT = "Embankment";
  const std::string WayDescriptionProcessor::LABEL_KEY_WAY_MAXSPEED = "MaxSpeed";
  const std::string WayDescriptionProcessor::LABEL_KEY_WAY_GRADE = "Grade";
  const std::string WayDescriptionProcessor::LABEL_KEY_WAY_WIDTH = "Width";
  const std::string WayDescriptionProcessor::LABEL_KEY_WAY_CLOCKWISE = "Clockwise";

  const std::string WayDescriptionProcessor::LABEL_KEY_WAY_LANES_LANES = "Lanes";
  const std::string WayDescriptionProcessor::LABEL_KEY_WAY_LANES_LANESFORWARD = "LanesForward";
  const std::string WayDescriptionProcessor::LABEL_KEY_WAY_LANES_LANESBACKWARD = "LanesBackward";
  const std::string WayDescriptionProcessor::LABEL_KEY_WAY_LANES_TURNFORWARD = "TurnForward";
  const std::string WayDescriptionProcessor::LABEL_KEY_WAY_LANES_TURNBACKWARD = "TurnBackward";
  const std::string WayDescriptionProcessor::LABEL_KEY_WAY_LANES_DESTINATIONFORWARD = "DestinationForward";
  const std::string WayDescriptionProcessor::LABEL_KEY_WAY_LANES_DESTINATIONBACKWARD = "DestinationBackward";

  const std::string WayDescriptionProcessor::LABEL_KEY_WAY_SIDEWAYS_CYCLELANE = "CycleLane";
  const std::string WayDescriptionProcessor::LABEL_KEY_WAY_SIDEWAYS_CYCLETRACK = "CycleTrack";
  const std::string WayDescriptionProcessor::LABEL_KEY_WAY_SIDEWAYS_WALKTRACK = "WalkTrack";

  const std::string WayDescriptionProcessor::LABEL_KEY_WAY_ACCESS_ONEWAY = "Oneway";
  const std::string WayDescriptionProcessor::LABEL_KEY_WAY_ACCESS_FOOT = "Foot";
  const std::string WayDescriptionProcessor::LABEL_KEY_WAY_ACCESS_BICYCLE = "Bicycle";
  const std::string WayDescriptionProcessor::LABEL_KEY_WAY_ACCESS_CAR = "Car";

  const std::string WayDescriptionProcessor::LABEL_KEY_WAY_ACCESSRESTRICTED_FOOT = "Foot";
  const std::string WayDescriptionProcessor::LABEL_KEY_WAY_ACCESSRESTRICTED_BICYCLE = "Bicycle";
  const std::string WayDescriptionProcessor::LABEL_KEY_WAY_ACCESSRESTRICTED_CAR = "Car";

  void WayDescriptionProcessor::HandlesLanesFeature(const FeatureValueBuffer &buffer, ObjectDescription &description) {
    if (const auto* value=dynamic_cast<LanesFeatureValue*>(GetFeatureValue(buffer,LanesFeature::NAME));
      value!=nullptr) {
      if (value->HasSingleLane()) {
        description.AddEntry(DescriptionEntry(SECTION_NAME_WAY,
                                              SUBSECTION_NAME_WAY_LANES,
                                              LABEL_KEY_WAY_LANES_LANES,
                                              "1"));
      }
      else {
        description.AddEntry(DescriptionEntry(SECTION_NAME_WAY,
                                              SUBSECTION_NAME_WAY_LANES,
                                              LABEL_KEY_WAY_LANES_LANESFORWARD,
                                              std::to_string(value->GetForwardLanes())));

        description.AddEntry(DescriptionEntry(SECTION_NAME_WAY,
                                              SUBSECTION_NAME_WAY_LANES,
                                              LABEL_KEY_WAY_LANES_LANESBACKWARD,
                                              std::to_string(value->GetBackwardLanes())));
      }

      if (!value->GetTurnForward().empty()) {
        std::string text;

        for (const auto& turn : value->GetTurnForward() |
                                std::views::transform([](const auto& turn) {return LaneTurnString(turn);})) {
          if (!text.empty()) {
            text+=" "+turn;
          }
          else {
            text=turn;
          }
        }

        description.AddEntry(DescriptionEntry(SECTION_NAME_WAY,
                                              SUBSECTION_NAME_WAY_LANES,
                                              LABEL_KEY_WAY_LANES_TURNFORWARD,
                                              text));

      }

      if (!value->GetTurnBackward().empty()) {
        std::string text;

        for (const auto& turn : value->GetTurnBackward() |
                                std::views::transform([](const auto& turn) {return LaneTurnString(turn);})) {
          if (!text.empty()) {
            text+=" "+turn;
          }
          else {
            text=turn;
          }
        }

        description.AddEntry(DescriptionEntry(SECTION_NAME_WAY,
                                              SUBSECTION_NAME_WAY_LANES,
                                              LABEL_KEY_WAY_LANES_TURNBACKWARD,
                                              text));

      }

      if (!value->GetDestinationForward().empty()) {
        description.AddEntry(DescriptionEntry(SECTION_NAME_WAY,
                                              SUBSECTION_NAME_WAY_LANES,
                                              LABEL_KEY_WAY_LANES_DESTINATIONFORWARD,
                                              value->GetDestinationForward()));
      }

      if (!value->GetDestinationBackward().empty()) {
        description.AddEntry(DescriptionEntry(SECTION_NAME_WAY,
                                              SUBSECTION_NAME_WAY_LANES,
                                              LABEL_KEY_WAY_LANES_DESTINATIONBACKWARD,
                                              value->GetDestinationBackward()));
      }
    }
  }

  void WayDescriptionProcessor::HandleSidewayFeature(const FeatureValueBuffer &buffer, ObjectDescription &description) {
    if (const auto* value=dynamic_cast<SidewayFeatureValue*>(GetFeatureValue(buffer,SidewayFeature::NAME));
      value!=nullptr) {
      if (value->HasCyclewayLaneLeft() || value->HasCyclewayLaneRight()) {
        std::string text;

        if (value->HasCyclewayLaneLeft() && value->HasCyclewayLaneRight()) {
          text="both";
        }
        else if (value->HasCyclewayLaneLeft()) {
          text="left";
        }
        else {
          text="right";
        }

        description.AddEntry(DescriptionEntry(SECTION_NAME_WAY,
                                              SUBSECTION_NAME_WAY_SIDEWAYS,
                                              LABEL_KEY_WAY_SIDEWAYS_CYCLELANE,
                                              text));
      }

      if (value->HasCyclewayTrackLeft() || value->HasCyclewayTrackRight()) {
        std::string text;

        if (value->HasCyclewayTrackLeft() && value->HasCyclewayTrackRight()) {
          text="both";
        }
        else if (value->HasCyclewayTrackLeft()) {
          text="left";
        }
        else {
          text="right";
        }

        description.AddEntry(DescriptionEntry(SECTION_NAME_WAY,
                                              SUBSECTION_NAME_WAY_SIDEWAYS,
                                              LABEL_KEY_WAY_SIDEWAYS_CYCLETRACK,
                                              text));
      }

      if (value->HasSidewalkTrackLeft() || value->HasSidewalkTrackRight()) {
        std::string text;

        if (value->HasSidewalkTrackLeft() && value->HasSidewalkTrackRight()) {
          text="both";
        }
        else if (value->HasSidewalkTrackLeft()) {
          text="left";
        }
        else {
          text="right";
        }

        description.AddEntry(DescriptionEntry(SECTION_NAME_WAY,
                                              SUBSECTION_NAME_WAY_SIDEWAYS,
                                              LABEL_KEY_WAY_SIDEWAYS_WALKTRACK,
                                              text));
      }
    }
  }

  void WayDescriptionProcessor::HandleAccessFeature(const FeatureValueBuffer &buffer, ObjectDescription &description) {
    if (const auto* value=dynamic_cast<AccessFeatureValue*>(GetFeatureValue(buffer,AccessFeature::NAME));
      value!=nullptr) {
      std::string onewayText;
      std::string footText;
      std::string bicycleText;
      std::string carText;

      if (value->IsOnewayForward()) {
        onewayText="forward";
      }
      else if (value->IsOnewayBackward()) {
        onewayText="backward";
      }

      if (value->CanRouteFootForward() && value->CanRouteFootBackward()) {
        footText="both";
      }
      else if (value->CanRouteFootForward()) {
        footText="forward";
      }
      else if (value->CanRouteFootBackward()) {
        footText="backward";
      }

      if (value->CanRouteBicycleForward() && value->CanRouteBicycleBackward()) {
        bicycleText="both";
      }
      else if (value->CanRouteBicycleForward()) {
        bicycleText="forward";
      }
      else if (value->CanRouteBicycleBackward()) {
        bicycleText="backward";
      }

      if (value->CanRouteCarForward() && value->CanRouteCarBackward()) {
        carText="both";
      }
      else if (value->CanRouteCarForward()) {
        carText="forward";
      }
      else if (value->CanRouteCarBackward()) {
        carText="backward";
      }

      if (!onewayText.empty()) {
        description.AddEntry(DescriptionEntry(SECTION_NAME_WAY,
                                              SUBSECTION_NAME_WAY_ACCESS,
                                              LABEL_KEY_WAY_ACCESS_ONEWAY,
                                              onewayText));
      }

      if (!footText.empty()) {
        description.AddEntry(DescriptionEntry(SECTION_NAME_WAY,
                                              SUBSECTION_NAME_WAY_ACCESS,
                                              LABEL_KEY_WAY_ACCESS_FOOT,
                                              footText));
      }

      if (!bicycleText.empty()) {
        description.AddEntry(DescriptionEntry(SECTION_NAME_WAY,
                                              SUBSECTION_NAME_WAY_ACCESS,
                                              LABEL_KEY_WAY_ACCESS_BICYCLE,
                                              bicycleText));
      }

      if (!carText.empty()) {
        description.AddEntry(DescriptionEntry(SECTION_NAME_WAY,
                                              SUBSECTION_NAME_WAY_ACCESS,
                                              LABEL_KEY_WAY_ACCESS_CAR,
                                              carText));
      }
    }
  }

  void WayDescriptionProcessor::HandelAccessRestricted(const FeatureValueBuffer &buffer, ObjectDescription &description) {
    if (const auto* value=dynamic_cast<AccessRestrictedFeatureValue*>(GetFeatureValue(buffer,AccessRestrictedFeature::NAME));
      value!=nullptr) {
      if (!value->CanAccessFoot()) {
        description.AddEntry(DescriptionEntry(SECTION_NAME_WAY,
                                              SUBSECTION_NAME_WAY_ACCESSRESTRICTED,
                                              LABEL_KEY_WAY_ACCESSRESTRICTED_FOOT,
                                              "true"));
      }
      if (!value->CanAccessBicycle()) {
        description.AddEntry(DescriptionEntry(SECTION_NAME_WAY,
                                              SUBSECTION_NAME_WAY_ACCESSRESTRICTED,
                                              LABEL_KEY_WAY_ACCESSRESTRICTED_BICYCLE,
                                              "true"));
      }
      if (!value->CanAccessCar()) {
        description.AddEntry(DescriptionEntry(SECTION_NAME_WAY,
                                              SUBSECTION_NAME_WAY_ACCESSRESTRICTED,
                                              LABEL_KEY_WAY_ACCESSRESTRICTED_CAR,
                                              "true"));
      }
    }
  }

  void WayDescriptionProcessor::Process(const FeatureValueBuffer &buffer, ObjectDescription &description) {
    if (GetFeatureValue(buffer,BridgeFeature::NAME)!=nullptr) {
      description.AddEntry(DescriptionEntry(SECTION_NAME_WAY,
                                            LABEL_KEY_WAY_BRIDGE,
                                            "true"));
    }

    if (GetFeatureValue(buffer,TunnelFeature::NAME)!=nullptr) {
      description.AddEntry(DescriptionEntry(SECTION_NAME_WAY,
                                            LABEL_KEY_WAY_TUNNEL,
                                            "true"));
    }

    if (GetFeatureValue(buffer,RoundaboutFeature::NAME)!=nullptr) {
      description.AddEntry(DescriptionEntry(SECTION_NAME_WAY,
                                            LABEL_KEY_WAY_ROUNDABOUT,
                                            "true"));
    }

    if (GetFeatureValue(buffer,EmbankmentFeature::NAME)!=nullptr) {
      description.AddEntry(DescriptionEntry(SECTION_NAME_WAY,
                                            LABEL_KEY_WAY_EMBANKMENT,
                                            "true"));
    }

    if (const auto* value=dynamic_cast<MaxSpeedFeatureValue*>(GetFeatureValue(buffer,MaxSpeedFeature::NAME));
      value!=nullptr) {
      description.AddEntry(DescriptionEntry(SECTION_NAME_WAY,
                                            LABEL_KEY_WAY_MAXSPEED,
                                            std::to_string(value->GetMaxSpeed())));
    }

    if (const auto* value=dynamic_cast<GradeFeatureValue*>(GetFeatureValue(buffer,GradeFeature::NAME));
        value!=nullptr) {
      description.AddEntry(DescriptionEntry(SECTION_NAME_WAY,
                                            LABEL_KEY_WAY_GRADE,
                                            std::to_string(value->GetGrade())));
    }

    if (const auto* value=dynamic_cast<WidthFeatureValue*>(GetFeatureValue(buffer,WidthFeature::NAME));
        value!=nullptr) {
      description.AddEntry(DescriptionEntry(SECTION_NAME_WAY,
                                            LABEL_KEY_WAY_WIDTH,
                                            std::to_string(value->GetWidth())));
    }

    if (GetFeatureValue(buffer,ClockwiseDirectionFeature::NAME)!=nullptr) {
      description.AddEntry(DescriptionEntry(SECTION_NAME_WAY,
                                            LABEL_KEY_WAY_CLOCKWISE,
                                            "true"));
    }

    HandlesLanesFeature(buffer, description);
    HandleSidewayFeature(buffer, description);
    HandleAccessFeature(buffer, description);
    HandelAccessRestricted(buffer, description);
  }

  const std::string LocationDescriptionProcessor::SECTION_NAME_LOCATION = "Location";

  const std::string LocationDescriptionProcessor::SUBSECTION_NAME_LOCATION_ADMINLEVEL = "AdminLevel";

  const std::string LocationDescriptionProcessor::LABEL_KEY_LOCATION_ADDRESS = "Address";
  const std::string LocationDescriptionProcessor::LABEL_KEY_LOCATION_LOCATION = "Location";
  const std::string LocationDescriptionProcessor::LABEL_KEY_LOCATION_POSTALCODE = "PostalCode";

  const std::string LocationDescriptionProcessor::LABEL_KEY_LOCATION_ADMINLEVEL_LEVEL = "Level";
  const std::string LocationDescriptionProcessor::LABEL_KEY_LOCATION_ADMINLEVEL_ISIN = "IsIn";

  void LocationDescriptionProcessor::Process(const FeatureValueBuffer &buffer, ObjectDescription &description)
  {
    if (const auto* value=dynamic_cast<AddressFeatureValue*>(GetFeatureValue(buffer,AddressFeature::NAME));
        value!=nullptr) {
      description.AddEntry(DescriptionEntry(SECTION_NAME_LOCATION,
                                            LABEL_KEY_LOCATION_ADDRESS,
                                            value->GetAddress()));
    }

    if (const auto* value=dynamic_cast<LocationFeatureValue*>(GetFeatureValue(buffer,LocationFeature::NAME));
        value!=nullptr) {
      description.AddEntry(DescriptionEntry(SECTION_NAME_LOCATION,
                                            LABEL_KEY_LOCATION_LOCATION,
                                            value->GetLocation()));
    }

    if (const auto* value=dynamic_cast<PostalCodeFeatureValue*>(GetFeatureValue(buffer,PostalCodeFeature::NAME));
        value!=nullptr) {
      description.AddEntry(DescriptionEntry(SECTION_NAME_LOCATION,
                                            LABEL_KEY_LOCATION_POSTALCODE,
                                            value->GetPostalCode()));
    }

    if (const auto* value=dynamic_cast<AdminLevelFeatureValue*>(GetFeatureValue(buffer,AdminLevelFeature::NAME));
        value!=nullptr) {
      description.AddEntry(DescriptionEntry(SECTION_NAME_LOCATION,
                                            SUBSECTION_NAME_LOCATION_ADMINLEVEL,
                                            LABEL_KEY_LOCATION_ADMINLEVEL_LEVEL,
                                            std::to_string(value->GetAdminLevel())));

      if (!value->GetIsIn().empty()) {
        description.AddEntry(DescriptionEntry(SECTION_NAME_LOCATION,
                                              SUBSECTION_NAME_LOCATION_ADMINLEVEL,
                                              LABEL_KEY_LOCATION_ADMINLEVEL_ISIN,
                                              value->GetIsIn()));
      }
    }
  }

  const std::string RoutingDescriptionProcessor::SECTION_NAME_ROUTING = "Navigation";

  const std::string RoutingDescriptionProcessor::LABEL_KEY_ROUTING_FROM = "From";
  const std::string RoutingDescriptionProcessor::LABEL_KEY_ROUTING_TO = "To";
  const std::string RoutingDescriptionProcessor::LABEL_KEY_ROUTING_DESTINATION = "Destination";

  void RoutingDescriptionProcessor::Process(const FeatureValueBuffer &buffer, ObjectDescription &description)
  {
    if (const auto* value=dynamic_cast<FromToFeatureValue*>(GetFeatureValue(buffer,FromToFeature::NAME));
        value!=nullptr) {
      description.AddEntry(DescriptionEntry(SECTION_NAME_ROUTING,
                                            LABEL_KEY_ROUTING_FROM,
                                            value->GetFrom()));
      description.AddEntry(DescriptionEntry(SECTION_NAME_ROUTING,
                                            LABEL_KEY_ROUTING_TO,
                                            value->GetTo()));
    }

    if (const auto* value=dynamic_cast<DestinationFeatureValue*>(GetFeatureValue(buffer,DestinationFeature::NAME));
        value!=nullptr) {
      description.AddEntry(DescriptionEntry(SECTION_NAME_ROUTING,
                                            LABEL_KEY_ROUTING_DESTINATION,
                                            value->GetDestination()));
    }
  }

  const std::string CommercialDescriptionProcessor::SECTION_NAME_COMMERCIAL = "Commercial";

  const std::string CommercialDescriptionProcessor::LABEL_KEY_COMMERCIAL_BRAND = "Brand";
  const std::string CommercialDescriptionProcessor::LABEL_KEY_COMMERCIAL_OPERATOR = "Operator";
  const std::string CommercialDescriptionProcessor::LABEL_KEY_COMMERCIAL_NETWORK = "Network";

  void CommercialDescriptionProcessor::Process(const FeatureValueBuffer &buffer, ObjectDescription &description)
  {
    if (const auto* value=dynamic_cast<BrandFeatureValue*>(GetFeatureValue(buffer,BrandFeature::NAME));
        value!=nullptr) {
      description.AddEntry(DescriptionEntry(SECTION_NAME_COMMERCIAL,
                                            LABEL_KEY_COMMERCIAL_BRAND,
                                            value->GetName()));
    }

    if (const auto* value=dynamic_cast<OperatorFeatureValue*>(GetFeatureValue(buffer,OperatorFeature::NAME));
        value!=nullptr) {
      description.AddEntry(DescriptionEntry(SECTION_NAME_COMMERCIAL,
                                            LABEL_KEY_COMMERCIAL_OPERATOR,
                                            value->GetOperator()));
    }

    if (const auto* value=dynamic_cast<NetworkFeatureValue*>(GetFeatureValue(buffer,NetworkFeature::NAME));
        value!=nullptr) {
      description.AddEntry(DescriptionEntry(SECTION_NAME_COMMERCIAL,
                                            LABEL_KEY_COMMERCIAL_NETWORK,
                                            value->GetNetwork()));
    }
  }

  const std::string PaymentDescriptionProcessor::SECTION_NAME_PAYMENT = "Payment";

  const std::string PaymentDescriptionProcessor::SUBSECTION_NAME_PAYMENT_FEE = "Fee";

  const std::string PaymentDescriptionProcessor::LABEL_KEY_PAYMENT_FEE_VALUE = "Value";
  const std::string PaymentDescriptionProcessor::LABEL_KEY_PAYMENT_FEE_CONDITION = "Condition";

  void PaymentDescriptionProcessor::Process(const FeatureValueBuffer &buffer, ObjectDescription &description)
  {
    if (const auto* value=dynamic_cast<FeeFeatureValue*>(GetFeatureValue(buffer,FeeFeature::NAME));
        value!=nullptr) {
      if (value->HasCondition()) {
        description.AddEntry(DescriptionEntry(SECTION_NAME_PAYMENT,
                                              SUBSECTION_NAME_PAYMENT_FEE,
                                              LABEL_KEY_PAYMENT_FEE_CONDITION,
                                              value->GetCondition()));
      }

      description.AddEntry(DescriptionEntry(SECTION_NAME_PAYMENT,
                                            SUBSECTION_NAME_PAYMENT_FEE,
                                            LABEL_KEY_PAYMENT_FEE_VALUE,
                                            EnumToString(value->GetValue())));
    }
  }

  const std::string ChargingStationDescriptionProcessor::SECTION_NAME_CHARGINGSTATION = "ChargingStation";

  const std::string ChargingStationDescriptionProcessor::SUBSECTION_NAME_CHARGINGSTATION_SOCKET="Socket";

  const std::string ChargingStationDescriptionProcessor::LABEL_KEY_CHARGINGSTATION_SOCKET_TYPE="Type";
  const std::string ChargingStationDescriptionProcessor::LABEL_KEY_CHARGINGSTATION_SOCKET_CAPACITY="Capacity";
  const std::string ChargingStationDescriptionProcessor::LABEL_KEY_CHARGINGSTATION_SOCKET_OUTPUT="Output";

  void ChargingStationDescriptionProcessor::Process(const FeatureValueBuffer &buffer, ObjectDescription &description) {
    if (const auto* value=dynamic_cast<ChargingStationFeatureValue*>(GetFeatureValue(buffer,ChargingStationFeature::NAME));
        value!=nullptr) {
      if (value->HasSockets()) {
        size_t index=0;

        for (const auto& socket : value->GetSockets()) {
          description.AddEntry(DescriptionEntry(SECTION_NAME_CHARGINGSTATION,
                                                SUBSECTION_NAME_CHARGINGSTATION_SOCKET,
                                                index,
                                                LABEL_KEY_CHARGINGSTATION_SOCKET_TYPE,
                                                EnumToString(socket.type)));

          if (socket.capacity!=0) {
            description.AddEntry(DescriptionEntry(SECTION_NAME_CHARGINGSTATION,
                                                  SUBSECTION_NAME_CHARGINGSTATION_SOCKET,
                                                  index,
                                                  LABEL_KEY_CHARGINGSTATION_SOCKET_CAPACITY,
                                                  std::to_string(socket.capacity)));
          }

          if (!socket.output.empty()) {
            description.AddEntry(DescriptionEntry(SECTION_NAME_CHARGINGSTATION,
                                                  SUBSECTION_NAME_CHARGINGSTATION_SOCKET,
                                                  index,
                                                  LABEL_KEY_CHARGINGSTATION_SOCKET_OUTPUT,
                                                  socket.output));
          }
        }
      }
    }
  }

  const std::string PresenceDescriptionProcessor::SECTION_NAME_PRESENCE = "Presence";

  const std::string PresenceDescriptionProcessor::SUBSECTION_NAME_PRESENCE_MAXSTAY="MaxStay";

  const std::string PresenceDescriptionProcessor::LABEL_KEY_PRESENCE_MAXSTAY_CONDITION="Condition";
  const std::string PresenceDescriptionProcessor::LABEL_KEY_PRESENCE_MAXSTAY_VALUE="Value";

  const std::string PresenceDescriptionProcessor::LABEL_KEY_PRESENCE_OPENINGHOURS="OpeningHours";

  void PresenceDescriptionProcessor::Process(const FeatureValueBuffer &buffer, ObjectDescription &description) {
    if (const auto* value=dynamic_cast<MaxStayFeatureValue*>(GetFeatureValue(buffer,MaxStayFeature::NAME));
        value!=nullptr) {
      if (value->HasCondition()) {
        description.AddEntry(DescriptionEntry(SECTION_NAME_PRESENCE,
                                              SUBSECTION_NAME_PRESENCE_MAXSTAY,
                                              LABEL_KEY_PRESENCE_MAXSTAY_CONDITION,
                                              value->GetCondition()));
      }

      description.AddEntry(DescriptionEntry(SECTION_NAME_PRESENCE,
                                            SUBSECTION_NAME_PRESENCE_MAXSTAY,
                                            LABEL_KEY_PRESENCE_MAXSTAY_VALUE,
                                            value->GetValue()));
    }

    if (const auto* value=dynamic_cast<OpeningHoursFeatureValue*>(GetFeatureValue(buffer,OpeningHoursFeature::NAME));
      value!=nullptr) {
      description.AddEntry(DescriptionEntry(SECTION_NAME_PRESENCE,
                                            LABEL_KEY_PRESENCE_OPENINGHOURS,
                                            value->GetValue()));
    }
  }

  const std::string ContactDescriptionProcessor::SECTION_NAME_CONTACT = "Contact";

  const std::string ContactDescriptionProcessor::LABEL_KEY_CONTACT_PHONE="Phone";
  const std::string ContactDescriptionProcessor::LABEL_KEY_CONTACT_WEBSIZE="Website";

  void ContactDescriptionProcessor::Process(const FeatureValueBuffer &buffer, ObjectDescription &description) {
    if (const auto* value=dynamic_cast<PhoneFeatureValue*>(GetFeatureValue(buffer,PhoneFeature::NAME));
        value!=nullptr) {
      description.AddEntry(DescriptionEntry(SECTION_NAME_CONTACT,
                                            LABEL_KEY_CONTACT_PHONE,
                                            value->GetPhone()));
    }

    if (const auto* value=dynamic_cast<WebsiteFeatureValue*>(GetFeatureValue(buffer,WebsiteFeature::NAME));
        value!=nullptr) {
      description.AddEntry(DescriptionEntry(SECTION_NAME_CONTACT,
                                            LABEL_KEY_CONTACT_WEBSIZE,
                                            value->GetWebsite()));
    }
  }

  DescriptionService::DescriptionService()
  {
    featureProcessors.push_back(std::make_shared<GeneralDescriptionProcessor>());
    featureProcessors.push_back(std::make_shared<GeometryDescriptionProcessor>());
    featureProcessors.push_back(std::make_shared<WayDescriptionProcessor>());
    featureProcessors.push_back(std::make_shared<LocationDescriptionProcessor>());
    featureProcessors.push_back(std::make_shared<RoutingDescriptionProcessor>());
    featureProcessors.push_back(std::make_shared<CommercialDescriptionProcessor>());
    featureProcessors.push_back(std::make_shared<PresenceDescriptionProcessor>());
    featureProcessors.push_back(std::make_shared<ContactDescriptionProcessor>());
    featureProcessors.push_back(std::make_shared<PaymentDescriptionProcessor>());
    featureProcessors.push_back(std::make_shared<ChargingStationDescriptionProcessor>());
  }

  void DescriptionService::GetDescription(const FeatureValueBuffer& buffer,
                                          ObjectDescription& description) const
  {
    for (const auto& processor : featureProcessors) {
      processor->Process(buffer,description);
    }
  }

  ObjectDescription DescriptionService::GetDescription(const FeatureValueBuffer& buffer) const
  {
    ObjectDescription  description;

    GetDescription(buffer,description);

    return description;
  }

  ObjectDescription DescriptionService::GetDescription(const Area& area) const
  {
    ObjectDescription  description;

    osmscout::GeoBox boundingBox=area.GetBoundingBox();

    description.AddEntry(DescriptionEntry(SECTION_NAME_ID,
                              LABEL_KEY_ID_KIND,
                              area.GetObjectFileRef().GetTypeName()));

    description.AddEntry(DescriptionEntry(SECTION_NAME_ID,
                          LABEL_KEY_ID_ID,
                          std::to_string(area.GetObjectFileRef().GetFileOffset())));

    description.AddEntry(DescriptionEntry(GeometryDescriptionProcessor::SECTION_NAME_GEOMETRY,
                                  GeometryDescriptionProcessor::LABEL_KEY_GEOMETRY_BOUNDINGBOX,
                                  boundingBox.GetDisplayText()));

    description.AddEntry(DescriptionEntry(GeometryDescriptionProcessor::SECTION_NAME_GEOMETRY,
                              GeometryDescriptionProcessor::LABEL_KEY_GEOMETRY_CENTER,
                              boundingBox.GetCenter().GetDisplayText()));

    description.AddEntry(DescriptionEntry(GeometryDescriptionProcessor::SECTION_NAME_GEOMETRY,
                              GeometryDescriptionProcessor::LABEL_KEY_GEOMETRY_CELLLEVEL,
                              std::to_string(CalculateCellLevel(boundingBox))));

    GetDescription(area.GetFeatureValueBuffer(),description);

    return description;
  }

  ObjectDescription DescriptionService::GetDescription(const Way& way) const
  {
    ObjectDescription description;

    description.AddEntry(DescriptionEntry(SECTION_NAME_ID,
                              LABEL_KEY_ID_KIND,
                              way.GetObjectFileRef().GetTypeName()));

    description.AddEntry(DescriptionEntry(SECTION_NAME_ID,
                          LABEL_KEY_ID_ID,
                          std::to_string(way.GetObjectFileRef().GetFileOffset())));

    osmscout::GeoBox boundingBox=way.GetBoundingBox();

    description.AddEntry(DescriptionEntry(GeometryDescriptionProcessor::SECTION_NAME_GEOMETRY,
                                          GeometryDescriptionProcessor::LABEL_KEY_GEOMETRY_BOUNDINGBOX,
                                          boundingBox.GetDisplayText()));

    description.AddEntry(DescriptionEntry(GeometryDescriptionProcessor::SECTION_NAME_GEOMETRY,
                              GeometryDescriptionProcessor::LABEL_KEY_GEOMETRY_CENTER,
                              boundingBox.GetCenter().GetDisplayText()));

    description.AddEntry(DescriptionEntry(GeometryDescriptionProcessor::SECTION_NAME_GEOMETRY,
                              GeometryDescriptionProcessor::LABEL_KEY_GEOMETRY_CELLLEVEL,
                              std::to_string(CalculateCellLevel(boundingBox))));

    GetDescription(way.GetFeatureValueBuffer(),description);

    return description;
  }

  const std::string DescriptionService::SECTION_NAME_ID = "Id";
  const std::string DescriptionService::LABEL_KEY_ID_KIND = "Kind";
  const std::string DescriptionService::LABEL_KEY_ID_ID = "Id";

  ObjectDescription DescriptionService::GetDescription(const Node& node) const
  {
    ObjectDescription description;

    description.AddEntry(DescriptionEntry(SECTION_NAME_ID,
                          LABEL_KEY_ID_KIND,
                          node.GetObjectFileRef().GetTypeName()));

    description.AddEntry(DescriptionEntry(SECTION_NAME_ID,
                          LABEL_KEY_ID_ID,
                          std::to_string(node.GetObjectFileRef().GetFileOffset())));

    description.AddEntry(DescriptionEntry(GeometryDescriptionProcessor::SECTION_NAME_GEOMETRY,
                                      GeometryDescriptionProcessor::LABEL_KEY_GEOMETRY_COORDINATE,
                                      node.GetCoords().GetDisplayText()));

    GetDescription(node.GetFeatureValueBuffer(),description);

    return description;
  }
}
