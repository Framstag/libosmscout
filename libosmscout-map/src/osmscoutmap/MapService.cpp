/*
  This source is part of the libosmscout library
  Copyright (C) 2014  Tim Teulings

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

#include <osmscoutmap/MapService.h>

#include <cstddef>
#include <functional>
#include <future>
#include <list>
#include <vector>

#include <osmscout/system/Assert.h>
#include <osmscout/system/Math.h>
#include <osmscout/async/Thread.h>

#include <osmscout/util/Geometry.h>
#include <osmscout/log/Logger.h>

namespace osmscout {

  namespace {

    /**
     * Hash set of file offsets with linear probing.
     *
     * The objects of a tile are looked up in the set once per object reference, and a view repeats an
     * object as often as the tiles of the tile list carry it, so the set is a flat table rather than a
     * node-based set: it allocates its table once and doubles it when it fills up, instead of
     * allocating a node per object and rehashing from an empty table. A slot holds the stored offset
     * plus one, so that zero marks a free slot; an object is never stored at offset zero, because the
     * data files start with a header.
     */
    class FileOffsetSet
    {
    private:
      static constexpr size_t kInitialSlots=1024;

      std::vector<FileOffset> table;
      size_t                  count{0};

      static size_t Hash(FileOffset offset)
      {
        // Knuth's multiplicative hash: the offsets of a data file are sorted and close to each other,
        // so the low bits alone would collide.
        return size_t((offset*11400714819323198485ull)>>40);
      }

      void Grow()
      {
        std::vector<FileOffset> grown(table.size()*2,0);

        for (const auto& entry : table) {
          if (entry==0) {
            continue;
          }

          size_t slot=Hash(entry-1)&(grown.size()-1);

          while (grown[slot]!=0) {
            slot=(slot+1)&(grown.size()-1);
          }

          grown[slot]=entry;
        }

        table.swap(grown);
      }

    public:
      FileOffsetSet()
      : table(kInitialSlots,0)
      {
        // no code
      }

      /**
       * Insert an offset. Return true if it was not in the set yet.
       */
      bool Insert(FileOffset offset)
      {
        if (count*4>=table.size()*3) {
          Grow();
        }

        size_t slot=Hash(offset)&(table.size()-1);

        for (;;) {
          FileOffset entry=table[slot];

          if (entry==0) {
            table[slot]=offset+1;
            count++;

            return true;
          }

          if (entry==offset+1) {
            return false;
          }

          slot=(slot+1)&(table.size()-1);
        }
      }
    };

    /**
     * Duration of one phase of a conversion.
     */
    struct ConversionPhase
    {
      const char* name;
      double      milliseconds;
    };

    /**
     * Durations of the phases of one conversion, one phase per source data file.
     */
    struct ConversionTiming
    {
      std::vector<ConversionPhase> phases;
    };

    /**
     * Report every phase of a conversion that took longer than the threshold, naming the phase.
     */
    void ReportSlowConversionPhases(const ConversionTiming& timing,
                                    double threshold)
    {
      for (const auto& phase : timing.phases) {
        if (phase.milliseconds>threshold) {
          log.Warn() << "Tile data conversion: phase " << phase.name
                     << " took " << phase.milliseconds << " ms";
        }
      }
    }

    /**
     * Convert one source data file of one kind.
     *
     * Every object of the tiles is appended to the result on its first sight, so an object that
     * several tiles carry appears exactly once. The objects seen so far are kept as a set of file
     * offsets, which identifies an object only within one data file: the plain and the optimized data
     * of a kind are read from different files and their offsets are independent of each other, so
     * every source data file gets its own set.
     *
     * The order of the result is the order in which the tiles of the given list present their stored
     * objects, which is reproducible for the same tile list and the same database, and which no
     * longer depends on the iteration order of an internal container.
     */
    template<typename O>
    void ConvertSource(const std::list<TileRef>& tiles,
                       const std::function<const TileData<O>&(const TileRef&)>& getData,
                       const TypeInfoSet* filter,
                       const char* sourceName,
                       std::vector<O>& out,
                       ConversionTiming& timing)
    {
      FileOffsetSet seen;

      /**
       * The state the tile hands its stored objects to. It is captured as one pointer, so that the
       * function a tile builds for its objects holds a pointer rather than three references and does
       * not allocate.
       */
      struct Sink
      {
        std::vector<O>* out;
        FileOffsetSet*  seen;
        const TypeInfoSet* filter;
      };

      Sink sink{&out,&seen,filter};

      StopClock phaseTimer;

      for (const auto& tile : tiles) {
        getData(tile).CopyData([&sink](const O& object) {
          if (sink.filter!=nullptr && !sink.filter->IsSet(object->GetType())) {
            return;
          }

          if (sink.seen->Insert(object->GetFileOffset())) {
            sink.out->push_back(object);
          }
        });
      }

      phaseTimer.Stop();

      timing.phases.push_back(ConversionPhase{sourceName,phaseTimer.GetMilliseconds()});
    }

    /**
     * Sum of the objects the given kind of the tiles holds. An upper bound for the result of a
     * conversion of that kind, used to reserve the result once. The bound over-estimates when a tile
     * list names a tile more than once.
     */
    template<typename O>
    size_t SumDataSize(const std::list<TileRef>& tiles,
                       const std::function<const TileData<O>&(const TileRef&)>& getData)
    {
      size_t sum=0;

      for (const auto& tile : tiles) {
        sum+=getData(tile).GetDataSize();
      }

      return sum;
    }

    const TileNodeData& GetNodeData(const TileRef& tile)
    {
      return tile->GetNodeData();
    }

    const TileWayData& GetWayData(const TileRef& tile)
    {
      return tile->GetWayData();
    }

    const TileWayData& GetOptimizedWayData(const TileRef& tile)
    {
      return tile->GetOptimizedWayData();
    }

    const TileAreaData& GetAreaData(const TileRef& tile)
    {
      return tile->GetAreaData();
    }

    const TileAreaData& GetOptimizedAreaData(const TileRef& tile)
    {
      return tile->GetOptimizedAreaData();
    }

    const TileRouteData& GetRouteData(const TileRef& tile)
    {
      return tile->GetRouteData();
    }
  }


  void AreaSearchParameter::SetMaximumAreaLevel(unsigned long maxAreaLevel)
  {
    this->maxAreaLevel=maxAreaLevel;
  }

  void AreaSearchParameter::SetUseLowZoomOptimization(bool useLowZoomOptimization)
  {
    this->useLowZoomOptimization=useLowZoomOptimization;
  }

  void AreaSearchParameter::SetUseMultithreading(bool useMultithreading)
  {
    this->useMultithreading=useMultithreading;
  }

  void AreaSearchParameter::SetResolveRouteMembers(bool resolveRouteMembers)
  {
    this->resolveRouteMembers=resolveRouteMembers;
  }

  bool AreaSearchParameter::GetResolveRouteMembers() const
  {
    return resolveRouteMembers;
  }

  void AreaSearchParameter::SetBreaker(const BreakerRef& breaker)
  {
    this->breaker=breaker;
  }

  unsigned long AreaSearchParameter::GetMaximumAreaLevel() const
  {
    return maxAreaLevel;
  }

  bool AreaSearchParameter::GetUseLowZoomOptimization() const
  {
    return useLowZoomOptimization;
  }

  bool AreaSearchParameter::GetUseMultithreading() const
  {
    return useMultithreading;
  }

  bool AreaSearchParameter::IsAborted() const
  {
    if (breaker) {
      return breaker->IsAborted();
    }
    else {
      return false;
    }
  }

  MapService::MapService(const DatabaseRef& database)
   : database(database),
     cache(25),
     nodeWorkerThread(&MapService::NodeWorkerLoop,this),
     wayWorkerThread(&MapService::WayWorkerLoop,this),
     wayLowZoomWorkerThread(&MapService::WayLowZoomWorkerLoop,this),
     areaWorkerThread(&MapService::AreaWorkerLoop,this),
     areaLowZoomWorkerThread(&MapService::AreaLowZoomWorkerLoop,this),
     routeWorkerThread(&MapService::RouteWorkerLoop,this),
     nextCallbackId(0)
  {
    // no code
  }

  MapService::~MapService()
  {
    nodeWorkerQueue.Stop();
    wayWorkerQueue.Stop();
    wayLowZoomWorkerQueue.Stop();
    areaWorkerQueue.Stop();
    routeWorkerQueue.Stop();
    areaLowZoomWorkerQueue.Stop();

    nodeWorkerThread.join();
    wayWorkerThread.join();
    wayLowZoomWorkerThread.join();
    areaWorkerThread.join();
    routeWorkerThread.join();
    areaLowZoomWorkerThread.join();
  }

  /**
   * Set the size of the tile data cache
   */
  void MapService::SetConversionPhaseWarningThreshold(double milliseconds)
  {
    conversionPhaseWarningThreshold.store(milliseconds);
  }

  double MapService::GetConversionPhaseWarningThreshold() const
  {
    return conversionPhaseWarningThreshold.load();
  }

  void MapService::SetCacheSize(size_t cacheSize)
  {
    std::lock_guard<std::mutex> lock(stateMutex);

    cache.SetSize(cacheSize);
  }

  size_t MapService::GetCacheSize() const
  {
    std::lock_guard<std::mutex> lock(stateMutex);

    return cache.GetSize();
  }

  size_t MapService::GetCurrentCacheSize() const
  {
    std::lock_guard<std::mutex> lock(stateMutex);

    return cache.GetCurrentSize();
  }

  /**
   * Evict tiles from cache until tile count <= cacheSize
   */
  void MapService::CleanupTileCache()
  {
    std::lock_guard<std::mutex> lock(stateMutex);

    cache.CleanupCache();
  }

  /**
   * Evict all tiles from cache (tile count == 0)
   */
  void MapService::FlushTileCache()
  {
    std::lock_guard<std::mutex> lock(stateMutex);

    size_t size=cache.GetSize();
    cache.SetSize(0);
    cache.SetSize(size);
  }

  /**
   * Mark all tiles in cache as incomplete, while keeping all data and type
   * information stored in it
   */
  void MapService::InvalidateTileCache()
  {
    std::lock_guard<std::mutex> lock(stateMutex);

    cache.InvalidateCache();
  }

  /**
   * Create a TypeDefinition based on the given parameter, StyleConfiguration and
   * magnifications. Effectly returns all types needed to load everything that is
   * visible using the given StyleConfig and the given Magnification.
   *
   * @param parameter
   *    Drawing parameter
   * @param styleConfig
   *    StyleConfig instance
   * @param magnification
   *    Magnification
   * @return
   *    Filled TypeDefinition
   */
  MapService::TypeDefinitionRef MapService::GetTypeDefinition(const AreaSearchParameter& parameter,
                                                              const StyleConfig& styleConfig,
                                                              const Magnification& magnification) const
  {
    OptimizeAreasLowZoomRef optimizeAreasLowZoom=database->GetOptimizeAreasLowZoom();
    OptimizeWaysLowZoomRef  optimizeWaysLowZoom=database->GetOptimizeWaysLowZoom();

    if (!optimizeAreasLowZoom ||
        !optimizeWaysLowZoom) {
      return nullptr;
    }

    TypeDefinitionRef typeDefinition=std::make_shared<TypeDefinition>();

    styleConfig.GetNodeTypesWithMaxMag(magnification,
                                       typeDefinition->nodeTypes);

    styleConfig.GetWayTypesWithMaxMag(magnification,
                                       typeDefinition->wayTypes);

    styleConfig.GetAreaTypesWithMaxMag(magnification,
                                       typeDefinition->areaTypes);

    styleConfig.GetRouteTypesWithMaxMag(magnification,
                                        typeDefinition->routeTypes);

    if (parameter.GetUseLowZoomOptimization()) {
      if (optimizeAreasLowZoom->HasOptimizations(magnification.GetMagnification())) {
        optimizeAreasLowZoom->GetTypes(magnification,
                                       typeDefinition->areaTypes,
                                       typeDefinition->optimizedAreaTypes);

        typeDefinition->areaTypes.Remove(typeDefinition->optimizedAreaTypes);
      }

      if (optimizeWaysLowZoom->HasOptimizations(magnification.GetMagnification())) {
        optimizeWaysLowZoom->GetTypes(magnification,
                                      typeDefinition->wayTypes,
                                      typeDefinition->optimizedWayTypes);

        typeDefinition->wayTypes.Remove(typeDefinition->optimizedWayTypes);
      }
    }

    return typeDefinition;
  }

  bool MapService::GetNodes(const AreaSearchParameter& parameter,
                            const TypeInfoSet& nodeTypes,
                            const GeoBox& boundingBox,
                            bool prefill,
                            const TileRef& tile) const
  {
    AreaNodeIndexRef areaNodeIndex=database->GetAreaNodeIndex();

    if (!areaNodeIndex) {
      if (!prefill) {
        // nodes data may be missing by purpose (e.g. for basemap),
        // lets mark the tile as complete and notify upstream to avoid endless loading
        tile->GetNodeData().SetComplete();
        NotifyTileStateCallbacks(tile);
      }
      return false;
    }

    if (tile->GetNodeData().IsComplete()) {
      return true;
    }

    if (parameter.IsAborted()) {
      return false;
    }

    TypeInfoSet             requestedNodeTypes(nodeTypes);
    TypeInfoSet             cachedNodeTypes(tile->GetNodeData().GetTypes());
    TypeInfoSet             loadedNodeTypes;
    std::vector<FileOffset> offsets;

    if (!cachedNodeTypes.Empty()) {
      requestedNodeTypes.Remove(cachedNodeTypes);
    }

    if (!requestedNodeTypes.Empty()) {
      if (!areaNodeIndex->GetOffsets(boundingBox,
                                     requestedNodeTypes,
                                     offsets,
                                     loadedNodeTypes)) {
        log.Error() << "Error getting nodes from area node index!";
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

        std::vector<NodeRef> nodes;

        if (!database->GetNodesByOffset(offsets,
                                        boundingBox,
                                        nodes)) {
          log.Error() << "Error reading nodes in area!";
          return false;
        }

        if (parameter.IsAborted()) {
          return false;
        }

        if (prefill)
        {
          tile->GetNodeData().AddPrefillData(loadedNodeTypes,std::move(nodes));
        }
        else {
          if (cachedNodeTypes.Empty()){
            tile->GetNodeData().SetData(loadedNodeTypes,std::move(nodes));
          }else{
            tile->GetNodeData().AddData(loadedNodeTypes,nodes);
          }
        }
      }
    }

    if (!prefill) {
      tile->GetNodeData().SetComplete();
    }

    NotifyTileStateCallbacks(tile);

    return !parameter.IsAborted();
  }

  bool MapService::GetAreasLowZoom(const AreaSearchParameter& parameter,
                                   const TypeInfoSet& areaTypes,
                                   const Magnification& magnification,
                                   const GeoBox& boundingBox,
                                   bool prefill,
                                   const TileRef& tile) const
  {
    OptimizeAreasLowZoomRef optimizeAreasLowZoom=database->GetOptimizeAreasLowZoom();

    if (!optimizeAreasLowZoom) {
      if (!prefill) {
        tile->GetOptimizedAreaData().SetComplete();
        NotifyTileStateCallbacks(tile);
      }
      return false;
    }

    if (!optimizeAreasLowZoom->HasOptimizations(magnification.GetMagnification())) {
      tile->GetOptimizedAreaData().SetComplete();
      NotifyTileStateCallbacks(tile);
      return true;
    }

    if (tile->GetOptimizedAreaData().IsComplete()) {
      return true;
    }

    if (parameter.IsAborted()) {
      return false;
    }

    TypeInfoSet cachedAreaTypes(tile->GetOptimizedAreaData().GetTypes());
    TypeInfoSet requestedAreaTypes(areaTypes);
    TypeInfoSet loadedAreaTypes;

    if (!cachedAreaTypes.Empty()) {
      requestedAreaTypes.Remove(cachedAreaTypes);
    }

    if (!requestedAreaTypes.Empty()) {
      std::vector<AreaRef> areas;

      if (!optimizeAreasLowZoom->GetAreas(boundingBox,
                                          magnification,
                                          requestedAreaTypes,
                                          areas,
                                          loadedAreaTypes)) {
        log.Error() << "Error getting areas from optimized areas index!";
        return false;
      }

      if (parameter.IsAborted()) {
        return false;
      }

      if (prefill) {
        tile->GetOptimizedAreaData().AddPrefillData(loadedAreaTypes,std::move(areas));
      }
      else {
        if (cachedAreaTypes.Empty()){
          tile->GetOptimizedAreaData().SetData(loadedAreaTypes,std::move(areas));
        }else{
          tile->GetOptimizedAreaData().AddData(loadedAreaTypes,areas);
        }
      }
    }

    if (!prefill) {
      tile->GetOptimizedAreaData().SetComplete();
    }

    NotifyTileStateCallbacks(tile);

    return !parameter.IsAborted();
  }

  bool MapService::GetAreas(const AreaSearchParameter& parameter,
                            const TypeInfoSet& areaTypes,
                            const Magnification& magnification,
                            const GeoBox& boundingBox,
                            bool prefill,
                            const TileRef& tile) const
  {
    AreaAreaIndexRef areaAreaIndex=database->GetAreaAreaIndex();

    if (!areaAreaIndex) {
      if (!prefill) {
        // area data may be missing by purpose (e.g. for basemap),
        // lets mark the tile as complete and notify upstream to avoid endless loading
        tile->GetAreaData().SetComplete();
        NotifyTileStateCallbacks(tile);
      }
      return false;
    }

    if (tile->GetAreaData().IsComplete()) {
      return true;
    }

    if (parameter.IsAborted()) {
      return false;
    }

    TypeInfoSet                cachedAreaTypes(tile->GetAreaData().GetTypes());
    TypeInfoSet                requestedAreaTypes(areaTypes);
    TypeInfoSet                loadedAreaTypes;
    std::vector<DataBlockSpan> spans;

    if (!cachedAreaTypes.Empty()) {
      requestedAreaTypes.Remove(cachedAreaTypes);
    }

    if (!requestedAreaTypes.Empty()) {
      if (!areaAreaIndex->GetAreasInArea(*database->GetTypeConfig(),
                                         boundingBox,
                                         magnification.GetLevel()+
                                         parameter.GetMaximumAreaLevel(),
                                         requestedAreaTypes,
                                         spans,
                                         loadedAreaTypes)) {
        log.Error() << "Error getting areas from area index!";
        return false;
      }

      if (parameter.IsAborted()) {
        return false;
      }

      if (!spans.empty()) {
        // Sort spans before loading to optimize disk access
        std::sort(spans.begin(),spans.end());

        if (parameter.IsAborted()) {
          return false;
        }

        std::vector<AreaRef> areas;

        if (!database->GetAreasByBlockSpans(spans,
                                            areas)) {
          log.Error() << "Error reading areas in area!";
          return false;
        }

        if (parameter.IsAborted()) {
          return false;
        }

        if (prefill) {
          tile->GetAreaData().AddPrefillData(loadedAreaTypes,std::move(areas));
        }
        else {
          if (cachedAreaTypes.Empty()){
            tile->GetAreaData().SetData(loadedAreaTypes,std::move(areas));
          }else{
            tile->GetAreaData().AddData(loadedAreaTypes,areas);
          }
        }
      }
    }

    if (!prefill) {
      tile->GetAreaData().SetComplete();
    }

    NotifyTileStateCallbacks(tile);

    return !parameter.IsAborted();
  }

  bool MapService::GetWaysLowZoom(const AreaSearchParameter& parameter,
                                  const TypeInfoSet& wayTypes,
                                  const Magnification& magnification,
                                  const GeoBox& boundingBox,
                                  bool prefill,
                                  const TileRef& tile) const
  {
    OptimizeWaysLowZoomRef optimizeWaysLowZoom=database->GetOptimizeWaysLowZoom();

    if (!optimizeWaysLowZoom) {
      if (!prefill) {
        tile->GetOptimizedWayData().SetComplete();
        NotifyTileStateCallbacks(tile);
      }
      return false;
    }

    if (!optimizeWaysLowZoom->HasOptimizations(magnification.GetMagnification())) {
      tile->GetOptimizedWayData().SetComplete();
      NotifyTileStateCallbacks(tile);
      return true;
    }

    if (tile->GetOptimizedWayData().IsComplete()) {
      return true;
    }

    if (parameter.IsAborted()) {
      return false;
    }

    TypeInfoSet cachedWayTypes(tile->GetOptimizedWayData().GetTypes());
    TypeInfoSet requestedWayTypes(wayTypes);
    TypeInfoSet loadedWayTypes;

    if (!cachedWayTypes.Empty()) {
      requestedWayTypes.Remove(cachedWayTypes);
    }

    if (!requestedWayTypes.Empty()) {
      std::vector<WayRef> ways;

      if (!optimizeWaysLowZoom->GetWays(boundingBox,
                                        magnification,
                                        requestedWayTypes,
                                        ways,
                                        loadedWayTypes)) {
        log.Error() << "Error getting ways from optimized ways index!";
        return false;
      }

      if (parameter.IsAborted()) {
        return false;
      }

      if (prefill) {
        tile->GetOptimizedWayData().AddPrefillData(loadedWayTypes,std::move(ways));
      }
      else {
        if (cachedWayTypes.Empty()){
          tile->GetOptimizedWayData().SetData(loadedWayTypes,std::move(ways));
        }else{
          tile->GetOptimizedWayData().AddData(loadedWayTypes,ways);
        }
      }
    }

    if (!prefill) {
      tile->GetOptimizedWayData().SetComplete();
    }

    NotifyTileStateCallbacks(tile);

    return !parameter.IsAborted();
  }

  bool MapService::GetWays(const AreaSearchParameter& parameter,
                           const TypeInfoSet& wayTypes,
                           const GeoBox& boundingBox,
                           bool prefill,
                           const TileRef& tile) const
  {
    using namespace std::string_view_literals;
    return GetObjects(parameter,
                      wayTypes,
                      boundingBox,
                      prefill,
                      tile,
                      tile->GetWayData(),
                      database->GetAreaWayIndex(),
                      [&db=this->database](const std::vector<FileOffset>& offsets, std::vector<WayRef>& ways){
                        return db->GetWaysByOffset(offsets, ways);
                      },
                      "way"sv, "ways"sv);
  }

  bool MapService::GetRoutes(const AreaSearchParameter& parameter,
                             const TypeInfoSet& routeTypes,
                             const GeoBox& boundingBox,
                             bool prefill,
                             const TileRef& tile) const
  {
    using namespace std::string_view_literals;
    return GetObjects(parameter,
                      routeTypes,
                      boundingBox,
                      prefill,
                      tile,
                      tile->GetRouteData(),
                      database->GetAreaRouteIndex(),
                      [&db=this->database, &parameter](const std::vector<FileOffset>& offsets, std::vector<RouteRef>& routes){

                        if (!db->GetRoutesByOffset(offsets, routes)){
                          return false;
                        }
                        if (parameter.GetResolveRouteMembers()){
                          for (RouteRef &route:routes){
                            if (!route->HasResolvedMembers()) {
                              std::unordered_map<FileOffset,WayRef> map;
                              if (!db->GetWaysByOffset(route->GetMemberOffsets(), map)){
                                return false;
                              }
                              route->SetResolvedMembers(map);
                            }
                          }
                        }
                        return true;
                      },
                      "route"sv, "routes"sv);
  }

  void MapService::NodeWorkerLoop()
  {
    SetThreadName("NodeLoader");

    while (auto task=nodeWorkerQueue.PopTask()) {
      task.value()();
    }
  }

  void MapService::WayWorkerLoop()
  {
    SetThreadName("WayLoader");

    while (auto task=wayWorkerQueue.PopTask()) {
      task.value()();
    }
  }

  void MapService::WayLowZoomWorkerLoop()
  {
    SetThreadName("WayLowZoomLoader");

    while (auto task=wayLowZoomWorkerQueue.PopTask()) {
      task.value()();
    }
  }

  void MapService::AreaWorkerLoop()
  {
    SetThreadName("AreaLoader");

    while (auto task=areaWorkerQueue.PopTask()) {
      task.value()();
    }
  }

  void MapService::AreaLowZoomWorkerLoop()
  {
    SetThreadName("AreaLowZoomLoader");

    while (auto task=areaLowZoomWorkerQueue.PopTask()) {
      task.value()();
    }
  }

  void MapService::RouteWorkerLoop()
  {
    SetThreadName("RouteLoader");

    while (auto task=routeWorkerQueue.PopTask()) {
      task.value()();
    }
  }

  std::future<bool> MapService::PushNodeTask(const AreaSearchParameter& parameter,
                                             const TypeInfoSet& nodeTypes,
                                             const GeoBox& boundingBox,
                                             bool prefill,
                                             const TileRef& tile) const
  {
    std::packaged_task<bool()> task(std::bind(&MapService::GetNodes,this,
                                              parameter,
                                              nodeTypes,
                                              boundingBox,
                                              prefill,
                                              tile));

    std::future<bool> future=task.get_future();

    nodeWorkerQueue.PushTask(std::move(task));

    return future;
  }

  std::future<bool> MapService::PushAreaLowZoomTask(const AreaSearchParameter& parameter,
                                                    const TypeInfoSet& areaTypes,
                                                    const Magnification& magnification,
                                                    const GeoBox& boundingBox,
                                                    bool prefill,
                                                    const TileRef& tile) const
  {
    std::packaged_task<bool()> task(std::bind(&MapService::GetAreasLowZoom,this,
                                              parameter,
                                              areaTypes,
                                              magnification,
                                              boundingBox,
                                              prefill,
                                              tile));

    std::future<bool> future=task.get_future();

    areaLowZoomWorkerQueue.PushTask(std::move(task));

    return future;
  }

  std::future<bool> MapService::PushAreaTask(const AreaSearchParameter& parameter,
                                             const TypeInfoSet& areaTypes,
                                             const Magnification& magnification,
                                             const GeoBox& boundingBox,
                                             bool prefill,
                                             const TileRef& tile) const
  {
    std::packaged_task<bool()> task(std::bind(&MapService::GetAreas,this,
                                              parameter,
                                              areaTypes,
                                              magnification,
                                              boundingBox,
                                              prefill,
                                              tile));

    std::future<bool> future=task.get_future();

    areaWorkerQueue.PushTask(std::move(task));

    return future;
  }

  std::future<bool> MapService::PushWayLowZoomTask(const AreaSearchParameter& parameter,
                                                   const TypeInfoSet& wayTypes,
                                                   const Magnification& magnification,
                                                   const GeoBox& boundingBox,
                                                   bool prefill,
                                                   const TileRef& tile) const
  {
    std::packaged_task<bool()> task(std::bind(&MapService::GetWaysLowZoom,this,
                                              parameter,
                                              wayTypes,
                                              magnification,
                                              boundingBox,
                                              prefill,
                                              tile));

    std::future<bool> future=task.get_future();

    wayLowZoomWorkerQueue.PushTask(std::move(task));

    return future;
  }

  std::future<bool> MapService::PushWayTask(const AreaSearchParameter& parameter,
                                            const TypeInfoSet& wayTypes,
                                            const GeoBox& boundingBox,
                                            bool prefill,
                                            const TileRef& tile) const
  {
    std::packaged_task<bool()> task(std::bind(&MapService::GetWays,this,
                                              parameter,
                                              wayTypes,
                                              boundingBox,
                                              prefill,
                                              tile));

    std::future<bool> future=task.get_future();

    wayWorkerQueue.PushTask(std::move(task));

    return future;
  }

  std::future<bool> MapService::PushRouteTask(const AreaSearchParameter& parameter,
                                              const TypeInfoSet& routeTypes,
                                              const GeoBox& boundingBox,
                                              bool prefill,
                                              const TileRef& tile) const
  {
    std::packaged_task<bool()> task(std::bind(&MapService::GetRoutes,this,
                                              parameter,
                                              routeTypes,
                                              boundingBox,
                                              prefill,
                                              tile));

    std::future<bool> future=task.get_future();

    routeWorkerQueue.PushTask(std::move(task));

    return future;
  }

  void MapService::NotifyTileStateCallbacks(const TileRef& tile) const
  {
    std::lock_guard<std::mutex> lock(callbackMutex);

    for (auto& callbackEntry : tileStateCallbacks) {
      callbackEntry.second(tile);
    }
  }

  /**
   * Return all tiles with the magnification defined by the projection
   * that cover the region covered by the projection
   *
   * Note, that tiles may be partially prefill or empty, if not already
   * cached.
   */
  void MapService::LookupTiles(const Projection& projection,
                               std::list<TileRef>& tiles) const
  {
    std::lock_guard<std::mutex> lock(stateMutex);

    StopClock cacheRetrievalTime;

    GeoBox boundingBox(projection.GetDimensions());

    cache.GetTilesForBoundingBox(projection.GetMagnification(),
                                 boundingBox,
                                 tiles);

    cacheRetrievalTime.Stop();

    //std::cout << "Cache retrieval time: " << cacheRetrievalTime.ResultString() << std::endl;
  }

  /**
   * Return all tiles with the given covering the region given by the boundingBox
   *
   * Note, that tiles may be partially prefill or empty, if not already
   * cached.
   */
  void MapService::LookupTiles(const Magnification& magnification,
                               const GeoBox& boundingBox,
                               std::list<TileRef>& tiles) const
  {
    std::lock_guard<std::mutex> lock(stateMutex);

    StopClock cacheRetrievalTime;

    cache.GetTilesForBoundingBox(magnification,
                                 boundingBox,
                                 tiles);

    cacheRetrievalTime.Stop();

    //std::cout << "Cache retrieval time: " << cacheRetrievalTime.ResultString() << std::endl;
  }

  /**
   * Return the given tile.
   *
   * Note, that tiles may be partially prefilled or empty, if not already
   * cached.
   */
  TileRef MapService::LookupTile(const TileKey& key) const
  {
    std::lock_guard<std::mutex> lock(stateMutex);

    StopClock cacheRetrievalTime;

    TileRef tile=cache.GetTile(key);

    cacheRetrievalTime.Stop();

    //std::cout << "Cache retrieval time: " << cacheRetrievalTime.ResultString() << std::endl;

    return tile;
  }

  /**
   * Load all missing data for the given tiles based on the given style config.
   */
  bool MapService::LoadMissingTileDataStyleSheet(const AreaSearchParameter& parameter,
                                                 const StyleConfig& styleConfig,
                                                 std::list<TileRef>& tiles,
                                                 bool async) const
  {
    std::lock_guard<std::mutex>  lock(stateMutex);

    StopClock                    overallTime;

    TypeDefinitionRef            typeDefinition;
    Magnification                typeDefinitionMagnification;

    std::list<std::future<bool>> results;

    for (auto& tile : tiles) {
      if (!tile->IsComplete()) {
        GeoBox        tileBoundingBox(tile->GetBoundingBox());
        StopClock     tileLoadingTime;
        Magnification magnification(MagnificationLevel(tile->GetKey().GetLevel()));

        //std::cout << "Loading tile: " << tile->GetId().DisplayText() << std::endl;

        // TODO: Cache the type definitions, perhaps already in the StyleConfig?

        if (!typeDefinition ||
            typeDefinitionMagnification!=magnification) {
          typeDefinition=GetTypeDefinition(parameter,
                                           styleConfig,
                                           magnification);
          typeDefinitionMagnification=magnification;
        }

        cache.PrefillDataFromCache(*tile,
                                   typeDefinition->nodeTypes,
                                   typeDefinition->wayTypes,
                                   typeDefinition->areaTypes,
                                   typeDefinition->routeTypes,
                                   typeDefinition->optimizedWayTypes,
                                   typeDefinition->optimizedAreaTypes);

        NotifyTileStateCallbacks(tile);

        results.push_back(PushNodeTask(parameter,
                                       typeDefinition->nodeTypes,
                                       tileBoundingBox,
                                       false,
                                       tile));

        if (parameter.GetUseLowZoomOptimization()) {
          results.push_back(PushAreaLowZoomTask(parameter,
                                                typeDefinition->optimizedAreaTypes,
                                                magnification,
                                                tileBoundingBox,
                                                false,
                                                tile));
        } else {
          tile->GetOptimizedAreaData().SetComplete();
        }

        results.push_back(PushAreaTask(parameter,
                                       typeDefinition->areaTypes,
                                       magnification,
                                       tileBoundingBox,
                                       false,
                                       tile));

        if (parameter.GetUseLowZoomOptimization()) {
          results.push_back(PushWayLowZoomTask(parameter,
                                               typeDefinition->optimizedWayTypes,
                                               magnification,
                                               tileBoundingBox,
                                               false,
                                               tile));
        } else {
          tile->GetOptimizedWayData().SetComplete();
        }

        results.push_back(PushWayTask(parameter,
                                      typeDefinition->wayTypes,
                                      tileBoundingBox,
                                      false,
                                      tile));

        results.push_back(PushRouteTask(parameter,
                                        typeDefinition->routeTypes,
                                        tileBoundingBox,
                                        false,
                                        tile));

        tileLoadingTime.Stop();

        //std::cout << "Tile loading time: " << tileLoadingTime.ResultString() << std::endl;

        if (tileLoadingTime.GetMilliseconds()>150) {
          log.Warn() << "Retrieving tile data for tile " << tile->GetKey().GetDisplayText() << " took " << tileLoadingTime.ResultString();
        }

      }
      else {
        //std::cout << "Using cached tile: " << tile->GetId().DisplayText() << std::endl;
      }
    }

    bool success=true;

    if (async) {
      results.clear();
    }
    else {
      for (auto& result : results) {
        if (!result.get()) {
          success=false;
        }
      }
    }

    overallTime.Stop();

    if (overallTime.GetMilliseconds()>200) {
      log.Warn() << "Retrieving all tile data took " << overallTime.ResultString();
    }

    cache.CleanupCache();

    return success;
  }

  bool MapService::LoadMissingTileDataTypeDefinition(const AreaSearchParameter& parameter,
                                                     const Magnification& magnification,
                                                     const TypeDefinition& typeDefinition,
                                                     std::list<TileRef>& tiles,
                                                     bool async) const
  {
    std::lock_guard<std::mutex>  lock(stateMutex);

    StopClock                    overallTime;

    std::list<std::future<bool>> results;

    for (auto& tile : tiles) {
      if (!tile->IsComplete()) {
        GeoBox tileBoundingBox(tile->GetBoundingBox());
        StopClock  tileLoadingTime;

        //std::cout << "Loading tile: " << (std::string)tile->GetId() << std::endl;

        cache.PrefillDataFromCache(*tile,
                                   typeDefinition.nodeTypes,
                                   typeDefinition.wayTypes,
                                   typeDefinition.areaTypes,
                                   typeDefinition.routeTypes,
                                   typeDefinition.optimizedWayTypes,
                                   typeDefinition.optimizedAreaTypes);

        NotifyTileStateCallbacks(tile);

        results.push_back(PushNodeTask(parameter,
                                       typeDefinition.nodeTypes,
                                       tileBoundingBox,
                                       true,
                                       tile));

        if (parameter.GetUseLowZoomOptimization()) {
          results.push_back(PushAreaLowZoomTask(parameter,
                                                typeDefinition.optimizedAreaTypes,
                                                magnification,
                                                tileBoundingBox,
                                                true,
                                                tile));
        }

        results.push_back(PushAreaTask(parameter,
                                       typeDefinition.areaTypes,
                                       magnification,
                                       tileBoundingBox,
                                       true,
                                       tile));

        if (parameter.GetUseLowZoomOptimization()) {
          results.push_back(PushWayLowZoomTask(parameter,
                                               typeDefinition.optimizedWayTypes,
                                               magnification,
                                               tileBoundingBox,
                                               true,
                                               tile));
        }

        results.push_back(PushWayTask(parameter,
                                      typeDefinition.wayTypes,
                                      tileBoundingBox,
                                      true,
                                      tile));

        results.push_back(PushRouteTask(parameter,
                                      typeDefinition.routeTypes,
                                      tileBoundingBox,
                                      true,
                                      tile));

        tileLoadingTime.Stop();

        //std::cout << "Tile loading time: " << tileLoadingTime.ResultString() << std::endl;

        if (tileLoadingTime.GetMilliseconds()>150) {
          log.Warn() << "Retrieving tile data for tile " << tile->GetKey().GetDisplayText() << " took " << tileLoadingTime.ResultString();
        }

      }
      else {
        //std::cout << "Using cached tile: " << (std::string)tile->GetId() << std::endl;
      }
    }

    bool success=true;

    if (async) {
      results.clear();
    }
    else {
      for (auto& result : results) {
        if (!result.get()) {
          success=false;
        }
      }
    }

    overallTime.Stop();

    if (overallTime.GetMilliseconds()>200) {
      log.Warn() << "Retrieving all tile data took " << overallTime.ResultString();
    }

    cache.CleanupCache();

    return success;
  }

  /**
   * Load all missing data for the given tiles based on the given style config. The
   * method returns, either after an error occurred or all tiles have been successfully
   * loaded.
   */
  bool MapService::LoadMissingTileData(const AreaSearchParameter& parameter,
                                       const StyleConfig& styleConfig,
                                       std::list<TileRef>& tiles) const
  {
    return LoadMissingTileDataStyleSheet(parameter,styleConfig,tiles,false);
  }

  /**
   * Load all missing data for the given tiles based on the given style config. This method
   * just triggers the loading but may return before all data has been loaded. Loading of tile
   * data happens in the background. You have to register a callback to get notified
   * about tile loading state.Dr
   *
   * You can be sure, that callbacks are not called in the context of the calling thread.
   */
  bool MapService::LoadMissingTileDataAsync(const AreaSearchParameter& parameter,
                                            const StyleConfig& styleConfig,
                                            std::list<TileRef>& tiles) const
  {
    auto result=std::async(std::launch::async,
                           &MapService::LoadMissingTileDataStyleSheet,this,
                           std::ref(parameter),
                           std::ref(styleConfig),
                           std::ref(tiles),
                           true);

    return result.get();
  }

  bool MapService::LoadMissingTileData(const AreaSearchParameter& parameter,
                                       const Magnification& magnification,
                                       const TypeDefinition& typeDefinition,
                                       std::list<TileRef>& tiles) const
  {
    return LoadMissingTileDataTypeDefinition(parameter,
                                             magnification,
                                             typeDefinition,
                                             tiles,
                                             false);
  }

  bool MapService::LoadMissingTileDataAsync(const AreaSearchParameter& parameter,
                                            const Magnification& magnification,
                                            const TypeDefinition& typeDefinition,
                                            std::list<TileRef>& tiles) const
  {
    auto result=std::async(std::launch::async,
                           &MapService::LoadMissingTileDataTypeDefinition,this,
                           std::ref(parameter),
                           std::ref(magnification),
                           std::ref(typeDefinition),
                           std::ref(tiles),
                           true);

    return result.get();
  }

  /**
   * Convert the data hold by the given tiles to the given MapData class instance.
   *
   * Every object of the tiles is placed in the result exactly once. The objects are grouped by the
   * data file they were read from in a fixed order - the objects of `nodes.dat`, `ways.dat`,
   * `areas.dat` and `routes.dat` first, then the objects of the optimized data files of a kind - and
   * within a source data file they follow the order in which the tiles present them. An offset only
   * identifies an object within one data file, so two data files that carry the same offset value
   * both contribute their object.
   *
   * The cost of the conversion follows the objects of the tiles: every stored object is examined
   * once, on a first sight it is appended, and a repeated sight costs one lookup in the set of the
   * offsets already seen, so a tile that repeats an object the view already holds does not make the
   * result grow.
   */
  void MapService::AddTileDataToMapData(std::list<TileRef>& tiles,
                                        MapData& data) const
  {
    ConversionTiming timing;

    size_t nodeCapacity=SumDataSize<NodeRef>(tiles,GetNodeData);
    size_t wayCapacity=SumDataSize<WayRef>(tiles,GetWayData);
    size_t optimizedWayCapacity=SumDataSize<WayRef>(tiles,GetOptimizedWayData);
    size_t areaCapacity=SumDataSize<AreaRef>(tiles,GetAreaData);
    size_t optimizedAreaCapacity=SumDataSize<AreaRef>(tiles,GetOptimizedAreaData);
    size_t routeCapacity=SumDataSize<RouteRef>(tiles,GetRouteData);

    data.nodes.reserve(data.nodes.size()+nodeCapacity);
    data.ways.reserve(data.ways.size()+wayCapacity+optimizedWayCapacity);
    data.areas.reserve(data.areas.size()+areaCapacity+optimizedAreaCapacity);
    data.routes.reserve(data.routes.size()+routeCapacity);

    ConvertSource<NodeRef>(tiles,GetNodeData,nullptr,"nodes",data.nodes,timing);
    ConvertSource<WayRef>(tiles,GetWayData,nullptr,"ways",data.ways,timing);
    ConvertSource<WayRef>(tiles,GetOptimizedWayData,nullptr,"optimized ways",data.ways,timing);
    ConvertSource<AreaRef>(tiles,GetAreaData,nullptr,"areas",data.areas,timing);
    ConvertSource<AreaRef>(tiles,GetOptimizedAreaData,nullptr,"optimized areas",data.areas,timing);
    ConvertSource<RouteRef>(tiles,GetRouteData,nullptr,"routes",data.routes,timing);

    ReportSlowConversionPhases(timing,conversionPhaseWarningThreshold.load());
  }

  /**
   * Convert the data hold by the given tiles to the given MapData class instance, restricted to the
   * object types of the given type definition.
   *
   * The restricted conversion holds the same uniqueness and order guarantees as the unrestricted one.
   * It reserves its result from the objects the tiles hold rather than from the objects that match the
   * type filter, because the matching count is only known once the objects have been visited.
   */
  void MapService::AddTileDataToMapData(std::list<TileRef>& tiles,
                                        const TypeDefinition& typeDefinition,
                                        MapData& data) const
  {
    ConversionTiming timing;

    size_t nodeCapacity=SumDataSize<NodeRef>(tiles,GetNodeData);
    size_t wayCapacity=SumDataSize<WayRef>(tiles,GetWayData);
    size_t optimizedWayCapacity=SumDataSize<WayRef>(tiles,GetOptimizedWayData);
    size_t areaCapacity=SumDataSize<AreaRef>(tiles,GetAreaData);
    size_t optimizedAreaCapacity=SumDataSize<AreaRef>(tiles,GetOptimizedAreaData);

    data.nodes.reserve(data.nodes.size()+nodeCapacity);
    data.ways.reserve(data.ways.size()+wayCapacity+optimizedWayCapacity);
    data.areas.reserve(data.areas.size()+areaCapacity+optimizedAreaCapacity);

    ConvertSource<NodeRef>(tiles,GetNodeData,&typeDefinition.nodeTypes,"nodes",data.nodes,timing);
    ConvertSource<WayRef>(tiles,GetWayData,&typeDefinition.wayTypes,"ways",data.ways,timing);
    ConvertSource<WayRef>(tiles,GetOptimizedWayData,&typeDefinition.optimizedWayTypes,"optimized ways",data.ways,timing);
    ConvertSource<AreaRef>(tiles,GetAreaData,&typeDefinition.areaTypes,"areas",data.areas,timing);
    ConvertSource<AreaRef>(tiles,GetOptimizedAreaData,&typeDefinition.optimizedAreaTypes,"optimized areas",data.areas,timing);

    ReportSlowConversionPhases(timing,conversionPhaseWarningThreshold.load());
  }

  /**
   * Return all ground tiles for the given projection data
   * (bounding box and magnification).
   *
   * \note The returned ground tiles may result in a bigger area than given.
   *
   * @param projection
   *    projection defining bounding box and magnification
   * @param tiles
   *    List of returned tiles
   * @return
   *    False, if there was an error, else true.
   */
  bool MapService::GetGroundTiles(const Projection& projection,
                                  std::list<GroundTile>& tiles) const
  {
    GeoBox boundingBox(projection.GetDimensions());

    return GetGroundTiles(boundingBox,
                          projection.GetMagnification(),
                          tiles);
  }

  /**
   * Return all ground tiles for the given area and the given magnification.
   *
   * \note The returned ground tiles may result in a bigger area than given.
   *
   * @param boundingBox
   *    Boundary coordinates
   * @param magnification
   *    Magnification
   * @param tiles
   *    List of returned tiles
   * @return
   *    False, if there was an error, else true.
   */
  bool MapService::GetGroundTiles(const GeoBox& boundingBox,
                                  const Magnification& magnification,
                                  std::list<GroundTile>& tiles) const
  {
    WaterIndexRef waterIndex=database->GetWaterIndex();

    if (!waterIndex) {
      return false;
    }

    StopClock timer;

    if (!waterIndex->GetRegions(boundingBox,
                                magnification,
                                tiles)) {
      log.Error() << "Error reading ground tiles in area!";
      return false;
    }

    timer.Stop();

    //std::cout << "Loading ground tiles took: " << timer.ResultString() << std::endl;

    return true;
  }

  SRTMDataRef MapService::GetSRTMData(const Projection& projection) const
  {
    return GetSRTMData(projection.GetDimensions());
  }

  SRTMDataRef MapService::GetSRTMData(const GeoBox& boundingBox) const
  {
    SRTMRef srtmIndex=database->GetSRTMIndex();

    if (!srtmIndex) {
      return nullptr;
    }

    StopClock timer;

    SRTMDataRef tile=srtmIndex->GetHeightInBoundingBox(boundingBox);

    if (!tile) {
      log.Error() << "Error reading SRTM data!";
      return nullptr;
    }

    timer.Stop();

    //std::cout << "Loading ground tiles took: " << timer.ResultString() << std::endl;

    return tile;
  }

  MapService::CallbackId MapService::RegisterTileStateCallback(TileStateCallback callback)
  {
    std::lock_guard<std::mutex> lock(callbackMutex);

    CallbackId id=nextCallbackId++;

    tileStateCallbacks.insert(std::make_pair(id,callback));

    return id;
  }

  void MapService::DeregisterTileStateCallback(CallbackId callbackId)
  {
    std::lock_guard<std::mutex> lock(callbackMutex);

    tileStateCallbacks.erase(callbackId);
  }
}
