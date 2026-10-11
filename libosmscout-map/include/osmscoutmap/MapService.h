#ifndef OSMSCOUT_MAPSERVICE_H
#define OSMSCOUT_MAPSERVICE_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2009  Tim Teulings

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

#include <atomic>
#include <list>
#include <memory>
#include <thread>
#include <vector>

#include <osmscoutmap/MapImportExport.h>

#include <osmscout/TypeConfig.h>

#include <osmscoutmap/MapPainter.h>
#include <osmscoutmap/StyleConfig.h>

#include <osmscout/db/Database.h>

#include <osmscout/async/Breaker.h>
#include <osmscout/util/GeoBox.h>
#include <osmscout/util/StopClock.h>
#include <osmscout/async/WorkQueue.h>

#include <osmscoutmap/DataTileCache.h>

namespace osmscout {

  /**
   * \ingroup Service
   * \ingroup Renderer
   *
   * Parameter to influence the search result for searching for (drawable)
   * objects in a given area.
   */
  class OSMSCOUT_MAP_API AreaSearchParameter
  {
  private:
    unsigned long maxAreaLevel=4;
    bool          useLowZoomOptimization=true;
    BreakerRef    breaker;
    bool          useMultithreading=false;
    bool          resolveRouteMembers=true;

  public:
    AreaSearchParameter() = default;

    void SetMaximumAreaLevel(unsigned long maxAreaLevel);

    void SetUseLowZoomOptimization(bool useLowZoomOptimization);

    void SetUseMultithreading(bool useMultithreading);

    void SetResolveRouteMembers(bool resolveRouteMembers);

    void SetBreaker(const BreakerRef& breaker);

    unsigned long GetMaximumAreaLevel() const;

    bool GetUseLowZoomOptimization() const;

    bool GetUseMultithreading() const;

    bool GetResolveRouteMembers() const;

    bool IsAborted() const;
  };

  /**
   * \ingroup Service
   * \ingroup Renderer
   *
   * MapService offers services for retrieving data in a way that is
   * helpful for drawing maps.
   *
   * Currently the following functionalities are supported:
   * - Get objects of a certain type in a given area and impose certain
   * limits on the resulting data (size of area, number of objects,
   * low zoom optimizations,...).
   */
  class OSMSCOUT_MAP_API MapService
  {
  public:
    class OSMSCOUT_MAP_API TypeDefinition CLASS_FINAL
    {
    public:
      TypeInfoSet nodeTypes;
      TypeInfoSet wayTypes;
      TypeInfoSet areaTypes;
      TypeInfoSet routeTypes;
      TypeInfoSet optimizedAreaTypes;
      TypeInfoSet optimizedWayTypes;
    };

    using TypeDefinitionRef = std::shared_ptr<TypeDefinition>;

  public:
    using CallbackId = size_t;
    using TileStateCallback = std::function<void (const TileRef &)>;

  private:
    mutable std::mutex           stateMutex;           //!< Mutex to protect internal state

    DatabaseRef                  database;             //!< The reference to the db
    mutable DataTileCache        cache;                //!< Data cache

    /**
     * Duration a phase of the tile data conversion may take before it is reported. Atomic, because a
     * conversion runs on a worker thread while a caller may set the threshold.
     */
    std::atomic<double>          conversionPhaseWarningThreshold{20.0};

    mutable WorkQueue<bool>      nodeWorkerQueue;
    std::thread                  nodeWorkerThread;

    mutable WorkQueue<bool>      wayWorkerQueue;
    std::thread                  wayWorkerThread;

    mutable WorkQueue<bool>      wayLowZoomWorkerQueue;
    std::thread                  wayLowZoomWorkerThread;

    mutable WorkQueue<bool>      areaWorkerQueue;
    std::thread                  areaWorkerThread;

    mutable WorkQueue<bool>      areaLowZoomWorkerQueue;
    std::thread                  areaLowZoomWorkerThread;

    mutable WorkQueue<bool>      routeWorkerQueue;
    std::thread                  routeWorkerThread;

    CallbackId                   nextCallbackId;
    std::map<CallbackId,TileStateCallback> tileStateCallbacks;
    mutable std::mutex           callbackMutex;        //<! Mutex to protect callback (de)registering


  private:
    TypeDefinitionRef GetTypeDefinition(const AreaSearchParameter& parameter,
                                        const StyleConfig& styleConfig,
                                        const Magnification& magnification) const;

    bool GetNodes(const AreaSearchParameter& parameter,
                  const TypeInfoSet& nodeTypes,
                  const GeoBox& boundingBox,
                  bool prefill,
                  const TileRef& tile) const;

    bool GetAreasLowZoom(const AreaSearchParameter& parameter,
                         const TypeInfoSet& areaTypes,
                         const Magnification& magnification,
                         const GeoBox& boundingBox,
                         bool prefill,
                         const TileRef& tile) const;

    bool GetAreas(const AreaSearchParameter& parameter,
                  const TypeInfoSet& areaTypes,
                  const Magnification& magnification,
                  const GeoBox& boundingBox,
                  bool prefill,
                  const TileRef& tile) const;

    bool GetWaysLowZoom(const AreaSearchParameter& parameter,
                        const TypeInfoSet& wayTypes,
                        const Magnification& magnification,
                        const GeoBox& boundingBox,
                        bool prefill,
                        const TileRef& tile) const;

    bool GetWays(const AreaSearchParameter& parameter,
                 const TypeInfoSet& wayTypes,
                 const GeoBox& boundingBox,
                 bool prefill,
                 const TileRef& tile) const;

    bool GetRoutes(const AreaSearchParameter& parameter,
                   const TypeInfoSet& routes,
                   const GeoBox& boundingBox,
                   bool prefill,
                   const TileRef& tile) const;

    template<typename Object, typename AreaObjectIndex, typename ObjectByOffsetFn>
    bool GetObjects(const AreaSearchParameter& parameter,
                    const TypeInfoSet& types,
                    const GeoBox& boundingBox,
                    bool prefill,
                    const TileRef& tile,
                    TileData<Object> &tileData,
                    AreaObjectIndex areaObjectIndex,
                    ObjectByOffsetFn objectByOffsetFn,
                    const std::string_view &objectTypeName,
                    const std::string_view &objectTypeNamePl) const
    {
      if (!areaObjectIndex) {
        if (!prefill) {
          tileData.SetComplete();
          NotifyTileStateCallbacks(tile);
        }
        return false;
      }

      if (tileData.IsComplete()) {
        return true;
      }

      if (parameter.IsAborted()) {
        return false;
      }

      TypeInfoSet             cachedTypes(tileData.GetTypes());
      TypeInfoSet             requestedTypes(types);
      TypeInfoSet             loadedTypes;
      std::vector<FileOffset> offsets;

      if (!cachedTypes.Empty()) {
        requestedTypes.Remove(cachedTypes);
      }

      if (!requestedTypes.Empty()) {
        if (!areaObjectIndex->GetOffsets(boundingBox,
                                         requestedTypes,
                                         offsets,
                                         loadedTypes)) {
          log.Error() << "Error getting " << objectTypeNamePl << " from area " << objectTypeName << " index!";
          return false;
        }

        if (parameter.IsAborted()) {
          return false;
        }

        if (!offsets.empty()) {
          // Sort offsets before loading to optimize disk access
          std::sort(offsets.begin(),offsets.end());

          if (parameter.IsAborted()) {
            return false;
          }

          std::vector<Object> objects;

          if (!objectByOffsetFn(offsets, objects)) {
            log.Error() << "Error reading " << objectTypeNamePl << " in area!";
            return false;
          }

          if (parameter.IsAborted()) {
            return false;
          }

          if (prefill) {
            tileData.AddPrefillData(loadedTypes, std::move(objects));
          }
          else {
            if (cachedTypes.Empty()){
              tileData.SetData(loadedTypes, std::move(objects));
            }else{
              tileData.AddData(loadedTypes, objects);
            }
          }
        }
      }

      if (!prefill) {
        tileData.SetComplete();
      }

      NotifyTileStateCallbacks(tile);

      return !parameter.IsAborted();
    }

    void NodeWorkerLoop();
    void WayWorkerLoop();
    void WayLowZoomWorkerLoop();
    void AreaWorkerLoop();
    void AreaLowZoomWorkerLoop();
    void RouteWorkerLoop();

    std::future<bool> PushNodeTask(const AreaSearchParameter& parameter,
                                   const TypeInfoSet& nodeTypes,
                                   const GeoBox& boundingBox,
                                   bool prefill,
                                   const TileRef& tile) const;

    std::future<bool> PushAreaLowZoomTask(const AreaSearchParameter& parameter,
                                          const TypeInfoSet& areaTypes,
                                          const Magnification& magnification,
                                          const GeoBox& boundingBox,
                                          bool prefill,
                                          const TileRef& tile) const;

    std::future<bool> PushAreaTask(const AreaSearchParameter& parameter,
                                   const TypeInfoSet& areaTypes,
                                   const Magnification& magnification,
                                   const GeoBox& boundingBox,
                                   bool prefill,
                                   const TileRef& tile) const;

    std::future<bool> PushWayLowZoomTask(const AreaSearchParameter& parameter,
                                         const TypeInfoSet& wayTypes,
                                         const Magnification& magnification,
                                         const GeoBox& boundingBox,
                                         bool prefill,
                                         const TileRef& tile) const;

    std::future<bool> PushWayTask(const AreaSearchParameter& parameter,
                                  const TypeInfoSet& wayTypes,
                                  const GeoBox& boundingBox,
                                  bool prefill,
                                  const TileRef& tile) const;

    std::future<bool> PushRouteTask(const AreaSearchParameter& parameter,
                                    const TypeInfoSet& routeTypes,
                                    const GeoBox& boundingBox,
                                    bool prefill,
                                    const TileRef& tile) const;

    void NotifyTileStateCallbacks(const TileRef& tile) const;

    bool LoadMissingTileDataStyleSheet(const AreaSearchParameter& parameter,
                                       const StyleConfig& styleConfig,
                                       std::list<TileRef>& tiles,
                                       bool async) const;

    bool LoadMissingTileDataTypeDefinition(const AreaSearchParameter& parameter,
                                           const Magnification& magnification,
                                           const TypeDefinition& typeDefinition,
                                           std::list<TileRef>& tiles,
                                           bool async) const;

  public:
    explicit MapService(const DatabaseRef& database);
    virtual ~MapService();

    void SetCacheSize(size_t cacheSize);
    size_t GetCacheSize() const;
    size_t GetCurrentCacheSize() const;

    void CleanupTileCache();
    void FlushTileCache();
    void InvalidateTileCache();

    void LookupTiles(const Magnification& magnification,
                     const GeoBox& boundingBox,
                     std::list<TileRef>& tiles) const;

    void LookupTiles(const Projection& projection,
                     std::list<TileRef>& tiles) const;

    TileRef LookupTile(const TileKey& key) const;

    bool LoadMissingTileData(const AreaSearchParameter& parameter,
                             const StyleConfig& styleConfig,
                             std::list<TileRef>& tiles) const;

    bool LoadMissingTileDataAsync(const AreaSearchParameter& parameter,
                                  const StyleConfig& styleConfig,
                                  std::list<TileRef>& tiles) const;

    bool LoadMissingTileData(const AreaSearchParameter& parameter,
                             const Magnification& magnification,
                             const TypeDefinition& typeDefinition,
                             std::list<TileRef>& tiles) const;

    bool LoadMissingTileDataAsync(const AreaSearchParameter& parameter,
                                  const Magnification& magnification,
                                  const TypeDefinition& typeDefinition,
                                  std::list<TileRef>& tiles) const;

    /**
     * Convert the data hold by the given tiles into the given map data.
     *
     * Every object of the tiles is placed in the result exactly once. The result is grouped by the
     * source data file of the objects in a fixed order - the objects of `nodes.dat`, `ways.dat`,
     * `areas.dat` and `routes.dat` first, then the objects of the optimized data files of a kind -
     * and within a source data file the objects follow the order in which the tiles of the given list
     * and their stored objects present them, which is reproducible for the same tile list.
     *
     * An offset only identifies an object within one data file, so the objects of two data files that
     * carry the same offset value are both part of the result. The heap allocation of the conversion
     * is bounded by the number of distinct objects of the result plus a constant: seen objects are
     * held as a set of file offsets, which allocates once per distinct object of a source data file,
     * and each result vector is reserved once.
     */
    void AddTileDataToMapData(std::list<TileRef>& tiles,
                              MapData& data) const;

    /**
     * Convert the data hold by the given tiles into the given map data, restricted to the object
     * types of the given type definition.
     *
     * The restricted conversion holds the same uniqueness and order guarantees as the unrestricted
     * one. It reserves its result from the objects the tiles hold rather than from the objects that
     * match the type filter, because the matching count is only known once the objects have been
     * visited; the reservation is therefore an upper bound.
     */
    void AddTileDataToMapData(std::list<TileRef>& tiles,
                              const TypeDefinition& typeDefinition,
                              MapData& data) const;

    /**
     * Set the duration a phase of the tile data conversion may take before it is reported.
     *
     * A conversion measures one phase per source data file - `nodes`, `ways`, `optimized ways`,
     * `areas`, `optimized areas` and `routes` - and logs one warning naming a phase that took longer
     * than this threshold, so that a slow pan step is attributable from the log of a user device. The
     * default is 20 milliseconds, the value the copy phase of the previous implementation warned at.
     * A threshold of zero reports every phase.
     */
    void SetConversionPhaseWarningThreshold(double milliseconds);

    /**
     * Duration a phase of the tile data conversion may take before it is reported, see
     * SetConversionPhaseWarningThreshold().
     */
    double GetConversionPhaseWarningThreshold() const;

    bool GetGroundTiles(const Projection& projection,
                        std::list<GroundTile>& tiles) const;

    bool GetGroundTiles(const GeoBox& boundingBox,
                        const Magnification& magnification,
                        std::list<GroundTile>& tiles) const;

    SRTMDataRef GetSRTMData(const Projection& projection) const;

    SRTMDataRef GetSRTMData(const GeoBox& boundingBox) const;

    CallbackId RegisterTileStateCallback(TileStateCallback callback);
    void DeregisterTileStateCallback(CallbackId callbackId);
  };

  //! \ingroup Service
  //! Reference counted reference to an Database instance
  using MapServiceRef = std::shared_ptr<MapService>;
}

#endif
