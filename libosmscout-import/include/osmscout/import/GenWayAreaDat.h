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

#include <osmscout/import/ImportFeatures.h>

#include <tuple>
#include <unordered_set>

#include <osmscout/Area.h>

#include <osmscout/CoordDataFile.h>
#include <osmscout/NumericIndex.h>

#include <osmscout/TypeInfoSet.h>

#include <osmscout/routing/TurnRestriction.h>

#include <osmscout/import/Import.h>
#include <osmscout/import/RawWay.h>

#include <osmscout/system/Compiler.h>

namespace osmscout {

  class WayAreaDataGenerator CLASS_FINAL : public ImportModule
  {
  public:
    static const char* WAYAREA_TMP;

  private:
    struct Distribution
    {
      uint32_t nodeCount;
      uint32_t wayCount;
      uint32_t areaCount;

      Distribution();
    };

    using BlacklistSet = std::unordered_set<OSMId>;
    using WayList      = std::list<RawWayRef>;

    std::tuple<BlacklistSet,bool> ReadWayBlacklist(const ImportParameter& parameter,
                                                   Progress& progress) const;

    void WriteArea(const ImportParameter& parameter,
                   Progress& progress,
                   const TypeConfig& typeConfig,
                   FileWriter& writer,
                   uint32_t& writtenWayCount,
                   const CoordDataFile::ResultMap& coordsMap,
                   const RawWay& rawWay);

  public:
    void GetDescription(const ImportParameter& parameter,
                        ImportModuleDescription& description) const override;

    bool Import(const TypeConfigRef& typeConfig,
                const ImportParameter& parameter,
                Progress& progress) override;
  };
}

#endif
