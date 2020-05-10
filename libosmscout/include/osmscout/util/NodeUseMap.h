#ifndef OSMSCOUT_UTIL_NODEUSEMAP_H
#define OSMSCOUT_UTIL_NODEUSEMAP_H

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

#include <osmscout/CoreImportExport.h>

#include <bitset>
#include <unordered_map>

#include <osmscout/OSMScoutTypes.h>

#include <osmscout/system/Compiler.h>

namespace osmscout {

  /**
   * \ingroup Util
   * The NodeUseMap can be used to efficiently check if an
   * id used at least twice. In concrete it is used, to
   * check if a node id is shared by multiple ways/areas.
   *
   * It is expected that ids are not sparse but continuously
   * used. So while the data structure works for Id, it will
   * likely not work for OSMId.
   *
   * It internally used large bitsets in a hashset to allow
   * efficient memory usage (only a few entries in hashtable
   * to reduce management overhead but no need for large
   * continuous memory areas) and at the same time fast access
   * (O(1)) for reading and writing.
   */
  class OSMSCOUT_API NodeUseMap CLASS_FINAL
  {
  private:
    using Bitset = std::bitset<4096>;
    using Map    = std::unordered_map<size_t, Bitset>;

  private:
    Map                                       nodeUseMap;
    size_t                                    nodeCount;
    size_t                                    duplicateCount;

  public:
    NodeUseMap();

    void SetNodeUsed(Id id);
    bool IsNodeUsedAtLeastTwice(Id id) const;
    size_t GetNodeUsedCount() const;
    size_t GetDuplicateCount() const;

    void Clear();
  };
}

#endif
