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

#include <algorithm>

#include <osmscout/system/Math.h>

namespace osmscout {

  OSMTileId::OSMTileId(uint32_t x,
                       uint32_t y)
    : x(x),
      y(y)
  {
    // no code
  }

  /**
   * Return the top left coordinate of the tile
   * @param magnification
   *    Magnification to complete the definition of the tile id (these are relative
   *    to a magnification)
   *
   * @return
   *    The resuting coordinate
   */
  GeoCoord OSMTileId::GetTopLeftCoord(const Magnification& magnification) const
  {
    double n = M_PI - 2.0 * M_PI * y / magnification.GetMagnification();

    return GeoCoord(180.0 / M_PI * atan(0.5 * (exp(n) - exp(-n))),
                    x / magnification.GetMagnification() * 360.0 - 180.0);
  }

  /**
   * Return the bounding box of the given tile
   *
   * @param magnification
   *    Magnification to complete the definition of the tile id (these are relative
   *    to a magnification)
   *
   * @return
   *    The GeoBox defining the resulting area
   */
  GeoBox OSMTileId::GetBoundingBox(const Magnification& magnification) const
  {
    return GeoBox(GetTopLeftCoord(magnification),
                  OSMTileId(x+1,y+1).GetTopLeftCoord(magnification));
  }

  OSMTileId OSMTileId::GetOSMTile(const GeoCoord& coord,
                                  const Magnification& magnification)
  {
    double latRad=coord.GetLat() * M_PI/180.0;

    return {(uint32_t)(floor((coord.GetLon() + 180.0) / 360.0 *magnification.GetMagnification())),
            (uint32_t)(floor((1.0 - log( tan(latRad) + 1.0 / cos(latRad)) / M_PI) / 2.0 * magnification.GetMagnification()))};
  }

  OSMTileIdBox::OSMTileIdBox(const OSMTileId& a,
                             const OSMTileId& b)
  : minTile(std::min(a.GetX(),b.GetX()),
            std::min(a.GetY(),b.GetY())),
    maxTile(std::max(a.GetX(),b.GetX()),
            std::max(a.GetY(),b.GetY()))
  {
    // no code
  }

  /**
   * Return the bounding box of the region defined by the box
   *
   * @param magnification
   *    Magnification to complete the definition of the tile ids (these are relative
   *    to a magnification)
   *
   * @return
   *    The GeoBox defining the resulting area
   */
  GeoBox OSMTileIdBox::GetBoundingBox(const Magnification& magnification) const
  {
    return GeoBox(minTile.GetTopLeftCoord(magnification),
                  OSMTileId(maxTile.GetX()+1,maxTile.GetY()+1).GetTopLeftCoord(magnification));
  }

  TileCalculator::TileCalculator(const Magnification& magnification)
  : cellWidth(360.0/magnification.GetMagnification()),
    cellHeight(180.0/magnification.GetMagnification())
  {
  }

  osmscout::Pixel TileCalculator::GetTileId(const GeoCoord& coord) const
  {
    return {uint32_t((coord.GetLon()+180.0)/cellWidth),
            uint32_t((coord.GetLat()+90.0)/cellHeight)};
  }
}

