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

#include <osmscout/import/Import.h>
#include <osmscout/import/RawWay.h>

namespace osmscout {

  class WayDataGenerator : public ImportModule
  {
  private:
    typedef OSMSCOUT_HASHMAP<Id,std::list<Id> > EndPointWayMap;
    typedef OSMSCOUT_HASHSET<Id>                EndPointAreaSet;
    typedef OSMSCOUT_HASHSET<Id>                BlacklistSet;

    bool ReadWayBlacklist(const ImportParameter& parameter,
                          Progress& progress,
                          BlacklistSet& wayBlacklist);

    bool ReadTurnRestrictions(const ImportParameter& parameter,
                              Progress& progress,
                              std::multimap<Id,TurnRestrictionRef>& restrictions);

    bool WriteTurnRestrictions(const ImportParameter& parameter,
                               Progress& progress,
                               std::multimap<Id,TurnRestrictionRef>& restrictions);

    bool ReadWayEndpoints(const ImportParameter& parameter,
                          Progress& progress,
                          const TypeConfig& typeConfig,
                          EndPointWayMap& endPointWayMap);

    bool ReadAreasIncludingEndpoints(const ImportParameter& parameter,
                                     Progress& progress,
                                     const TypeConfig& typeConfig,
                                     const EndPointWayMap& endPointWayMap,
                                     EndPointAreaSet& endPointAreaSet);

    void GetWayMergeCandidates(const RawWay& way,
                               const EndPointWayMap& endPointWayMap,
                               const BlacklistSet& wayBlacklist,
                               std::set<Id>& candidates);

    bool LoadWays(Progress& progress,
                  FileScanner& scanner,
                  NumericIndex<Id>& rawWayIndex,
                  const std::set<Id>& ids,
                  std::map<Id,RawWayRef>& ways);

    bool CompareWays(const RawWay& a,
                     const RawWay& b) const;

    void UpdateRestrictions(std::multimap<Id,TurnRestrictionRef>& restrictions,
                            Id oldId,
                            Id newId);

    bool JoinWays(Progress& progress,
                  const TypeConfig& typeConfig,
                  FileScanner& scanner,
                  std::vector<RawWayRef>& rawWays,
                  size_t blockCount,
                  EndPointWayMap& endPointWayMap,
                  NumericIndex<Id>& rawWayIndex,
                  BlacklistSet& wayBlacklist,
                  std::multimap<Id,TurnRestrictionRef>& restrictions,
                  size_t& mergeCount);

  public:
    std::string GetDescription() const;
    bool Import(const ImportParameter& parameter,
                Progress& progress,
                const TypeConfig& typeConfig);
  };
}

#endif
