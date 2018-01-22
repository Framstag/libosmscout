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

#include <unordered_map>

#include <osmscout/Way.h>

#include <osmscout/Coord.h>
#include <osmscout/CoordDataFile.h>
#include <osmscout/NumericIndex.h>
#include <osmscout/routing/TurnRestriction.h>

#include <osmscout/import/Import.h>
#include <osmscout/import/RawWay.h>

#include <osmscout/system/Compiler.h>

namespace osmscout {

  class WayWayDataGenerator CLASS_FINAL : public ImportModule
  {
  public:
    static const char* WAYWAY_TMP;
    static const char* TURNRESTR_DAT;

  private:
    struct RestrictionData
    {
      std::multimap<OSMId,TurnRestrictionRef> restrictions;
    };

    typedef std::list<RawWayRef>                     WayList;
    typedef WayList::iterator                        WayListPtr;
    typedef std::list<WayListPtr>                    WayListPtrList;
    typedef std::unordered_map<OSMId,WayListPtrList> WaysByNodeMap;

    bool ReadTurnRestrictions(const ImportParameter& parameter,
                              Progress& progress,
                              RestrictionData& restrictions);

    bool WriteTurnRestrictions(const ImportParameter& parameter,
                               Progress& progress,
                               const RestrictionData& restrictions);

    bool GetWays(const ImportParameter& parameter,
                 Progress& progress,
                 const TypeConfig& typeConfig,
                 TypeInfoSet& types,
                 FileScanner& scanner,
                 std::vector<std::list<RawWayRef> >& ways);

    void UpdateRestrictions(RestrictionData& restrictions,
                            OSMId oldId,
                            OSMId newId);

    bool IsRestricted(const RestrictionData& restrictions,
                      OSMId wayId,
                      OSMId nodeId) const;

    bool MergeWays(Progress& progress,
                   std::list<RawWayRef>& ways,
                   RestrictionData& restrictions);

    bool SplitLongWays(Progress& progress,
                       std::list<RawWayRef>& ways,
                       CoordDataFile::ResultMap& coordsMap);

    void WriteWay(Progress& progress,
                  const TypeConfig& typeConfig,
                  FileWriter& writer,
                  uint32_t& writtenWayCount,
                  const CoordDataFile::ResultMap& coordsMap,
                  const RawWay& rawWay);

    bool HandleLowMemoryFallback(Progress& progress,
                                 const TypeConfig& typeConfig,
                                 FileScanner& scanner,
                                 const TypeInfoSet& types,
                                 FileWriter& writer,
                                 uint32_t& writtenWayCount,
                                 const CoordDataFile& coordDataFile);

  public:
    void GetDescription(const ImportParameter& parameter,
                        ImportModuleDescription& description) const override;

    bool Import(const TypeConfigRef& typeConfig,
                const ImportParameter& parameter,
                Progress& progress) override;
  };
}

#endif
