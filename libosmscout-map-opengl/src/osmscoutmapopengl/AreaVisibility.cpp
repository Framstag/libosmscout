/*
  This source is part of the libosmscout-map library
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
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <osmscoutmapopengl/AreaVisibility.h>

#include <osmscout/projection/Projection.h>
#include <osmscout/util/GeoBox.h>
#include <osmscout/util/ScreenBox.h>

namespace osmscout {

  bool IsAreaRingVisible(const Projection& projection,
                         const GeoBox& boundingBox,
                         // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) the tolerance is in pixels, the smallest dimension in millimetres
                         double tolerancePixel,
                         double minDimensionMM)
  {
    ScreenBox areaScreenBox;

    if (!projection.BoundingBoxToPixel(boundingBox,
                                       areaScreenBox)) {
      return false;
    }

    // The tolerance is the reach of the border the ring draws, already converted by the caller, so it
    // enlarges the screen box of the ring directly.
    areaScreenBox=areaScreenBox.Resize(tolerancePixel);

    double areaMinDimension=projection.ConvertWidthToPixel(minDimensionMM);

    if (areaScreenBox.GetWidth()<=areaMinDimension &&
        areaScreenBox.GetHeight()<=areaMinDimension) {
      return false;
    }

    return areaScreenBox.Intersects(projection.GetScreenBox());
  }

}
