#ifndef OSMSCOUT_TILEDDATACACHE_H
#define OSMSCOUT_TILEDDATACACHE_H

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

#include <osmscout/TileId.h>

namespace osmscout {

  /**
   * \ingroup tiledcache
   *
   * Temlate for storing sets of data of the same type in a tile. Normally data will either be NodeRef, WayRef or AreaRef.
   */
  template<typename O>
  class OSMSCOUT_MAP_API TileData
  {
  private:
    mutable std::mutex mutex;

    TypeInfoSet        types;

    std::vector<O>     prefillData;
    std::vector<O>     data;

    bool               complete;

  public:
    /**
     * Create an empty and unassigned TileData
     */
    TileData()
    : complete(false)
    {
      // no code
    }

    bool IsEmpty() const
    {
      std::lock_guard<std::mutex> guard(mutex);

      return types.Empty();
    }

    /**
     * Marks the tile as inpcomplete again, without actually clearing data and types.
     */
    void Invalidate()
    {
      std::lock_guard<std::mutex> guard(mutex);

      complete=false;
    }

    /**
     * Assign data to the tile that was derived from existing tiles. Resets the list of loaded types
     * to the list given.
     */
    void AddPrefillData(const TypeInfoSet& types,
                        const std::vector<O>& data)
    {
      std::lock_guard<std::mutex> guard(mutex);

      if (this->types.Empty()) {
        this->types=types;
      }
      else {
        this->types.Add(types);
      }

      if (this->prefillData.empty()) {
        this->prefillData=data;
      }
      else {
        this->prefillData.reserve(this->prefillData.size()+data.size());
        this->prefillData.insert(this->prefillData.end(),data.begin(),data.end());
      }

      complete=false;
    }

    /**
     * Assign data to the tile that was derived from existing tiles. Resets the list of loaded types
     * to the list given. This version has move semantics for the data.
     */
    void AddPrefillData(const TypeInfoSet& types,
                        std::vector<O>&& data)
    {
      std::lock_guard<std::mutex> guard(mutex);

      if (this->types.Empty()) {
        this->types=types;
      }
      else {
        this->types.Add(types);
      }

      if (this->prefillData.empty()) {
        this->prefillData=std::move(data);
      }
      else {
        this->prefillData.reserve(this->prefillData.size()+data.size());
        this->prefillData.insert(this->prefillData.end(),data.begin(),data.end());
      }

      complete=false;
    }

    /**
     * Assign data to the tile and mark the tile as completed.
     */
    void SetData(const TypeInfoSet& types,
                 const std::vector<O>& data)
    {
      std::lock_guard<std::mutex> guard(mutex);

      this->data=data;
      this->types.Add(types);

      complete=true;
    }

    /**
     * Assign data to the tile and mark the tile as completed.  This version has move semantics for the data.
     */
    void SetData(const TypeInfoSet& types,
                 std::vector<O>&& data)
    {
      std::lock_guard<std::mutex> guard(mutex);

      this->data=std::move(data);
      this->types.Add(types);

      complete=true;
    }

    /**
     * Mark the tile as completed (useful if prefill data is already complete and
     * no more actual data has to be loaded)
     */
    void SetComplete()
    {
      std::lock_guard<std::mutex> guard(mutex);

      complete=true;
    }

    /**
     * Return 'true' if there was data already assigned to the tile
     */
    bool IsComplete() const
    {
      std::lock_guard<std::mutex> guard(mutex);

      return complete;
    }

    /**
     * Return the list of types of the data stored in the tile.
     *
     * Note that it is stll possibly that there is no acutal data for this type in the
     * TileData stored.
     */
    const TypeInfoSet GetTypes() const
    {
      std::lock_guard<std::mutex> guard(mutex);

      return types;
    }

    size_t GetDataSize() const
    {
      std::lock_guard<std::mutex> guard(mutex);

      return prefillData.size()+data.size();
    }

    void CopyData(std::function<void(const O&)> function) const
    {
      std::lock_guard<std::mutex> guard(mutex);

      std::for_each(prefillData.begin(),prefillData.end(),function);
      std::for_each(data.begin(),data.end(),function);
    }
  };

  /**
   * \ingroup tiledcache
   *
   * TileData for nodes
   */
  typedef TileData<NodeRef> TileNodeData;

  /**
   * \ingroup tiledcache
   *
   * TileData for ways
   */
  typedef TileData<WayRef>  TileWayData;

  /**
   * \ingroup tiledcache
   *
   * TileData for areas
   */
  typedef TileData<AreaRef> TileAreaData;

  // Forward declaration of DataTileCache for friend declaration in Tile
  class DataTileCache;

  /**
   * \ingroup tiledcache
   *
   * Result of a cache lookup. If there is a cache hit,
   * data is set, else data is null and you must load
   * and add it to the cache afterwards.
   */
  class OSMSCOUT_MAP_API Tile
  {
  private:
    TileId       id;                //!< Id of the tile
    TileNodeData nodeData;          //!< Node data
    TileWayData  wayData;           //!< Way data
    TileAreaData areaData;          //!< Area data
    TileWayData  optimizedWayData;  //!< Optimized way data
    TileAreaData optimizedAreaData; //!< Optimized area data

  private:
    Tile(const TileId& id);

  public:
    friend class DataTileCache;

    ~Tile();

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

    /**
     * Return a read-only reference to the node data
     */
    inline const TileNodeData& GetNodeData() const
    {
      return nodeData;
    }

    /**
     * Return a read-only reference to the way data
     */
    inline const TileWayData& GetWayData() const
    {
      return wayData;
    }

    /**
     * Return a read-only reference to the area data
     */
    inline const TileAreaData& GetAreaData() const
    {
      return areaData;
    }

    /**
     * Return a read-only reference to the optimized way data
     */
    inline const TileWayData& GetOptimizedWayData() const
    {
      return optimizedWayData;
    }

    /**
     * Return a read-only reference to the optimized area data
     */
    inline const TileAreaData& GetOptimizedAreaData() const
    {
      return optimizedAreaData;
    }

    /**
     * Return a read-write reference to the node data
     */
    inline TileNodeData& GetNodeData()
    {
      return nodeData;
    }

    /**
     * Return a read-write reference to the way data
     */
    inline TileWayData& GetWayData()
    {
      return wayData;
    }

    /**
     * Return a read-write reference to the area data
     */
    inline TileAreaData& GetAreaData()
    {
      return areaData;
    }

    /**
     * Return a read-write reference to the optimized way data
     */
    inline TileWayData& GetOptimizedWayData()
    {
      return optimizedWayData;
    }

    /**
     * Return a read-write reference to the optimized area data
     */
    inline TileAreaData& GetOptimizedAreaData()
    {
      return optimizedAreaData;
    }

    /**
     * Return 'true' if no data at all has been assigned
     */
    inline bool IsComplete() const
    {
      return nodeData.IsComplete() &&
             wayData.IsComplete() &&
             areaData.IsComplete() &&
             optimizedWayData.IsComplete() &&
             optimizedAreaData.IsComplete();
    }

    /**
     * Return 'true' if no data for any type has been assigned
     */
    inline bool IsEmpty() const
    {
      return nodeData.IsEmpty() &&
             wayData.IsEmpty() &&
             areaData.IsEmpty() &&
             optimizedWayData.IsEmpty() &&
             optimizedAreaData.IsEmpty();
    }
  };

  /**
   * \ingroup tiledcache
   *
   * Reference counted reference to a tile
   */
  typedef std::shared_ptr<Tile> TileRef;

  /**
   * \ingroup tiledcache
   *
   * Data cache using tile based cache pages. The cache holds a number of of tiles. The
   * maximum number of tiles hold can be configured. Tiles however will only be freed
   * if a cleanup is explicitely triggered. So temporary overbooking can happen. This should
   * assure that prefilling of tiles is possible even with a very low limit.
   *
   * The cache will free least recently used tiles first,
   *
   */
  class OSMSCOUT_MAP_API DataTileCache
  {
  private:
    /**
     * Internally used cache entry
     */
    struct OSMSCOUT_MAP_API CacheEntry
    {
      TileId  id;
      //TileWeakRef tile;
      TileRef tile;

      CacheEntry(const TileId& id,
                 const TileRef /*TileWeakRef*/& tile)
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

    void ResolveNodesFromParent(Tile& tile,
                                const Tile& parentTile,
                                const GeoBox& boundingBox,
                                const TypeInfoSet& nodeTypes);
    void ResolveOptimizedWaysFromParent(Tile& tile,
                                        const Tile& parentTile,
                                        const GeoBox& boundingBox,
                                        const TypeInfoSet& optimizedWayTypes);
    void ResolveWaysFromParent(Tile& tile,
                               const Tile& parentTile,
                               const GeoBox& boundingBox,
                               const TypeInfoSet& wayTypes);
    void ResolveOptimizedAreasFromParent(Tile& tile,
                                         const Tile& parentTile,
                                         const GeoBox& boundingBox,
                                         const TypeInfoSet& optimizedAreaTypes);
    void ResolveAreasFromParent(Tile& tile,
                                const Tile& parentTile,
                                const GeoBox& boundingBox,
                                const TypeInfoSet& areaTypes);

  public:
    DataTileCache(size_t cacheSize);

    void SetSize(size_t cacheSize);

    inline size_t GetSize() const
    {
      return cacheSize;
    }

    void CleanupCache();

    void InvalidateCache();

    TileRef GetCachedTile(const TileId& id) const;
    TileRef GetTile(const TileId& id) const;

    void GetTilesForBoundingBox(const Magnification& magnification,
                                const GeoBox& boundingBox,
                                std::list<TileRef>& tiles) const;

    void PrefillDataFromCache(Tile& tile,
                              const TypeInfoSet& nodeTypes,
                              const TypeInfoSet& wayTypes,
                              const TypeInfoSet& areaTypes,
                              const TypeInfoSet& optimizedWayTypes,
                              const TypeInfoSet& optimizedAreaTypes);
  };

  /**
   * \ingroup tiledcache
   *
   * Reference counted reference to a DataTileCache instance
   */
  typedef std::shared_ptr<DataTileCache> TiledDataCacheRef;

  /**
   * \defgroup tiledcache Classes for caching map data per tile
   */
}

#endif
