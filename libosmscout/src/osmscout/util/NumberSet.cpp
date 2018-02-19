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

#include <osmscout/util/NumberSet.h>

#include <limits>

namespace osmscout {

  NumberSet::NumberSet()
    : count(0)
  {
    // no code
  }

  void NumberSet::Set(Id id)
  {
    size_t resolvedId=id+std::numeric_limits<Id>::min();
    size_t offset=resolvedId/(4096/2);

    auto entry=map.find(offset);

    if (entry==map.end()) {
      entry=map.insert(std::make_pair(offset,Bitset())).first;
    }

    size_t index=resolvedId%4096;

    if (!entry->second[index]) {
      count++;
      entry->second.set(index);
    }
  }

  bool NumberSet::IsSet(Id id) const
  {
    size_t resolvedId=id+std::numeric_limits<Id>::min();
    size_t offset=resolvedId/(4096/2);

    auto entry=map.find(offset);

    if (entry==map.end()) {
      return false;
    }

    size_t index=resolvedId%4096;

    return entry->second[index];
  }

  size_t NumberSet::GetNodeUsedCount() const
  {
    return count;
  }

  void NumberSet::Clear()
  {
    map.clear();
    count=0;
  }
}
