#ifndef OSMSCOUT_MAP_OPENGL_AREA_VISIBILITY_H
#define OSMSCOUT_MAP_OPENGL_AREA_VISIBILITY_H

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

#include <osmscoutmapopengl/MapOpenGLImportExport.h>

#include <osmscout/projection/Projection.h>
#include <osmscout/util/GeoBox.h>

namespace osmscout {

  /**
   * Decides whether an area ring can contribute to the frame, given the tolerance the ring is
   * decided with.
   *
   * The ring's bounding box is projected into the frame's screen space, the resulting screen box is
   * enlarged by the given tolerance and intersected with the frame's view. A ring is rejected when
   * that enlarged box does not intersect the view, or when it is not larger than the smallest
   * dimension the draw parameters draw an area with.
   *
   * The tolerance is a length in the pixels of the frame, because the units a style sheet declares
   * have to be converted before a length can enlarge a screen box: the caller converts them, see
   * GetAreaRingTolerancePixel in libosmscout-map. The smallest dimension is a length a draw
   * parameter declares in millimetres and is converted here.
   *
   * @param projection the projection of the frame to decide for
   * @param boundingBox the bounding box of the ring, in geographical coordinates
   * @param tolerancePixel the tolerance the ring is enlarged by, in pixels of the frame
   * @param minDimensionMM the smallest dimension a drawn area may have, in millimetres as the draw
   *                      parameters declare it
   * @return true if the ring can contribute to the frame
   */
  bool OSMSCOUT_MAP_OPENGL_API IsAreaRingVisible(const Projection& projection,
                                                 const GeoBox& boundingBox,
                                                 double tolerancePixel,
                                                 double minDimensionMM);

}

#endif
