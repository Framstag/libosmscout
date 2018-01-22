#ifndef OSMSCOUT_IMPORT_GENOPTIMIZEAREAWAYIDS_H
#define OSMSCOUT_IMPORT_GENOPTIMIZEAREAWAYIDS_H

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

#include <osmscout/import/Import.h>

#include <osmscout/Area.h>
#include <osmscout/Way.h>

#include <osmscout/util/NodeUseMap.h>

#include <osmscout/system/Compiler.h>

namespace osmscout {

  class OptimizeAreaWayIdsGenerator CLASS_FINAL : public ImportModule
  {
  public:
    static const char* AREAS3_TMP;
    static const char* WAYS_TMP;

  private:
    bool ScanAreaIds(const ImportParameter& parameter,
                     Progress& progress,
                     const TypeConfig& typeConfig,
                     std::unordered_set<Id>& usedIdSet,
                     std::unordered_set<Id>& usedIdAtLeastTwiceSet);

    bool ScanWayIds(const ImportParameter& parameter,
                    Progress& progress,
                    const TypeConfig& typeConfig,
                    std::unordered_set<Id>& usedIdSet,
                    std::unordered_set<Id>& usedIdAtLeastTwiceSet);

    bool CopyAreas(const ImportParameter& parameter,
                   Progress& progress,
                   const TypeConfig& typeConfig,
                   const std::unordered_set<Id>& usedIdAtLeastTwiceSet);

    bool CopyWays(const ImportParameter& parameter,
                  Progress& progress,
                  const TypeConfig& typeConfig,
                  const std::unordered_set<Id>& usedIdAtLeastTwiceSet);
  public:
    void GetDescription(const ImportParameter& parameter,
                        ImportModuleDescription& description) const override;

    bool Import(const TypeConfigRef& typeConfig,
                const ImportParameter& parameter,
                Progress& progress) override;
  };
}

#endif
