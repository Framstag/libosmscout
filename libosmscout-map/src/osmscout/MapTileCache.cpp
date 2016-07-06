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

#include <osmscout/MapTileCache.h>

#include <osmscout/util/Logger.h>
#include <osmscout/util/String.h>
#include <osmscout/util/Tiling.h>

#include <iostream>
namespace osmscout {

  /**
   * Create a new tile with the given id.
   */
  MapTile::MapTile(const TileId& id)
  : id(id)
  {
    // no code
  }

  MapTile::~MapTile()
  {
    // no code
  }

  /**
   * Create a new tile cache with the given cache size
   */
  MapTileCache::MapTileCache(size_t cacheSize)
  : cacheSize(cacheSize)
  {
    // no code
  }

  /**
   * Change the size of the cache. Cache will be cleaned immediately.
   */
  void MapTileCache::SetSize(size_t cacheSize)
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
  void MapTileCache::CleanupCache()
  {
    if (tileCache.size()>cacheSize) {
      auto currentEntry=tileCache.rbegin();

      while (currentEntry!=tileCache.rend() &&
              tileCache.size()>cacheSize) {
        if (currentEntry->tile.use_count()==1) {
          tileIndex.erase(currentEntry->id);

          ++currentEntry;
          currentEntry=std::reverse_iterator<Cache::iterator>(tileCache.erase(currentEntry.base()));
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
  MapTileRef MapTileCache::GetCachedTile(const TileId& id) const
  {
    std::map<TileId,CacheRef>::iterator existingEntry=tileIndex.find(id);

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
  MapTileRef MapTileCache::GetTile(const TileId& id) const
  {
    std::map<TileId,CacheRef>::iterator existingEntry=tileIndex.find(id);

    if (existingEntry==tileIndex.end()) {
      MapTileRef tile(new MapTile(id));

      // Updating cache
      CacheEntry cacheEntry(id,tile);

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
  void MapTileCache::GetTilesForBoundingBox(const Magnification& magnification,
                                            const GeoBox& boundingBox,
                                            std::list<MapTileRef>& tiles) const
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
}
