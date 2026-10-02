/*
  This source is part of the libosmscout library
  Copyright (C) 2026  Tim Teulings

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

#include <osmscoutmap/MapDataAccounting.h>

#include <cstddef>

namespace osmscout {

  size_t MapDataAccounting::Cost::GetEntryCount() const
  {
    return nodeCount+
           wayCount+
           areaCount+
           routeCount+
           indexEntryCount;
  }

  size_t MapDataAccounting::GetWeight(const Cost& cost)
  {
    return (cost.nodeCount*weightPerNode)+
           (cost.wayCount*weightPerWay)+
           (cost.areaCount*weightPerArea)+
           (cost.routeCount*weightPerRoute)+
           (cost.indexEntryCount*weightPerIndexEntry);
  }

  size_t MapDataAccounting::GetBytes(size_t weight)
  {
    return weight*bytesPerWeight;
  }

  size_t MapDataAccounting::GetWeightForBytes(size_t bytes)
  {
    if (bytes==0) {
      return 0;
    }

    // Round up, so that a non-zero number of bytes never accounts as no weight at all
    return (bytes+bytesPerWeight-1)/bytesPerWeight;
  }
}
