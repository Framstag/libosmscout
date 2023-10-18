/*
  This source is part of the libosmscout library
  Copyright (C) 2022  Tim Teulings

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

#include <osmscout/util/ScreenBox.h>

namespace osmscout {

  const ScreenBox ScreenBox::EMPTY(Vertex2D(0.0,
                                            0.0),
                                   Vertex2D(0.0,
                                            0.0));

  ScreenBox::ScreenBox(const Vertex2D& coordA,
                       const Vertex2D& coordB)
  : minCoord(std::min(coordA.GetX(),coordB.GetX()),
             std::min(coordA.GetY(),coordB.GetY())),
    maxCoord(std::max(coordA.GetX(),coordB.GetX()),
             std::max(coordA.GetY(),coordB.GetY()))
  {
    // no code
  }

  bool ScreenBox::operator==(const ScreenBox& other) const
  {
    return minCoord==other.minCoord &&
           maxCoord == other.maxCoord;
  }

  bool ScreenBox::Intersects(const ScreenBox& other) const
  {
    return Intersects(other,true);
  }

  bool ScreenBox::Intersects(const ScreenBox& other,
                             bool openInterval) const
  {
    if (openInterval) {
      return !(other.GetMaxX()<minCoord.GetX() ||
               other.GetMinX()>=maxCoord.GetX() ||
               other.GetMaxY()<minCoord.GetY() ||
               other.GetMinY()>=maxCoord.GetY());
    }

    return !(other.GetMaxX()<minCoord.GetX() ||
             other.GetMinX()>maxCoord.GetX() ||
             other.GetMaxY()<minCoord.GetY() ||
             other.GetMinY()>maxCoord.GetY());
  }

  ScreenBox ScreenBox::Resize(double offset) const
  {
    double minX, minY, maxX, maxY;

    if (offset < GetWidth()/-2) {
      maxX=minX=(maxCoord.GetX()+minCoord.GetX()) / 2;
    } else {
      minX=minCoord.GetX()-offset;
      maxX=maxCoord.GetX()+offset;
    }

    if(offset < GetHeight()/-2){
      maxY=minY=(maxCoord.GetY()+minCoord.GetY()) / 2;
    } else {
      minY=minCoord.GetY()-offset;
      maxY=maxCoord.GetY()+offset;
    }

    return {Vertex2D(minX,minY),
            Vertex2D(maxX,maxY)};
  }

  ScreenBox ScreenBox::Merge(const ScreenBox& other) const
  {
    return {Vertex2D(std::min(minCoord.GetX(),other.minCoord.GetX()),
                     std::min(minCoord.GetY(),other.minCoord.GetY())),
            Vertex2D(std::max(maxCoord.GetX(),other.maxCoord.GetX()),
                     std::max(maxCoord.GetY(),other.maxCoord.GetY()))};
  }
}
