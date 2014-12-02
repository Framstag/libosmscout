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

#include <osmscout/util/Tiling.h>

#include <osmscout/system/Math.h>

namespace osmscout {

  // See http://wiki.openstreetmap.org/wiki/Slippy_map_tilenames for details about
  // coordinate transformation

  size_t LonToTileX(double lon,
                     const Magnification& magnification)
  {
    return (size_t)(floor((lon + 180.0) / 360.0 *magnification.GetMagnification()));
  }

  size_t LatToTileY(double lat,
                    const Magnification& magnification)
  {
    return (size_t)(floor((1.0 - log( tan(lat * M_PI/180.0) + 1.0 / cos(lat * M_PI/180.0)) / M_PI) / 2.0 * magnification.GetMagnification()));
  }

  double TileXToLon(int x,
                    const Magnification& magnification)
  {
    return x / magnification.GetMagnification() * 360.0 - 180;
  }

  double TileYToLat(int y,
                    const Magnification& magnification)
  {
    double n = M_PI - 2.0 * M_PI * y / magnification.GetMagnification();

    return 180.0 / M_PI * atan(0.5 * (exp(n) - exp(-n)));
  }
}

