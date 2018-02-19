#ifndef OSMSCOUT_TILEID_H
#define OSMSCOUT_TILEID_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2016  Tim Teulings

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

#include <map>
#include <memory>
#include <mutex>
#include <vector>

#include <osmscout/private/MapImportExport.h>

#include <osmscout/Node.h>
#include <osmscout/Way.h>
#include <osmscout/Area.h>

#include <osmscout/TypeConfig.h>

#include <osmscout/util/GeoBox.h>
#include <osmscout/util/Magnification.h>
#include <osmscout/system/Assert.h>

namespace osmscout {

  /**
   * \ingroup tiledcache
   *
   * A Tile id, uniquely identifing a tile by its level and (unique for the given level) tile coordinates.
   *
   * Note that this are libosmscout tiles, that evenly split the whole world into - for each level -
   * equaly sized (regarding their latitude and longitude interval) rectangles.
   *
   * Classic OSM tiles are calculated differently.
   */
  class OSMSCOUT_MAP_API TileId
  {
  private:
    uint32_t      level;         //!< the zoom level (0..n)
    size_t        x;             //!< The x coordinate of the tile in relation to the zoom level
    size_t        y;             //!< The y coordinate of the tile in relation to the zoom level
    GeoBox        boundingBox;   //!< Bounding box of the tile

  public:
    TileId(const Magnification& magnification,
           size_t x,
           size_t y);

    /**
     * Return the zoom level of the tile
     */
    inline uint32_t GetLevel() const
    {
      return level;
    }

    /**
     * Return the X coordinate fo the tile
     */
    inline size_t GetX() const
    {
      return x;
    }

    /**
     * Return the y coordinate fo the tile
     */
    inline size_t GetY() const
    {
      return y;
    }

    /**
     * Return the bounding box of the tile
     */
    inline const GeoBox& GetBoundingBox() const
    {
      return boundingBox;
    }

    std::string DisplayText() const;

    TileId GetParent() const;

    bool operator==(const TileId& other) const;

    bool operator!=(const TileId& other) const;

    bool operator<(const TileId& other) const;
  };

  /**
   * \defgroup tiledcache Classes for caching map data per tile
   */
}

#endif
