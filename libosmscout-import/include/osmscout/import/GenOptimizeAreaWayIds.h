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

#include <map>

#include <osmscout/import/Import.h>

#include <osmscout/Area.h>
#include <osmscout/Way.h>

#include <osmscout/util/NodeUseMap.h>

namespace osmscout {

  class OptimizeAreaWayIdsGenerator : public ImportModule
  {
  private:
    bool ScanWayAreaIds(const ImportParameter& parameter,
                        Progress& progress,
                        const TypeConfig& typeConfig,
                        NodeUseMap& nodeUseMap);

    bool ScanRelAreaIds(const ImportParameter& parameter,
                        Progress& progress,
                        const TypeConfig& typeConfig,
                        NodeUseMap& nodeUseMap);

    bool ScanWayWayIds(const ImportParameter& parameter,
                       Progress& progress,
                       const TypeConfig& typeConfig,
                       NodeUseMap& nodeUseMap);

    bool CopyWayArea(const ImportParameter& parameter,
                     Progress& progress,
                     const TypeConfig& typeConfig,
                     NodeUseMap& nodeUseMap);

    bool CopyRelArea(const ImportParameter& parameter,
                     Progress& progress,
                     const TypeConfig& typeConfig,
                     NodeUseMap& nodeUseMap);

    bool CopyWayWay(const ImportParameter& parameter,
                    Progress& progress,
                    const TypeConfig& typeConfig,
                    NodeUseMap& nodeUseMap);
  public:
    std::string GetDescription() const;
    bool Import(const TypeConfigRef& typeConfig,
                const ImportParameter& parameter,
                Progress& progress);
  };
}

#endif
