/*
  This source is part of the libosmscout library
  Copyright (C) 2015  Tim Teulings

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

#include <osmscout/DataTileCache.h>

#include <osmscout/util/Logger.h>
#include <osmscout/util/String.h>
#include <osmscout/util/Tiling.h>

namespace osmscout {

  /**
   * Create a new tile with the given id.
   */
  Tile::Tile(const TileKey& key)
  : key(key),
    boundingBox(key.GetBoundingBox())
  {
    // no code
  }

  Tile::~Tile()
  {
    // no code
  }

  /**
   * Create a new tile cache with the given cache size
   */
  DataTileCache::DataTileCache(size_t cacheSize)
  : cacheSize(cacheSize)
  {
    // no code
  }

  /**
   * Change the size of the cache. Cache will be cleaned immediately.
   */
  void DataTileCache::SetSize(size_t cacheSize)
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
  void DataTileCache::CleanupCache()
  {
    if (tileCache.size()>cacheSize) {
      auto currentEntry=tileCache.rbegin();

      while (currentEntry!=tileCache.rend() &&
              tileCache.size()>cacheSize) {
        //if (currentEntry->tile.expired()) {
        if (currentEntry->tile.use_count()==1) {
          //std::cout << "Dropping tile " << (std::string)currentEntry->id << " from cache " << cache.size() << "/" << cacheSize << std::endl;
          tileIndex.erase(currentEntry->key);

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
   * Mark all tiles as in cache as "incomplete".
   */
  void DataTileCache::InvalidateCache()
  {
    for (CacheEntry &entry: tileCache) {
      entry.tile->GetAreaData().Invalidate();
      entry.tile->GetNodeData().Invalidate();
      entry.tile->GetWayData().Invalidate();
      entry.tile->GetOptimizedAreaData().Invalidate();
      entry.tile->GetOptimizedWayData().Invalidate();
    }
  }

  /**
   * Return the cache tiles with the given id. If the tiles is not cache,
   * an empty reference will be returned.
   */
  TileRef DataTileCache::GetCachedTile(const TileKey& key) const
  {
    auto existingEntry=tileIndex.find(key);

    if (existingEntry!=tileIndex.end()) {
      tileCache.splice(tileCache.begin(),
                       tileCache,
                       existingEntry->second);
      existingEntry->second=tileCache.begin();

      return existingEntry->second->tile;//.lock();
    }

    return nullptr;
  }

  /**
   * Return the tile with the given id. If the tile is not currently cached
   * return an empty and unassigned tile and move it to the front of the cache.
   */
  TileRef DataTileCache::GetTile(const TileKey& key) const
  {
    auto existingEntry=tileIndex.find(key);

    if (existingEntry==tileIndex.end()) {
      TileRef tile(new Tile(key));

      // Updating cache
      CacheEntry cacheEntry(key,
                            tile);

      tileCache.push_front(cacheEntry);
      tileIndex[key]=tileCache.begin();

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
  void DataTileCache::GetTilesForBoundingBox(const Magnification& magnification,
                                             const GeoBox& boundingBox,
                                             std::list<TileRef>& tiles) const
  {
    tiles.clear();

    //log.Debug() << "Creating tile data for level " << level << " and bounding box " << boundingBox.GetDisplayText();

    TileIdBox box(TileId::GetTile(magnification,boundingBox.GetMinCoord()),
                  TileId::GetTile(magnification,boundingBox.GetMaxCoord()));

    for (const auto& tileId : box) {
      tiles.push_back(GetTile(TileKey(magnification,tileId)));
    }
  }

  void DataTileCache::ResolveNodesFromParent(Tile& tile,
                                              const Tile& parentTile,
                                              const GeoBox& boundingBox,
                                              const TypeInfoSet& nodeTypes)
  {
    TypeInfoSet subset(nodeTypes);

    // We remove all types that are already loaded
    subset.Remove(tile.GetNodeData().GetTypes());

    if (subset.Intersects(parentTile.GetNodeData().GetTypes())) {
      // We only retrieve types that both tiles have in common
      subset.Intersection(parentTile.GetNodeData().GetTypes());

      std::vector<NodeRef> data;

      data.reserve(parentTile.GetNodeData().GetDataSize());

      parentTile.GetNodeData().CopyData([&](const NodeRef& node) {
        if (subset.IsSet(node->GetType())) {
          if (boundingBox.Includes(node->GetCoords())) {
            data.push_back(node);
          }
        }
      });

      tile.GetNodeData().AddPrefillData(subset,
                                        data);
    }
  }

  void DataTileCache::ResolveWaysFromParent(Tile& tile,
                                             const Tile& parentTile,
                                             const GeoBox& boundingBox,
                                             const TypeInfoSet& wayTypes)
  {
    TypeInfoSet subset(wayTypes);

    // We remove all types that are already loaded
    subset.Remove(tile.GetWayData().GetTypes());

    if (subset.Intersects(parentTile.GetWayData().GetTypes())) {
      // We only retrieve types that both tiles have in common
      subset.Intersection(parentTile.GetWayData().GetTypes());

      std::vector<WayRef> data;

      data.reserve(parentTile.GetWayData().GetDataSize());

      parentTile.GetWayData().CopyData([&](const WayRef& way) {
        if (subset.IsSet(way->GetType())) {
          if (way->GetBoundingBox().Intersects(boundingBox)) {
            data.push_back(way);
          }
        }
      });

      tile.GetWayData().AddPrefillData(subset,
                                       data);
    }
  }

  void DataTileCache::ResolveAreasFromParent(Tile& tile,
                                             const Tile& parentTile,
                                             const GeoBox& boundingBox,
                                             const TypeInfoSet& areaTypes)
  {
    TypeInfoSet subset(areaTypes);

    // We remove all types that are already loaded
    subset.Remove(tile.GetAreaData().GetTypes());

    if (subset.Intersects(parentTile.GetAreaData().GetTypes())) {
      // We only retrieve types that both tiles have in common
      subset.Intersection(parentTile.GetAreaData().GetTypes());

      std::vector<AreaRef> data;

      data.reserve(parentTile.GetAreaData().GetDataSize());

      parentTile.GetAreaData().CopyData([&](const AreaRef& area) {
        if (subset.IsSet(area->GetType())) {
          if (area->GetBoundingBox().Intersects(boundingBox)) {
            data.push_back(area);
          }
        }
      });

      tile.GetAreaData().AddPrefillData(subset,
                                        data);
    }
  }

  void DataTileCache::ResolveRoutesFromParent(Tile& tile,
                                              const Tile& parentTile,
                                              const GeoBox& boundingBox,
                                              const TypeInfoSet& routeTypes)
  {
    TypeInfoSet subset(routeTypes);

    // We remove all types that are already loaded
    subset.Remove(tile.GetRouteData().GetTypes());

    if (subset.Intersects(parentTile.GetRouteData().GetTypes())) {
      // We only retrieve types that both tiles have in common
      subset.Intersection(parentTile.GetRouteData().GetTypes());

      std::vector<RouteRef> data;

      data.reserve(parentTile.GetRouteData().GetDataSize());

      parentTile.GetRouteData().CopyData([&](const RouteRef& route) {
        if (subset.IsSet(route->GetType())) {
          if (route->GetBoundingBox().Intersects(boundingBox)) {
            data.push_back(route);
          }
        }
      });

      tile.GetRouteData().AddPrefillData(subset,
                                        data);
    }
  }

  /**
   * (Partially) prefill the given tiles with data already cached with data of the given types.
   *
   * Currently prefill is done by looking for the parent tile in the cache
   * and copying data that intersects the bounding box of the given tile.
   */
  void DataTileCache::PrefillDataFromCache(Tile& tile,
                                           const TypeInfoSet& nodeTypes,
                                           const TypeInfoSet& wayTypes,
                                           const TypeInfoSet& areaTypes,
                                           const TypeInfoSet& routeTypes,
                                           const TypeInfoSet& /*optimizedWayTypes*/,
                                           const TypeInfoSet& /*optimizedAreaTypes*/)
  {
    if (tile.GetLevel()>0) {
      TileKey parentTileKey=tile.GetKey().GetParent();
      TileRef parentTile=GetCachedTile(parentTileKey);

      GeoBox boundingBox=tile.GetBoundingBox();

      if (parentTile) {
        ResolveNodesFromParent(tile,*parentTile,boundingBox,nodeTypes);
        ResolveWaysFromParent(tile,*parentTile,boundingBox,wayTypes);
        ResolveAreasFromParent(tile,*parentTile,boundingBox,areaTypes);
        ResolveRoutesFromParent(tile,*parentTile,boundingBox,routeTypes);

        return;
      }
    }

    /*
    Magnification zoomedInMagnification;

    zoomedInMagnification.SetLevel(tile.GetId().GetLevel()+1);

    TileId childIds[4]={TileId(zoomedInMagnification,tile.GetId().GetX()*2,tile.GetId().GetY()*2),
                        TileId(zoomedInMagnification,tile.GetId().GetX()*2+1,tile.GetId().GetY()*2),
                        TileId(zoomedInMagnification,tile.GetId().GetX()*2,tile.GetId().GetY()*2+1),
                        TileId(zoomedInMagnification,tile.GetId().GetX()*2+1,tile.GetId().GetY()*2+1)};

    TileRef childTiles[4];

    for (size_t i=0; i<4; i++) {
      childTiles[i]=GetCachedTile(childIds[i]);
    }

    if (childTiles[0] && childTiles[1] && childTiles[2] && childTiles[3]) {
      std::cout << "Prefilling from children..." << std::endl;
    }*/
  }
}
