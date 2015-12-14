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

#include <osmscout/TiledDataCache.h>

#include <osmscout/util/Logger.h>
#include <osmscout/util/String.h>
#include <osmscout/util/Tiling.h>

#include <iostream>
namespace osmscout {

  /**
   * Ceate a new tile by passing magnification and tile coordinates
   */
  TileId::TileId(const Magnification& magnification,
                 size_t x,
                 size_t y)
  : magnification(magnification),
    x(x),
    y(y),
    boundingBox(GeoCoord(y*cellDimension[magnification.GetLevel()].height-90.0,
                         x*cellDimension[magnification.GetLevel()].width-180.0),
                GeoCoord((y+1)*cellDimension[magnification.GetLevel()].height-90.0,
                         (x+1)*cellDimension[magnification.GetLevel()].width-180.0))
  {
    // no code
  }

  /**
   * Return a short human readable description of the tile id
   */
  std::string TileId::DisplayText() const
  {
    return NumberToString(magnification.GetLevel())+ "." + NumberToString(y) + "." + NumberToString(x);
  }

  /**
   * Return the parent tile.
   *
   * Note that the parent tile will cover a 4 times bigger region than the current tile.
   *
   * Note that for tiles with level 0 no parent tile will exist. The method will assert in this case!
   */
  TileId TileId::GetParent() const
  {
    Magnification zoomedOutMagnification;

    assert(magnification.GetLevel()>0);

    zoomedOutMagnification.SetLevel(magnification.GetLevel()-1);

    return TileId(zoomedOutMagnification,x/2,y/2);
  }

  /**
   * Compare tile ids for equality
   */
  bool TileId::operator==(const TileId& other) const
  {
    return magnification==other.magnification &&
           y==other.y &&
           x==other.x;
  }

  /**
   * Compare tile ids for inequality
   */
  bool TileId::operator!=(const TileId& other) const
  {
    return magnification!=other.magnification ||
           y!=other.y ||
           x!=other.x;
  }

  /**
   * Compare tile ids by their order. Needed for sorting tile ids and placing them into (some)
   * containers.
   */
  bool TileId::operator<(const TileId& other) const
  {
    if (magnification!=other.magnification) {
      return magnification<other.magnification;
    }

    if (y!=other.y) {
      return y<other.y;
    }

    return x<other.x;
  }

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
  TiledDataCache::TiledDataCache(size_t cacheSize)
  : cacheSize(cacheSize)
  {
    // no code
  }

  /**
   * Change the size of the cache. Cache will be cleaned immediately.
   */
  void TiledDataCache::SetSize(size_t cacheSize)
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
  void TiledDataCache::CleanupCache()
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
   * Return the cache tiles with the given id. If the tiles is not cache,
   * an empty reference will be returned.
   */
  TileRef TiledDataCache::GetCachedTile(const TileId& id) const
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
  TileRef TiledDataCache::GetTile(const TileId& id) const
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
  void TiledDataCache::GetTilesForBoundingBox(const Magnification& magnification,
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

  void TiledDataCache::ResolveNodesFromParent(Tile& tile,
                                              const Tile& parentTile,
                                              const GeoBox& boundingBox,
                                              const TypeInfoSet& nodeTypes)
  {
    if (nodeTypes.Intersects(parentTile.GetNodeData().GetTypes())) {
      TypeInfoSet subset(nodeTypes);

      subset.Intersection(parentTile.GetNodeData().GetTypes());

      std::vector<NodeRef> data;

      data.reserve(parentTile.GetNodeData().GetData().size());

      for (const auto& node : parentTile.GetNodeData().GetData()) {
        if (nodeTypes.IsSet(node->GetType())) {
          if (boundingBox.Includes(node->GetCoords())) {
            data.push_back(node);
          }
        }
      }

      tile.GetNodeData().SetData(subset,
                                 data);
    }
  }

  void TiledDataCache::ResolveOptimizedWaysFromParent(Tile& tile,
                                                      const Tile& parentTile,
                                                      const GeoBox& boundingBox,
                                                      const TypeInfoSet& optimizedWayTypes,
                                                      const TypeInfoSet& wayTypes)
  {
    if (optimizedWayTypes.Intersects(parentTile.GetOptimizedWayData().GetTypes())) {
      TypeInfoSet subset(wayTypes);

      subset.Intersection(parentTile.GetOptimizedWayData().GetTypes());

      std::vector<WayRef> data;

      data.reserve(parentTile.GetOptimizedWayData().GetData().size());

      for (const auto& way : parentTile.GetOptimizedWayData().GetData()) {
        if (wayTypes.IsSet(way->GetType())) {
          GeoBox wayBoundingBox;

          way->GetBoundingBox(wayBoundingBox);

          if (wayBoundingBox.Intersects(boundingBox)) {
            data.push_back(way);
          }
        }
      }

      tile.GetOptimizedWayData().SetData(subset,
                                         data);
    }
  }

  void TiledDataCache::ResolveWaysFromParent(Tile& tile,
                                             const Tile& parentTile,
                                             const GeoBox& boundingBox,
                                             const TypeInfoSet& wayTypes)
  {
    if (wayTypes.Intersects(parentTile.GetWayData().GetTypes())) {
      TypeInfoSet subset(wayTypes);

      subset.Intersection(parentTile.GetWayData().GetTypes());

      std::vector<WayRef> data;

      data.reserve(parentTile.GetWayData().GetData().size());

      for (const auto& way : parentTile.GetWayData().GetData()) {
        if (wayTypes.IsSet(way->GetType())) {
          GeoBox wayBoundingBox;

          way->GetBoundingBox(wayBoundingBox);

          if (wayBoundingBox.Intersects(boundingBox)) {
            data.push_back(way);
          }
        }
      }

      tile.GetWayData().SetData(subset,
                                data);
    }
  }

  void TiledDataCache::ResolveOptimizedAreasFromParent(Tile& tile,
                                                       const Tile& parentTile,
                                                       const GeoBox& boundingBox,
                                                       const TypeInfoSet& optimizedAreaTypes,
                                                       const TypeInfoSet& areaTypes)
  {
    if (optimizedAreaTypes.Intersects(parentTile.GetOptimizedAreaData().GetTypes())) {
      TypeInfoSet subset(areaTypes);

      subset.Intersection(parentTile.GetOptimizedAreaData().GetTypes());

      std::vector<AreaRef> data;

      data.reserve(parentTile.GetOptimizedAreaData().GetData().size());

      for (const auto& area : parentTile.GetOptimizedAreaData().GetData()) {
        if (areaTypes.IsSet(area->GetType())) {
          GeoBox areaBoundingBox;

          area->GetBoundingBox(areaBoundingBox);

          if (areaBoundingBox.Intersects(boundingBox)) {
            data.push_back(area);
          }
        }
      }

      tile.GetOptimizedAreaData().SetData(subset,
                                          data);
    }

  }

  void TiledDataCache::ResolveAreasFromParent(Tile& tile,
                                              const Tile& parentTile,
                                              const GeoBox& boundingBox,
                                              const TypeInfoSet& areaTypes)
  {
    if (areaTypes.Intersects(parentTile.GetAreaData().GetTypes())) {
      TypeInfoSet subset(areaTypes);

      subset.Intersection(parentTile.GetAreaData().GetTypes());

      std::vector<AreaRef> data;

      data.reserve(parentTile.GetAreaData().GetData().size());

      for (const auto& area : parentTile.GetAreaData().GetData()) {
        if (areaTypes.IsSet(area->GetType())) {
          GeoBox areaBoundingBox;

          area->GetBoundingBox(areaBoundingBox);

          if (areaBoundingBox.Intersects(boundingBox)) {
            data.push_back(area);
          }
        }
      }

      tile.GetAreaData().SetData(subset,
                                 data);
    }
  }

  /**
   * (Partially) prefill the given tiles with data already cached with data of the given types.
   *
   * Currently prefill is done by looking for the parent tile in the cache
   * and copying data that intersects the bounding box of the given tile.
   */
  void TiledDataCache::PrefillDataFromCache(Tile& tile,
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
        ResolveOptimizedWaysFromParent(tile,*parentTile,boundingBox,optimizedWayTypes,wayTypes);
        ResolveWaysFromParent(tile,*parentTile,boundingBox,wayTypes);
        ResolveOptimizedWaysFromParent(tile,*parentTile,boundingBox,optimizedAreaTypes,areaTypes);
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
