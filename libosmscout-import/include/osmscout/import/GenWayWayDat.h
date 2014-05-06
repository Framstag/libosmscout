#ifndef OSMSCOUT_IMPORT_GENWAYWAYDAT_H
#define OSMSCOUT_IMPORT_GENWAYWAYDAT_H

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

#include <osmscout/CoordDataFile.h>
#include <osmscout/NumericIndex.h>
#include <osmscout/TurnRestriction.h>

#include <osmscout/util/HashMap.h>
#include <osmscout/util/HashSet.h>

#include <osmscout/import/Import.h>
#include <osmscout/import/RawWay.h>

namespace osmscout {

  class WayWayDataGenerator : public ImportModule
  {
  private:
    typedef std::list<RawWayRef>                   WayList;
    typedef WayList::iterator                      WayListPtr;
    typedef std::list<WayListPtr>                  WayListPtrList;
    typedef OSMSCOUT_HASHMAP<OSMId,WayListPtrList> WaysByNodeMap;

    bool ReadTurnRestrictions(const ImportParameter& parameter,
                              Progress& progress,
                              std::multimap<OSMId,TurnRestrictionRef>& restrictions);

    bool WriteTurnRestrictions(const ImportParameter& parameter,
                               Progress& progress,
                               std::multimap<OSMId,TurnRestrictionRef>& restrictions);

    bool GetWays(const ImportParameter& parameter,
                 Progress& progress,
                 std::set<TypeId>& types,
                 FileScanner& scanner,
                 std::vector<std::list<RawWayRef> >& ways);

    void UpdateRestrictions(std::multimap<OSMId,TurnRestrictionRef>& restrictions,
                            OSMId oldId,
                            OSMId newId);

    bool IsRestricted(const std::multimap<OSMId,TurnRestrictionRef>& restrictions,
                      OSMId wayId,
                      OSMId nodeId) const;

    bool MergeWays(Progress& progress,
                   const TypeConfig& typeConfig,
                   std::list<RawWayRef>& ways,
                   std::multimap<OSMId,TurnRestrictionRef>& restrictions);

    bool WriteWay(Progress& progress,
                  const TypeConfig& typeConfig,
                  FileWriter& writer,
                  uint32_t& writtenWayCount,
                  const CoordDataFile::CoordResultMap& coordsMap,
                  const RawWay& rawWay);

  public:
    std::string GetDescription() const;
    bool Import(const ImportParameter& parameter,
                Progress& progress,
                const TypeConfig& typeConfig);
  };
}

#endif
