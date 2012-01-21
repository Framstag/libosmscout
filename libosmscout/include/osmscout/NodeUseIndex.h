#ifndef OSMSCOUT_NODEUSEINDEX_H
#define OSMSCOUT_NODEUSEINDEX_H

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

#include <list>
#include <set>

#include <osmscout/TypeConfig.h>

namespace osmscout {

  /**
   * Index entry.
   *
   * Contains "count" entries. Each entry consists of an way id, a count and "count" ids that are joins
   * for the given way id.
   */
  struct NodeUseIndexEntry
  {
    uint32_t   interval; // Unique identifier
    FileOffset offset;   // File offset of the data of this entry
    uint32_t   count;    // Number of entries stored at file offset
  };

  class NodeUseIndex
  {
  private:
    struct IndexEntry
    {
      FileOffset offset; // File offset of this entry
      uint32_t   count;  // Number of way ids stored at file offset
    };

  private:
    std::map<size_t,IndexEntry> nodeUseIndex; //! The index, mapping intervals to IndexEntry
    uint32_t                    intervalSize; //! The id range of an interval
    long                        datSize;      //! The size of the nodeuse.dat file

  public:
    bool LoadNodeUseIndex(const std::string& path,
                          bool useMmap);

    size_t GetIntervalSize() const;
    void GetNodeIndexEntries(const std::set<Id>& ids,
                             std::list<NodeUseIndexEntry>& entries) const;

    void DumpStatistics();
  };
}

#endif
