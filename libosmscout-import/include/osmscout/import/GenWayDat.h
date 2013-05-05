#ifndef OSMSCOUT_IMPORT_GENWAYDAT_H
#define OSMSCOUT_IMPORT_GENWAYDAT_H

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

#include <osmscout/ImportFeatures.h>

#include <map>

#include <osmscout/Way.h>
#include <osmscout/NumericIndex.h>
#include <osmscout/TurnRestriction.h>

#include <osmscout/util/HashMap.h>
#include <osmscout/util/HashSet.h>

#include <osmscout/import/CoordDataFile.h>
#include <osmscout/import/Import.h>
#include <osmscout/import/RawWay.h>

namespace osmscout {

  class WayDataGenerator : public ImportModule
  {
  private:
    typedef OSMSCOUT_HASHMAP<PageId,uint32_t>      NodeUseMap;
    typedef OSMSCOUT_HASHSET<OSMId>                BlacklistSet;

    typedef std::list<RawWayRef>                   WayList;
    typedef WayList::iterator                      WayListPtr;
    typedef std::list<WayListPtr>                  WayListPtrList;
    typedef OSMSCOUT_HASHMAP<OSMId,WayListPtrList> WaysByNodeMap;

    void GetWayTypes(const TypeConfig& typeConfig,
                     std::set<TypeId>& types) const;

    void SetNodeUsed(NodeUseMap& nodeUseMap,
                     OSMId id);

    bool IsNodeUsedAtLeastTwice(const NodeUseMap& nodeUseMap,
                                OSMId id) const;

    bool ReadWayBlacklist(const ImportParameter& parameter,
                          Progress& progress,
                          BlacklistSet& wayBlacklist);

    bool ReadTurnRestrictions(const ImportParameter& parameter,
                              Progress& progress,
                              std::multimap<OSMId,TurnRestrictionRef>& restrictions);

    bool WriteTurnRestrictions(const ImportParameter& parameter,
                               Progress& progress,
                               std::multimap<OSMId,TurnRestrictionRef>& restrictions);

    bool GetWays(const ImportParameter& parameter,
                 Progress& progress,
                 const TypeConfig& typeConfig,
                 std::set<TypeId>& types,
                 const BlacklistSet& blacklist,
                 FileScanner& scanner,
                 std::vector<std::list<RawWayRef> >& ways,
                 std::vector<std::list<RawWayRef> >& areas,
                 NodeUseMap& nodeUseMap,
                 bool buildNodeUseMap);

    void UpdateRestrictions(std::multimap<OSMId,TurnRestrictionRef>& restrictions,
                            OSMId oldId,
                            OSMId newId);

    bool IsRestricted(const std::multimap<OSMId,TurnRestrictionRef>& restrictions,
                      OSMId wayId,
                      OSMId nodeId) const;

    bool MergeWays(Progress& progress,
                   const TypeConfig& typeConfig,
                   std::list<RawWayRef>& ways,
                   BlacklistSet& wayBlacklist,
                   std::multimap<OSMId,TurnRestrictionRef>& restrictions);

    bool WriteWay(const ImportParameter& parameter,
                  Progress& progress,
                  const TypeConfig& typeConfig,
                  FileWriter& writer,
                  uint32_t& writtenWayCount,
                  const CoordDataFile::CoordResultMap& coordsMap,
                  const NodeUseMap& nodeUseMap,
                  const RawWay& rawWay);

  public:
    std::string GetDescription() const;
    bool Import(const ImportParameter& parameter,
                Progress& progress,
                const TypeConfig& typeConfig);
  };
}

#endif
