#ifndef OSMSCOUT_MAPTILECACHE_H
#define OSMSCOUT_MAPTILECACHE_H

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

#include <osmscout/util/GeoBox.h>
#include <osmscout/util/Magnification.h>

#include <osmscout/TileId.h>

namespace osmscout {

  class OSMSCOUT_MAP_API MapTile
  {
  private:
    TileId       id;                //!< Id of the tile

  private:
    MapTile(const TileId& id);

  public:
    friend class MapTileCache;

    ~MapTile();

    /**
     * Return the id of the tile
     */
    inline TileId GetId() const
    {
      return id;
    }

    /**
     * Return the boundingbox of the tile (shortcut for GetId().GetBoundingBox())
     */
    inline GeoBox GetBoundingBox() const
    {
      return id.GetBoundingBox();
    }
  };

  typedef std::shared_ptr<MapTile> MapTileRef;

  class OSMSCOUT_MAP_API MapTileCache
  {
  private:
    /**
     * Internally used cache entry
     */
    struct OSMSCOUT_MAP_API CacheEntry
    {
      TileId     id;
      MapTileRef tile;

      CacheEntry(const TileId& id,
                 const MapTileRef& tile)
              : id(id),
                tile(tile)
      {
        // no code
      }
    };

    //! A list of cached tiles
    typedef std::list<CacheEntry>     Cache;

    //! References to a tile in above list
    typedef Cache::iterator           CacheRef;

    //! An index from TileIds to cache entries
    typedef std::map<TileId,CacheRef> CacheIndex;

  private:
    size_t             cacheSize;

    mutable CacheIndex tileIndex;
    mutable Cache      tileCache;

  public:
    MapTileCache(size_t cacheSize);

    void SetSize(size_t cacheSize);

    void CleanupCache();

    MapTileRef GetCachedTile(const TileId& id) const;
    MapTileRef GetTile(const TileId& id) const;

    void GetTilesForBoundingBox(const Magnification& magnification,
                                const GeoBox& boundingBox,
                                std::list<MapTileRef>& tiles) const;
  };

  typedef std::shared_ptr<MapTileCache> MapTileCacheRef;
}

#endif
