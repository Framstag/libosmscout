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

#include <osmscoutimport/private/AreaWayIdReferenceRule.h>

#include <algorithm>
#include <cstddef>
#include <vector>

#include <osmscout/OSMScoutTypes.h>
#include <osmscout/Point.h>

namespace osmscout {

  /**
   * Collects the distinct node ids of one object into the scratch buffer. The
   * buffer keeps its capacity across objects, so an object costs no allocation.
   */
  size_t AreaWayIdReferenceRule::CollectDistinctIds(const std::vector<Point>& nodes)
  {
    if (distinctIds.capacity()<nodes.size()) {
      distinctIds.reserve(nodes.size());
    }

    distinctIds.clear();

    for (const auto& node : nodes) {
      distinctIds.push_back(node.GetId());
    }

    std::ranges::sort(distinctIds);

    distinctIds.erase(std::ranges::unique(distinctIds).begin(),
                      distinctIds.end());

    return distinctIds.size();
  }

  void AreaWayIdReferenceRule::ReserveReferencedIds(size_t count)
  {
    referencedIds.reserve(count);
  }

  size_t AreaWayIdReferenceRule::AddObject(const std::vector<Point>& nodes)
  {
    size_t distinctIdCount=CollectDistinctIds(nodes);

    for (const auto id : distinctIds) {
      if (!referencedIds.contains(id)) {
        referencedIds.insert(id);
      }
      else {
        usedAtLeastTwiceIds.insert(id);
      }
    }

    return distinctIdCount;
  }

  void AreaWayIdReferenceRule::ForceSerial(Id id)
  {
    usedAtLeastTwiceIds.insert(id);
  }
}
