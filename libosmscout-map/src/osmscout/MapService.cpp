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

#include <osmscout/MapService.h>

#include <algorithm>
#include <future>

#if _OPENMP
#include <omp.h>
#endif

#include <osmscout/system/Assert.h>
#include <osmscout/system/Math.h>

#include <osmscout/util/Geometry.h>
#include <osmscout/util/Logger.h>

namespace osmscout {

  AreaSearchParameter::AreaSearchParameter()
  : maxAreaLevel(4),
    useLowZoomOptimization(true),
    useMultithreading(false)
  {
    // no code
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
     cache(25)
  {
    // no code
  }

  MapService::~MapService()
  {
    // no code
  }

  /**
   * Set the size of the tile data cache
   */
  void MapService::SetCacheSize(size_t cacheSize)
  {
    cache.SetSize(cacheSize);
  }

  MapService::TypeDefinitionRef MapService::GetTypeDefinition(const AreaSearchParameter& parameter,
                                                              const StyleConfig& styleConfig,
                                                              const Magnification& magnification) const
  {
    // TODO: Make sure that the styleConfig has not changed!

    if (typeDefinition &&
        typeDefinition->magnification==magnification) {
      return typeDefinition;
    }

    OptimizeAreasLowZoomRef optimizeAreasLowZoom=database->GetOptimizeAreasLowZoom();
    OptimizeWaysLowZoomRef  optimizeWaysLowZoom=database->GetOptimizeWaysLowZoom();

    if (!optimizeAreasLowZoom ||
        !optimizeWaysLowZoom) {
      return NULL;
    }

    TypeDefinitionRef typeDefinition=std::make_shared<TypeDefinition>();

    styleConfig.GetNodeTypesWithMaxMag(magnification,
                                       typeDefinition->nodeTypes);

    styleConfig.GetWayTypesWithMaxMag(magnification,
                                       typeDefinition->wayTypes);

    styleConfig.GetAreaTypesWithMaxMag(magnification,
                                       typeDefinition->areaTypes);

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
                            TileRef tile) const
  {
    AreaNodeIndexRef areaNodeIndex=database->GetAreaNodeIndex();

    if (!areaNodeIndex) {
      return false;
    }

    if (parameter.IsAborted()) {
      return false;
    }

    TypeInfoSet             cachedNodeTypes(tile->GetNodeData().GetTypes());
    TypeInfoSet             requestedNodeTypes(nodeTypes);
    TypeInfoSet             loadedNodeTypes;
    std::vector<FileOffset> offsets;

    if (!cachedNodeTypes.Empty()) {
      requestedNodeTypes.Remove(tile->GetNodeData().GetTypes());
    }

    if (!requestedNodeTypes.Empty()) {
      if (!areaNodeIndex->GetOffsets(boundingBox,
                                     requestedNodeTypes,
                                     offsets,
                                     loadedNodeTypes)) {
        log.Error() << "Error getting nodes from area node index!";
        return false;
      }
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

      if (!database->GetNodesByOffset(offsets,
                                      tile->GetNodeData().GetData())) {
        log.Error() << "Error reading nodes in area!";
        return false;
      }
    }

    loadedNodeTypes.Add(cachedNodeTypes);

    tile->GetNodeData().SetData(loadedNodeTypes,tile->GetNodeData().GetData());

    return !parameter.IsAborted();
  }

  bool MapService::GetAreasLowZoom(const AreaSearchParameter& parameter,
                                   const TypeInfoSet& areaTypes,
                                   const Magnification& magnification,
                                   const GeoBox& boundingBox,
                                   TileRef tile) const
  {
    OptimizeAreasLowZoomRef optimizeAreasLowZoom=database->GetOptimizeAreasLowZoom();

    if (!optimizeAreasLowZoom) {
      return false;
    }

    if (!optimizeAreasLowZoom->HasOptimizations(magnification.GetMagnification())) {
      return true;
    }

    if (parameter.IsAborted()) {
      return false;
    }

    TypeInfoSet cachedAreaTypes(tile->GetOptimizedAreaData().GetTypes());
    TypeInfoSet requestedAreaTypes(areaTypes);
    TypeInfoSet loadedAreaTypes;

    if (!cachedAreaTypes.Empty()) {
      requestedAreaTypes.Remove(tile->GetOptimizedAreaData().GetTypes());
    }

    if (!requestedAreaTypes.Empty()) {
      if (!optimizeAreasLowZoom->GetAreas(boundingBox,
                                          magnification,
                                          requestedAreaTypes,
                                          tile->GetOptimizedAreaData().GetData(),
                                          loadedAreaTypes)) {
        log.Error() << "Error getting areas from optimized areas index!";
        return false;
      }
    }

    loadedAreaTypes.Add(cachedAreaTypes);
    tile->GetOptimizedAreaData().SetData(loadedAreaTypes,tile->GetOptimizedAreaData().GetData());

    return !parameter.IsAborted();
  }

  bool MapService::GetAreas(const AreaSearchParameter& parameter,
                            const TypeInfoSet& areaTypes,
                            const Magnification& magnification,
                            const GeoBox& boundingBox,
                            TileRef tile) const
  {
    AreaAreaIndexRef areaAreaIndex=database->GetAreaAreaIndex();

    if (!areaAreaIndex) {
      return false;
    }

    if (parameter.IsAborted()) {
      return false;
    }

    TypeInfoSet                cachedAreaTypes(tile->GetAreaData().GetTypes());
    TypeInfoSet                requestedAreaTypes(areaTypes);
    TypeInfoSet                loadedAreaTypes;
    std::vector<DataBlockSpan> spans;

    if (!cachedAreaTypes.Empty()) {
      requestedAreaTypes.Remove(tile->GetAreaData().GetTypes());
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
    }

    if (parameter.IsAborted()) {
      return false;
    }

    if (!spans.empty()) {
      // Sort spans before loading to optimize disk access
      std::sort(spans.begin(),spans.end());

      if (!database->GetAreasByBlockSpans(spans,
                                          tile->GetAreaData().GetData())) {
        log.Error() << "Error reading areas in area!";
        return false;
      }
    }

    loadedAreaTypes.Add(cachedAreaTypes);

    tile->GetAreaData().SetData(loadedAreaTypes,tile->GetAreaData().GetData());

    return !parameter.IsAborted();
  }

  bool MapService::GetWaysLowZoom(const AreaSearchParameter& parameter,
                                  const TypeInfoSet& wayTypes,
                                  const Magnification& magnification,
                                  const GeoBox& boundingBox,
                                  TileRef tile) const
  {
    OptimizeWaysLowZoomRef optimizeWaysLowZoom=database->GetOptimizeWaysLowZoom();

    if (!optimizeWaysLowZoom) {
      return false;
    }

    if (!optimizeWaysLowZoom->HasOptimizations(magnification.GetMagnification())) {
      return true;
    }

    if (parameter.IsAborted()) {
      return false;
    }

    TypeInfoSet cachedWayTypes(tile->GetOptimizedWayData().GetTypes());
    TypeInfoSet requestedWayTypes(wayTypes);
    TypeInfoSet loadedWayTypes;

    if (!cachedWayTypes.Empty()) {
      requestedWayTypes.Remove(tile->GetOptimizedWayData().GetTypes());
    }

    if (!requestedWayTypes.Empty()) {
      if (!optimizeWaysLowZoom->GetWays(boundingBox,
                                        magnification,
                                        requestedWayTypes,
                                        tile->GetOptimizedWayData().GetData(),
                                        loadedWayTypes)) {
        log.Error() << "Error getting ways from optimized ways index!";
        return false;
      }
    }

    loadedWayTypes.Add(cachedWayTypes);
    tile->GetOptimizedWayData().SetData(loadedWayTypes,tile->GetOptimizedWayData().GetData());

    return !parameter.IsAborted();
  }

  bool MapService::GetWays(const AreaSearchParameter& parameter,
                           const TypeInfoSet& wayTypes,
                           const GeoBox& boundingBox,
                           TileRef tile) const
  {
    AreaWayIndexRef areaWayIndex=database->GetAreaWayIndex();

    if (!areaWayIndex) {
      return false;
    }

    if (parameter.IsAborted()) {
      return false;
    }

    TypeInfoSet             cachedWayTypes(tile->GetWayData().GetTypes());
    TypeInfoSet             requestedWayTypes(wayTypes);
    TypeInfoSet             loadedWayTypes;
    std::vector<FileOffset> offsets;

    if (!cachedWayTypes.Empty()) {
      requestedWayTypes.Remove(tile->GetWayData().GetTypes());
    }

    if (!requestedWayTypes.Empty()) {
      if (!areaWayIndex->GetOffsets(boundingBox,
                                    requestedWayTypes,
                                    offsets,
                                    loadedWayTypes)) {
        log.Error() << "Error getting ways from area way index!";
        return false;
      }
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

      if (!database->GetWaysByOffset(offsets,
                                     tile->GetWayData().GetData())) {
        log.Error() << "Error reading ways in area!";
        return false;
      }
    }

    loadedWayTypes.Add(cachedWayTypes);

    tile->GetWayData().SetData(loadedWayTypes,tile->GetWayData().GetData());

    return !parameter.IsAborted();
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
    StopClock cacheRetrievalTime;

    GeoBox boundingBox;

    projection.GetDimensions(boundingBox);

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
   * Note, that tiles may be partially prefill or empty, if not already
   * cached.
   */
  TileRef MapService::LookupTile(const TileId& id) const
  {
    StopClock cacheRetrievalTime;

    TileRef tile=cache.GetTile(id);

    cacheRetrievalTime.Stop();

    //std::cout << "Cache retrieval time: " << cacheRetrievalTime.ResultString() << std::endl;

    return tile;
  }

  /**
   * Load all missing data for the given tiles based on the given style config.
   */
  bool MapService::LoadMissingTileData(const AreaSearchParameter& parameter,
                                       const StyleConfig& styleConfig,
                                       std::list<TileRef>& tiles) const
  {
    StopClock         overallTime;

    TypeDefinitionRef typeDefinition;

    StopClock         dataLoadingTime;

    for (auto& tile : tiles) {
      GeoBox          tileBoundingBox(tile->GetBoundingBox());

      if (tile->IsEmpty()) {
        StopClock         tileLoadingTime;
        Magnification     magnification;
        std::future<bool> nodeResult;
        std::future<bool> areasLowZoomResult;
        std::future<bool> areasResult;
        std::future<bool> waysLowZoomResult;
        std::future<bool> waysResult;

        //std::cout << "Loading tile: " << (std::string)tile->GetId() << std::endl;

        magnification.SetLevel(tile->GetId().GetLevel());

        // TODO: Cache the type definitions, perhaps already in the StyleConfig?
        typeDefinition=GetTypeDefinition(parameter,
                                         styleConfig,
                                         magnification);
        cache.PrefillDataFromCache(*tile,
                                   typeDefinition->nodeTypes,
                                   typeDefinition->wayTypes,
                                   typeDefinition->areaTypes,
                                   typeDefinition->optimizedWayTypes,
                                   typeDefinition->optimizedAreaTypes);

        nodeResult=std::async(std::launch::async,
                              &MapService::GetNodes,this,
                              parameter,
                              typeDefinition->nodeTypes,
                              tileBoundingBox,
                              tile);

        if (parameter.GetUseLowZoomOptimization()) {
          areasLowZoomResult=std::async(std::launch::async,
                                        &MapService::GetAreasLowZoom,this,
                                        parameter,
                                        typeDefinition->optimizedAreaTypes,
                                        magnification,
                                        tileBoundingBox,
                                        tile);
        }

        areasResult=std::async(std::launch::async,
                               &MapService::GetAreas,this,
                               parameter,
                               typeDefinition->areaTypes,
                               magnification,
                               tileBoundingBox,
                               tile);

        if (parameter.GetUseLowZoomOptimization()) {
          waysLowZoomResult=std::async(std::launch::async,
                                       &MapService::GetWaysLowZoom,this,
                                       parameter,
                                       typeDefinition->optimizedWayTypes,
                                       magnification,
                                       tileBoundingBox,
                                       tile);
        }

        waysResult=std::async(std::launch::async,
                              &MapService::GetWays,this,
                              parameter,
                              typeDefinition->wayTypes,
                              tileBoundingBox,
                              tile);

        if (!nodeResult.get() ||
            (areasLowZoomResult.valid() && !areasLowZoomResult.get()) ||
            !areasResult.get() ||
            (waysLowZoomResult.valid() && !waysLowZoomResult.get()) ||
            !waysResult.get()) {
          return false;
        }

        tileLoadingTime.Stop();

        //std::cout << "Tile loading time: " << tileLoadingTime.ResultString() << std::endl;

        if (tileLoadingTime.GetMilliseconds()>150) {
          log.Warn() << "Retrieving tile data for tile " << tile->GetId().DisplayText() << " took " << tileLoadingTime.ResultString();
        }

      }
      else {
        //std::cout << "Using cached tile: " << (std::string)tile->GetId() << std::endl;
      }
    }

    dataLoadingTime.Stop();

    //std::cout << "DataLoadingTime: " << dataLoadingTime.ResultString() << std::endl;

    overallTime.Stop();

    if (overallTime.GetMilliseconds()>200) {
      log.Warn() << "Retrieving all tile data took " << overallTime.ResultString();
    }

    cache.CleanupCache();

    return true;
  }

  /**
   * Convert the data hold by the given tiles to the given MapData class instance.
   */
  void MapService::ConvertTilesToMapData(std::list<TileRef>& tiles,
                                         MapData& data) const
  {
    // TODO: Use a set and higher level fill functions
    std::unordered_map<FileOffset,NodeRef> nodeMap(10000);
    std::unordered_map<FileOffset,WayRef>  wayMap(10000);
    std::unordered_map<FileOffset,AreaRef> areaMap(10000);
    std::unordered_map<FileOffset,WayRef>  optimizedWayMap(10000);
    std::unordered_map<FileOffset,AreaRef> optimizedAreaMap(10000);

    StopClock uniqueTime;

    for (auto tile : tiles) {
      for (const auto& node : tile->GetNodeData().GetData()) {
        nodeMap[node->GetFileOffset()]=node;
      }

      for (const auto& way : tile->GetWayData().GetData()) {
        wayMap[way->GetFileOffset()]=way;
      }

      for (const auto& area : tile->GetAreaData().GetData()) {
        areaMap[area->GetFileOffset()]=area;
      }

      for (const auto& way : tile->GetOptimizedWayData().GetData()) {
        optimizedWayMap[way->GetFileOffset()]=way;
      }

      for (const auto& area : tile->GetOptimizedAreaData().GetData()) {
        optimizedAreaMap[area->GetFileOffset()]=area;
      }
    }

    uniqueTime.Stop();

    //std::cout << "Make data unique time: " << uniqueTime.ResultString() << std::endl;

    StopClock copyTime;

    data.nodes.clear();
    data.ways.clear();
    data.areas.clear();

    data.nodes.reserve(nodeMap.size());
    data.ways.reserve(wayMap.size()+optimizedWayMap.size());
    data.areas.reserve(areaMap.size()+optimizedAreaMap.size());

    for (const auto& nodeEntry : nodeMap) {
      data.nodes.push_back(nodeEntry.second);
    }

    for (const auto& wayEntry : wayMap) {
      data.ways.push_back(wayEntry.second);
    }

    for (const auto& wayEntry : optimizedWayMap) {
      data.ways.push_back(wayEntry.second);
    }

    for (const auto& areaEntry : areaMap) {
      data.areas.push_back(areaEntry.second);
    }

    for (const auto& areaEntry : optimizedAreaMap) {
      data.areas.push_back(areaEntry.second);
    }

    copyTime.Stop();

    //std::cout << "Tile data copy time: " << copyTime.ResultString() << std::endl;
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
    GeoBox boundingBox;

    projection.GetDimensions(boundingBox);

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
}
