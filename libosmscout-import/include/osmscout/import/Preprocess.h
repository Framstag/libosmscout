#ifndef OSMSCOUT_IMPORT_PREPROCESS_H
#define OSMSCOUT_IMPORT_PREPROCESS_H

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

#include <osmscout/import/Import.h>
#include <osmscout/import/RawRelation.h>

#include <osmscout/util/HashMap.h>
#include <osmscout/util/FileWriter.h>

namespace osmscout {
  class Preprocess : public ImportModule
  {
  private:
    typedef OSMSCOUT_HASHMAP<Id,FileOffset> CoordPageOffsetMap;

  private:
    FileWriter          nodeWriter;
    FileWriter          wayWriter;
    FileWriter          relationWriter;
    FileWriter          coastlineWriter;
    FileWriter          coordWriter;

    Id                  coordPageCount;
    CoordPageOffsetMap  coordIndex;

    std::vector<Tag>    tags;

    uint32_t            nodeCount;
    uint32_t            wayCount;
    uint32_t            areaCount;
    uint32_t            relationCount;
    uint32_t            coastlineCount;

    uint32_t            lastNodeId;
    uint32_t            lastWayId;
    uint32_t            lastRelationId;

    bool                nodeSortingError;
    bool                waySortingError;
    bool                relationSortingError;

    Id                  currentPageId;
    FileOffset          currentPageOffset;
    std::vector<double> lats;
    std::vector<double> lons;
    std::vector<bool>   isSet;

  private:
    bool StoreCurrentPage();
    bool StoreCoord(Id id, double lat, double lon);

  public:
    std::string GetDescription() const;
    bool Import(const ImportParameter& parameter,
                Progress& progress,
                const TypeConfig& typeConfig);

    bool Initialize(const ImportParameter& parameter);

    void ProcessNode(const TypeConfig& typeConfig,
                     const Id& id,
                     const double& lon, const double& lat,
                     const std::map<TagId,std::string>& tags);
    void ProcessWay(const TypeConfig& typeConfig,
                    const Id& id,
                    std::vector<Id>& nodes,
                    const std::map<TagId,std::string>& tags);
    void ProcessRelation(const TypeConfig& typeConfig,
                         const Id& id,
                         const std::vector<RawRelation::Member>& members,
                         const std::map<TagId,std::string>& tags);

    bool Cleanup(const ImportParameter& parameter,
                 Progress& progress);
  };
}

#endif
