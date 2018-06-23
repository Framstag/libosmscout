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

#include <memory>

#include <osmscout/CoreImportExport.h>

#include <osmscout/Pixel.h>

#include <osmscout/util/GeoBox.h>
#include <osmscout/util/Magnification.h>

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
  class OSMSCOUT_API TileId
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

    inline Pixel GetPixel() const
    {
      return Pixel(x,y);
    }

    /**
     * Return the bounding box of the tile
     */
    inline const GeoBox& GetBoundingBox() const
    {
      return boundingBox;
    }

    std::string GetDisplayText() const;

    TileId GetParent() const;

    bool operator==(const TileId& other) const;

    bool operator!=(const TileId& other) const;

    bool operator<(const TileId& other) const;

    static TileId GetTile(const Magnification& magnification,
                          const GeoCoord& coord);
  };

  class OSMSCOUT_API TileIdBoxConstIterator CLASS_FINAL : public std::iterator<std::input_iterator_tag, const TileId>
  {
  private:
    TileId currentTile;
    TileId minTile;
    TileId maxTile;

  public:
    TileIdBoxConstIterator(const TileId& currentTile,
                           const TileId& minTile,
                           const TileId& maxTile)
      : currentTile(currentTile),
        minTile(minTile),
        maxTile(maxTile)
    {
      // no code
    }

    TileIdBoxConstIterator(const TileIdBoxConstIterator& other)
      : currentTile(other.currentTile),
        minTile(other.minTile),
        maxTile(other.maxTile)
    {
      // no code
    }

    TileIdBoxConstIterator& operator++()
    {
      if (currentTile.GetX()>maxTile.GetX()) {
        currentTile=TileId(minTile.GetLevel(),
                           minTile.GetX(),
                           currentTile.GetY()+1);
      }
      else {
        currentTile=TileId(minTile.GetLevel(),
                           currentTile.GetX()+1,
                           currentTile.GetY());
      }

      return *this;
    }

    const TileIdBoxConstIterator operator++(int)
    {
      TileIdBoxConstIterator tmp(*this);

      operator++();

      return tmp;
    }

    bool operator==(const TileIdBoxConstIterator& other)
    {
      return currentTile==other.currentTile;
    }

    bool operator!=(const TileIdBoxConstIterator& other)
    {
      return currentTile!=other.currentTile;
    }

    const TileId& operator*()
    {
      return currentTile;
    }
  };

  /**
   * \ingroup OSMTile
   *
   *  A bounding box defined by two tile ids that span a rectangular region (in
   *  tile coordinate system)
   */
  class OSMSCOUT_API TileIdBox CLASS_FINAL
  {
  private:
    TileId minTile;
    TileId maxTile;

  public:
    TileIdBox(const TileId& a,
              const TileId& b);

    inline TileId GetMin() const
    {
      return minTile;
    }

    inline TileId GetMax() const
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

    inline uint32_t GetCount() const
    {
      return GetWidth()*GetHeight();
    }

    inline TileIdBoxConstIterator begin() const
    {
      return TileIdBoxConstIterator(minTile,
                                    minTile,
                                    maxTile);
    }

    inline TileIdBoxConstIterator end() const
    {
      return TileIdBoxConstIterator(TileId(maxTile.GetLevel(),
                                           maxTile.GetX()+1,
                                           maxTile.GetY()),
                                       minTile,
                                       maxTile);
    }

    GeoBox GetBoundingBox() const;

    inline std::string GetDisplayText() const
    {
      return std::string("["+minTile.GetDisplayText()+" - "+maxTile.GetDisplayText()+"]");
    }
  };

  /**
   * \defgroup tiledcache Classes for caching map data per tile
   */
}

#endif
