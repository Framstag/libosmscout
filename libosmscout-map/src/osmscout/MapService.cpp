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
    maxNodes(2000),
    maxWays(10000),
    maxAreas(std::numeric_limits<unsigned long>::max()),
    useLowZoomOptimization(true),
    useMultithreading(false)
  {
    // no code
  }

  void AreaSearchParameter::SetMaximumAreaLevel(unsigned long maxAreaLevel)
  {
    this->maxAreaLevel=maxAreaLevel;
  }

  void AreaSearchParameter::SetMaximumNodes(unsigned long maxNodes)
  {
    this->maxNodes=maxNodes;
  }

  void AreaSearchParameter::SetMaximumWays(unsigned long maxWays)
  {
    this->maxWays=maxWays;
  }

  void AreaSearchParameter::SetMaximumAreas(unsigned long maxAreas)
  {
    this->maxAreas=maxAreas;
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

  unsigned long AreaSearchParameter::GetMaximumNodes() const
  {
    return maxNodes;
  }

  unsigned long AreaSearchParameter::GetMaximumWays() const
  {
    return maxWays;
  }

  unsigned long AreaSearchParameter::GetMaximumAreas() const
  {
    return maxAreas;
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
   : database(database)
  {
    // no code
  }

  MapService::~MapService()
  {
    // no code
  }

  bool MapService::GetObjectsNodes(const AreaSearchParameter& parameter,
                                   const TypeSet &requestedNodeTypes,
                                   const GeoBox& boundingBox,
                                   std::string& nodeIndexTime,
                                   std::string& nodesTime,
                                   std::vector<NodeRef>& nodes) const
  {
    AreaNodeIndexRef areaNodeIndex=database->GetAreaNodeIndex();
    TypeInfoSet      loadedNodeTypes;

    nodes.clear();

    if (!areaNodeIndex) {
      return false;
    }

    if (parameter.IsAborted()) {
      return false;
    }

    std::vector<FileOffset> nodeOffsets;
    StopClock               nodeIndexTimer;

    if (requestedNodeTypes.HasTypes()) {
      if (!areaNodeIndex->GetOffsets(*database->GetTypeConfig(),
                                     boundingBox,
                                     requestedNodeTypes,
                                     parameter.GetMaximumNodes(),
                                     nodeOffsets,
                                     loadedNodeTypes)) {
        log.Error() << "Error getting nodes from area node index!";
        return false;
      }
    }

    nodeIndexTimer.Stop();
    nodeIndexTime=nodeIndexTimer.ResultString();

    if (parameter.IsAborted()) {
      return false;
    }

    // Sort offsets before loading to optimize disk access
    std::sort(nodeOffsets.begin(),nodeOffsets.end());

    if (parameter.IsAborted()) {
      return false;
    }

    StopClock nodesTimer;

    if (!database->GetNodesByOffset(nodeOffsets,
                                    nodes)) {
      log.Error() << "Error reading nodes in area!";
      return false;
    }

    nodesTimer.Stop();
    nodesTime=nodesTimer.ResultString();

    if (parameter.IsAborted()) {
      return false;
    }

    return true;
  }

  bool MapService::GetObjectsAreas(const AreaSearchParameter& parameter,
                                   const TypeSet& requestedAreaTypes,
                                   const Magnification& magnification,
                                   const GeoBox& boundingBox,
                                   std::string& areaOptimizedTime,
                                   std::string& areaIndexTime,
                                   std::string& areasTime,
                                   std::vector<AreaRef>& areas) const
  {
    AreaAreaIndexRef        areaAreaIndex=database->GetAreaAreaIndex();
    OptimizeAreasLowZoomRef optimizeAreasLowZoom=database->GetOptimizeAreasLowZoom();
    TypeInfoSet             loadedAreaTypes;

    areas.clear();

    if (!areaAreaIndex ||
        !optimizeAreasLowZoom) {
      return false;
    }

    TypeSet internalAreaTypes(requestedAreaTypes);

    if (parameter.IsAborted()) {
      return false;
    }

    StopClock areaOptimizedTimer;

    if (internalAreaTypes.HasTypes()) {
      if (parameter.GetUseLowZoomOptimization() &&
          optimizeAreasLowZoom->HasOptimizations(magnification.GetMagnification())) {
        optimizeAreasLowZoom->GetAreas(boundingBox,
                                       magnification,
                                       internalAreaTypes,
                                       areas,
                                       loadedAreaTypes);
      }
    }

    for (const auto& type : loadedAreaTypes) {
      internalAreaTypes.UnsetType(type->GetAreaId());
    }

    areaOptimizedTimer.Stop();
    areaOptimizedTime=areaOptimizedTimer.ResultString();

    if (parameter.IsAborted()) {
      return false;
    }

    std::vector<DataBlockSpan> spans;
    StopClock                  areaIndexTimer;

    if (internalAreaTypes.HasTypes()) {
      if (!areaAreaIndex->GetAreasInArea(*database->GetTypeConfig(),
                                         boundingBox,
                                         magnification.GetLevel()+
                                         parameter.GetMaximumAreaLevel(),
                                         internalAreaTypes,
                                         parameter.GetMaximumAreas(),
                                         spans,
                                         loadedAreaTypes)) {
        log.Error() << "Error getting areas from area index!";
        return false;
      }
    }

    areaIndexTimer.Stop();
    areaIndexTime=areaIndexTimer.ResultString();

    if (parameter.IsAborted()) {
      return false;
    }

    StopClock areasTimer;

    if (!spans.empty()) {
      std::sort(spans.begin(),spans.end());

      if (!database->GetAreasByBlockSpans(spans,
                                          areas)) {
        log.Error() << "Error reading areas in area!";
        return false;
      }
    }

    areasTimer.Stop();
    areasTime=areasTimer.ResultString();

    return !parameter.IsAborted();
  }

  bool MapService::GetObjectsWays(const AreaSearchParameter& parameter,
                                  const std::vector<TypeSet>& wayTypes,
                                  const Magnification& magnification,
                                  const GeoBox& boundingBox,
                                  std::string& wayOptimizedTime,
                                  std::string& wayIndexTime,
                                  std::string& waysTime,
                                  std::vector<WayRef>& ways) const
  {
    AreaWayIndexRef        areaWayIndex=database->GetAreaWayIndex();
    OptimizeWaysLowZoomRef optimizeWaysLowZoom=database->GetOptimizeWaysLowZoom();
    TypeInfoSet            loadedWayTypes;

    ways.clear();

    if (!areaWayIndex ||
        !optimizeWaysLowZoom) {
      return false;
    }

    std::unordered_map<FileOffset,WayRef> cachedWays;

    for (auto& way : ways) {
      if (way->GetFileOffset()!=0) {
        cachedWays[way->GetFileOffset()]=way;
      }
    }

    ways.clear();

    std::vector<TypeSet> internalWayTypes(wayTypes);

    if (parameter.IsAborted()) {
      return false;
    }

    StopClock wayOptimizedTimer;

    if (!internalWayTypes.empty()) {
      if (parameter.GetUseLowZoomOptimization() &&
          optimizeWaysLowZoom->HasOptimizations(magnification.GetMagnification())) {
        optimizeWaysLowZoom->GetWays(boundingBox,
                                     magnification,
                                     internalWayTypes,
                                     ways,
                                     loadedWayTypes);
      }
    }

    for (auto& typeSet : internalWayTypes) {
      for (const auto& type : loadedWayTypes) {
        typeSet.UnsetType(type->GetWayId());
      }
    }

    wayOptimizedTimer.Stop();
    wayOptimizedTime=wayOptimizedTimer.ResultString();

    if (parameter.IsAborted()) {
      return false;
    }

    std::vector<FileOffset> offsets;
    StopClock               wayIndexTimer;

    if (!internalWayTypes.empty()) {
      if (!areaWayIndex->GetOffsets(boundingBox,
                                    internalWayTypes,
                                    parameter.GetMaximumWays(),
                                    offsets,
                                    loadedWayTypes)) {
        log.Error() << "Error getting ways from area way index!";
        return false;
      }
    }

    wayIndexTimer.Stop();
    wayIndexTime=wayIndexTimer.ResultString();

    if (parameter.IsAborted()) {
      return false;
    }

    ways.reserve(offsets.size());

    std::vector<FileOffset> restOffsets;

    restOffsets.reserve(offsets.size());

    for (const auto& offset : offsets) {
      auto entry=cachedWays.find(offset);

      if (entry!=cachedWays.end()) {
        ways.push_back(entry->second);
      }
      else {
        restOffsets.push_back(offset);
      }
    }

    std::sort(restOffsets.begin(),restOffsets.end());

    if (parameter.IsAborted()) {
      return false;
    }

    StopClock waysTimer;

    if (!restOffsets.empty()) {
      if (!database->GetWaysByOffset(restOffsets,
                                     ways)) {
        log.Error() << "Error reading ways in area!";
        return false;
      }
    }

    waysTimer.Stop();
    waysTime=waysTimer.ResultString();

    return !parameter.IsAborted();
  }

  /**
   * Returns all objects conforming to the given restrictions.
   *
   * @param parameter
   *    Further restrictions
   * @param styleConfig
   *    Style configuration, defining which types are loaded for the
   *    magnification defined by the projection
   * @param projection
   *    Projection defining the area and the magnification
   * @param data
   *    the returned data
   * @return
   *    true, if loading of data was successfull else false
   */
  bool MapService::GetObjects(const AreaSearchParameter& parameter,
                              const StyleConfig& styleConfig,
                              const Projection& projection,
                              MapData& data) const
  {
    osmscout::TypeSet              nodeTypes;
    std::vector<osmscout::TypeSet> wayTypes;
    osmscout::TypeSet              areaTypes;
    GeoBox                         boundingBox;

    projection.GetDimensions(boundingBox);

    styleConfig.GetNodeTypesWithMaxMag(projection.GetMagnification(),
                                       nodeTypes);

    styleConfig.GetWayTypesByPrioWithMaxMag(projection.GetMagnification(),
                                            wayTypes);

    styleConfig.GetAreaTypesWithMaxMag(projection.GetMagnification(),
                                       areaTypes);

    return GetObjects(parameter,
                      projection.GetMagnification(),
                      nodeTypes,
                      boundingBox,
                      data.nodes,
                      wayTypes,
                      boundingBox,
                      data.ways,
                      areaTypes,
                      boundingBox,
                      data.areas);
  }

  /**
   * Returns all objects conforming to the given restrictions.
   *
   * @param parameter
   *    Further restrictions
   * @param magnification
   *    Magnification
   * @param nodeTypes
   *    Allowed node types
   * @param nodeBoundingBox
   *    Boundary coordinates for loading nodes
   * @param nodes
   *    Found nodes
   * @param wayTypes
   *    Allowed way types
   * @param wayBoundingBox
   *    Boundary coordinates for loading ways
   * @param ways
   *    Found ways
   * @param areaTypes
   *    Allowed area types
   * @param areaBoundingBox
   *    Boundary coordinates for loading areas
   * @param areas
   *    Found areas
   * @return
   *    False, if there was an error, else true.
   */
  bool MapService::GetObjects(const AreaSearchParameter& parameter,
                              const Magnification& magnification,
                              const TypeSet &nodeTypes,
                              const GeoBox& nodeBoundingBox,
                              std::vector<NodeRef>& nodes,
                              const std::vector<TypeSet>& wayTypes,
                              const GeoBox& wayBoundingBox,
                              std::vector<WayRef>& ways,
                              const TypeSet& areaTypes,
                              const GeoBox& areaBoundingBox,
                              std::vector<AreaRef>& areas) const
  {
    std::string nodeIndexTime;
    std::string nodesTime;

    std::string areaOptimizedTime;
    std::string areaIndexTime;
    std::string areasTime;

    std::string wayOptimizedTime;
    std::string wayIndexTime;
    std::string waysTime;

    bool nodesSuccess=true;
    bool waysSuccess=true;
    bool areasSuccess=true;

#pragma omp parallel if(parameter.GetUseMultithreading())
#pragma omp sections
    {
#pragma omp section
      nodesSuccess=GetObjectsNodes(parameter,
                                   nodeTypes,
                                   nodeBoundingBox,
                                   nodeIndexTime,
                                   nodesTime,
                                   nodes);

#pragma omp section
      waysSuccess=GetObjectsWays(parameter,
                                 wayTypes,
                                 magnification,
                                 wayBoundingBox,
                                 wayOptimizedTime,
                                 wayIndexTime,
                                 waysTime,
                                 ways);

#pragma omp section
      areasSuccess=GetObjectsAreas(parameter,
                                   areaTypes,
                                   magnification,
                                   areaBoundingBox,
                                   areaOptimizedTime,
                                   areaIndexTime,
                                   areasTime,
                                   areas);
    }

    if (!nodesSuccess ||
        !waysSuccess ||
        !areasSuccess) {
      nodes.clear();
      areas.clear();
      ways.clear();

      return false;
    }

    /*
    if (database->IsDebugPerformance()) {
      std::cout << "Query: ";
      std::cout << "n " << nodeIndexTime << " ";
      std::cout << "w " << wayIndexTime << " ";
      std::cout << "a " << areaIndexTime << std::endl;

      std::cout << "Load: ";
      std::cout << "n " << nodesTime << " ";
      std::cout << "w " << wayOptimizedTime << "/" << waysTime << " ";
      std::cout << "a " << areaOptimizedTime << "/" << areasTime;
      std::cout << std::endl;
    }*/

    return true;
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

    if (!waterIndex->GetRegions(boundingBox.GetMinLon(),
                                boundingBox.GetMinLat(),
                                boundingBox.GetMaxLon(),
                                boundingBox.GetMaxLat(),
                                magnification,
                                tiles)) {
      std::cerr << "Error reading ground tiles in area!" << std::endl;
      return false;
    }

    timer.Stop();

    return true;
  }
}
