#ifndef OSMSCOUT_IMPORT_GENROUTEDAT_H
#define OSMSCOUT_IMPORT_GENROUTEDAT_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2009  Tim Teulings

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

#include <list>
#include <map>
#include <unordered_map>
#include <vector>

#include <osmscout/NumericIndex.h>
#include <osmscout/routing/RouteNode.h>
#include <osmscout/routing/TurnRestriction.h>

#include <osmscout/Types.h>
#include <osmscout/TypeFeatures.h>

#include <osmscout/Area.h>
#include <osmscout/Way.h>

#include <osmscout/ObjectRef.h>

#include <osmscout/util/FileWriter.h>
#include <osmscout/util/NodeUseMap.h>

#include <osmscout/import/Import.h>

#include <osmscout/system/Compiler.h>

namespace osmscout {

  class RouteDataGenerator CLASS_FINAL : public ImportModule
  {
  private:

    struct RawRouteNode
    {
      Id                       id;
      Pixel                    cell;
      std::list<ObjectFileRef> objects;
    };

    struct TurnRestrictionData
    {
      enum Type
      {
        Allow  = 0,
        Forbit = 1
      };

      FileOffset fromWayOffset; //!< FileOffset of the from way
      Id         viaNodeId;     //!< Libosmscout specific Id for the via node, from and to way should have a node with this id
      FileOffset toWayOffset;   //!< FileOffset of the from way
      Type       type;          //!< Type of restriction
    };

    struct PendingOffset
    {
      FileOffset routeNodeOffset;
      size_t     index;

      inline PendingOffset(FileOffset routeNodeOffset,
                           size_t index)
      : routeNodeOffset(routeNodeOffset),
        index(index)
      {

      }
    };

    typedef std::unordered_map<FileOffset,WayRef>                   FileOffsetWayMap;
    typedef std::unordered_map<FileOffset,AreaRef>                  FileOffsetAreaMap;
    typedef std::unordered_set<Id>                                  RouteNodeIdSet;
    typedef std::vector<RawRouteNode>                               RawRouteNodeList;
    typedef std::unordered_map<Id,FileOffset>                       NodeIdOffsetMap;
    typedef std::map<Id,std::list<ObjectFileRef>>                   NodeIdObjectsMap;
    typedef std::unordered_map<Id,std::list<PendingOffset>>         PendingRouteNodeOffsetsMap;
    typedef std::unordered_map<Id,std::vector<TurnRestrictionData>> ViaTurnRestrictionMap;

    AccessFeatureValueReader           *accessReader;
    AccessRestrictedFeatureValueReader *accessRestrictedReader;
    MaxSpeedFeatureValueReader         *maxSpeedReader;
    GradeFeatureValueReader            *gradeReader;

  private:
    AccessFeatureValue GetAccess(const FeatureValueBuffer& buffer) const;

    inline AccessFeatureValue GetAccess(const Way& way) const
    {
      return GetAccess(way.GetFeatureValueBuffer());
    }

    uint8_t GetMaxSpeed(const Way& way) const;
    uint8_t GetGrade(const Way& way) const;

    uint8_t CopyFlags(const Area::Ring& ring) const;
    uint8_t CopyFlagsForward(const Way& way) const;
    uint8_t CopyFlagsBackward(const Way& way) const;

    uint16_t RegisterOrUseObjectVariantData(std::map<ObjectVariantData,uint16_t>& routeDataMap,
                                            const TypeInfoRef& type,
                                            uint8_t maxSpeed,
                                            uint8_t grade);

    bool IsAnyRoutable(Progress& progress,
                       const RawRouteNode& node,
                       const std::unordered_map<FileOffset,WayRef>& waysMap,
                       const std::unordered_map<FileOffset,AreaRef>&  areasMap,
                       VehicleMask vehicles) const;

    /**
     * Read turn restrictions and return a map of OSM way ids and OSM node ids together with their (set to 0) file offset
     */
    bool ReadTurnRestrictionIds(const ImportParameter& parameter,
                                Progress& progress,
                                std::map<OSMId,FileOffset>& wayIdOffsetMap,
                                std::map<OSMId,Id>& nodeMap);

    /**
     * Resove the file offsets for the way ids given in the wayIdOffsetMap
     */
    bool ResolveWayIds(const ImportParameter& parameter,
                       Progress& progress,
                       std::map<OSMId,FileOffset>& wayIdOffsetMap);

    /**
     * Resove the node ids from the OSM node ids given in the nodeIdMap
     */
    bool ResolveNodeIds(const ImportParameter& parameter,
                        Progress& progress,
                        std::map<OSMId,Id>& nodeIdMap);

    /**
     * Red the turn restriction again using the "way id to file offset" map and create a ViaTurnRestrictionMap.
     */
    bool ReadTurnRestrictionData(const ImportParameter& parameter,
                                 Progress& progress,
                                 const std::map<OSMId,Id>& nodeIdMap,
                                 const std::map<OSMId,FileOffset>& wayIdOffsetMap,
                                 ViaTurnRestrictionMap& restrictions);

    /**
     * Helper method that sequentially calls ReadTurnRestrictionWayIds(),
     * ResolveWayIdsToFileOffsets() and ReadTurnRestrictionData()
     */
    bool ReadTurnRestrictions(const ImportParameter& parameter,
                              Progress& progress,
                              ViaTurnRestrictionMap& restrictions);

    /**
     * Calculates if one can travel from Way "from" to Way "to" with the restrictions given for the
     * intersecting node.
     */
    bool CanTurn(const std::vector<TurnRestrictionData>& restrictions,
                 FileOffset from,
                 FileOffset to) const;

    /**
     * Reads all relevant ways and areas and returns all nodes where these intersect.
     */
    bool ReadIntersections(const ImportParameter& parameter,
                           Progress& progress,
                           const TypeConfig& typeConfig,
                           NodeUseMap& nodeUseMap);

    /**
     * Builds up a list of ObjectFileRefs for every junction node.
     */
    bool ReadObjectsAtIntersections(const ImportParameter& parameter,
                                    Progress& progress,
                                    const TypeConfig& typeConfig,
                                    const NodeUseMap& nodeUseMap,
                                    NodeIdObjectsMap& nodeObjectsMap);

    bool WriteIntersections(const ImportParameter& parameter,
                            Progress& progress,
                            NodeIdObjectsMap& nodeIdObjectsMap);

    /**
     * Loads ways based on their file offset.
     */
    FileOffsetWayMap LoadWays(Progress& progress,
                              const TypeConfig& typeConfig,
                              FileScanner& scanner,
                              RawRouteNodeList::const_iterator startNode,
                              RawRouteNodeList::const_iterator endNode);

    /**
     * Loads areas based on their file offset.
     */
    FileOffsetAreaMap LoadAreas(Progress& progress,
                                const TypeConfig& typeConfig,
                                FileScanner& scanner,
                                RawRouteNodeList::const_iterator startNode,
                                RawRouteNodeList::const_iterator endNode);

    bool GetRouteNodePoint(Progress& progress,
                           const RawRouteNode& node,
                           FileOffsetWayMap& waysMap,
                           FileOffsetAreaMap& areasMap,
                           Point& point) const;

    /*
    uint8_t CalculateEncodedBearing(const Way& way,
                                    size_t currentNode,
                                    size_t nextNode,
                                    bool clockwise) const;*/

    /**
     * Calculate all possible route from the given route node for the given area
     */
    void CalculateAreaPaths(RouteNode& routeNode,
                            const Area& area,
                            uint16_t objectVariantIndex,
                            const RouteNodeIdSet& routeNodeIdSet,
                            const NodeIdOffsetMap& nodeIdOffsetMap,
                            PendingRouteNodeOffsetsMap& pendingOffsetsMap);

    /**
     * Calculate all possible route from the given route node for the given circular way
     */
    void CalculateCircularWayPaths(RouteNode& routeNode,
                                   const Way& way,
                                   uint16_t objectVariantIndex,
                                   const RouteNodeIdSet& routeNodeIdSet,
                                   const NodeIdOffsetMap& nodeIdOffsetMap,
                                   PendingRouteNodeOffsetsMap& pendingOffsetsMap);

    /**
     * Calculate all possible route from the given route node for the given non-circular way
     */
    void CalculateWayPaths(RouteNode& routeNode,
                           const Way& way,
                           uint16_t objectVariantIndex,
                           const RouteNodeIdSet& routeNodeIdSet,
                           const NodeIdOffsetMap& nodeIdOffsetMap,
                           PendingRouteNodeOffsetsMap& pendingOffsetsMap);

    /**
     * Adds the result of the turn restriction evaluation to the route node.
     */
    void FillRoutePathExcludes(RouteNode& routeNode,
                               const RawRouteNode& node,
                               const ViaTurnRestrictionMap& restrictions);

    /**
     * Adds missing file offsets to route nodes that were not written at the time the referencing route node
     * was stored.
     */
    bool HandlePendingOffsets(Progress& progress,
                              const NodeIdOffsetMap& routeNodeIdOffsetMap,
                              PendingRouteNodeOffsetsMap& pendingOffsetsMap,
                              FileWriter& routeNodeWriter,
                              RawRouteNodeList::const_iterator nodesBegin,
                              RawRouteNodeList::const_iterator nodesEnd);

    bool WriteObjectVariantData(Progress& progress,
                                const std::string& variantFilename,
                                const std::map<ObjectVariantData,uint16_t>& routeDataMap);

    bool WriteRouteGraph(const ImportParameter& parameter,
                         Progress& progress,
                         const TypeConfig& typeConfig,
                         const RawRouteNodeList& rawRouteNodes,
                         const RouteNodeIdSet& routeNodeIdSet,
                         const ViaTurnRestrictionMap& restrictions,
                         VehicleMask vehicles,
                         const std::string& dataFilename,
                         const std::string& variantFilename);

  public:
    RouteDataGenerator();

    void GetDescription(const ImportParameter& parameter,
                        ImportModuleDescription& description) const override;

    bool Import(const TypeConfigRef& typeConfig,
                const ImportParameter& parameter,
                Progress& progress) override;
  };
}

#endif
