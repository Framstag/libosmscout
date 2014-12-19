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
#include <iostream>

#if _OPENMP
#include <omp.h>
#endif

#include <osmscout/system/Assert.h>
#include <osmscout/system/Math.h>

#include <osmscout/util/Geometry.h>

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
    if (breaker.Valid()) {
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
                                   const TypeSet &nodeTypes,
                                   double lonMin, double latMin,
                                   double lonMax, double latMax,
                                   std::string& nodeIndexTime,
                                   std::string& nodesTime,
                                   std::vector<NodeRef>& nodes) const
  {
    AreaNodeIndexRef areaNodeIndex=database->GetAreaNodeIndex();

    if (areaNodeIndex.Invalid()) {
      return false;
    }

    nodes.clear();

    if (parameter.IsAborted()) {
      return false;
    }

    std::vector<FileOffset> nodeOffsets;
    StopClock               nodeIndexTimer;

    if (nodeTypes.HasTypes()) {
      if (!areaNodeIndex->GetOffsets(lonMin,latMin,lonMax,latMax,
                                     nodeTypes,
                                     parameter.GetMaximumNodes(),
                                     nodeOffsets)) {
        std::cout << "Error getting nodes from area node index!" << std::endl;
        return false;
      }
    }

    nodeIndexTimer.Stop();
    nodeIndexTime=nodeIndexTimer.ResultString();

    if (parameter.IsAborted()) {
      return false;
    }

    std::sort(nodeOffsets.begin(),nodeOffsets.end());

    if (parameter.IsAborted()) {
      return false;
    }

    StopClock nodesTimer;

    if (!database->GetNodesByOffset(nodeOffsets,
                                    nodes)) {
      std::cout << "Error reading nodes in area!" << std::endl;
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
                                   const TypeSet& areaTypes,
                                   const Magnification& magnification,
                                   double lonMin, double latMin,
                                   double lonMax, double latMax,
                                   std::string& areaOptimizedTime,
                                   std::string& areaIndexTime,
                                   std::string& areasTime,
                                   std::vector<AreaRef>& areas) const
  {
    AreaAreaIndexRef        areaAreaIndex=database->GetAreaAreaIndex();
    OptimizeAreasLowZoomRef optimizeAreasLowZoom=database->GetOptimizeAreasLowZoom();

    if (areaAreaIndex.Invalid() ||
        optimizeAreasLowZoom.Invalid()) {
      return false;
    }

    OSMSCOUT_HASHMAP<FileOffset,AreaRef> cachedAreas;

    for (std::vector<AreaRef>::const_iterator area=areas.begin();
        area!=areas.end();
        ++area) {
      if ((*area)->GetFileOffset()!=0) {
        cachedAreas[(*area)->GetFileOffset()]=*area;
      }
    }

    areas.clear();

    TypeSet internalAreaTypes(areaTypes);

    if (parameter.IsAborted()) {
      return false;
    }

    StopClock areaOptimizedTimer;

    if (internalAreaTypes.HasTypes()) {
      if (parameter.GetUseLowZoomOptimization() &&
          optimizeAreasLowZoom->HasOptimizations(magnification.GetMagnification())) {
        optimizeAreasLowZoom->GetAreas(lonMin,
                                       latMin,
                                       lonMax,
                                       latMax,
                                       magnification,
                                       parameter.GetMaximumWays(),
                                       internalAreaTypes,
                                       areas);
      }
    }

    areaOptimizedTimer.Stop();
    areaOptimizedTime=areaOptimizedTimer.ResultString();

    if (parameter.IsAborted()) {
      return false;
    }

    std::vector<FileOffset> offsets;
    StopClock               areaIndexTimer;

    if (internalAreaTypes.HasTypes()) {
      if (!areaAreaIndex->GetOffsets(database->GetTypeConfig(),
                                     lonMin,
                                     latMin,
                                     lonMax,
                                     latMax,
                                     magnification.GetLevel()+
                                     parameter.GetMaximumAreaLevel(),
                                     internalAreaTypes,
                                     parameter.GetMaximumAreas(),
                                     offsets)) {
        std::cout << "Error getting areas from area index!" << std::endl;
        return false;
      }
    }

    areaIndexTimer.Stop();
    areaIndexTime=areaIndexTimer.ResultString();

    if (parameter.IsAborted()) {
      return false;
    }

    areas.reserve(offsets.size());

    std::vector<FileOffset> restOffsets;

    restOffsets.reserve(offsets.size());

    for (std::vector<FileOffset>::const_iterator offset=offsets.begin();
        offset!=offsets.end();
        ++offset) {
      OSMSCOUT_HASHMAP<FileOffset,AreaRef>::const_iterator entry=cachedAreas.find(*offset);

      if (entry!=cachedAreas.end()) {
        areas.push_back(entry->second);
      }
      else {
        restOffsets.push_back(*offset);
      }
    }

    std::sort(restOffsets.begin(),restOffsets.end());

    if (parameter.IsAborted()) {
      return false;
    }

    StopClock areasTimer;

    if (!restOffsets.empty()) {
      if (!database->GetAreasByOffset(restOffsets,
                                      areas)) {
        std::cout << "Error reading areas in area!" << std::endl;
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
                                  double lonMin, double latMin,
                                  double lonMax, double latMax,
                                  std::string& wayOptimizedTime,
                                  std::string& wayIndexTime,
                                  std::string& waysTime,
                                  std::vector<WayRef>& ways) const
  {
    AreaWayIndexRef        areaWayIndex=database->GetAreaWayIndex();
    OptimizeWaysLowZoomRef optimizeWaysLowZoom=database->GetOptimizeWaysLowZoom();

    if (areaWayIndex.Invalid() ||
        optimizeWaysLowZoom.Invalid()) {
      return false;
    }

    OSMSCOUT_HASHMAP<FileOffset,WayRef> cachedWays;

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
        optimizeWaysLowZoom->GetWays(lonMin,
                                     latMin,
                                     lonMax,
                                     latMax,
                                     magnification,
                                     parameter.GetMaximumWays(),
                                     internalWayTypes,
                                     ways);
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
      if (!areaWayIndex->GetOffsets(lonMin,
                                    latMin,
                                    lonMax,
                                    latMax,
                                    internalWayTypes,
                                    parameter.GetMaximumWays(),
                                    offsets)) {
        std::cout << "Error getting ways from area way index!" << std::endl;
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

    for (std::vector<FileOffset>::const_iterator offset=offsets.begin();
        offset!=offsets.end();
        ++offset) {
      OSMSCOUT_HASHMAP<FileOffset,WayRef>::const_iterator entry=cachedWays.find(*offset);

      if (entry!=cachedWays.end()) {
        ways.push_back(entry->second);
      }
      else {
        restOffsets.push_back(*offset);
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
        std::cout << "Error reading ways in area!" << std::endl;
        return false;
      }
    }

    waysTimer.Stop();
    waysTime=waysTimer.ResultString();

    return !parameter.IsAborted();
  }

  bool MapService::GetObjects(const AreaSearchParameter& parameter,
                              const StyleConfig& styleConfig,
                              const Projection& projection,
                              MapData& data) const
  {
    osmscout::TypeSet              nodeTypes;
    std::vector<osmscout::TypeSet> wayTypes;
    osmscout::TypeSet              areaTypes;
    double                         lonMin,lonMax,latMin,latMax;

    projection.GetDimensions(lonMin,latMin,lonMax,latMax);

    styleConfig.GetNodeTypesWithMaxMag(projection.GetMagnification(),
                                       nodeTypes);

    styleConfig.GetWayTypesByPrioWithMaxMag(projection.GetMagnification(),
                                            wayTypes);

    styleConfig.GetAreaTypesWithMaxMag(projection.GetMagnification(),
                                       areaTypes);

    return GetObjects(parameter,
                      projection.GetMagnification(),
                      nodeTypes,
                      lonMin,
                      latMin,
                      lonMax,
                      latMax,
                      data.nodes,
                      wayTypes,
                      lonMin,
                      latMin,
                      lonMax,
                      latMax,
                      data.ways,
                      areaTypes,
                      lonMin,
                      latMin,
                      lonMax,
                      latMax,
                      data.areas);
  }

  /**
   * Returns all objects conforming to the given restrictions.
   *
   * @param nodeTypes
   *    Allowed node types
   * @param wayTypes
   *    Allowed way types
   * @param areaTypes
   *    Allowed area types
   * @param lonMin
   *    Boundary coordinate
   * @param latMin
   *    Boundary coordinate
   * @param lonMax
   *    Boundary coordinate
   * @param latMax
   *    Boundary coordinate
   * @param magnification
   *    Magnification
   * @param parameter
   *    Further restrictions
   * @param nodes
   *    Found nodes
   * @param ways
   *    Found ways
   * @param areas
   *    Found area
   * @return
   *    False, if there was an error, else true.
   */

  /**
   * Returns all objects conforming to the given restrictions.
   *
   * @param parameter
   *    Further restrictions
   * @param magnification
   *    Magnification
   * @param nodeTypes
   *    Allowed node types
   * @param nodeLonMin
   *    Boundary coordinate
   * @param nodeLatMin
   *    Boundary coordinate
   * @param nodeLonMax
   *    Boundary coordinate
   * @param nodeLatMax
   *    Boundary coordinate
   * @param nodes
   *    Found nodes
   * @param wayTypes
   *    Allowed way types
   * @param wayLonMin
   *    Boundary coordinate
   * @param wayLatMin
   *    Boundary coordinate
   * @param wayLonMax
   *    Boundary coordinate
   * @param wayLatMax
   *    Boundary coordinate
   * @param ways
   *    Found ways
   * @param areaTypes
   *    Allowed area types
   * @param areaLonMin
   *    Boundary coordinate
   * @param areaLatMin
   *    Boundary coordinate
   * @param areaLonMax
   *    Boundary coordinate
   * @param areaLatMax
   *    Boundary coordinate
   * @param areas
   *    Found areas
   * @return
   *    False, if there was an error, else true.
   */
  bool MapService::GetObjects(const AreaSearchParameter& parameter,
                              const Magnification& magnification,
                              const TypeSet &nodeTypes,
                              double nodeLonMin, double nodeLatMin,
                              double nodeLonMax, double nodeLatMax,
                              std::vector<NodeRef>& nodes,
                              const std::vector<TypeSet>& wayTypes,
                              double wayLonMin, double wayLatMin,
                              double wayLonMax, double wayLatMax,
                              std::vector<WayRef>& ways,
                              const TypeSet& areaTypes,
                              double areaLonMin, double areaLatMin,
                              double areaLonMax, double areaLatMax,
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
                                   nodeLonMin,
                                   nodeLatMin,
                                   nodeLonMax,
                                   nodeLatMax,
                                   nodeIndexTime,
                                   nodesTime,
                                   nodes);

#pragma omp section
      waysSuccess=GetObjectsWays(parameter,
                                 wayTypes,
                                 magnification,
                                 wayLonMin,
                                 wayLatMin,
                                 wayLonMax,
                                 wayLatMax,
                                 wayOptimizedTime,
                                 wayIndexTime,
                                 waysTime,
                                 ways);

#pragma omp section
      areasSuccess=GetObjectsAreas(parameter,
                                   areaTypes,
                                   magnification,
                                   areaLonMin,
                                   areaLatMin,
                                   areaLonMax,
                                   areaLatMax,
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

  bool MapService::GetGroundTiles(const Projection& projection,
                                  std::list<GroundTile>& tiles) const
  {
    double lonMin,lonMax,latMin,latMax;

    projection.GetDimensions(lonMin,latMin,lonMax,latMax);

    return GetGroundTiles(lonMin,latMin,lonMax,latMax,
                          projection.GetMagnification(),
                          tiles);
  }

  /**
   * Return all ground tiles for the given area and the given magnification.
   *
   * \note The returned ground tiles may result in a bigger area than given.
   *
   * @param lonMin
   *    Boundary coordinate
   * @param latMin
   *    Boundary coordinate
   * @param lonMax
   *    Boundary coordinate
   * @param latMax
   *    Boundary coordinate
   * @param magnification
   *    Magnification
   * @param tiles
   *    List of returned tiles
   * @return
   *    False, if there was an error, else true.
   */
  bool MapService::GetGroundTiles(double lonMin, double latMin,
                                  double lonMax, double latMax,
                                  const Magnification& magnification,
                                  std::list<GroundTile>& tiles) const
  {
    WaterIndexRef waterIndex=database->GetWaterIndex();

    if (waterIndex.Invalid()) {
      return false;
    }

    StopClock timer;

    if (!waterIndex->GetRegions(lonMin,
                                latMin,
                                lonMax,
                                latMax,
                                magnification,
                                tiles)) {
      std::cerr << "Error reading ground tiles in area!" << std::endl;
      return false;
    }

    timer.Stop();

    return true;
  }
}
