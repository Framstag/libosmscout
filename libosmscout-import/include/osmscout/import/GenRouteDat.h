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

#include <osmscout/Area.h>
#include <osmscout/Way.h>

#include <osmscout/ObjectRef.h>

#include <osmscout/util/FileWriter.h>
#include <osmscout/util/HashMap.h>
#include <osmscout/util/HashSet.h>

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

    typedef OSMSCOUT_HASHMAP<PageId,uint32_t>              NodeUseMap;
    typedef OSMSCOUT_HASHMAP<Id, FileOffset>               NodeIdOffsetMap;
    typedef std::map<Id,std::list<ObjectFileRef> >         NodeIdObjectsMap;
    typedef std::map<Id,std::list<PendingOffset> >         PendingRouteNodeOffsetsMap;
    typedef std::map<Id,std::vector<TurnRestrictionData> > ViaTurnRestrictionMap;

  private:
    bool ReadTurnRestrictionWayIds(const ImportParameter& parameter,
                                   Progress& progress,
                                   std::map<Id,FileOffset>& wayIdOffsetMap);

    void SetNodeUsed(NodeUseMap& nodeUseMap,
                    OSMId id);
    bool IsNodeUsedAtLeastTwice(const NodeUseMap& nodeUseMap,
                                OSMId id) const;

    bool ResolveWayIdsToFileOffsets(const ImportParameter& parameter,
                                    Progress& progress,
                                    std::map<Id,FileOffset>& wayIdOffsetMap);

    bool ReadTurnRestrictionData(const ImportParameter& parameter,
                                 Progress& progress,
                                 const std::map<Id,FileOffset>& wayIdOffsetMap,
                                 ViaTurnRestrictionMap& restrictions);

    bool ReadTurnRestrictions(const ImportParameter& parameter,
                              Progress& progress,
                              ViaTurnRestrictionMap& restrictions);

    bool CanTurn(const std::vector<TurnRestrictionData>& restrictions,
                 FileOffset from,
                 FileOffset to) const;

    bool ReadJunctions(const ImportParameter& parameter,
                       Progress& progress,
                       const TypeConfig& typeConfig,
                       NodeUseMap& nodeUseMap);

    bool ReadWayObjectsAtJunctions(const ImportParameter& parameter,
                                   Progress& progress,
                                   const TypeConfig& typeConfig,
                                   const NodeUseMap& nodeUseMap,
                                   NodeIdObjectsMap& nodeObjectsMap);

    bool LoadWays(Progress& progress,
                  FileScanner& scanner,
                  const std::set<FileOffset>& fileOffsets,
                  OSMSCOUT_HASHMAP<FileOffset,WayRef>& waysMap);

    bool LoadAreas(Progress& progress,
                   FileScanner& scanner,
                   const std::set<FileOffset>& fileOffsets,
                   OSMSCOUT_HASHMAP<FileOffset,AreaRef>& areasMap);

    /*
    uint8_t CalculateEncodedBearing(const Way& way,
                                    size_t currentNode,
                                    size_t nextNode,
                                    bool clockwise) const;*/

    void CalculateAreaPaths(RouteNode& routeNode,
                            const Area& area,
                            FileOffset routeNodeOffset,
                            const NodeIdObjectsMap& nodeObjectsMap,
                            const NodeIdOffsetMap& nodeIdOffsetMap,
                            PendingRouteNodeOffsetsMap& pendingOffsetsMap);
    void CalculateCircularWayPaths(RouteNode& routeNode,
                                   const Way& way,
                                   FileOffset routeNodeOffset,
                                   const NodeIdObjectsMap& nodeObjectsMap,
                                   const NodeIdOffsetMap& nodeIdOffsetMap,
                                   PendingRouteNodeOffsetsMap& pendingOffsetsMap);
    void CalculateWayPaths(RouteNode& routeNode,
                           const Way& way,
                           FileOffset routeNodeOffset,
                           const NodeIdObjectsMap& nodeObjectsMap,
                           const NodeIdOffsetMap& nodeIdOffsetMap,
                           PendingRouteNodeOffsetsMap& pendingOffsetsMap);

    void FillRoutePathExcludes(RouteNode& routeNode,
                               const std::list<ObjectFileRef>& objects,
                               const ViaTurnRestrictionMap& restrictions);

    bool HandlePendingOffsets(const ImportParameter& parameter,
                              Progress& progress,
                              const NodeIdOffsetMap& routeNodeIdOffsetMap,
                              PendingRouteNodeOffsetsMap& pendingOffsetsMap,
                              FileWriter& routeNodeWriter,
                              std::vector<NodeIdObjectsMap::iterator>& block,
                              size_t blockCount);

  public:
    std::string GetDescription() const;
    bool Import(const ImportParameter& parameter,
                Progress& progress,
                const TypeConfig& typeConfig);
  };
}

#endif
