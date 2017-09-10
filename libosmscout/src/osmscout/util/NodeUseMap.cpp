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

#include <osmscout/util/NodeUseMap.h>

#include <limits>

namespace osmscout {

  NodeUseMap::NodeUseMap()
  : nodeCount(0),
    duplicateCount(0)
  {
    // no code
  }

  void NodeUseMap::SetNodeUsed(Id id)
  {
    PageId resolvedId=id+std::numeric_limits<Id>::min();
    PageId offset=resolvedId/(4096/2);

    Map::iterator entry=nodeUseMap.find(offset);

    if (entry==nodeUseMap.end()) {
      entry=nodeUseMap.insert(std::make_pair(offset,Bitset())).first;
    }

    uint32_t index=(resolvedId%(4096/2))*2;

    if (entry->second[index+1]) {
      // do nothing
    }
    else if (entry->second[index]) {
      entry->second.set(index+1);
      duplicateCount++;
    }
    else {
      nodeCount++;
      entry->second.set(index);
    }
  }

  bool NodeUseMap::IsNodeUsedAtLeastTwice(Id id) const
  {
    PageId resolvedId=id+std::numeric_limits<Id>::min();
    PageId offset=resolvedId/(4096/2);

    Map::const_iterator entry=nodeUseMap.find(offset);

    if (entry==nodeUseMap.end()) {
      return false;
    }

    uint32_t index=(resolvedId%(4096/2))*2+1;

    return entry->second[index];
  }

  size_t NodeUseMap::GetNodeUsedCount() const
  {
    return nodeCount;
  }

  size_t NodeUseMap::GetDuplicateCount() const
  {
    return duplicateCount;
  }

  void NodeUseMap::Clear()
  {
    nodeUseMap.clear();
    nodeCount=0;
    duplicateCount=0;
  }
}
