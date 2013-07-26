#ifndef OSMSCOUT_IMPORT_GENWAYAREADAT_H
#define OSMSCOUT_IMPORT_GENWAYAREADAT_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2013  Tim Teulings

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

#include <osmscout/Area.h>

#include <osmscout/CoordDataFile.h>
#include <osmscout/NumericIndex.h>
#include <osmscout/TurnRestriction.h>

#include <osmscout/util/HashMap.h>
#include <osmscout/util/HashSet.h>

#include <osmscout/import/Import.h>
#include <osmscout/import/RawWay.h>

namespace osmscout {

  class WayAreaDataGenerator : public ImportModule
  {
  private:
    typedef OSMSCOUT_HASHSET<OSMId>                BlacklistSet;

    typedef std::list<RawWayRef>                   WayList;

    void GetWayTypes(const TypeConfig& typeConfig,
                     std::set<TypeId>& types) const;

    bool ReadWayBlacklist(const ImportParameter& parameter,
                          Progress& progress,
                          BlacklistSet& wayBlacklist);

    bool GetWays(const ImportParameter& parameter,
                 Progress& progress,
                 const TypeConfig& typeConfig,
                 std::set<TypeId>& types,
                 std::set<TypeId>& slowFallbackTypes,
                 const BlacklistSet& blacklist,
                 FileScanner& scanner,
                 std::vector<std::list<RawWayRef> >& areas);

    bool WriteWay(const ImportParameter& parameter,
                  Progress& progress,
                  const TypeConfig& typeConfig,
                  FileWriter& writer,
                  uint32_t& writtenWayCount,
                  const CoordDataFile::CoordResultMap& coordsMap,
                  const RawWay& rawWay);

    bool HandleLowMemoryFallback(const ImportParameter& parameter,
                                 Progress& progress,
                                 const TypeConfig& typeConfig,
                                 FileScanner& scanner,
                                 std::set<TypeId>& types,
                                 const BlacklistSet& blacklist,
                                 FileWriter& writer,
                                 uint32_t& writtenWayCount,
                                 const CoordDataFile& coordDataFile);

  public:
    std::string GetDescription() const;
    bool Import(const ImportParameter& parameter,
                Progress& progress,
                const TypeConfig& typeConfig);
  };
}

#endif
