#ifndef OSMSCOUT_UTIL_TILING_H
#define OSMSCOUT_UTIL_TILING_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2014  Tim Teulings

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

#include <osmscout/private/CoreImportExport.h>

#include <osmscout/util/Magnification.h>

namespace osmscout {

  /**
   * \defgroup Tiling Tiling Helper
   *
   * Collection of classes and methods related to tiles
   */

  /**
   * \ingroup Tiling
   */
  extern OSMSCOUT_API size_t LonToTileX(double lon,
                                        const Magnification& magnification);
  /**
   * \ingroup Tiling
   */
  extern OSMSCOUT_API size_t LatToTileY(double lat,
                                        const Magnification& magnification);

  /**
   * \ingroup Tiling
   */
  extern OSMSCOUT_API double TileXToLon(int x,
                                        const Magnification& magnification);
  /**
   * \ingroup Tiling
   */
  extern OSMSCOUT_API double TileYToLat(int y,
                                        const Magnification& magnification);

}

#endif
