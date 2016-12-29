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

#include <iostream>
namespace osmscout {

  /**
   * Create a new tile with the given id.
   */
  Tile::Tile(const TileId& id)
  : id(id)
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
  TileRef DataTileCache::GetCachedTile(const TileId& id) const
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
  TileRef DataTileCache::GetTile(const TileId& id) const
  {
    std::map<TileId,CacheRef>::iterator existingEntry=tileIndex.find(id);

    if (existingEntry==tileIndex.end()) {
      TileRef tile(new Tile(id));

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
  void DataTileCache::GetTilesForBoundingBox(const Magnification& magnification,
                                              const GeoBox& boundingBox,
                                              std::list<TileRef>& tiles) const
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

  void DataTileCache::ResolveNodesFromParent(Tile& tile,
                                              const Tile& parentTile,
                                              const GeoBox& boundingBox,
                                              const TypeInfoSet& nodeTypes)
  {
    TypeInfoSet subset(nodeTypes);

    // We remove all types that are already loaded
    subset.Remove(tile.GetNodeData().GetTypes());

    if (nodeTypes.Intersects(parentTile.GetNodeData().GetTypes())) {
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

  void DataTileCache::ResolveOptimizedWaysFromParent(Tile& tile,
                                                      const Tile& parentTile,
                                                      const GeoBox& boundingBox,
                                                      const TypeInfoSet& optimizedWayTypes)
  {
    TypeInfoSet subset(optimizedWayTypes);

    // We remove all types that are already loaded
    subset.Remove(tile.GetOptimizedWayData().GetTypes());

    if (optimizedWayTypes.Intersects(parentTile.GetOptimizedWayData().GetTypes())) {
      // We only retrieve types that both tiles have in common
      subset.Intersection(parentTile.GetOptimizedWayData().GetTypes());

      std::vector<WayRef> data;

      data.reserve(parentTile.GetOptimizedWayData().GetDataSize());

      parentTile.GetOptimizedWayData().CopyData([&](const WayRef& way) {
        if (subset.IsSet(way->GetType())) {
          GeoBox wayBoundingBox;

          way->GetBoundingBox(wayBoundingBox);

          if (wayBoundingBox.Intersects(boundingBox)) {
            data.push_back(way);
          }
        }
      });

      tile.GetOptimizedWayData().AddPrefillData(subset,
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

    if (wayTypes.Intersects(parentTile.GetWayData().GetTypes())) {
      // We only retrieve types that both tiles have in common
      subset.Intersection(parentTile.GetWayData().GetTypes());

      std::vector<WayRef> data;

      data.reserve(parentTile.GetWayData().GetDataSize());

      parentTile.GetWayData().CopyData([&](const WayRef& way) {
        if (subset.IsSet(way->GetType())) {
          GeoBox wayBoundingBox;

          way->GetBoundingBox(wayBoundingBox);

          if (wayBoundingBox.Intersects(boundingBox)) {
            data.push_back(way);
          }
        }
      });

      tile.GetWayData().AddPrefillData(subset,
                                       data);
    }
  }

  void DataTileCache::ResolveOptimizedAreasFromParent(Tile& tile,
                                                       const Tile& parentTile,
                                                       const GeoBox& boundingBox,
                                                       const TypeInfoSet& optimizedAreaTypes)
  {
    TypeInfoSet subset(optimizedAreaTypes);

    // We remove all types that are already loaded
    subset.Remove(tile.GetOptimizedAreaData().GetTypes());

    if (optimizedAreaTypes.Intersects(parentTile.GetOptimizedAreaData().GetTypes())) {
      // We only retrieve types that both tiles have in common
      subset.Intersection(parentTile.GetOptimizedAreaData().GetTypes());

      std::vector<AreaRef> data;

      data.reserve(parentTile.GetOptimizedAreaData().GetDataSize());

      parentTile.GetOptimizedAreaData().CopyData([&](const AreaRef& area) {
        if (subset.IsSet(area->GetType())) {
          GeoBox areaBoundingBox;

          area->GetBoundingBox(areaBoundingBox);

          if (areaBoundingBox.Intersects(boundingBox)) {
            data.push_back(area);
          }
        }
      });

      tile.GetOptimizedAreaData().AddPrefillData(subset,
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

    if (areaTypes.Intersects(parentTile.GetAreaData().GetTypes())) {
      // We only retrieve types that both tiles have in common
      subset.Intersection(parentTile.GetAreaData().GetTypes());

      std::vector<AreaRef> data;

      data.reserve(parentTile.GetAreaData().GetDataSize());

      parentTile.GetAreaData().CopyData([&](const AreaRef& area) {
        if (subset.IsSet(area->GetType())) {
          GeoBox areaBoundingBox;

          area->GetBoundingBox(areaBoundingBox);

          if (areaBoundingBox.Intersects(boundingBox)) {
            data.push_back(area);
          }
        }
      });

      tile.GetAreaData().AddPrefillData(subset,
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
                                            const TypeInfoSet& optimizedWayTypes,
                                            const TypeInfoSet& optimizedAreaTypes)
  {
    if (tile.GetId().GetLevel()>0) {
      TileId parentTileId=tile.GetId().GetParent();
      TileRef parentTile=GetCachedTile(parentTileId);

      GeoBox boundingBox=tile.GetBoundingBox();

      if (parentTile) {
        ResolveNodesFromParent(tile,*parentTile,boundingBox,nodeTypes);
        ResolveOptimizedWaysFromParent(tile,*parentTile,boundingBox,optimizedWayTypes);
        ResolveWaysFromParent(tile,*parentTile,boundingBox,wayTypes);
        ResolveOptimizedAreasFromParent(tile,*parentTile,boundingBox,optimizedAreaTypes);
        ResolveAreasFromParent(tile,*parentTile,boundingBox,areaTypes);

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
