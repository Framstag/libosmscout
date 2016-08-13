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

  template<class T>
  class MapTileCache
  {
  public:
    typedef typename std::shared_ptr<T> TRef;

  private:
    /**
     * Internally used cache entry
     */
    struct OSMSCOUT_MAP_API CacheEntry
    {
      TileId id;
      TRef   tile;

      CacheEntry(const TileId& id,
                 const TRef& tile)
              : id(id),
                tile(tile)
      {
        // no code
      }
    };

    //! A list of cached tiles
    typedef typename std::list<CacheEntry>     Cache;

    //! References to a tile in above list
    typedef typename Cache::iterator           CacheRef;

    //! An index from TileIds to cache entries
    typedef typename std::map<TileId,CacheRef> CacheIndex;

  private:
    size_t             cacheSize;

    mutable CacheIndex tileIndex;
    mutable Cache      tileCache;

  public:
    MapTileCache(size_t cacheSize);

    void SetSize(size_t cacheSize);

    void CleanupCache();

    MapTileCache::TRef GetCachedTile(const TileId& id) const;
    MapTileCache::TRef GetTile(const TileId& id) const;

    void GetTilesForBoundingBox(const Magnification& magnification,
                                const GeoBox& boundingBox,
                                std::list<MapTileCache::TRef>& tiles) const;
  };

  /**
   * Create a new tile cache with the given cache size
   */
  template <class T>
  MapTileCache<T>::MapTileCache(size_t cacheSize)
    : cacheSize(cacheSize)
  {
    // no code
  }

  /**
   * Change the size of the cache. Cache will be cleaned immediately.
   */
  template <class T>
  void MapTileCache<T>::SetSize(size_t cacheSize)
  {
    bool cleanupCache=cacheSize<this->cacheSize;

    this->cacheSize=cacheSize;

    if (cleanupCache) {
      CleanupCache();
    }
  }

  /**
   * Cleanup the cache. Free least recently used tiles until the given maximum cache
   * size is reached again.
   */
  template <class T>
  void MapTileCache<T>::CleanupCache()
  {
    if (tileCache.size()>cacheSize) {
      auto currentEntry=tileCache.rbegin();

      while (currentEntry!=tileCache.rend() &&
             tileCache.size()>cacheSize) {
        if (currentEntry->tile.use_count()==1) {
          tileIndex.erase(currentEntry->id);

          ++currentEntry;
          currentEntry=std::reverse_iterator<MapTileCache<T>::CacheRef>(tileCache.erase(currentEntry.base()));
        }
        else {
          ++currentEntry;
        }
      }
    }
  }

  /**
   * Return the cache tiles with the given id. If the tiles is not cache,
   * an empty reference will be returned.
   */
  template <class T>
  typename MapTileCache<T>::TRef MapTileCache<T>::GetCachedTile(const TileId& id) const
  {
    typename std::map<TileId,MapTileCache<T>::CacheRef>::iterator existingEntry=tileIndex.find(id);

    if (existingEntry!=tileIndex.end()) {
      tileCache.splice(tileCache.begin(),tileCache,existingEntry->second);
      existingEntry->second=tileCache.begin();

      return existingEntry->second->tile;//.lock();
    }

    return NULL;
  }

  /**
   * Return the tile with the given id. If the tile is not currently cached
   * return an empty and unassigned tile and move it to the front of the cache.
   */
  template <class T>
  typename MapTileCache<T>::TRef MapTileCache<T>::GetTile(const TileId& id) const
  {
    typename std::map<TileId,MapTileCache<T>::CacheRef>::iterator existingEntry=tileIndex.find(id);

    if (existingEntry==tileIndex.end()) {
      T tile(new T(id));

      // Updating cache
      typename MapTileCache<T>::CacheEntry cacheEntry(id,tile);

      tileCache.push_front(cacheEntry);
      tileIndex[id]=tileCache.begin();

      return tile;
    }
    else {
      tileCache.splice(tileCache.begin(),tileCache,existingEntry->second);
      existingEntry->second=tileCache.begin();

      return existingEntry->second->tile;
    }
  }

  /**
   * Return all tile necessary for covering the given boundingbox using the given magnification.
   */
  template <class T>
  void MapTileCache<T>::GetTilesForBoundingBox(const Magnification& magnification,
                                              const GeoBox& boundingBox,
                                              std::list<typename MapTileCache<T>::TRef>& tiles) const
  {
    tiles.clear();

    //log.Debug() << "Creating tile data for level " << level << " and bounding box " << boundingBox.GetDisplayText();

    uint32_t level=magnification.GetLevel();

    uint32_t cx1=(uint32_t)floor((boundingBox.GetMinLon()+180.0)/cellDimension[level].width);
    uint32_t cy1=(uint32_t)floor((boundingBox.GetMinLat()+90.0)/cellDimension[level].height);

    uint32_t cx2=(uint32_t)floor((boundingBox.GetMaxLon()+180.0)/cellDimension[level].width);
    uint32_t cy2=(uint32_t)floor((boundingBox.GetMaxLat()+90.0)/cellDimension[level].height);

    //std::cout << "Tile bounding box: " << cx1 << "," << cy1 << " - "  << cx2 << "," << cy2 << std::endl;

    for (size_t y=cy1; y<=cy2; y++) {
      for (size_t x=cx1; x<=cx2; x++) {
        tiles.push_back(GetTile(TileId(magnification,x,y)));
      }
    }
  }

  class OSMSCOUT_MAP_API MapTile
  {
  private:
    TileId  id;                //!< Id of the tile

  public:
    MapTile(const TileId& id);

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
}

#endif
