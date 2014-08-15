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

#include <osmscout/NumericIndex.h>
#include <osmscout/RouteNode.h>
#include <osmscout/TurnRestriction.h>
#include <osmscout/Types.h>

#include <osmscout/Area.h>
#include <osmscout/Way.h>

#include <osmscout/ObjectRef.h>

#include <osmscout/util/FileWriter.h>
#include <osmscout/util/HashMap.h>
#include <osmscout/util/HashSet.h>
#include <osmscout/util/NodeUseMap.h>

#include <osmscout/import/Import.h>

namespace osmscout {

  class RouteDataGenerator : public ImportModule
  {
  private:
    struct TurnRestrictionData
    {
      enum Type
      {
        Allow  = 0,
        Forbit = 1
      };

      FileOffset fromWayOffset;
      Id         viaNodeId;
      FileOffset toWayOffset;
      Type       type;
    };

    struct PendingOffset
    {
      FileOffset routeNodeOffset;
      size_t     index;
    };

    typedef OSMSCOUT_HASHMAP<Id, FileOffset>               NodeIdOffsetMap;
    typedef std::map<Id,std::list<ObjectFileRef> >         NodeIdObjectsMap;
    typedef std::map<Id,std::list<PendingOffset> >         PendingRouteNodeOffsetsMap;
    typedef std::map<Id,std::vector<TurnRestrictionData> > ViaTurnRestrictionMap;

    AccessFeatureValueReader   *accessReader;
    MaxSpeedFeatureValueReader *maxSpeedReader;
    GradeFeatureValueReader    *gradeReader;

  private:
    AccessFeatureValue GetAccess(const Way& way) const;
    uint8_t GetMaxSpeed(const Way& way) const;
    uint8_t GetGrade(const Way& way) const;

    uint8_t CopyFlagsForward(const Way& way) const;
    uint8_t CopyFlagsBackward(const Way& way) const;

    /**
     * Read turn restrictions and return a map of way ids together with their (set to 0) file offset
     */
    bool ReadTurnRestrictionWayIds(const ImportParameter& parameter,
                                   Progress& progress,
                                   std::map<Id,FileOffset>& wayIdOffsetMap);

    /**
     * Resove the file offsets for the way ids given in the wayIdOffsetMap
     */
    bool ResolveWayIdsToFileOffsets(const ImportParameter& parameter,
                                    Progress& progress,
                                    std::map<Id,FileOffset>& wayIdOffsetMap);

    /**
     * Red the turn restriction again using the "way id to file offset" map and create a ViaTurnRestrictionMap.
     */
    bool ReadTurnRestrictionData(const ImportParameter& parameter,
                                 Progress& progress,
                                 const std::map<Id,FileOffset>& wayIdOffsetMap,
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
    bool LoadWays(const TypeConfig& typeConfig,
                  Progress& progress,
                  FileScanner& scanner,
                  const std::set<FileOffset>& fileOffsets,
                  OSMSCOUT_HASHMAP<FileOffset,WayRef>& waysMap);

    /**
     * Loads areas based on their file offset.
     */
    bool LoadAreas(const TypeConfig& typeConfig,
                   Progress& progress,
                   FileScanner& scanner,
                   const std::set<FileOffset>& fileOffsets,
                   OSMSCOUT_HASHMAP<FileOffset,AreaRef>& areasMap);

    /*
    uint8_t CalculateEncodedBearing(const Way& way,
                                    size_t currentNode,
                                    size_t nextNode,
                                    bool clockwise) const;*/

    /**
     * Calculate all possible route from the given route node for the given area
     */
    void CalculateAreaPaths(const TypeConfig& typeConfig,
                            RouteNode& routeNode,
                            const Area& area,
                            FileOffset routeNodeOffset,
                            const NodeIdObjectsMap& nodeObjectsMap,
                            const NodeIdOffsetMap& nodeIdOffsetMap,
                            PendingRouteNodeOffsetsMap& pendingOffsetsMap);

    /**
     * Calculate all possible route from the given route node for the given circular way
     */
    void CalculateCircularWayPaths(RouteNode& routeNode,
                                   const Way& way,
                                   FileOffset routeNodeOffset,
                                   const NodeIdObjectsMap& nodeObjectsMap,
                                   const NodeIdOffsetMap& nodeIdOffsetMap,
                                   PendingRouteNodeOffsetsMap& pendingOffsetsMap);

    /**
     * Calculate all possible route from the given route node for the given non-circular way
     */
    void CalculateWayPaths(RouteNode& routeNode,
                           const Way& way,
                           FileOffset routeNodeOffset,
                           const NodeIdObjectsMap& nodeObjectsMap,
                           const NodeIdOffsetMap& nodeIdOffsetMap,
                           PendingRouteNodeOffsetsMap& pendingOffsetsMap);

    /**
     * Adds the result of the turn restriction evaluation to the route node.
     */
    void FillRoutePathExcludes(RouteNode& routeNode,
                               const std::list<ObjectFileRef>& objects,
                               const ViaTurnRestrictionMap& restrictions);

    /**
     * Adds missing file offsets to route nodes that were not written at the time the referencing route node
     * was stored.
     */
    bool HandlePendingOffsets(Progress& progress,
                              const NodeIdOffsetMap& routeNodeIdOffsetMap,
                              PendingRouteNodeOffsetsMap& pendingOffsetsMap,
                              FileWriter& routeNodeWriter,
                              std::vector<NodeIdObjectsMap::const_iterator>& block,
                              size_t blockCount);

    bool WriteRouteGraph(const ImportParameter& parameter,
                         Progress& progress,
                         const TypeConfig& typeConfig,
                         const NodeIdObjectsMap& nodeObjectsMap,
                         const ViaTurnRestrictionMap& restrictions,
                         Vehicle vehicle,
                         const std::string& filename);

  public:
    RouteDataGenerator();
    std::string GetDescription() const;
    bool Import(const TypeConfigRef& typeConfig,
                const ImportParameter& parameter,
                Progress& progress);
  };
}

#endif
