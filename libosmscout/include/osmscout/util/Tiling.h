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

#include <osmscout/system/Compiler.h>

namespace osmscout {

  /**
   * \defgroup OSMTile classes for handling OSM Tiles
   *
   * Class for representing OSM tiles and other data structures build on top
   * of them.
   *
   * \note OSM tiles walk from top left to bottom right over the earth.
   *
   * \note OSM tiles only cover the region that is valid for the mercator
   * projection
   *
   * \see http://wiki.openstreetmap.org/wiki/Slippy_map_tilenames for details about
   * coordinate transformation behind OSM tiles.
   */

  // forward declaration to avoid circular header includes
  class GeoCoord;
  class GeoBox;

  /**
   * \ingroup OSMTile
   *
   * Representation of the x and y coordinate of a OSM tile.
   *
   * \note that OSM tile coordinates are only unique in content fo the
   * zoom level of the tile. Because in most cases the zoom level beeing
   * redundant we have defined OSMTile without the zoom level to save in memory
   * space.
   */
  class OSMSCOUT_API OSMTileId CLASS_FINAL
  {
  private:
    uint32_t x;
    uint32_t y;

  public:
    OSMTileId(uint32_t x,
              uint32_t y);

    inline uint32_t GetX() const
    {
      return x;
    }

    inline uint32_t GetY() const
    {
      return y;
    }

    GeoCoord GetTopLeftCoord(const Magnification& magnification) const;
    GeoBox GetBoundingBox(const Magnification& magnification) const;
  };

  /**
   * \ingroup OSMTile
   *
   *  A bounding box defined by two tile ids that span a rectangular region (in
   *  tile coordinate system)
   */
  class OSMSCOUT_API OSMTileIdBox CLASS_FINAL
  {
  private:
    OSMTileId minTile;
    OSMTileId maxTile;

  public:
    OSMTileIdBox(const OSMTileId& a,
                 const OSMTileId& b);

    inline OSMTileId GetMin() const
    {
      return minTile;
    }

    inline OSMTileId GetMax() const
    {
      return maxTile;
    }

    inline uint32_t GetMinX() const
    {
      return minTile.GetX();
    }

    inline uint32_t GetMaxX() const
    {
      return maxTile.GetX();
    }

    inline uint32_t GetMinY() const
    {
      return minTile.GetY();
    }

    inline uint32_t GetMaxY() const
    {
      return maxTile.GetY();
    }

    inline uint32_t GetWidth() const
    {
      return maxTile.GetX()-minTile.GetX()+1;
    }

    inline uint32_t GetHeight() const
    {
      return maxTile.GetY()-minTile.GetY()+1;
    }

    GeoBox GetBoundingBox(const Magnification& magnification) const;
  };
}

#endif
