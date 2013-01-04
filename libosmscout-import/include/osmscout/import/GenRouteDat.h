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
#include <osmscout/Way.h>

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

    typedef std::map<Id,std::list<FileOffset> > NodeIdWayOffsetMap;

  private:
    bool ReadTurnRestrictions(const ImportParameter& parameter,
                              Progress& progress,
                              std::map<Id,std::vector<TurnRestrictionData> >& restrictions);

    bool CanTurn(const std::vector<TurnRestrictionData>& restrictions,
                 FileOffset from,
                 FileOffset to) const;

    bool ReadJunctions(const ImportParameter& parameter,
                       Progress& progress,
                       const TypeConfig& typeConfig,
                       OSMSCOUT_HASHSET<Id>& junctions);

    bool ReadWayEndpoints(const ImportParameter& parameter,
                          Progress& progress,
                          const TypeConfig& typeConfig,
                          const OSMSCOUT_HASHSET<Id>& junctions,
                          NodeIdWayOffsetMap& endPointWayMap);

    bool LoadWays(Progress& progress,
                  FileScanner& scanner,
                  const std::set<FileOffset>& fileOffsets,
                  OSMSCOUT_HASHMAP<FileOffset,WayRef>& waysMap);

    uint8_t CalculateEncodedBearing(const Way& way,
                                    size_t currentNode,
                                    size_t nextNode,
                                    bool clockwise) const;

    void CalculateAreaPaths(RouteNode& routeNode,
                            const Way& way,
                            const NodeIdWayOffsetMap& nodeWayMap);
    void CalculateCircularWayPaths(RouteNode& routeNode,
                                   const Way& way,
                                   const NodeIdWayOffsetMap& nodeWayMap);
    void CalculateWayPaths(RouteNode& routeNode,
                           const Way& way,
                           const NodeIdWayOffsetMap& nodeWayMap);

    bool AddFileOffsetsToRouteNodes(const ImportParameter& parameter,
                                    Progress& progress,
                                    FileWriter& writer,
                                    uint32_t writtenRouteNodeCount);

  public:
    std::string GetDescription() const;
    bool Import(const ImportParameter& parameter,
                Progress& progress,
                const TypeConfig& typeConfig);
  };
}

#endif
