#ifndef OSMSCOUT_IMPORT_GENMMERGEAREAS_H
#define OSMSCOUT_IMPORT_GENMERGEAREAS_H

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

#include <osmscout/util/HashSet.h>
#include <osmscout/util/NodeUseMap.h>

namespace osmscout {

  class MergeAreasGenerator : public ImportModule
  {
  private:
    size_t GetFirstOuterRingWithId(const Area& area,
                                   Id id) const;

    void EraseAreaInCache(const NodeUseMap& nodeUseMap,
                          OSMSCOUT_HASHMAP<Id,std::list<AreaRef> >& idAreaMap,
                          const AreaRef& area);

    bool ScanAreaIds(const ImportParameter& parameter,
                     Progress& progress,
                     const TypeConfig& typeConfig,
                     const TypeInfoSet& mergeTypes,
                     std::vector<NodeUseMap>& nodeUseMap);

    bool GetAreas(const ImportParameter& parameter,
                  Progress& progress,
                  const TypeConfig& typeConfig,
                  TypeInfoSet& types,
                  std::vector<NodeUseMap>& nodeUseMap,
                  FileScanner& scanner,
                  std::vector<std::list<AreaRef> >& areas);

    void MergeAreas(const NodeUseMap& nodeUseMap,
                    std::list<AreaRef>& areas,
                    std::list<AreaRef>& merges,
                    OSMSCOUT_HASHSET<FileOffset>& blacklist);

  public:
    std::string GetDescription() const;
    bool Import(const TypeConfigRef& typeConfig,
                const ImportParameter& parameter,
                Progress& progress);
  };
}

#endif
