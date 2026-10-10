#ifndef OSMSCOUT_MAP_AREABORDERREACH_H
#define OSMSCOUT_MAP_AREABORDERREACH_H

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

#include <vector>

#include <osmscoutmap/MapImportExport.h>

#include <osmscout/projection/Projection.h>

#include <osmscoutmap/Styles.h>

namespace osmscout {

  /**
   * Visibility tolerance of an area ring, in the pixels of the frame: the largest reach any of the
   * border styles the ring resolves draws, i.e. half of the declared width plus the offset the
   * border is drawn at.
   *
   * A ring is drawn by every border style it resolves, and a border drawn at an offset reaches
   * further out than its width alone, so a decision that read one of the styles would reject a ring
   * the drawing step shows. A painter's early rejection of areas extends an area by the frame-wide
   * upper bound of this value, so both decisions cover the same geometry.
   *
   * The width and the display offset a style declares are lengths in millimetres, the offset is a
   * length in map units; all three are converted with the frame's projection here, so the tolerance
   * is a screen-space length that grows with the DPI of the frame.
   *
   * @param projection the projection of the frame to decide for
   * @param borderStyles the border styles the ring resolves, as the style config returns them
   * @return the tolerance in pixels of the frame, 0.0 when the ring draws no border
   */
  double OSMSCOUT_MAP_API GetAreaRingTolerancePixel(const Projection& projection,
                                                    const std::vector<BorderStyleRef>& borderStyles);
}

#endif
