#ifndef OSMSCOUT_IMPORT_GENNODEUSEINDEX_H
#define OSMSCOUT_IMPORT_GENNODEUSEINDEX_H

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

#include <list>
#include <map>

namespace osmscout {

  class NodeUseIndexGenerator : public ImportModule
  {
  private:
    struct IndexEntry
    {
      TypeId type;
      Id     wayId;

      inline bool operator<(const IndexEntry& other) const
      {
        if (wayId<other.wayId) {
          return true;
        }

        if (type<other.type) {
          return true;
        }

        return true;
      }

      inline bool operator==(const IndexEntry& other) const
      {
        return type==other.type && wayId==other.wayId;
      }
    };

  private:
    bool GetNodeDistribution(const ImportParameter& parameter,
                             Progress& progress,
                             std::vector<uint32_t>& nodeDistribution);
    bool GetNodeWayMap(const ImportParameter& parameter,
                       Progress& progress,
                       const std::set<TypeId>& types,
                       size_t start,
                       size_t end,
                       std::map<Id,std::list<IndexEntry> >& nodeWayMap);

    bool ResolveReferences(const ImportParameter& parameter,
                           Progress& progress,
                           const std::set<TypeId>& types,
                           size_t start,
                           size_t end,
                           const std::map<Id,std::list<IndexEntry> >& nodeWayMap,
                           std::map<Id,std::list<IndexEntry> >& wayWayMap);

  public:
    std::string GetDescription() const;
    bool Import(const ImportParameter& parameter,
                Progress& progress,
                const TypeConfig& typeConfig);
  };
}

#endif
