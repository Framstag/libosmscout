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
                         // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) the units are part of the names
                         double borderWidthMM,
                         double minDimensionMM)
  {
    ScreenBox areaScreenBox;

    if (!projection.BoundingBoxToPixel(boundingBox,
                                       areaScreenBox)) {
      return false;
    }

    // The style sheet declares the border width in millimetres, the decision enlarges the screen box
    // by pixels, so the tolerance has to be converted with the frame's projection. Half of the
    // declared width is how far the border a style sheet declares reaches beyond the ring.
    constexpr double borderReachFactor=0.5;

    areaScreenBox=areaScreenBox.Resize(projection.ConvertWidthToPixel(borderWidthMM*borderReachFactor));

    double areaMinDimension=projection.ConvertWidthToPixel(minDimensionMM);

    if (areaScreenBox.GetWidth()<=areaMinDimension &&
        areaScreenBox.GetHeight()<=areaMinDimension) {
      return false;
    }

    return areaScreenBox.Intersects(projection.GetScreenBox());
  }

}
