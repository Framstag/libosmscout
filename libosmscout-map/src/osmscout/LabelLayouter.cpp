/*
  This source is part of the libosmscout-map library
  Copyright (C) 2016  Tim Teulings

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

#include <osmscout/LabelLayouter.h>

//#define LABEL_LAYOUTER_DEBUG

#if defined(LABEL_LAYOUTER_DEBUG)
#include <iostream>
#endif

namespace osmscout {
  OSMSCOUT_MAP_API void Mask::prepare(const IntRectangle &rect)
  {
    // clear
    for (int c = std::max(0, cellFrom); c <= std::min((int) d.size() - 1, cellTo); c++) {
      d[c] = 0;
    }

    // setup
    cellFrom = rect.x / 64;
    int cellFromBit = rect.x % 64;
    cellTo = (rect.x + rect.width) / 64;
    int cellToBit = (rect.x + rect.width) % 64;
    rowFrom = rect.y;
    rowTo = rect.y + rect.height;

    if (cellFromBit<0){
      cellFrom--;
      cellFromBit=64+cellFromBit;
    }
    if (cellToBit<0){
      cellTo--;
      cellToBit=64+cellToBit;
    }

    uint64_t mask = ~0;
    for (int c = std::max(0, cellFrom); c <= std::min((int) d.size() - 1, cellTo); c++) {
      d[c] = mask;
    }
    if (cellFrom >= 0 && cellFrom < size()) {
      d[cellFrom] = d[cellFrom] >> cellFromBit;
    }
    if (cellTo >= 0 && cellTo < size()) {
      d[cellTo] = d[cellTo] << (64 - cellToBit);
    }
  }
}
