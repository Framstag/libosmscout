/*
  This source is part of the libosmscout library
  Copyright (C) 2010  Tim Teulings

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

#include <osmscout/projection/Projection.h>

#include <algorithm>

#include <osmscout/system/Assert.h>

namespace osmscout {

  bool Projection::BoundingBoxToPixel(const GeoBox& boundingBox,
                                      ScreenBox& screenBox) const
  {
    assert(boundingBox.IsValid());

    Vertex2D pixel;

    if (!GeoToPixel(boundingBox.GetMinCoord(),
                    pixel)) {
      return false;
    }

    double xMin=pixel.GetX();
    double xMax=pixel.GetX();
    double yMin=pixel.GetY();
    double yMax=pixel.GetY();

    if (!GeoToPixel(boundingBox.GetMaxCoord(),
                    pixel)) {
      return false;
    }

    xMin=std::min(xMin,
                  pixel.GetX());
    xMax=std::max(xMax,
                  pixel.GetX());
    yMin=std::min(yMin,
                  pixel.GetY());
    yMax=std::max(yMax,
                  pixel.GetY());

    if (!GeoToPixel(GeoCoord(boundingBox.GetMinLat(),
                             boundingBox.GetMaxLon()),
                    pixel)) {
      return false;
    }

    xMin=std::min(xMin,
                  pixel.GetX());
    xMax=std::max(xMax,
                  pixel.GetX());
    yMin=std::min(yMin,
                  pixel.GetY());
    yMax=std::max(yMax,
                  pixel.GetY());

    if (!GeoToPixel(GeoCoord(boundingBox.GetMaxLat(),
                             boundingBox.GetMinLon()),
                    pixel)) {
      return false;
    }

    xMin=std::min(xMin,
                  pixel.GetX());
    xMax=std::max(xMax,
                  pixel.GetX());
    yMin=std::min(yMin,
                  pixel.GetY());
    yMax=std::max(yMax,
                  pixel.GetY());

    screenBox=ScreenBox(Vertex2D(xMin,yMin),
                        Vertex2D(xMax,yMax));

    return true;
  }
}
