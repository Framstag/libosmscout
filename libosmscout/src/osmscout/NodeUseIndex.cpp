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

#include <osmscout/NodeUseIndex.h>

#include <cassert>
#include <iostream>

#include <osmscout/FileScanner.h>

#include <osmscout/Util.h>

#include <osmscout/util/String.h>

namespace osmscout {

  bool NodeUseIndex::LoadNodeUseIndex(const std::string& path)
  {
    std::string file=path+"/"+"nodeuse.idx";
    FileScanner scanner;

    if (!GetFileSize(file,datSize)) {
      return false;
    }

    if (!scanner.Open(file)) {
      return false;
    }

    uint32_t intervalCount;

    scanner.ReadNumber(intervalSize);  // Size of interval
    scanner.ReadNumber(intervalCount); // Number of intervals

    std::cout << intervalCount << " entries of page size " << intervalSize << "..." << std::endl;

    for (size_t i=0; i<intervalCount; i++) {
      IndexEntry entry;
      uint32_t   interval;

      scanner.Read(interval);     // The interval
      scanner.Read(entry.offset); // The offset into the way.dat file
      scanner.Read(entry.count);  // The number of entries in the interval

      nodeUseIndex[interval]=entry;
    }

   return !scanner.HasError() && scanner.Close();
  }

  size_t NodeUseIndex::GetIntervalSize() const
  {
    return intervalSize;
  }

  void NodeUseIndex::GetNodeIndexEntries(const std::set<Id>& ids,
                                         std::list<NodeUseIndexEntry>& entries) const
  {
    std::set<size_t> intervals;

    for (std::set<Id>::const_iterator node=ids.begin();
         node!=ids.end();
         ++node) {
      intervals.insert(*node/intervalSize);
    }

    for (std::set<size_t>::const_iterator interval=intervals.begin();
         interval!=intervals.end();
         ++interval) {
      std::map<size_t,IndexEntry>::const_iterator entry;

      entry=nodeUseIndex.find(*interval);

      if (entry!=nodeUseIndex.end()) {
        NodeUseIndexEntry tmp;

        tmp.interval=entry->first;;
        tmp.offset=entry->second.offset;
        tmp.count=entry->second.count;

        entry++;

        if (entry!=nodeUseIndex.end()) {
          tmp.size=entry->second.offset-tmp.offset;
        }
        else {
          tmp.size=datSize-tmp.offset;
        }

        entries.push_back(tmp);
      }
    }
  }

  void NodeUseIndex::DumpStatistics()
  {
    size_t memory=0;

    memory+=nodeUseIndex.size()*sizeof(size_t);     // Key
    memory+=nodeUseIndex.size()*sizeof(IndexEntry); // Value

    std::cout << "Node index use size " << nodeUseIndex.size() << ", memory usage " << memory << std::endl;
  }
}

