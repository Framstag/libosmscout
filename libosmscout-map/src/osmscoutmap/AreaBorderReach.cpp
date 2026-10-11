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
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
*/

#include <osmscoutmap/AreaBorderReach.h>

#include <algorithm>
#include <cmath>
#include <vector>

#include <osmscout/projection/Projection.h>

#include <osmscoutmap/Styles.h>

namespace osmscout {

  double GetAreaRingTolerancePixel(const Projection& projection,
                                   const std::vector<BorderStyleRef>& borderStyles)
  {
    // The drawing steps reach half of the declared width beyond the ring
    constexpr double borderReachFactor=0.5;

    double           tolerance=0.0;

    for (const auto& borderStyle : borderStyles) {
      if (!borderStyle) {
        continue;
      }

      // The same reach the drawing steps apply to the border: half of the declared width plus the
      // offsets the border is drawn at, the width and the display offset in millimetres, the offset
      // in the map units of the style sheet.
      double reach=projection.ConvertWidthToPixel(borderStyle->GetWidth()*borderReachFactor);

      reach+=projection.ConvertWidthToPixel(std::fabs(borderStyle->GetDisplayOffset()));
      reach+=std::fabs(borderStyle->GetOffset())/projection.GetPixelSize();

      tolerance=std::max(tolerance,reach);
    }

    return tolerance;
  }
}
