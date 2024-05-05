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

#include <osmscoutmap/LabelLayouterHelper.h>

namespace osmscout {
  ScreenRectMask::ScreenRectMask(size_t screenWidth,
                                 const ScreenPixelRectangle &rect)
  {
    constexpr size_t   bitsPerCell=64u;
    constexpr uint64_t allBitsSet=~0;

    size_t rowLength=screenWidth / bitsPerCell +1;

    if (screenWidth % bitsPerCell!=0u) {
      rowLength++;
    }

    rowFrom=rect.y;
    rowTo=rect.y+rect.height-1;

    bitmask.resize(rowLength);

    // Rectangle is to the right of the screen
    if (rect.x>(int)screenWidth) {
      return;
    }

    // Rectangle is to the left of the screen
    if (rect.x+rect.width-1<0) {
      return;
    }

    size_t startX=std::max(0,rect.x);
    size_t endX=std::min(size_t(std::max(rect.x+rect.width-1,0)),screenWidth-1);

    cellFrom=startX / bitsPerCell;

    cellTo=endX / bitsPerCell;

    if (endX % bitsPerCell==0u) {
      cellTo++;
    }

    if (cellFrom>=int(rowLength)) {
      return;
    }

    // Fill everything between and including start and end with a "full" msk
    // start and end will be corrected later
    for (int x=cellFrom; x<=cellTo; x++) {
      bitmask[x]=allBitsSet;
    }

    // Starting mask cell
    size_t bitOffsetStart=startX % bitsPerCell;
    if (bitOffsetStart!=0u) {
      bitmask[cellFrom] =  bitmask[cellFrom] & (allBitsSet << bitOffsetStart);
    }

    // Final mask cell (may also be the starting cell!)
    size_t bitOffsetEnd=endX % bitsPerCell;
    if (bitOffsetEnd!=0u) {
      bitmask[cellTo] = bitmask[cellTo] & (allBitsSet >> (bitsPerCell-bitOffsetEnd-1));
    }
  }

  uint64_t ScreenRectMask::GetCell(size_t idx) const
  {
    assert(idx<bitmask.size());

    return bitmask[idx];
  }


  bool ScreenRectMask::Intersects(const ScreenRectMask& other) const
  {
    // Other is to the right of us
    if (other.rowFrom>rowTo) {
      return false;
    }

    // Other is to the left of us
    if (other.rowTo<rowFrom) {
      return false;
    }

    for (size_t idx=0; idx<std::min(bitmask.size(),other.bitmask.size()); idx++) {
      if ((bitmask[idx] & other.bitmask[idx]) != 0u) {
        return true;
      }
    }

    return false;
  }

  ScreenMask::ScreenMask(size_t width, size_t height)
  : height(height)
  {
    size_t bitsPerCell=64u;

    rowLength=width / bitsPerCell;

    if (width % bitsPerCell!=0) {
      rowLength++;
    }

    bitmask.resize(height*rowLength);
  }

  void ScreenMask::AddMask(const ScreenRectMask& mask)
  {
    for (int r=std::max(0,mask.GetFirstRow()); r<=std::min((int)height-1, mask.GetLastRow()); r++) {
      for (int c=std::max(0,mask.GetFirstCell()); c<=std::min((int)rowLength-1, mask.GetLastCell()); c++) {
        int idx= r * rowLength + c;

        assert(idx>=0);
        assert(idx<(int)bitmask.size());

        bitmask[idx] = (bitmask[idx] | mask.GetCell(c));
      }
    }
  }

  bool ScreenMask::HasCollision(const ScreenRectMask& mask) const
  {
    for (int r=std::max(0,mask.GetFirstRow()); r<=std::min((int)height-1, mask.GetLastRow()); r++) {
      for (int c=std::max(0,mask.GetFirstCell()); c<=std::min((int)rowLength-1,mask.GetLastCell()); c++) {
        int idx= r * rowLength + c;

        assert(idx>=0);
        assert(idx<(int)bitmask.size());

        if ((bitmask[idx] & mask.GetCell(c))) {
          return true;
        }
      }
    }

    return false;
  }
}
