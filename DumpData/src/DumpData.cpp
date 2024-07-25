/*
  DumpData - a demo program for libosmscout
  Copyright (C) 2012  Tim Teulings

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

#include <cstring>
#include <iostream>
#include <list>
#include <map>
#include <string>
#include <vector>

#include <osmscout/db/Database.h>
#include <osmscout/db/DebugDatabase.h>

#include <osmscout/routing/RouteNode.h>

#include <osmscout/feature/AccessFeature.h>
#include <osmscout/feature/AccessRestrictedFeature.h>
#include <osmscout/feature/AddressFeature.h>
#include <osmscout/feature/AdminLevelFeature.h>
#include <osmscout/feature/BrandFeature.h>
#include <osmscout/feature/ChargingStationFeature.h>
#include <osmscout/feature/FeeFeature.h>
#include <osmscout/feature/IsInFeature.h>
#include <osmscout/feature/LanesFeature.h>
#include <osmscout/feature/LayerFeature.h>
#include <osmscout/feature/LocationFeature.h>
#include <osmscout/feature/MaxStayFeature.h>
#include <osmscout/feature/NameFeature.h>
#include <osmscout/feature/NameAltFeature.h>
#include <osmscout/feature/RefFeature.h>
#include <osmscout/feature/SidewayFeature.h>
#include <osmscout/feature/WidthFeature.h>

#include <osmscout/routing/RouteNodeDataFile.h>
#include <osmscout/routing/RoutingService.h>

/*
 * Example:
 *   src/DumpData ../TravelJinni/ -n 25293125 -w 4290108 -w 26688152 -r 531985
 */

struct Job
{
  osmscout::ObjectOSMRef  osmRef;
  osmscout::ObjectFileRef fileRef;

  Job() = default;

  Job(osmscout::OSMRefType type, osmscout::Id id)
  : osmRef(id,type)
  {
    // no code
  }

  Job(osmscout::RefType type, osmscout::FileOffset fileOffset)
  : fileRef(fileOffset,type)
  {
    // no code
  }
};

static const size_t IDENT=2;

static bool ParseArguments(int argc,
                           char* argv[],
                           std::string& map,
                           std::set<osmscout::OSMId>& coordIds,
                           std::set<osmscout::OSMId>& routeNodeCoordIds,
                           std::set<osmscout::Id>& routeNodeIds,
                           std::list<Job>& jobs)
{
  if (argc<2) {
    std::cerr << "DumpData <map directory> {Search arguments}" << std::endl;
    std::cerr << std::endl;
    std::cerr << "Search arguments:" << std::endl;
    std::cerr << "   -c  <OSMId>        OSM coord ids" << std::endl;
    std::cerr << std::endl;
    std::cerr << "   -n  <OSMId>        OSM node id" << std::endl;
    std::cerr << "   -no <FileOffset>   osmscout node file offset" << std::endl;
    std::cerr << std::endl;
    std::cerr << "   -w  <OSMId>        OSM way id" << std::endl;
    std::cerr << "   -wo <FileOffset>   osmscout way file offset" << std::endl;
    std::cerr << std::endl;
    std::cerr << "   -r  <OSMId>        OSM relation id" << std::endl;
    std::cerr << "   -ao <FileOffset>   osmscout area file offset" << std::endl;
    std::cerr << std::endl;
    std::cerr << "   -rn <OSMId>        route node by OSM node id" << std::endl;
    std::cerr << "   -ri <RouteNodeId>  osmscout route node id" << std::endl;
    return false;
  }

  int arg=1;

  map=argv[arg];

  arg++;

  while (arg<argc) {
    if (strcmp(argv[arg],"-c")==0) {
      long id;

      arg++;
      if (arg>=argc) {
        std::cerr << "Option -c requires parameter!" << std::endl;
        return false;
      }

      if (sscanf(argv[arg],"%ld",&id)!=1) {
        std::cerr << "Node id is not numeric!" << std::endl;
        return false;
      }

      coordIds.insert(id);

      arg++;
    }

    //
    // OSM types (nodes, ways, relations)
    //

    else if (strcmp(argv[arg],"-n")==0) {
      unsigned long id;

      arg++;
      if (arg>=argc) {
        std::cerr << "Option -n requires parameter!" << std::endl;
        return false;
      }

      if (sscanf(argv[arg],"%lu",&id)!=1) {
        std::cerr << "Node id is not numeric!" << std::endl;
        return false;
      }

      jobs.emplace_back(osmscout::osmRefNode,id);

      arg++;
    }
    else if (strcmp(argv[arg],"-w")==0) {
      unsigned long id;

      arg++;
      if (arg>=argc) {
        std::cerr << "Option -w requires parameter!" << std::endl;
        return false;
      }

      if (sscanf(argv[arg],"%lu",&id)!=1) {
        std::cerr << "Node id is not numeric!" << std::endl;
        return false;
      }

      jobs.emplace_back(osmscout::osmRefWay,id);

      arg++;
    }
    else if (strcmp(argv[arg],"-r")==0) {
      unsigned long id;

      arg++;
      if (arg>=argc) {
        std::cerr << "Option -r requires parameter!" << std::endl;
        return false;
      }

      if (sscanf(argv[arg],"%lu",&id)!=1) {
        std::cerr << "Relation id is not numeric!" << std::endl;
        return false;
      }

      jobs.emplace_back(osmscout::osmRefRelation,id);

      arg++;
    }
    else if (strcmp(argv[arg],"-rn")==0) {
      unsigned long id;

      arg++;
      if (arg>=argc) {
        std::cerr << "Option -rn requires parameter!" << std::endl;
        return false;
      }

      if (sscanf(argv[arg],"%lu",&id)!=1) {
        std::cerr << "Route node coord id is not numeric!" << std::endl;
        return false;
      }

      routeNodeCoordIds.insert(id);

      arg++;
    }
    else if (strcmp(argv[arg],"-ri")==0) {
      unsigned long id;

      arg++;
      if (arg>=argc) {
        std::cerr << "Option -ri requires parameter!" << std::endl;
        return false;
      }

      if (sscanf(argv[arg],"%lu",&id)!=1) {
        std::cerr << "Route node id is not numeric!" << std::endl;
        return false;
      }

      routeNodeIds.insert(id);

      arg++;
    }

    //
    // libosmscout types (nodes, ways, areas)
    //

    else if (strcmp(argv[arg],"-no")==0) {
      unsigned long fileOffset;

      arg++;
      if (arg>=argc) {
        std::cerr << "Option -no requires parameter!" << std::endl;
        return false;
      }

      if (sscanf(argv[arg],"%lu",&fileOffset)!=1) {
        std::cerr << "Node id is not numeric!" << std::endl;
        return false;
      }

      jobs.emplace_back(osmscout::refNode,fileOffset);

      arg++;
    }
    else if (strcmp(argv[arg],"-wo")==0) {
      unsigned long fileOffset;

      arg++;
      if (arg>=argc) {
        std::cerr << "Option -wo requires parameter!" << std::endl;
        return false;
      }

      if (sscanf(argv[arg],"%lu",&fileOffset)!=1) {
        std::cerr << "Way file offset is not numeric!" << std::endl;
        return false;
      }

      jobs.emplace_back(osmscout::refWay,fileOffset);

      arg++;
    }
    else if (strcmp(argv[arg],"-ao")==0) {
      unsigned long fileOffset;

      arg++;
      if (arg>=argc) {
        std::cerr << "Option -ao requires parameter!" << std::endl;
        return false;
      }

      if (sscanf(argv[arg],"%lu",&fileOffset)!=1) {
        std::cerr << "Area file offset is not numeric!" << std::endl;
        return false;
      }

      jobs.emplace_back(osmscout::refArea,fileOffset);

      arg++;
    }


    else {
      std::cerr << "Unknown parameter '" << argv[arg] << "'!" << std::endl;
      return false;
    }
  }

  return true;
}

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

static void DumpIndent(size_t indent)
{
  for (size_t i=1; i<=indent; i++) {
    std::cout << " ";
  }
}

static void DumpPoint(osmscout::OSMId osmId,
                      const osmscout::Point& point)
{

  std::cout << "Point {" << std::endl;
  std::cout << "  OSMId: " << osmId << std::endl;
  std::cout << "  Serial: " << point.GetSerial() << std::endl;
  std::cout << "  OSMScoutId: " << point.GetId() << std::endl;

  std::streamsize         oldPrecision=std::cout.precision(5);
  std::ios_base::fmtflags oldFlags=std::cout.setf(std::ios::fixed,std::ios::floatfield);

  std::cout << "  lat: " << point.GetCoord().GetLat() << std::endl;
  std::cout << "  lon: " << point.GetCoord().GetLon() << std::endl;

  std::cout.setf(oldFlags,std::ios::floatfield);
  std::cout.precision(oldPrecision);

  std::cout << "}" << std::endl;
}

static void DumpRouteNode(const osmscout::RouteNode& routeNode)
{
  std::cout << "RouteNode {" << std::endl;
  std::cout << "  fileOffset: " << routeNode.GetFileOffset() << std::endl;
  std::cout << "  RouteNodeId: " << routeNode.GetId() << std::endl;

  std::streamsize         oldPrecision=std::cout.precision(5);
  std::ios_base::fmtflags oldFlags=std::cout.setf(std::ios::fixed,std::ios::floatfield);

  std::cout << "  lat: " << routeNode.GetCoord().GetLat() << std::endl;
  std::cout << "  lon: " << routeNode.GetCoord().GetLon() << std::endl;

  std::cout.setf(oldFlags,std::ios::floatfield);
  std::cout.precision(oldPrecision);

  for (const auto& object : routeNode.objects) {
    std::cout << std::endl;
    std::cout << "  object {" << std::endl;
    std::cout << "    object: " << object.object.GetName() << std::endl;
    std::cout << "    variant: " << object.objectVariantIndex << std::endl;
    std::cout << "  }" << std::endl;
  }

  for (const auto& path : routeNode.paths) {
    std::cout << std::endl;
    std::cout << "  path {" << std::endl;
    std::cout << "    object: " << routeNode.objects[path.objectIndex].object.GetName() << std::endl;
    std::cout << "  }" << std::endl;
  }

  for (const auto& exclude : routeNode.excludes) {
    std::cout << std::endl;
    std::cout << "  exclude {" << std::endl;
    std::cout << "    from: " << exclude.source.GetName() << std::endl;
    std::cout << "    to: " << routeNode.objects[exclude.targetIndex].object.GetName() << std::endl;
    std::cout << "  }" << std::endl;
  }

  std::cout << "}" << std::endl;
}

static void DumpAccessFeatureValue(const osmscout::AccessFeatureValue& accessValue,
                                   size_t indent,
                                   bool defaultValue)
{
  DumpIndent(indent);

  if (defaultValue) {
    std::cout << "(Access) {" << std::endl;
  }
  else {
    std::cout << "Access {" << std::endl;
  }

  if (accessValue.IsOnewayForward()) {
    DumpIndent(indent+2);
    std::cout << "oneway: forward" << std::endl;
  }
  else if (accessValue.IsOnewayBackward()) {
    DumpIndent(indent+2);
    std::cout << "oneway: backward" << std::endl;
  }

  if (accessValue.CanRouteFootForward() && accessValue.CanRouteFootBackward()) {
    DumpIndent(indent+2);
    std::cout << "foot: both" << std::endl;
  }
  else if (accessValue.CanRouteFootForward()) {
    DumpIndent(indent+2);
    std::cout << "foot: forward" << std::endl;
  }
  else if (accessValue.CanRouteFootBackward()) {
    DumpIndent(indent+2);
    std::cout << "foot: backward" << std::endl;
  }

  if (accessValue.CanRouteBicycleForward() && accessValue.CanRouteBicycleBackward()) {
    DumpIndent(indent+2);
    std::cout << "bicycle: both" << std::endl;
  }
  else if (accessValue.CanRouteBicycleForward()) {
    DumpIndent(indent+2);
    std::cout << "bicycle: forward" << std::endl;
  }
  else if (accessValue.CanRouteBicycleBackward()) {
    DumpIndent(indent+2);
    std::cout << "bicycle: backward" << std::endl;
  }

  if (accessValue.CanRouteCarForward() && accessValue.CanRouteCarBackward()) {
    DumpIndent(indent+2);
    std::cout << "car: both" << std::endl;
  }
  else if (accessValue.CanRouteCarForward()) {
    DumpIndent(indent+2);
    std::cout << "car: forward" << std::endl;
  }
  else if (accessValue.CanRouteCarBackward()) {
    DumpIndent(indent+2);
    std::cout << "car: backward" << std::endl;
  }

  DumpIndent(indent);
  std::cout << "}" << std::endl;
}

static void DumpAccessRestrictedFeatureValue(const osmscout::AccessRestrictedFeatureValue& accessValue,
                                             size_t indent)
{
  DumpIndent(indent);
  std::cout << "AccessRestricted {" << std::endl;

  if (!accessValue.CanAccessFoot()) {
    DumpIndent(indent+2);
    std::cout << "foot: restricted" << std::endl;
  }

  if (!accessValue.CanAccessBicycle()) {
    DumpIndent(indent+2);
    std::cout << "bicycle: restricted" << std::endl;
  }

  if (!accessValue.CanAccessCar()) {
    DumpIndent(indent+2);
    std::cout << "car: restricted" << std::endl;
  }

  DumpIndent(indent);
  std::cout << "}" << std::endl;
}

static void DumpSidewayFeatureValue(const osmscout::SidewayFeatureValue& sidewayValue,
                                    size_t indent)
{
  DumpIndent(indent);
  std::cout << "Sideway {" << std::endl;

  if (sidewayValue.HasSidewalkTrackLeft() &&
      sidewayValue.HasSidewalkTrackRight()) {
    DumpIndent(indent+2);
    std::cout << "sidewalk: track both" << std::endl;
  }
  else if (sidewayValue.HasSidewalkTrackLeft()) {
    DumpIndent(indent+2);
    std::cout << "sidewalk: track left" << std::endl;
  }
  else if (sidewayValue.HasSidewalkTrackRight()) {
    DumpIndent(indent+2);
    std::cout << "sidewalk: track right" << std::endl;
  }

  if (sidewayValue.HasCyclewayLaneLeft() &&
      sidewayValue.HasCyclewayLaneRight()) {
    DumpIndent(indent+2);
    std::cout << "cycleway: lane both" << std::endl;
  }
  else if (sidewayValue.HasCyclewayLaneLeft()) {
    DumpIndent(indent+2);
    std::cout << "cycleway: lane left" << std::endl;
  }
  else if (sidewayValue.HasCyclewayLaneRight()) {
    DumpIndent(indent+2);
    std::cout << "cycleway: lane right" << std::endl;
  }

  if (sidewayValue.HasCyclewayTrackLeft() &&
      sidewayValue.HasCyclewayTrackRight()) {
    DumpIndent(indent+2);
    std::cout << "cycleway: track both" << std::endl;
  }
  else if (sidewayValue.HasCyclewayTrackLeft()) {
    DumpIndent(indent+2);
    std::cout << "cycleway: track left" << std::endl;
  }
  else if (sidewayValue.HasCyclewayTrackRight()) {
    DumpIndent(indent+2);
    std::cout << "cycleway: track right" << std::endl;
  }

  DumpIndent(indent);
  std::cout << "}" << std::endl;
}

static void DumpLanesFeatureValue(const osmscout::LanesFeatureValue& lanesValue,
                                  size_t indent)
{
  DumpIndent(indent);
  std::cout << "Lanes {" << std::endl;

  DumpIndent(indent+2);
  std::cout << "Lanes: ";
  if (lanesValue.HasSingleLane()) {
    std::cout << "1" << std::endl;
  }
  else {
    std::cout << (size_t)lanesValue.GetForwardLanes() << " " << (size_t)lanesValue.GetBackwardLanes() << std::endl;
  }

  if (!lanesValue.GetTurnForward().empty()) {
    DumpIndent(indent+2);
    std::cout << "TurnForward: ";
    for (auto turn: lanesValue.GetTurnForward()) {
      std::cout << LaneTurnString(turn);
      std::cout << " ";
    }
    std::cout << std::endl;
  }

  if (!lanesValue.GetTurnBackward().empty()) {
    DumpIndent(indent+2);
    std::cout << "TurnBackward: ";
    for (auto turn: lanesValue.GetTurnForward()) {
      std::cout << LaneTurnString(turn);
      std::cout << " ";
    }
    std::cout << std::endl;
  }

  if (!lanesValue.GetDestinationForward().empty()) {
    DumpIndent(indent+2);
    std::cout << "DestinationForward: " << lanesValue.GetDestinationForward() << std::endl;
  }

  if (!lanesValue.GetDestinationBackward().empty()) {
    DumpIndent(indent+2);
    std::cout << "DestinationBackward: " << lanesValue.GetDestinationBackward() << std::endl;
  }

  DumpIndent(indent);
  std::cout << "}" << std::endl;
}

static void DumpChargingStationFeatureValue(const osmscout::ChargingStationFeatureValue& chargingStationValue,
                                            size_t indent)
{
  DumpIndent(indent);
  std::cout << "ChargingStation {" << std::endl;

  for (const auto& socket : chargingStationValue.GetSockets()) {
    DumpIndent(indent+2);
    std::cout << "Socket {" << std::endl;

    DumpIndent(indent+4);
    std::cout << "capacity: " << (unsigned int) socket.capacity << std::endl;
    DumpIndent(indent+4);
    std::cout << "type: " << osmscout::EnumToString(socket.type) << std::endl;

    if (!socket.output.empty()) {
      DumpIndent(indent+4);
      std::cout << "output: " << socket.output << std::endl;
    }

    DumpIndent(indent+2);
    std::cout << "}" << std::endl;
  }

  DumpIndent(indent);
  std::cout << "}" << std::endl;

}

static void DumpFeatureValueBuffer(const osmscout::FeatureValueBuffer& buffer,
                                   size_t indent)
{
  for (size_t idx=0; idx<buffer.GetFeatureCount(); idx++) {
    osmscout::FeatureInstance meta=buffer.GetFeature(idx);

    if (buffer.HasFeature(idx)) {
      if (meta.GetFeature()->HasValue()) {
        osmscout::FeatureValue *value=buffer.GetValue(idx);

        if (const auto* nameValue = dynamic_cast<osmscout::NameFeatureValue*>(value);
            nameValue != nullptr) {

          DumpIndent(indent);
          std::cout << "Name: " << nameValue->GetName() << std::endl;
        }
        else if (const auto* nameAltValue = dynamic_cast<osmscout::NameAltFeatureValue*>(value);
                 nameAltValue != nullptr) {

          DumpIndent(indent);
          std::cout << "NameAlt: " << nameAltValue->GetNameAlt() << std::endl;
        }
        else if (const auto* refValue = dynamic_cast<osmscout::RefFeatureValue*>(value);
                 refValue != nullptr) {

          DumpIndent(indent);
          std::cout << "Ref: " << refValue->GetRef() << std::endl;
        }
        else if (const auto* locationValue = dynamic_cast<osmscout::LocationFeatureValue*>(value);
                 locationValue != nullptr) {

          DumpIndent(indent);
          std::cout << "Location: "<< locationValue->GetLocation() << std::endl;
        }
        else if (const auto* addressValue = dynamic_cast<osmscout::AddressFeatureValue*>(value);
                 addressValue != nullptr) {

          DumpIndent(indent);
          std::cout << "Address: " << addressValue->GetAddress() << std::endl;
        }
        else if (const auto* accessValue = dynamic_cast<osmscout::AccessFeatureValue*>(value);
                 accessValue != nullptr) {

          DumpAccessFeatureValue(*accessValue,
                                 indent,
                                 false);
        }
        else if (const auto* accessValue=dynamic_cast<osmscout::AccessRestrictedFeatureValue*>(value);
                 accessValue != nullptr) {

          DumpAccessRestrictedFeatureValue(*accessValue,
                                           indent);
        }
        else if (const auto* layerValue = dynamic_cast<osmscout::LayerFeatureValue*>(value);
                 layerValue != nullptr) {

          DumpIndent(indent);
          std::cout << "Layer: " << (int)layerValue->GetLayer() << std::endl;
        }
        else if (const auto* widthValue = dynamic_cast<osmscout::WidthFeatureValue*>(value);
                 widthValue != nullptr) {

          DumpIndent(indent);
          std::cout << "Width: " << (int)widthValue->GetWidth() << std::endl;
        }
        else if (const auto* maxSpeedValue = dynamic_cast<osmscout::MaxSpeedFeatureValue*>(value);
                 maxSpeedValue != nullptr) {

          DumpIndent(indent);
          std::cout << "MaxSpeed: " << (int)maxSpeedValue->GetMaxSpeed() << std::endl;
        }
        else if (const auto* gradeValue = dynamic_cast<osmscout::GradeFeatureValue*>(value);
                 gradeValue != nullptr) {

          DumpIndent(indent);
          std::cout << "Grade: " << (int)gradeValue->GetGrade() << std::endl;
        }
        else if (const auto* adminLevelValue = dynamic_cast<osmscout::AdminLevelFeatureValue*>(value);
                 adminLevelValue != nullptr) {

          DumpIndent(indent);
          std::cout << "AdminLevel: " << (unsigned int)adminLevelValue->GetAdminLevel();

          if (!adminLevelValue->GetIsIn().empty()) {
            std::cout << " is in " << adminLevelValue->GetIsIn();
          }

          std::cout << std::endl;
        }
        else if (const auto* isInValue = dynamic_cast<osmscout::IsInFeatureValue*>(value);
                 isInValue != nullptr) {

          DumpIndent(indent);
          std::cout << "IsIn: " << isInValue->GetIsIn() << std::endl;
        }
        else if (const auto* brandValue = dynamic_cast<osmscout::BrandFeatureValue*>(value);
                brandValue != nullptr) {

          DumpIndent(indent);
          std::cout << "Brand: " << brandValue->GetName() << std::endl;
        }
        else if (const auto* sidewayValue = dynamic_cast<osmscout::SidewayFeatureValue*>(value);
                 sidewayValue != nullptr) {

          DumpSidewayFeatureValue(*sidewayValue,indent);
        }
        else if (const auto* lanesValue = dynamic_cast<osmscout::LanesFeatureValue*>(value);
                 lanesValue != nullptr) {

          DumpLanesFeatureValue(*lanesValue,
                                indent);
        }
        else if (const auto* chargingStationValue = dynamic_cast<osmscout::ChargingStationFeatureValue*>(value);
                chargingStationValue != nullptr) {

          DumpChargingStationFeatureValue(*chargingStationValue,indent);
        }
        else if (const auto* maxStayValue = dynamic_cast<osmscout::MaxStayFeatureValue*>(value);
                maxStayValue != nullptr) {
          DumpIndent(indent);
          std::cout << "MaxStay {" << std::endl;

          DumpIndent(indent+2);
          std::cout << "value: " << maxStayValue->GetValue() << std::endl;

          if (maxStayValue->HasCondition()) {
            DumpIndent(indent+2);
            std::cout << "condition: " << maxStayValue->GetCondition() << std::endl;
          }

          DumpIndent(indent);
          std::cout << "}" << std::endl;
        }
        else if (const auto* feeValue = dynamic_cast<osmscout::FeeFeatureValue*>(value);
                feeValue != nullptr) {
          DumpIndent(indent);
          std::cout << "Fee {" << std::endl;

          DumpIndent(indent+2);
          std::cout << "value: " << EnumToString(feeValue->GetValue()) << std::endl;

          if (feeValue->HasCondition()) {
            DumpIndent(indent+2);
            std::cout << "condition: " << feeValue->GetCondition() << std::endl;
          }

          DumpIndent(indent);
          std::cout << "}" << std::endl;

        }
        else if (meta.GetFeature()->HasLabel()) {
          DumpIndent(indent);
          std::cout << meta.GetFeature()->GetName() << ": ";
          std::cout << value->GetLabel(osmscout::Locale(), 0);
          std::cout << std::endl;
        }
        else {
          DumpIndent(indent);
          std::cout << meta.GetFeature()->GetName() << ": ";
          std::cout << "<Unknown value>";
          std::cout << std::endl;
        }
      }
      // Flag-like Features
      else {
        // We are just a flag...
        DumpIndent(indent);
        std::cout << meta.GetFeature()->GetName() << ": true";
        std::cout << std::endl;
      }
    }
    // Features with default value
    else {
      if (meta.GetFeature()->GetName()==osmscout::AccessFeature::NAME) {
        osmscout::AccessFeatureValue accessValue(buffer.GetType()->GetDefaultAccess());

        DumpAccessFeatureValue(accessValue,
                               indent,
                               true);
      }
      else if (!meta.GetFeature()->HasValue()) {
        // We are just a flag...
        DumpIndent(indent);
        std::cout << "(" << meta.GetFeature()->GetName() << ")" << ": false";
        std::cout << std::endl;
      }
    }
  }
}

static void DumpNode(const osmscout::NodeRef& node,
                     osmscout::OSMId id)
{
  std::cout << "Node {" << std::endl;
  std::cout << "  OSM id: " << id << std::endl;
  std::cout << "  fileOffset: " << node->GetFileOffset() << std::endl;
  std::cout << "  type: " << node->GetType()->GetName() << std::endl;

  std::cout << std::endl;

  DumpFeatureValueBuffer(node->GetFeatureValueBuffer(),
                         IDENT);

  std::cout << std::endl;

  std::cout << "  lat: " << node->GetCoords().GetLat() << std::endl;
  std::cout << "  lon: " << node->GetCoords().GetLon() << std::endl;

  std::cout << "}" << std::endl;

}

static void DumpWay(const osmscout::WayRef& way,
                    osmscout::OSMId id)
{
  osmscout::GeoBox boundingBox=way->GetBoundingBox();

  std::cout << "Way {" << std::endl;

  std::cout << "  OSM id: " << id << std::endl;
  std::cout << "  fileOffset: " << way->GetFileOffset() << std::endl;
  std::cout << "  type: " << way->GetType()->GetName() << std::endl;
  std::cout << "  boundingBox: " << boundingBox.GetDisplayText() << std::endl;
  std::cout << "  center: " << boundingBox.GetCenter().GetDisplayText() << std::endl;
  std::cout << "  cell level: " << CalculateCellLevel(boundingBox) << std::endl;

  std::cout << std::endl;

  DumpFeatureValueBuffer(way->GetFeatureValueBuffer(),
                         IDENT);

  if (!way->nodes.empty()) {
    std::cout << std::endl;

    for (size_t n=0; n<way->nodes.size(); n++) {
      std::cout << "  node[" << n << "] {";

      if (way->GetSerial(n)!=0) {
        std::cout << " serial: " << way->GetSerial(n);
        std::cout << " id: " << way->GetId(n);
      }

      std::cout << " lat: " << way->GetCoord(n).GetLat() << " lon: "<< way->GetCoord(n).GetLon() << " }" << std::endl;
    }
  }

  std::cout << "}" << std::endl;
}

static void DumpArea(const osmscout::AreaRef& area,
                     osmscout::OSMId id)
{
  osmscout::GeoBox boundingBox=area->GetBoundingBox();

  std::cout << "Area {" << std::endl;

  std::cout << "  OSM id: " << id << std::endl;
  std::cout << "  fileOffset: " << area->GetFileOffset() << std::endl;
  std::cout << "  type: " << area->GetType()->GetName() << std::endl;
  std::cout << "  boundingBox: " << boundingBox.GetDisplayText() << std::endl;
  std::cout << "  center: " << boundingBox.GetCenter().GetDisplayText() << std::endl;
  std::cout << "  cell level: " << CalculateCellLevel(boundingBox) << std::endl;

  std::cout << std::endl;

  DumpFeatureValueBuffer(area->rings.front().GetFeatureValueBuffer(),
                         IDENT);

  for (size_t r=0; r<area->rings.size(); r++) {
    std::cout << std::endl;

    size_t ident;
    if (area->rings[r].IsMaster()) {
      ident=IDENT;
    }
    else {
      std::cout << "  role[" << r << "] {" << std::endl;
      ident=IDENT+2;
    }

    if (area->rings[r].IsMaster()) {
      DumpIndent(ident);
      std::cout << "master" << std::endl;
    }
    else if (area->rings[r].IsTopOuter()) {
      DumpIndent(ident);
      std::cout << "outer" << std::endl;
      DumpIndent(ident);
      std::cout << "type: " << area->rings[r].GetType()->GetName() << std::endl;
    }
    else {
      DumpIndent(ident);
      std::cout << "ring: " << (size_t)area->rings[r].GetRing() << std::endl;
      DumpIndent(ident);
      std::cout << "type: " << area->rings[r].GetType()->GetName() << std::endl;
    }

    if (!area->rings[r].nodes.empty()) {
      DumpIndent(ident);
      std::cout << "boundingBox: " << boundingBox.GetDisplayText() << std::endl;
      DumpIndent(ident);
      std::cout << "center of bounding box: " << boundingBox.GetCenter().GetDisplayText() << std::endl;

      if (area->rings[r].center) {
        DumpIndent(ident);
        std::cout << "visual center: " << area->rings[r].center.value().GetDisplayText() << std::endl;
      }
    }

    DumpFeatureValueBuffer(area->rings[r].GetFeatureValueBuffer(),
                           ident);

    if (!area->rings[r].nodes.empty()) {
      std::cout << std::endl;

      for (size_t n=0; n<area->rings[r].nodes.size(); n++) {
        DumpIndent(ident);
        std::cout << "node[" << n << "] {";

        if (area->rings[r].GetSerial(n)!=0) {
          std::cout << "serial: " << area->rings[r].GetSerial(n);
        }

        std::cout << " lat: " << area->rings[r].nodes[n].GetLat() << " lon: "<< area->rings[r].nodes[n].GetLon() << " }" << std::endl;
      }
    }

    if (!area->rings[r].IsMaster()) {
      ident-=2;
      DumpIndent(ident);
      std::cout << "}" << std::endl;
    }
  }

  std::cout << "}" << std::endl;
}

int main(int argc, char* argv[])
{
  std::string                    map;
  std::list<Job>                 jobs;
  std::set<osmscout::OSMId>      coordIds;

  std::set<osmscout::OSMId>      routeNodeCoordIds;
  std::set<osmscout::Id>         routeNodeIds;

  try {
    std::locale::global(std::locale(""));
  }
  catch (const std::runtime_error&) {
    std::cerr << "ERROR: Cannot set locale" << std::endl;
  }

  if (!ParseArguments(argc,
                      argv,
                      map,
                      coordIds,
                      routeNodeCoordIds,
                      routeNodeIds,
                      jobs)) {
    return 1;
  }

  osmscout::DatabaseParameter      databaseParameter;
  osmscout::Database               database(databaseParameter);
  osmscout::DebugDatabaseParameter debugDatabaseParameter;
  osmscout::DebugDatabase          debugDatabase(debugDatabaseParameter);

  osmscout::RouteNodeDataFile routeNodeDataFile(
      osmscout::RoutingService::GetDataFilename(osmscout::RoutingService::DEFAULT_FILENAME_BASE),
      1000);

  if (!database.Open(map)) {
    std::cerr << "Cannot open db" << std::endl;
  }

  if (!debugDatabase.Open(map)) {
    std::cerr << "Cannot open debug db" << std::endl;
  }

  if (!routeNodeDataFile.Open(database.GetTypeConfig(),
                              map,
                              true)) {
    std::cerr << "Cannot open routing db" << std::endl;
  }

  // OSM ids
  std::set<osmscout::ObjectOSMRef>  osmRefs;
  std::set<osmscout::ObjectFileRef> fileRefs;

  for (const auto& job : jobs) {
    switch (job.osmRef.GetType()) {
    case osmscout::osmRefNone:
      break;
    case osmscout::osmRefNode:
    case osmscout::osmRefWay:
    case osmscout::osmRefRelation:
      osmRefs.insert(job.osmRef);
      break;
    }

    switch (job.fileRef.GetType()) {
    case osmscout::refNone:
      break;
    case osmscout::refNode:
    case osmscout::refArea:
    case osmscout::refWay:
      fileRefs.insert(job.fileRef);
      break;
    }
  }

  std::multimap<osmscout::ObjectOSMRef,osmscout::ObjectFileRef> idFileOffsetMap;
  std::map<osmscout::ObjectFileRef,osmscout::ObjectOSMRef> fileOffsetIdMap;

  if (!osmRefs.empty() ||
      !fileRefs.empty()) {
    if (!debugDatabase.ResolveReferences(osmRefs,
                                         fileRefs,
                                         idFileOffsetMap,
                                         fileOffsetIdMap)) {
      std::cerr << "Error while resolving node ids and file offsets" << std::endl;
    }
  }

  osmscout::CoordDataFile::ResultMap coordsMap;
  std::vector<osmscout::NodeRef>     nodes;
  std::vector<osmscout::AreaRef>     areas;
  std::vector<osmscout::WayRef>      ways;

  osmscout::CoordDataFile::ResultMap                      routeCoordsMap;
  std::unordered_map<osmscout::Id,osmscout::RouteNodeRef> routeNodeMap;

  if (!coordIds.empty()) {

    if (!debugDatabase.GetCoords(coordIds,
                                 coordsMap)) {
      std::cerr << "Error whole loading coords by id" << std::endl;
    }
  }

  if (!routeNodeCoordIds.empty()) {

    if (!debugDatabase.GetCoords(routeNodeCoordIds,
                                 routeCoordsMap)) {
      std::cerr << "Error whole loading route node coords by id" << std::endl;
    }
  }

  if (!fileOffsetIdMap.empty()) {
    std::list<osmscout::FileOffset> offsets;

    for (std::map<osmscout::ObjectFileRef,osmscout::ObjectOSMRef>::const_iterator entry=fileOffsetIdMap.begin();
         entry!=fileOffsetIdMap.end();
         ++entry) {
      if (entry->first.GetType()==osmscout::refNode) {
        offsets.push_back(entry->first.GetFileOffset());
      }
    }

    if (!database.GetNodesByOffset(offsets,
                                   nodes)) {
      std::cerr << "Error whole loading nodes by offset" << std::endl;
    }
  }

  if (!fileOffsetIdMap.empty()) {
    std::list<osmscout::FileOffset> offsets;

    for (std::map<osmscout::ObjectFileRef,osmscout::ObjectOSMRef>::const_iterator entry=fileOffsetIdMap.begin();
         entry!=fileOffsetIdMap.end();
         ++entry) {
      if (entry->first.GetType()==osmscout::refArea) {
        offsets.push_back(entry->first.GetFileOffset());
      }
    }

    if (!database.GetAreasByOffset(offsets,
                                   areas)) {
      std::cerr << "Error whole loading areas by offset" << std::endl;
    }
  }

  if (!fileOffsetIdMap.empty()) {
    std::list<osmscout::FileOffset> offsets;

    for (const auto& entry : fileOffsetIdMap) {
      if (entry.first.GetType()==osmscout::refWay) {
        offsets.push_back(entry.first.GetFileOffset());
      }
    }

    if (!database.GetWaysByOffset(offsets,
                                  ways)) {
      std::cerr << "Error whole loading ways by offset" << std::endl;
    }
  }

  for (const auto id : routeNodeCoordIds) {
    auto coordsEntry=routeCoordsMap.find(id);

    if (coordsEntry!=routeCoordsMap.end()) {
      routeNodeIds.insert(coordsEntry->second.GetId());
    }
    else {
      std::cerr << "Cannot find route node coord with id " << id << std::endl;
    }
  }

  if (!routeNodeIds.empty() &&
      routeNodeDataFile.IsOpen()) {
    for (const osmscout::Id id:routeNodeIds){
      osmscout::RouteNodeRef node;
      if (!routeNodeDataFile.Get(id,node)) {
        std::cerr << "Error loading route nodes by id" << std::endl;
        continue;
      }
      if (!node){
        std::cerr << "Error loading route nodes by id" << std::endl;
        continue;
      }
      routeNodeMap[id]=std::move(node);
    }
  }

  bool firstCoord=true;
  for (const auto id : coordIds) {
    auto coordsEntry=coordsMap.find(id);

    if (coordsEntry!=coordsMap.end()) {
      if (!firstCoord) {
        std::cout << std::endl;
      }

      DumpPoint(coordsEntry->first,
                coordsEntry->second);
    }
    else {
      std::cerr << "Cannot find coord with id " << id << std::endl;
    }

    firstCoord=false;
  }

  bool firstRouteNode=true;
  for (const auto id : routeNodeIds) {
    auto routeNodeEntry=routeNodeMap.find(id);

    if (routeNodeEntry!=routeNodeMap.end()) {
      if (!firstRouteNode) {
        std::cout << std::endl;
      }

      DumpRouteNode(*routeNodeEntry->second);
    }
    else {
      std::cerr << "Cannot find route node with id " << id << std::endl;
    }

    firstRouteNode=false;
  }

  std::streamsize         oldPrecision=std::cout.precision(5);
  std::ios_base::fmtflags oldFlags=std::cout.setf(std::ios::fixed,std::ios::floatfield);

  for (std::list<Job>::const_iterator job=jobs.begin();
       job!=jobs.end();
       ++job) {
    if (job!=jobs.begin() ||
        !coordIds.empty()) {
      std::cout << std::endl;
    }

    if (job->osmRef.GetType()!=osmscout::osmRefNone) {
      std::map<osmscout::ObjectOSMRef,osmscout::ObjectFileRef>::const_iterator reference=idFileOffsetMap.lower_bound(job->osmRef);

      if (reference==idFileOffsetMap.end()) {
        std::cerr << "Cannot find '" << job->osmRef.GetTypeName() << "' with id " << job->osmRef.GetId() << std::endl;
        continue;
      }

      for (; reference!=idFileOffsetMap.upper_bound(job->osmRef); ++reference) {
        switch (reference->second.GetType()) {
          case osmscout::refNone:
            break;
          case osmscout::refNode:
            for (const auto &node : nodes) {
              if (reference->second.GetFileOffset() == node->GetFileOffset()) {
                DumpNode(node, reference->first.GetId());
                break;
              }
            }
            break;
          case osmscout::refArea:
            for (const auto &area : areas) {
              if (reference->second.GetFileOffset() == area->GetFileOffset()) {
                DumpArea(area, reference->first.GetId());
                break;
              }
            }
            break;
          case osmscout::refWay:
            for (const auto &way : ways) {
              if (reference->second.GetFileOffset() == way->GetFileOffset()) {
                DumpWay(way, reference->first.GetId());
                break;
              }
            }
            break;
        }
      }
    }
    else if (job->fileRef.GetType()!=osmscout::refNone) {
      std::map<osmscout::ObjectFileRef,osmscout::ObjectOSMRef>::const_iterator reference=fileOffsetIdMap.find(job->fileRef);

      if (reference==fileOffsetIdMap.end()) {
        std::cerr << "Cannot find '" << job->fileRef.GetTypeName() << "' with offset " << job->fileRef.GetFileOffset() << std::endl;
        continue;
      }

      switch (reference->first.GetType()) {
      case osmscout::refNone:
        break;
      case osmscout::refNode:
        for (const auto& node : nodes) {
          if (reference->first.GetFileOffset()==node->GetFileOffset()) {
            DumpNode(node,reference->second.GetId());
            break;
          }
        }
        break;
      case osmscout::refArea:
        for (const auto& area : areas) {
          if (reference->first.GetFileOffset()==area->GetFileOffset()) {
            DumpArea(area,reference->second.GetId());
            break;
          }
        }
        break;
      case osmscout::refWay:
        for (const auto& way : ways) {
          if (reference->first.GetFileOffset()==way->GetFileOffset()) {
            DumpWay(way,reference->second.GetId());
            break;
          }
        }
        break;
      }
    }
  }

  std::cout.setf(oldFlags,std::ios::floatfield);
  std::cout.precision(oldPrecision);

  routeNodeDataFile.Close();

  database.Close();

  debugDatabase.Close();

  return 0;
}
