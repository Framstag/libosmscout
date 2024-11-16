/*
  This source is part of the libosmscout library
  Copyright (C) 2012  Tim Teulings

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

#include <osmscoutimport/GenRouteDat.h>

#include <algorithm>
#include <future>

#include <osmscout/ObjectRef.h>

#include <osmscout/db/CoordDataFile.h>

#include <osmscout/routing/RoutingService.h>

#include <osmscout/system/Assert.h>
#include <osmscout/system/Math.h>

#include <osmscout/util/Geometry.h>

#include <osmscoutimport/Preprocess.h>
#include <osmscoutimport/GenWayWayDat.h>

namespace osmscout {

  void RouteDataGenerator::GetDescription(const ImportParameter& parameter,
                                          ImportModuleDescription& description) const
  {
    description.SetName("RouteDataGenerator");
    description.SetDescription("Generate routing graph(s)");

    description.AddRequiredFile(CoordDataFile::COORD_DAT);

    description.AddRequiredFile(WayDataFile::WAYS_DAT);
    description.AddRequiredFile(AreaDataFile::AREAS_DAT);

    description.AddRequiredFile(WayDataFile::WAYS_IDMAP);

    description.AddRequiredFile(WayWayDataGenerator::TURNRESTR_DAT);

    for (const auto& router : parameter.GetRouter()) {
      description.AddProvidedFile(router.GetDataFilename());
      description.AddProvidedFile(router.GetVariantFilename());
    }

    description.AddProvidedFile(RoutingService::FILENAME_INTERSECTIONS_DAT);
  }

  AccessFeatureValue RouteDataGenerator::GetAccess(const FeatureValueBuffer& buffer) const
  {
    const AccessFeatureValue *accessValue=accessReader->GetValue(buffer);

    if (accessValue!=nullptr) {
      return *accessValue;
    }

    return AccessFeatureValue (buffer.GetType()->GetDefaultAccess());
  }

  uint8_t RouteDataGenerator::GetMaxSpeed(const Way& way) const
  {
    const MaxSpeedFeatureValue *maxSpeedValue=maxSpeedReader->GetValue(way.GetFeatureValueBuffer());

    if (maxSpeedValue!=nullptr) {
      return maxSpeedValue->GetMaxSpeed();
    }

    return 0;
  }

  uint8_t RouteDataGenerator::GetGrade(const Way& way) const
  {
    const GradeFeatureValue *gradeValue=gradeReader->GetValue(way.GetFeatureValueBuffer());

    if (gradeValue!=nullptr) {
      return gradeValue->GetGrade();
    }

    return 1;
  }

  uint8_t RouteDataGenerator::CopyFlags(const Area::Ring& ring) const
  {
    uint8_t                            flags=0;
    AccessFeatureValue                 access=GetAccess(ring.GetFeatureValueBuffer());
    const AccessRestrictedFeatureValue *accessRestrictedValue=accessRestrictedReader->GetValue(ring.GetFeatureValueBuffer());


    if (accessRestrictedValue!=nullptr) {
      if (!accessRestrictedValue->CanAccessFoot()) {
        flags|=RouteNode::restrictedForFoot;
      }

      if (!accessRestrictedValue->CanAccessBicycle()) {
        flags|=RouteNode::restrictedForFoot;
      }

      if (!accessRestrictedValue->CanAccessCar()) {
        flags|=RouteNode::restrictedForCar;
      }
    }

    if (access.CanRouteFoot()) {
      flags|=RouteNode::usableByFoot;
    }

    if (access.CanRouteBicycle()) {
      flags|=RouteNode::usableByBicycle;
    }

    if (access.CanRouteCar()) {
      flags|=RouteNode::usableByCar;
    }

    return flags;
  }

  uint8_t RouteDataGenerator::CopyFlagsForward(const Way& way) const
  {
    uint8_t                            flags=0;
    AccessFeatureValue                 access=GetAccess(way.GetFeatureValueBuffer());
    const AccessRestrictedFeatureValue *accessRestrictedValue=accessRestrictedReader->GetValue(way.GetFeatureValueBuffer());

    if (accessRestrictedValue!=nullptr) {
      if (!accessRestrictedValue->CanAccessFoot()) {
        flags|=RouteNode::restrictedForFoot;
      }

      if (!accessRestrictedValue->CanAccessBicycle()) {
        flags|=RouteNode::restrictedForFoot;
      }

      if (!accessRestrictedValue->CanAccessCar()) {
        flags|=RouteNode::restrictedForCar;
      }
    }

    if (access.CanRouteFootForward()) {
      flags|=RouteNode::usableByFoot;
    }

    if (access.CanRouteBicycleForward()) {
      flags|=RouteNode::usableByBicycle;
    }

    if (access.CanRouteCarForward()) {
      flags|=RouteNode::usableByCar;
    }

    return flags;
  }

  uint8_t RouteDataGenerator::CopyFlagsBackward(const Way& way) const
  {
    uint8_t                            flags=0;
    AccessFeatureValue                 access=GetAccess(way.GetFeatureValueBuffer());
    const AccessRestrictedFeatureValue *accessRestrictedValue=accessRestrictedReader->GetValue(way.GetFeatureValueBuffer());

    if (accessRestrictedValue!=nullptr) {
      if (!accessRestrictedValue->CanAccessFoot()) {
        flags|=RouteNode::restrictedForFoot;
      }

      if (!accessRestrictedValue->CanAccessBicycle()) {
        flags|=RouteNode::restrictedForBicycle;
      }

      if (!accessRestrictedValue->CanAccessCar()) {
        flags|=RouteNode::restrictedForCar;
      }
    }

    if (access.CanRouteFootBackward()) {
      flags|=RouteNode::usableByFoot;
    }

    if (access.CanRouteBicycleBackward()) {
      flags|=RouteNode::usableByBicycle;
    }

    if (access.CanRouteCarBackward()) {
      flags|=RouteNode::usableByCar;
    }

    return flags;
  }

  uint16_t RouteDataGenerator::RegisterOrUseObjectVariantData(std::map<ObjectVariantData,uint16_t>& routeDataMap,
                                                              const TypeInfoRef& type,
                                                              uint8_t maxSpeed,
                                                              uint8_t grade)
  {
    ObjectVariantData routeData;
    uint16_t          objectVariantIndex;

    routeData.type=type;
    routeData.maxSpeed=maxSpeed;
    routeData.grade=grade;

    const auto entry=routeDataMap.find(routeData);

    if (entry==routeDataMap.end()) {
      objectVariantIndex=(uint16_t)routeDataMap.size();
      routeDataMap.emplace(routeData,objectVariantIndex);
    }
    else {
      objectVariantIndex=entry->second;
    }

    return objectVariantIndex;
  }

  bool RouteDataGenerator::IsAnyRoutable(Progress& progress,
                                         const RawRouteNode& node,
                                         const std::unordered_map<FileOffset,WayRef>& waysMap,
                                         const std::unordered_map<FileOffset,AreaRef>&  areasMap,
                                         VehicleMask vehicles) const
  {
    for (const auto& ref : node.objects) {
      if (ref.GetType()==refWay) {
        const auto& wayEntry=waysMap.find(ref.GetFileOffset());

        if (wayEntry==waysMap.end()) {
          progress.Error("Error while loading way at offset "+
                         std::to_string(ref.GetFileOffset()) +
                         " (Internal error?)");
          continue;
        }

        if (GetAccess(*wayEntry->second).CanRoute(vehicles)) {
          return true;
        }

      }
      else if (ref.GetType()==refArea) {
        const auto& areaEntry=areasMap.find(ref.GetFileOffset());

        if (areaEntry==areasMap.end()) {
          progress.Error("Error while loading areaEntry at offset "+
                         std::to_string(ref.GetFileOffset()) +
                         " (Internal error?)");
          continue;
        }

        if (areaEntry->second->GetType()->CanRoute()) {
          return true;
        }
      }
    }

    return false;
  }

  bool RouteDataGenerator::ReadTurnRestrictionIds(const ImportParameter& parameter,
                                                  Progress& progress,
                                                  std::map<OSMId,FileOffset>& wayIdOffsetMap,
                                                  std::map<OSMId,Id>& nodeIdMap)
  {
    FileScanner scanner;

    progress.Info("Reading turn restriction way ids");

    try {
      scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                   WayWayDataGenerator::TURNRESTR_DAT),
                   FileScanner::Sequential,
                   true);

      uint32_t restrictionCount=scanner.ReadUInt32();

      for (uint32_t r=1; r<=restrictionCount; r++) {
        progress.SetProgress(r,restrictionCount);

        TurnRestrictionRef restriction=std::make_shared<TurnRestriction>();

        restriction->Read(scanner);

        wayIdOffsetMap.emplace(restriction->GetFrom(),0);
        wayIdOffsetMap.emplace(restriction->GetTo(),0);

        nodeIdMap.emplace(restriction->GetVia(),0);
      }

      scanner.Close();
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());
      return false;
    }

    return true;
  }

  bool RouteDataGenerator::ResolveWayIds(const ImportParameter& parameter,
                                         Progress& progress,
                                         std::map<OSMId,FileOffset>& wayIdOffsetMap)
  {
    FileScanner scanner;

    progress.Info("Resolving turn restriction way ids to way file offsets");

    try {
      uint32_t resolveCount=0;

      scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                   WayDataFile::WAYS_IDMAP),
                   FileScanner::Sequential,
                   parameter.GetWayDataMemoryMaped());

      uint32_t wayCount=scanner.ReadUInt32();

      for (uint32_t w=1; w<=wayCount; w++) {
        progress.SetProgress(w,wayCount);

        OSMId      wayId=scanner.ReadInt64();
        uint8_t   typeByte=scanner.ReadUInt8();
        FileOffset wayOffset=scanner.ReadFileOffset();
        OSMRefType type=(OSMRefType)typeByte;

        if (type!=osmRefWay) {
          continue;
        }

        std::map<OSMId,FileOffset>::iterator idOffsetEntry;

        idOffsetEntry=wayIdOffsetMap.find(wayId);

        if (idOffsetEntry!=wayIdOffsetMap.end()) {
          idOffsetEntry->second=wayOffset;
          resolveCount++;
        }
      }

      progress.Info(std::to_string(wayIdOffsetMap.size())+" turn restriction way(s) found, "+std::to_string(resolveCount)+" way(s) resolved");

      scanner.Close();
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());
      return false;
    }

    return true;
  }

  bool RouteDataGenerator::ResolveNodeIds(const ImportParameter& parameter,
                                          Progress& progress,
                                          std::map<OSMId,Id>& nodeIdMap)
  {
    progress.Info("Resolving turn restriction OSM node ids to node ids");

    CoordDataFile coordDataFile;
    uint32_t      resolveCount=0;

    if (!coordDataFile.Open(parameter.GetDestinationDirectory(),
                            parameter.GetCoordDataMemoryMaped())) {
      progress.Error("Cannot open coord file!");
      return false;
    }

    std::set<OSMId>          nodeIds;
    CoordDataFile::ResultMap coordsMap;

    for (const auto& entry : nodeIdMap) {
      nodeIds.insert(entry.first);
    }

    if (!coordDataFile.Get(nodeIds,
                           coordsMap)) {
      progress.Error("Cannot read nodes!");
      return false;
    }

    for (const auto& entry : coordsMap) {
      auto nodeIdEntry=nodeIdMap.find(entry.first);

      if (nodeIdEntry!=nodeIdMap.end()) {
        nodeIdEntry->second=entry.second.GetId();
        resolveCount++;
      }
    }

    if (!coordDataFile.Close()) {
      progress.Error(std::string("Cannot close coord file"));
      return false;
    }

    progress.Info(std::to_string(nodeIds.size())+" via node(s) found, "+std::to_string(resolveCount)+" node(s) resolved");

    return true;
  }

  bool RouteDataGenerator::ReadTurnRestrictionData(const ImportParameter& parameter,
                                                   Progress& progress,
                                                   const std::map<OSMId,Id>& nodeIdMap,
                                                   const std::map<OSMId,FileOffset>& wayIdOffsetMap,
                                                   ViaTurnRestrictionMap& restrictions)
  {
    FileScanner scanner;

    progress.Info("Creating turn restriction data structures");

    try {

      scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                   WayWayDataGenerator::TURNRESTR_DAT),
                   FileScanner::Sequential,
                   true);

      uint32_t restrictionCount=scanner.ReadUInt32();

      for (uint32_t r=1; r<=restrictionCount; r++) {
        progress.SetProgress(r,restrictionCount);

        TurnRestrictionRef restriction=std::make_shared<TurnRestriction>();

        restriction->Read(scanner);

        TurnRestrictionData                        data;
        std::map<OSMId,FileOffset>::const_iterator idOffsetEntry;
        std::map<OSMId,Id>::const_iterator         nodeIdEntry;


        idOffsetEntry=wayIdOffsetMap.find(restriction->GetFrom());

        if (idOffsetEntry==wayIdOffsetMap.end() || idOffsetEntry->second==0) {
          progress.Error(std::string("Error while retrieving way offset for way id ")+
                         std::to_string(restriction->GetFrom()));
          continue;
        }

        data.fromWayOffset=idOffsetEntry->second;

        nodeIdEntry=nodeIdMap.find(restriction->GetVia());

        if (nodeIdEntry==nodeIdMap.end() || nodeIdEntry->second==0) {
          progress.Error(std::string("Error while retrieving node id for node OSM id ")+
                         std::to_string(restriction->GetVia()));
          continue;
        }

        data.viaNodeId=nodeIdEntry->second;

        idOffsetEntry=wayIdOffsetMap.find(restriction->GetTo());

        if (idOffsetEntry==wayIdOffsetMap.end() || idOffsetEntry->second==0) {
          progress.Error(std::string("Error while retrieving way offset for way id ")+
                         std::to_string(restriction->GetTo()));
          continue;
        }

        data.toWayOffset=idOffsetEntry->second;

        switch (restriction->GetType()) {
        case TurnRestriction::Allow:
          data.type=TurnRestrictionData::Allow;
          break;
        case TurnRestriction::Forbit:
          data.type=TurnRestrictionData::Forbit;
          break;
        }

        restrictions[data.viaNodeId].push_back(data);
      }

      progress.Info(std::string("Read ")+std::to_string(restrictionCount)+" turn restrictions");

      scanner.Close();
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());
      return false;
    }

    return true;
  }

  bool RouteDataGenerator::ReadTurnRestrictions(const ImportParameter& parameter,
                                                Progress& progress,
                                                ViaTurnRestrictionMap& restrictions)
  {
    std::map<OSMId,FileOffset> wayIdOffsetMap;
    std::map<OSMId,Id>         nodeIdMap;

    //
    // Just read the way ids
    //

    if (!ReadTurnRestrictionIds(parameter,
                                progress,
                                wayIdOffsetMap,
                                nodeIdMap)) {
      return false;
    }

    //
    // Now map way ids to file offsets
    //

    if (!ResolveWayIds(parameter,
                       progress,
                       wayIdOffsetMap)) {
      return false;
    }

    //
    // Now map node ids to file offsets
    //

    if (!ResolveNodeIds(parameter,
                        progress,
                        nodeIdMap)) {
      return false;
    }

    //
    // Finally read restrictions again and replace way ids with way offsets
    //

    return ReadTurnRestrictionData(parameter,
                                   progress,
                                   nodeIdMap,
                                   wayIdOffsetMap,
                                   restrictions);
  }

  bool RouteDataGenerator::CanTurn(const std::vector<TurnRestrictionData>& restrictions,
                                   FileOffset from,
                                   FileOffset to) const
  {
    bool defaultReturn=true;

    if (restrictions.empty()) {
      return true;
    }

    for (const auto& restriction : restrictions) {
      if (restriction.fromWayOffset==from) {
        if (restriction.type==TurnRestrictionData::Allow) {
          if (restriction.toWayOffset==to) {
            return true;
          }

          // If there are allow restrictions,everything else is forbidden
          defaultReturn=false;
        }
        else if (restriction.type==TurnRestrictionData::Forbit) {
          if (restriction.toWayOffset==to) {
            return false;
          }

          // If there are forbid restrictions, everything else is allowed
          defaultReturn=true;
        }
      }
    }

    // Now lets hope nobody is mixing allow and forbid!
    return defaultReturn;
  }

  bool RouteDataGenerator::ReadIntersections(const ImportParameter& parameter,
                                             Progress& progress,
                                             const TypeConfig& typeConfig,
                                             NodeUseMap& nodeUseMap)
  {
    FileScanner scanner;

    progress.Info("Scanning ways");

    try {
      scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                   WayDataFile::WAYS_DAT),
                   FileScanner::Sequential,
                   parameter.GetWayDataMemoryMaped());

      uint32_t dataCount=scanner.ReadUInt32();

      for (uint32_t d=1; d<=dataCount; d++) {
        progress.SetProgress(d,dataCount);

        Way way;

        way.Read(typeConfig,
                 scanner);

        if (way.GetType()->GetIgnore()) {
          continue;
        }

        if (!GetAccess(way).CanRoute()) {
          continue;
        }

        std::unordered_set<Id> nodeIds;

        for (const auto& node : way.nodes) {
          if (!node.IsRelevant()) {
            continue;
          }

          Id id=node.GetId();

          if (!nodeIds.contains(id)) {
            nodeUseMap.SetNodeUsed(id);
            nodeIds.insert(id);
          }
        }
      }

      scanner.Close();

      progress.Info("Scanning areas");

      scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                   AreaDataFile::AREAS_DAT),
                   FileScanner::Sequential,
                   parameter.GetAreaDataMemoryMaped());

      dataCount=scanner.ReadUInt32();

      for (uint32_t d=1; d<=dataCount; d++) {
        progress.SetProgress(d,dataCount);

        Area area;

        area.Read(typeConfig,
                  scanner);

        if (area.GetType()->GetIgnore()) {
          continue;
        }

        if (!area.GetType()->CanRoute()) {
          continue;
        }

        // We currently route only on simple areas, multipolygon relations we skip
        if (!area.IsSimple()) {
          continue;
        }

        std::unordered_set<Id> nodeIds;

        for (const auto& node : area.rings.front().nodes) {
          if (!node.IsRelevant()) {
            continue;
          }

          Id id=node.GetId();

          if (!nodeIds.contains(id)) {
            nodeUseMap.SetNodeUsed(id);

            nodeIds.insert(id);
          }
        }
      }

      scanner.Close();

      progress.Info("Found "+std::to_string(nodeUseMap.GetNodeUsedCount())+" possible routing nodes, "+
                    std::to_string(nodeUseMap.GetDuplicateCount())+" at least used twice");
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());
      return false;
    }

    return true;
  }

  bool RouteDataGenerator::ReadObjectsAtIntersections(const ImportParameter& parameter,
                                                      Progress& progress,
                                                      const TypeConfig& typeConfig,
                                                      const NodeUseMap& nodeUseMap,
                                                      NodeIdObjectsMap& nodeObjectsMap)
  {
    using namespace std::string_literals;
    FileScanner scanner;

    progress.Info("Scanning ways");

    try {
      uint32_t junctionWayCount=0;
      uint32_t junctionAreaCount=0;
      uint32_t junctionNodeCount=0;

      scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                   WayDataFile::WAYS_DAT),
                   FileScanner::Sequential,
                   parameter.GetWayDataMemoryMaped());

      uint32_t dataCount=scanner.ReadUInt32();

      for (uint32_t d=1; d<=dataCount; d++) {
        progress.SetProgress(d,dataCount);

        Way way;

        way.Read(typeConfig,
                 scanner);

        if (way.GetType()->GetIgnore()) {
          continue;
        }

        if (!GetAccess(way).CanRoute()) {
          continue;
        }

        std::unordered_set<Id> nodeIds;

        for (const auto& node : way.nodes) {
          if (!node.IsRelevant()) {
            continue;
          }

          Id id=node.GetId();

          if (!nodeIds.contains(id)) {
            if (nodeUseMap.IsNodeUsedAtLeastTwice(id)) {
              nodeObjectsMap[id].push_back(way.GetObjectFileRef());
              junctionWayCount++;
            }

            nodeIds.insert(id);
          }
        }
      }

      scanner.Close();

      progress.Info("Scanning areas");

      scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                   AreaDataFile::AREAS_DAT),
                   FileScanner::Sequential,
                   parameter.GetAreaDataMemoryMaped());

      dataCount=scanner.ReadUInt32();

      for (uint32_t d=1; d<=dataCount; d++) {
        progress.SetProgress(d,dataCount);

        Area area;

        area.Read(typeConfig,
                  scanner);

        if (area.GetType()->GetIgnore()) {
          continue;
        }

        if (!(area.GetType()->CanRoute())) {
          continue;
        }

        // We currently route only on simple areas, no multipolygon relations
        if (!area.IsSimple()) {
          continue;
        }

        std::unordered_set<Id> nodeIds;

        for (const auto& node : area.rings.front().nodes) {
          if (!node.IsRelevant()) {
            continue;
          }

          Id id=node.GetId();

          if (!nodeIds.contains(id)) {
            if (nodeUseMap.IsNodeUsedAtLeastTwice(id)) {
              nodeObjectsMap[id].push_back(area.GetObjectFileRef());
              junctionAreaCount++;
            }

            nodeIds.insert(id);
          }
        }
      }

      scanner.Close();

      TypeInfoSet junctionNodeTypes(typeConfig);
      TypeInfoRef miniRoundabout = typeConfig.GetTypeInfo("highway_mini_roundabout");
      if (miniRoundabout) {
        junctionNodeTypes.Set(miniRoundabout);
      } else {
        progress.Warning("Unknown type \"highway_mini_roundabout\".");
      }
      TypeInfoRef motorwayJunction = typeConfig.GetTypeInfo("highway_motorway_junction");
      if (motorwayJunction) {
        junctionNodeTypes.Set(motorwayJunction);
      } else {
        progress.Warning("Unknown type \"highway_motorway_junction\".");
      }
      if (!junctionNodeTypes.Empty()) {
        progress.Info("Scanning nodes");

        scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                     NodeDataFile::NODES_DAT),
                     FileScanner::Sequential,
                     parameter.GetRawNodeDataMemoryMaped());

        dataCount=scanner.ReadUInt32();
        for (uint32_t d=1; d<=dataCount; d++) {
          progress.SetProgress(d,dataCount);

          Node node;

          node.Read(typeConfig,
                    scanner);

          if (node.GetType()->GetIgnore() || !junctionNodeTypes.IsSet(node.GetType())) {
            continue;
          }

          // Node doesn't have serial number, let's assign node to the first junction on these coordinates.
          // It is possible that we don't assign junction node correctly, but such error should be rare.
          Id id=Point(1, node.GetCoords()).GetId();

          if (nodeUseMap.IsNodeUsedAtLeastTwice(id)) {
            nodeObjectsMap[id].push_back(node.GetObjectFileRef());
            junctionNodeCount++;
          } else {
            progress.Warning("Cannot find junction for node offset "s + std::to_string(node.GetFileOffset()));
          }
        }

        scanner.Close();
      }

      progress.Info("Found "+std::to_string(nodeObjectsMap.size())+" routing nodes, with in sum "+
                    std::to_string(junctionWayCount)+" ways, "+std::to_string(junctionAreaCount)+" areas and "+
                    std::to_string(junctionNodeCount)+" nodes");
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());
      return false;
    }

    return true;
  }

  bool RouteDataGenerator::WriteIntersections(const ImportParameter& parameter,
                                              Progress& progress,
                                              NodeIdObjectsMap& nodeIdObjectsMap)
  {
    FileWriter writer;

    try {
      writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                  RoutingService::FILENAME_INTERSECTIONS_DAT));

      writer.Write((uint32_t)nodeIdObjectsMap.size());

      for (const auto& junction : nodeIdObjectsMap) {

        writer.WriteNumber(junction.first);
        writer.WriteNumber((uint32_t)junction.second.size());

        ObjectFileRefStreamWriter objectFileRefWriter(writer);

        for (const auto& object : junction.second) {
          objectFileRefWriter.Write(object);
        }
      }

      writer.Close();
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());
      return false;
    }

    return true;
  }

  RouteDataGenerator::FileOffsetWayMap RouteDataGenerator::LoadWays(Progress& progress,
                                                                    const TypeConfig& typeConfig,
                                                                    FileScanner& scanner,
                                                                    RawRouteNodeList::const_iterator startNode,
                                                                    RawRouteNodeList::const_iterator endNode)
  {
    std::set<FileOffset> wayOffsets;

    for (auto nodeEntry=startNode; nodeEntry!=endNode; nodeEntry++) {
      const auto& node=*nodeEntry;

      for (const auto& ref : node.objects) {
        if (ref.GetType()==refWay) {
          wayOffsets.insert(ref.GetFileOffset());
        }
      }
    }

    progress.Info("Loading " +std::to_string(wayOffsets.size())+" ways");

    FileOffset           oldPos;
    FileOffsetWayMap     waysMap;

    oldPos=scanner.GetPos();

    for (auto offset : wayOffsets) {
      scanner.SetPos(offset);

      WayRef way=std::make_shared<Way>();

      way->Read(typeConfig,
                scanner);

      waysMap[offset]=std::move(way);
    }

    scanner.SetPos(oldPos);

    return waysMap;
  }

  RouteDataGenerator::FileOffsetAreaMap RouteDataGenerator::LoadAreas(Progress& progress,
                                                                      const TypeConfig& typeConfig,
                                                                      FileScanner& scanner,
                                                                      RawRouteNodeList::const_iterator startNode,
                                                                      RawRouteNodeList::const_iterator endNode)
  {
    std::set<FileOffset> areaOffsets;

    for (auto nodeEntry=startNode; nodeEntry!=endNode; nodeEntry++) {
      const auto& node=*nodeEntry;

      for (const auto& ref : node.objects) {
        if (ref.GetType()==refArea) {
          areaOffsets.insert(ref.GetFileOffset());
        }
      }
    }

    progress.Info("Loading " +std::to_string(areaOffsets.size())+" areas");

    FileOffsetAreaMap areasMap;
    FileOffset        oldPos;

    oldPos=scanner.GetPos();

    for (auto offset : areaOffsets) {
      scanner.SetPos(offset);

      AreaRef area=std::make_shared<Area>();

      area->Read(typeConfig,
                 scanner);

      areasMap[offset]=std::move(area);
    }

    scanner.SetPos(oldPos);

    return areasMap;
  }

  bool RouteDataGenerator::GetRouteNodePoint(Progress& progress,
                                             const RawRouteNode& node,
                                             std::unordered_map<FileOffset,WayRef>& waysMap,
                                             std::unordered_map<FileOffset,AreaRef>& areasMap,
                                             Point& point) const
  {
    for (const auto& ref : node.objects) {
      if (ref.GetType()==refWay) {
        const WayRef& way=waysMap[ref.GetFileOffset()];

        if (!way) {
          progress.Error("Error while loading way at offset "+
                         std::to_string(ref.GetFileOffset()) +
                         " (Internal error?)");
          continue;
        }

        size_t currentNode;

        if (!way->GetNodeIndexByNodeId(node.id,
                                       currentNode)) {
          assert(true);
        }

        point=way->nodes[currentNode];

        return true;
      }

      if (ref.GetType()==refArea) {
        const AreaRef& area=areasMap[ref.GetFileOffset()];

        if (!area) {
          progress.Error("Error while loading area at offset "+
                         std::to_string(ref.GetFileOffset()) +
                         " (Internal error?)");
          continue;
        }

        size_t            currentNode;
        const Area::Ring& ring=area->rings.front();

        if (!ring.GetNodeIndexByNodeId(node.id,
                                       currentNode)) {
          assert(true);
        }

        assert(currentNode<ring.nodes.size());

        point=ring.nodes[currentNode];

        return true;
      }
    }

    return false;
  }


  /*
  uint8_t RouteDataGenerator::CalculateEncodedBearing(const Way& way,
                                                      size_t currentNode,
                                                      size_t nextNode,
                                                      bool clockwise) const
  {
    size_t currentNodeFollower;
    size_t nextNodePrecursor;

    if (clockwise) {
      currentNodeFollower=currentNode==way.nodes.size()-1 ? 0 : currentNode+1;
      nextNodePrecursor=nextNode>0 ? nextNode-1 :way.nodes.size()-1;
    }
    else {
      currentNodeFollower=currentNode>0 ? currentNode-1 :way.nodes.size()-1;
      nextNodePrecursor=nextNode==way.nodes.size()-1 ? 0 : nextNode+1;
    }

    double initialBearing=GetSphericalBearingInitial(way.nodes[currentNode].GetLon(),
                                                     way.nodes[currentNode].GetLat(),
                                                     way.nodes[currentNodeFollower].GetLon(),
                                                     way.nodes[currentNodeFollower].GetLat());
    double finalBearing=GetSphericalBearingInitial(way.nodes[nextNodePrecursor].GetLon(),
                                                   way.nodes[nextNodePrecursor].GetLat(),
                                                   way.nodes[nextNode].GetLon(),
                                                   way.nodes[nextNode].GetLat());

    // Transform in 0..360 Degree

    initialBearing=fmod(initialBearing*180.0/M_PI+360.0,360.0);
    finalBearing=fmod(initialBearing*180.0/M_PI+360.0,360.0);

    uint8_t bearing=initialBearing/10+100+finalBearing/10;

    return bearing;
  }*/

  void RouteDataGenerator::CalculateAreaPaths(RouteNode& routeNode,
                                              const Area& area,
                                              uint16_t objectVariantIndex,
                                              const RouteNodeIdSet& routeNodeIdSet)
  {
    const Area::Ring& ring=area.rings.front();
    size_t            currentNode;
    Distance          distance;

    if (!ring.GetNodeIndexByNodeId(routeNode.GetId(),
                                   currentNode)) {
      assert(true);
    }

    // Find next routing node in order

    size_t nextNode=currentNode+1;

    if (nextNode>=ring.nodes.size()) {
      nextNode=0;
    }

    distance=GetSphericalDistance(ring.nodes[currentNode].GetCoord(),
                                  ring.nodes[nextNode].GetCoord());

    while (nextNode!=currentNode &&
           !routeNodeIdSet.contains(ring.GetId(nextNode))) {
      size_t lastNode=nextNode;

      nextNode++;

      if (nextNode>=ring.nodes.size()) {
        nextNode=0;
      }

      if (nextNode!=currentNode) {
        distance+=GetSphericalDistance(ring.nodes[lastNode].GetCoord(),
                                       ring.nodes[nextNode].GetCoord());
      }
    }

    // Found next routing node in order
    if (nextNode!=currentNode &&
        ring.GetId(nextNode)!=routeNode.GetId()) {
      RouteNode::Path path;

      path.id=ring.GetId(nextNode);
      path.objectIndex=routeNode.AddObject(ObjectFileRef(area.GetFileOffset(),refArea),
                                           objectVariantIndex);
      //path.bearing=CalculateEncodedBearing(way,currentNode,nextNode,true);
      path.flags=CopyFlags(ring);
      path.distance=distance;

      routeNode.paths.push_back(path);
    }

    // Find next routing node against order

    size_t prevNode;

    if (currentNode==0) {
      prevNode=ring.nodes.size()-1;
    }
    else {
      prevNode=currentNode-1;
    }

    distance=GetSphericalDistance(ring.nodes[currentNode].GetCoord(),
                                  ring.nodes[prevNode].GetCoord());

    while (prevNode!=currentNode &&
           !routeNodeIdSet.contains(ring.GetId(prevNode))) {
      size_t lastNode=prevNode;

      if (prevNode==0) {
        prevNode=ring.nodes.size()-1;
      }
      else {
        --prevNode;
      }

      if (prevNode!=currentNode) {
        distance+=GetSphericalDistance(ring.nodes[lastNode].GetCoord(),
                                       ring.nodes[prevNode].GetCoord());
      }
    }

    // Found next routing node against order

    if (prevNode!=currentNode &&
        prevNode!=nextNode &&
        ring.GetId(prevNode)!=routeNode.GetId()) {
      RouteNode::Path path;

      path.id=ring.GetId(prevNode);
      path.objectIndex=routeNode.AddObject(ObjectFileRef(area.GetFileOffset(),refArea),
                                           objectVariantIndex);
      //path.bearing=CalculateEncodedBearing(way,currentNode,prevNode,false);
      path.flags=CopyFlags(ring);
      path.distance=distance;

      routeNode.paths.push_back(path);
    }
  }

  void RouteDataGenerator::CalculateCircularWayPaths(RouteNode& routeNode,
                                                     const Way& way,
                                                     uint16_t objectVariantIndex,
                                                     const RouteNodeIdSet& routeNodeIdSet)
  {
    size_t currentNode;
    Distance distance;

    if (!way.GetNodeIndexByNodeId(routeNode.GetId(),
                                  currentNode)) {
      assert(true);
    }

    // In path direction

    size_t nextNode=currentNode+1;
    if (GetAccess(way).CanRouteForward()) {

      if (nextNode>=way.nodes.size()) {
        nextNode=0;
      }

      distance=GetSphericalDistance(way.GetCoord(currentNode),
                                    way.GetCoord(nextNode));

      while (nextNode!=currentNode &&
             !routeNodeIdSet.contains(way.GetId(nextNode))) {
        size_t lastNode=nextNode;

        nextNode++;

        if (nextNode>=way.nodes.size()) {
          nextNode=0;
        }

        if (nextNode!=currentNode) {
          distance+=GetSphericalDistance(way.GetCoord(lastNode),
                                         way.GetCoord(nextNode));
        }
      }

      if (nextNode!=currentNode &&
          way.GetId(nextNode)!=routeNode.GetId()) {
        RouteNode::Path path;

        path.id=way.GetId(nextNode);
        path.objectIndex=routeNode.AddObject(ObjectFileRef(way.GetFileOffset(),refWay),
                                             objectVariantIndex);
        //path.bearing=CalculateEncodedBearing(way,currentNode,nextNode,true);
        path.flags=CopyFlagsForward(way);
        path.distance=distance;

        routeNode.paths.push_back(path);
      }
    }

    // Against path direction

    if (GetAccess(way).CanRouteBackward()) {
      size_t prevNode;

      if (currentNode==0) {
        prevNode=way.nodes.size()-1;
      }
      else {
        prevNode=currentNode-1;
      }

      distance=GetSphericalDistance(way.nodes[currentNode].GetCoord(),
                                    way.nodes[prevNode].GetCoord());

      while (prevNode!=currentNode &&
             !routeNodeIdSet.contains(way.GetId(prevNode))) {
        size_t lastNode=prevNode;

        if (prevNode==0) {
          prevNode=way.nodes.size()-1;
        }
        else {
          --prevNode;
        }

        if (prevNode!=currentNode) {
          distance+=GetSphericalDistance(way.nodes[lastNode].GetCoord(),
                                         way.nodes[prevNode].GetCoord());
        }
      }

      if (prevNode!=currentNode &&
          prevNode!=nextNode &&
          way.GetId(prevNode)!=routeNode.GetId()) {
        RouteNode::Path path;

        path.id=way.GetId(prevNode);
        path.objectIndex=routeNode.AddObject(ObjectFileRef(way.GetFileOffset(),refWay),
                                             objectVariantIndex);
        //path.bearing=CalculateEncodedBearing(way,prevNode,nextNode,false);
        path.flags=CopyFlagsBackward(way);
        path.distance=distance;

        routeNode.paths.push_back(path);
      }
    }
  }

  void RouteDataGenerator::CalculateWayPaths(RouteNode& routeNode,
                                             const Way& way,
                                             uint16_t objectVariantIndex,
                                             const RouteNodeIdSet& routeNodeIdSet)
  {
    size_t currentNode;

    if (!way.GetNodeIndexByNodeId(routeNode.GetId(),
                                  currentNode)) {
      assert(true);
    }

    // Route backward
    if (GetAccess(way).CanRouteBackward() &&
        currentNode>0) {
      int j=currentNode-1;

      // Search for previous routing node on way
      while (j>=0) {
        if (routeNodeIdSet.contains(way.GetId(j))) {
          break;
        }

        j--;
      }

      if (j>=0 &&
          way.GetId(j)!=routeNode.GetId()) {
        RouteNode::Path path;

        path.id=way.GetId(j);
        path.objectIndex=routeNode.AddObject(ObjectFileRef(way.GetFileOffset(),refWay),
                                             objectVariantIndex);
        //path.bearing=CalculateEncodedBearing(way,i,j,false);
        path.flags=CopyFlagsBackward(way);

        path.distance=Distance::Of<Meter>(0.0);
        for (size_t d=j;d<currentNode; d++) {
          path.distance+=GetSphericalDistance(way.nodes[d].GetCoord(),
                                              way.nodes[d+1].GetCoord());
        }

        routeNode.paths.push_back(path);
      }
    }

    // Route forward
    if (GetAccess(way).CanRouteForward() &&
      currentNode+1<way.nodes.size()) {
      size_t j=currentNode+1;

      // Search for next routing node on way
      while (j<way.nodes.size()) {
        if (routeNodeIdSet.contains(way.GetId(j))) {
          break;
        }

        j++;
      }

      if (j<way.nodes.size() &&
          way.GetId(j)!=routeNode.GetId()) {
        RouteNode::Path path;

        path.id=way.GetId(j);
        path.objectIndex=routeNode.AddObject(ObjectFileRef(way.GetFileOffset(),refWay),
                                             objectVariantIndex);
        //path.bearing=CalculateEncodedBearing(way,i,j,true);
        path.flags=CopyFlagsForward(way);

        path.distance=Distance::Of<Meter>(0.0);
        for (size_t d=currentNode;d<j; d++) {
          path.distance+=GetSphericalDistance(way.nodes[d].GetCoord(),
                                              way.nodes[d+1].GetCoord());
        }

        routeNode.paths.push_back(path);
      }
    }
  }

  void RouteDataGenerator::FillRoutePathExcludes(RouteNode& routeNode,
                                                 const RawRouteNode& node,
                                                 const ViaTurnRestrictionMap& restrictions)
  {
    auto turnConstraints=restrictions.find(routeNode.GetId());

    if (turnConstraints==restrictions.end()) {
      return;
    }

    for (const auto& source : node.objects) {

        // TODO: Fix CanTurn
        if (source.GetType()!=refWay) {
          continue;
        }

      for (const auto& dest: node.objects) {
        // TODO: Fix CanTurn
        if (dest.GetType()!=refWay) {
          continue;
        }

        if (source==dest) {
          continue;
        }

        if (!CanTurn(turnConstraints->second,
                     source.GetFileOffset(),
                     dest.GetFileOffset())) {
          RouteNode::Exclude exclude;

          exclude.source=source;
          exclude.targetIndex=0;

          while (exclude.targetIndex<routeNode.paths.size() &&
              routeNode.objects[routeNode.paths[exclude.targetIndex].objectIndex].object!=dest) {
            exclude.targetIndex++;
          }

          if (exclude.targetIndex<routeNode.paths.size()) {
            routeNode.excludes.push_back(exclude);
          }
        }
      }
    }
  }

  bool RouteDataGenerator::WriteObjectVariantData(Progress& progress,
                                                  const std::string& variantFilename,
                                                  const std::map<ObjectVariantData,uint16_t>& routeDataMap)
  {
    FileWriter writer;

    try {
      writer.Open(variantFilename);

      std::vector<ObjectVariantData> objectVariantData;

      objectVariantData.resize(routeDataMap.size());

      for (const auto& entry : routeDataMap) {
        objectVariantData[entry.second]=entry.first;
      }

      writer.Write((uint32_t)objectVariantData.size());

      for (const auto& entry : objectVariantData) {
        entry.Write(writer);
      }

      progress.Info(std::to_string(objectVariantData.size()) + " object variant(s)");

      writer.Close();
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());
      return false;
    }

    return true;
  }

  bool RouteDataGenerator::WriteRouteGraph(const ImportParameter& parameter,
                                           Progress& progress,
                                           const TypeConfig& typeConfig,
                                           const RawRouteNodeList& rawRouteNodes,
                                           const RouteNodeIdSet& routeNodeIdSet,
                                           const ViaTurnRestrictionMap& restrictions,
                                           VehicleMask vehicles,
                                           const std::string& dataFilename,
                                           const std::string& variantFilename)
  {
    FileScanner                wayScanner;
    FileScanner                areaScanner;
    FileWriter                 writer;

    std::map<Pixel,IndexEntry> indexMap;

    std::map<ObjectVariantData,uint16_t> routeDataMap;

    //
    // Writing route nodes
    //

    try {
      uint32_t   handledRouteNodeCount=0;
      FileOffset indexFileOffset=0;
      uint32_t   writtenRouteNodeCount=0;
      size_t     objectCount=0;
      size_t     pathCount=0;
      size_t     excludeCount=0;
      size_t     simpleNodesCount=0;

      writer.Open(dataFilename);

      writer.Write(indexFileOffset);
      writer.Write(writtenRouteNodeCount);
      writer.Write(parameter.GetRouteNodeTileMag());

      wayScanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                      WayDataFile::WAYS_DAT),
                      FileScanner::Sequential,
                      parameter.GetWayDataMemoryMaped());

      areaScanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                       AreaDataFile::AREAS_DAT),
                       FileScanner::Sequential,
                       parameter.GetAreaDataMemoryMaped());

      Pixel      currentCell(0,0);
      IndexEntry currentIndex(0);

      auto startOfBlock=rawRouteNodes.cbegin();
      while (startOfBlock!=rawRouteNodes.cend()) {
        auto   endOfBlock=startOfBlock;
        size_t blockSize=0;

        while (endOfBlock!=rawRouteNodes.cend() &&
               blockSize<parameter.GetRouteNodeBlockSize()) {
          endOfBlock++;
          blockSize++;
        }

        progress.Info("Processing " +std::to_string(blockSize) + " route nodes");

        progress.Info("Loading intersecting ways and areas");

        auto waysMapFuture=std::async(std::launch::async,
                                      &RouteDataGenerator::LoadWays,this,
                                      std::ref(progress),
                                      std::ref(typeConfig),
                                      std::ref(wayScanner),
                                      std::ref(startOfBlock),
                                      std::ref(endOfBlock));

        auto areasMapFuture=std::async(std::launch::async,
                                       &RouteDataGenerator::LoadAreas,this,
                                       std::ref(progress),
                                       std::ref(typeConfig),
                                       std::ref(areaScanner),
                                       std::ref(startOfBlock),
                                       std::ref(endOfBlock));

        FileOffsetWayMap waysMap=waysMapFuture.get();
        FileOffsetAreaMap areasMap=areasMapFuture.get();

        progress.Info("Storing route nodes");

        for (auto nodeEntry=startOfBlock; nodeEntry!=endOfBlock; nodeEntry++) {
          const auto& node=*nodeEntry;

          if (node.cell!=currentCell) {
            if (currentIndex.count>0) {
              indexMap[currentCell]=currentIndex;
            }

            currentCell=node.cell;
            currentIndex=IndexEntry(writer.GetPos());
            currentIndex.count=1;
          }
          else {
            currentIndex.count++;
          }

          handledRouteNodeCount++;
          progress.SetProgress(handledRouteNodeCount,
                               (uint32_t)rawRouteNodes.size());

          //
          // Find out if any of the areas/ways at the intersection is routable
          // for us for the given vehicle (we already only loaded those objects
          // that are routable at all).
          // If none of the objects is routable the complete node is not routable and
          // we can safely drop this node from the routing graph.
          //

          if (!IsAnyRoutable(progress,
                             node,
                             waysMap,
                             areasMap,
                             vehicles)) {
            continue;
          }

          Point point;

          if (!GetRouteNodePoint(progress,
                                 node,
                                 waysMap,
                                 areasMap,
                                 point)) {
            continue;
          }

          RouteNode  routeNode;

          routeNode.Initialize(writer.GetPos(),
                               point);

          //
          // Calculate all outgoing paths
          //

          for (const auto& ref : node.objects) {
            if (ref.GetType()==refWay) {
              const WayRef& way=waysMap[ref.GetFileOffset()];

              if (!way) {
                progress.Error("Error while loading way at offset "+
                               std::to_string(ref.GetFileOffset()) +
                               " (Internal error?)");
                continue;
              }

              if (!GetAccess(*way).CanRoute(vehicles)) {
                continue;
              }

              uint16_t objectVariantIndex=RegisterOrUseObjectVariantData(routeDataMap,
                                                                         way->GetType(),
                                                                         GetMaxSpeed(*way),
                                                                         GetGrade(*way));

              if (way->IsCircular()) {
                // Circular way routing (similar to current area routing, but respecting isOneway())
                CalculateCircularWayPaths(routeNode,
                                          *way,
                                          objectVariantIndex,
                                          routeNodeIdSet);
              }
              else {
                // Normal way routing
                CalculateWayPaths(routeNode,
                                  *way,
                                  objectVariantIndex,
                                  routeNodeIdSet);
              }
            }
            else if (ref.GetType()==refArea) {
              const AreaRef& area=areasMap[ref.GetFileOffset()];

              if (!area) {
                progress.Error("Error while loading area at offset "+
                               std::to_string(ref.GetFileOffset()) +
                               " (Internal error?)");
                continue;
              }

              if (!area->GetType()->CanRoute()) {
                continue;
              }

              uint16_t objectVariantIndex=RegisterOrUseObjectVariantData(routeDataMap,
                                                                         area->GetType(),
                                                                         0,
                                                                         1);

              routeNode.AddObject(ref,
                                  objectVariantIndex);

              CalculateAreaPaths(routeNode,
                                 *area,
                                 objectVariantIndex,
                                 routeNodeIdSet);
            }
          }

          FillRoutePathExcludes(routeNode,
                                node,
                                restrictions);

          if (routeNode.paths.size()==1) {
            simpleNodesCount++;
          }

          objectCount+=routeNode.objects.size();
          pathCount+=routeNode.paths.size();
          excludeCount+=routeNode.excludes.size();

          routeNode.Write(writer);

          writtenRouteNodeCount++;
        }

        writer.Flush();

        startOfBlock=endOfBlock;
      }

      if (currentIndex.count>0) {
        indexMap[currentCell]=currentIndex;
      }

      indexFileOffset=writer.GetPos();

      writer.SetPos(0);
      writer.WriteFileOffset(indexFileOffset);
      writer.Write(writtenRouteNodeCount);

      writer.SetPos(indexFileOffset);

      progress.Info("Writing index starting at position "+std::to_string(indexFileOffset));

      writer.Write(uint32_t(indexMap.size()));
      for (const auto& indexEntry : indexMap) {
        writer.Write(indexEntry.first.x);
        writer.Write(indexEntry.first.y);
        writer.WriteFileOffset(indexEntry.second.offset);
        writer.Write(indexEntry.second.count);
      }

      progress.Info(std::to_string(writtenRouteNodeCount) + " route node(s) written");
      progress.Info(std::to_string(simpleNodesCount)+ " route node(s) are simple and only have 1 path");
      progress.Info(std::to_string(objectCount)+ " object(s)");
      progress.Info(std::to_string(pathCount) + " path(s)");
      progress.Info(std::to_string(excludeCount) + " exclude(s)");
      progress.Info(std::to_string(indexMap.size()) + " index entries");

      wayScanner.Close();
      areaScanner.Close();
      writer.Close();
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());
      wayScanner.CloseFailsafe();
      areaScanner.CloseFailsafe();
      writer.CloseFailsafe();
      return false;
    }

    return WriteObjectVariantData(progress,
                                  variantFilename,
                                  routeDataMap);
  }

  bool RouteDataGenerator::Import(const TypeConfigRef& typeConfig,
                                  const ImportParameter& parameter,
                                  Progress& progress)
  {
    using namespace std::string_literals;

    // List of restrictions for a way
    ViaTurnRestrictionMap              restrictions;

    NodeUseMap                         nodeUseMap;
    NodeIdObjectsMap                   nodeObjectsMap;
    AccessRestrictedFeatureValueReader accessRestrictedReader(*typeConfig);
    AccessFeatureValueReader           accessReader(*typeConfig);
    MaxSpeedFeatureValueReader         maxSpeedReader(*typeConfig);
    GradeFeatureValueReader            gradeReader(*typeConfig);

    this->accessRestrictedReader=&accessRestrictedReader;
    this->accessReader=&accessReader;
    this->maxSpeedReader=&maxSpeedReader;
    this->gradeReader=&gradeReader;

    //
    // Handling of restriction relations
    //

    progress.SetAction("Scanning for restriction relations");

    if (!ReadTurnRestrictions(parameter,
                              progress,
                              restrictions)) {
      return false;
    }

    progress.Info(std::string("Restrictions for ") + std::to_string(restrictions.size()) + " objects loaded");

    //
    // Building a map of nodes and the number of ways that contain this way
    //

    progress.SetAction("Scanning for intersections");

    if (!ReadIntersections(parameter,
                           progress,
                           *typeConfig,
                           nodeUseMap)) {
      return false;
    }

    //
    // Building a map of endpoint ids and list of objects (ObjectFileRef for ways and areas) having this point as junction point
    //

    progress.SetAction("Collecting objects at intersections");

    if (!ReadObjectsAtIntersections(parameter,
                                    progress,
                                    *typeConfig,
                                    nodeUseMap,
                                    nodeObjectsMap)) {
      return false;
    }

    // We now have the nodeObjectsMap, we do not need this information anymore
    nodeUseMap.Clear();

    progress.SetAction("Postprocessing intersections");


    // We sort objects by increasing file offset, for more efficient storage
    // in route node
    for (auto& entry : nodeObjectsMap) {
      entry.second.sort(ObjectFileRefByFileOffsetComparator());
    }

    progress.SetAction("Writing intersection file '{}'",RoutingService::FILENAME_INTERSECTIONS_DAT);

    if (!WriteIntersections(parameter,
                            progress,
                            nodeObjectsMap)) {
      return false;
    }

    Magnification    magnification(std::pow(2,parameter.GetRouteNodeTileMag()));
    RawRouteNodeList rawRouteNodes;

    std::transform(nodeObjectsMap.begin(),
                   nodeObjectsMap.end(),
                   std::back_inserter(rawRouteNodes),
                   [&magnification](NodeIdObjectsMap::value_type& entry) {
      return RawRouteNode{entry.first,
                          TileId::GetTile(magnification,Point::GetCoordFromId(entry.first)).AsPixel(),
                          std::move(entry.second)};
    });

    nodeObjectsMap.clear();

    progress.SetAction("Sorting route nodes");

    rawRouteNodes.sort([](const RawRouteNode& a, const RawRouteNode& b) {
      return a.cell<b.cell;
    });

    progress.SetAction("Generating routeNodeIdSet");

    RouteNodeIdSet routeNodeIdSet;

    std::for_each(rawRouteNodes.begin(),
                  rawRouteNodes.end(),
                  [&routeNodeIdSet](const RawRouteNode& node) {
                    routeNodeIdSet.insert(node.id);
                  });

    for (const auto& router : parameter.GetRouter()) {
      std::string dataFilename=AppendFileToDir(parameter.GetDestinationDirectory(),
                                               router.GetDataFilename());
      std::string variantFilename=AppendFileToDir(parameter.GetDestinationDirectory(),
                                                  router.GetVariantFilename());

      progress.SetAction("Writing route graph to '"s+dataFilename+"' and '"s+variantFilename+"'"s);

      WriteRouteGraph(parameter,
                      progress,
                      *typeConfig,
                      rawRouteNodes,
                      routeNodeIdSet,
                      restrictions,
                      router.GetVehicleMask(),
                      dataFilename,
                      variantFilename);
    }

    // Cleaning up...

    nodeObjectsMap.clear();
    restrictions.clear();

    return true;
  }
}
