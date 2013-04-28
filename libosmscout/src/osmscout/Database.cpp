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

#include <osmscout/Database.h>

#include <algorithm>
#include <iostream>

#if _OPENMP
#include <omp.h>
#endif

#include <osmscout/TypeConfigLoader.h>

#include <osmscout/system/Assert.h>
#include <osmscout/system/Math.h>

namespace osmscout {

  DatabaseParameter::DatabaseParameter()
  : areaAreaIndexCacheSize(1000),
    areaNodeIndexCacheSize(1000),
    nodeCacheSize(1000),
    wayCacheSize(8000),
    relationCacheSize(1000),
    debugPerformance(false)
  {
    // no code
  }

  void DatabaseParameter::SetAreaAreaIndexCacheSize(unsigned long areaAreaIndexCacheSize)
  {
    this->areaAreaIndexCacheSize=areaAreaIndexCacheSize;
  }

  void DatabaseParameter::SetAreaNodeIndexCacheSize(unsigned long areaNodeIndexCacheSize)
  {
    this->areaNodeIndexCacheSize=areaNodeIndexCacheSize;
  }

  void DatabaseParameter::SetNodeCacheSize(unsigned long nodeCacheSize)
  {
    this->nodeCacheSize=nodeCacheSize;
  }

  void DatabaseParameter::SetWayCacheSize(unsigned long wayCacheSize)
  {
    this->wayCacheSize=wayCacheSize;
  }

  void DatabaseParameter::SetRelationCacheSize(unsigned long relationCacheSize)
  {
    this->relationCacheSize=relationCacheSize;
  }

  void DatabaseParameter::SetDebugPerformance(bool debug)
  {
    debugPerformance=debug;
  }

  unsigned long DatabaseParameter::GetAreaAreaIndexCacheSize() const
  {
    return areaAreaIndexCacheSize;
  }

  unsigned long DatabaseParameter::GetAreaNodeIndexCacheSize() const
  {
    return areaNodeIndexCacheSize;
  }

  unsigned long DatabaseParameter::GetNodeCacheSize() const
  {
    return nodeCacheSize;
  }

  unsigned long DatabaseParameter::GetWayCacheSize() const
  {
    return wayCacheSize;
  }

  unsigned long DatabaseParameter::GetRelationCacheSize() const
  {
    return relationCacheSize;
  }

  bool DatabaseParameter::IsDebugPerformance() const
  {
    return debugPerformance;
  }

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

  Database::Database(const DatabaseParameter& parameter)
   : isOpen(false),
     debugPerformance(parameter.IsDebugPerformance()),
     areaNodeIndex(/*parameter.GetAreaNodeIndexCacheSize()*/),
     areaWayIndex(),
     areaAreaIndex(parameter.GetAreaAreaIndexCacheSize()),
     nodeDataFile("nodes.dat",
                  parameter.GetNodeCacheSize()),
     wayDataFile("ways.dat",
                  parameter.GetWayCacheSize()),
     relationDataFile("relations.dat",
                      parameter.GetRelationCacheSize()),
     typeConfig(NULL),
     hashFunction(NULL)
  {
    // no code
  }

  Database::~Database()
  {
    delete typeConfig;
  }

  bool Database::Open(const std::string& path,
                      std::string (*hashFunction) (std::string))
  {
    assert(!path.empty());

    this->path=path;
    this->hashFunction=hashFunction;

    typeConfig=new TypeConfig();

    if (!LoadTypeData(path,*typeConfig)) {
      std::cerr << "Cannot load 'types.dat'!" << std::endl;
      delete typeConfig;
      typeConfig=NULL;
      return false;
    }

    FileScanner scanner;
    std::string file=AppendFileToDir(path,"bounding.dat");

    if (!scanner.Open(file,FileScanner::Normal,true)) {
      std::cerr << "Cannot open 'bounding.dat'" << std::endl;
      delete typeConfig;
      typeConfig=NULL;
      return false;
    }

    uint32_t minLonDat;
    uint32_t minLatDat;
    uint32_t maxLonDat;
    uint32_t maxLatDat;

    scanner.ReadNumber(minLatDat);
    scanner.ReadNumber(minLonDat);
    scanner.ReadNumber(maxLatDat);
    scanner.ReadNumber(maxLonDat);

    if (scanner.HasError() || !scanner.Close()) {
      std::cerr << "Error while reading/closing '" << file << "'" << std::endl;
      delete typeConfig;
      typeConfig=NULL;
      return false;
    }

    minLon=minLonDat/conversionFactor-180.0;
    minLat=minLatDat/conversionFactor-90.0;
    maxLon=maxLonDat/conversionFactor-180.0;
    maxLat=maxLatDat/conversionFactor-90.0;

    if (!nodeDataFile.Open(path,FileScanner::LowMemRandom,true)) {
      std::cerr << "Cannot open 'nodes.dat'!" << std::endl;
      delete typeConfig;
      typeConfig=NULL;
      return false;
    }

    if (!wayDataFile.Open(path,FileScanner::LowMemRandom,true)) {
      std::cerr << "Cannot open 'ways.dat'!" << std::endl;
      delete typeConfig;
      typeConfig=NULL;
      return false;
    }

    if (!relationDataFile.Open(path,FileScanner::LowMemRandom,true)) {
      std::cerr << "Cannot open 'relations.dat'!" << std::endl;
      delete typeConfig;
      typeConfig=NULL;
      return false;
    }

    if (!optimizeLowZoom.Open(path)) {
      std::cerr << "Cannot load low zoom optimizations!" << std::endl;
      delete typeConfig;
      typeConfig=NULL;
      return false;
    }

    if (!areaAreaIndex.Load(path)) {
      std::cerr << "Cannot load area area index!" << std::endl;
      delete typeConfig;
      typeConfig=NULL;
      return false;
    }

    if (!areaNodeIndex.Load(path)) {
      std::cerr << "Cannot load area node index!" << std::endl;
      delete typeConfig;
      typeConfig=NULL;
      return false;
    }

    if (!areaWayIndex.Load(path)) {
      std::cerr << "Cannot load area way index!" << std::endl;
      delete typeConfig;
      typeConfig=NULL;
      return false;
    }

    if (!cityStreetIndex.Load(path, hashFunction)) {
      std::cerr << "Cannot load city street index!" << std::endl;
      delete typeConfig;
      typeConfig=NULL;
      return false;
    }

    if (!waterIndex.Load(path)) {
      std::cerr << "Cannot load water index!" << std::endl;
      delete typeConfig;
      typeConfig=NULL;
      return false;
    }

    isOpen=true;

    return true;
  }

  bool Database::IsOpen() const
  {
    return isOpen;
  }


  void Database::Close()
  {
    nodeDataFile.Close();
    wayDataFile.Close();

    isOpen=false;
  }

  void Database::FlushCache()
  {
    nodeDataFile.FlushCache();
    wayDataFile.FlushCache();
    relationDataFile.FlushCache();
  }

  TypeConfig* Database::GetTypeConfig() const
  {
    return typeConfig;
  }

  bool Database::GetBoundingBox(double& minLat,double& minLon,
                                double& maxLat,double& maxLon) const
  {
    if (!IsOpen()) {
      return false;
    }

    minLat=this->minLat;
    minLon=this->minLon;
    maxLat=this->maxLat;
    maxLon=this->maxLon;

    return true;
  }

  bool Database::GetObjectsNodes(const AreaSearchParameter& parameter,
                                 const TypeSet &nodeTypes,
                                 double lonMin, double latMin,
                                 double lonMax, double latMax,
                                 std::string& nodeIndexTime,
                                 std::string& nodesTime,
                                 std::vector<NodeRef>& nodes) const
  {
    std::vector<FileOffset> nodeOffsets;

    if (parameter.IsAborted()) {
      return false;
    }

    nodes.clear();

    if (parameter.IsAborted()) {
      return false;
    }

    StopClock nodeIndexTimer;

    if (nodeTypes.HasTypes()) {
      if (!areaNodeIndex.GetOffsets(lonMin,latMin,lonMax,latMax,
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

    if (!GetNodesByOffset(nodeOffsets,
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

  bool Database::GetObjectsWayOffsets(const AreaSearchParameter& parameter,
                                      const std::vector<TypeSet>& wayTypes,
                                      const Magnification& magnification,
                                      double lonMin, double latMin,
                                      double lonMax, double latMax,
                                      std::string& wayOptimizedTime,
                                      std::string& wayIndexTime,
                                      std::vector<FileOffset>& wayWayOffsets,
                                      std::vector<FileOffset>& relationWayOffsets,
                                      std::vector<WayRef>& ways) const
  {
    std::vector<TypeSet> internalWayTypes(wayTypes);

    if (parameter.IsAborted()) {
      return false;
    }

    if (parameter.IsAborted()) {
      return false;
    }

    StopClock wayOptimizedTimer;

    if (!internalWayTypes.empty()) {
      if (parameter.GetUseLowZoomOptimization() &&
          optimizeLowZoom.HasOptimizations(magnification.GetMagnification())) {
        optimizeLowZoom.GetWays(lonMin,
                                latMin,
                                lonMax,
                                latMax,
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

    StopClock wayIndexTimer;

    if (!internalWayTypes.empty()) {
      if (!areaWayIndex.GetOffsets(lonMin,
                                   latMin,
                                   lonMax,
                                   latMax,
                                   internalWayTypes,
                                   parameter.GetMaximumWays(),
                                   wayWayOffsets,
                                   relationWayOffsets)) {
        std::cout << "Error getting ways Glations from area way index!" << std::endl;
        return false;
      }
    }

    wayIndexTimer.Stop();
    wayIndexTime=wayIndexTimer.ResultString();

    if (parameter.IsAborted()) {
      return false;
    }

    std::sort(wayWayOffsets.begin(),wayWayOffsets.end());
    std::sort(relationWayOffsets.begin(),relationWayOffsets.end());

    if (parameter.IsAborted()) {
      return false;
    }

    return true;
  }

  bool Database::GetObjectsAreaOffsets(const AreaSearchParameter& parameter,
                                       const TypeSet& areaTypes,
                                       const Magnification& magnification,
                                       double lonMin, double latMin,
                                       double lonMax, double latMax,
                                       std::string& areaIndexTime,
                                       std::vector<FileOffset>& wayAreaOffsets,
                                       std::vector<FileOffset>& relationAreaOffsets) const
  {
    if (parameter.IsAborted()) {
      return false;
    }

    StopClock areaIndexTimer;

    if (areaTypes.HasTypes()) {
      if (!areaAreaIndex.GetOffsets(lonMin,
                                    latMin,
                                    lonMax,
                                    latMax,
                                    magnification.GetLevel()+
                                    parameter.GetMaximumAreaLevel(),
                                    areaTypes,
                                    parameter.GetMaximumAreas(),
                                    wayAreaOffsets,
                                    relationAreaOffsets)) {
        std::cout << "Error getting areas from area index!" << std::endl;
        return false;
      }
    }

    areaIndexTimer.Stop();
    areaIndexTime=areaIndexTimer.ResultString();

    if (parameter.IsAborted()) {
      return false;
    }

    std::sort(wayAreaOffsets.begin(),wayAreaOffsets.end());
    std::sort(relationAreaOffsets.begin(),relationAreaOffsets.end());

    if (parameter.IsAborted()) {
      return false;
    }

    return true;
  }

  bool Database::GetObjectsWaysAndAreas(const AreaSearchParameter& parameter,
                                        const std::vector<FileOffset>& wayWayOffsets,
                                        const std::vector<FileOffset>& wayAreaOffsets,
                                        std::string& waysTime,
                                        std::string& areasTime,
                                        std::vector<WayRef>& ways,
                                        std::vector<WayRef>& areas) const
  {
    if (parameter.IsAborted()) {
      return false;
    }

    StopClock waysTimer;

    if (!wayWayOffsets.empty()) {
      if (!GetWaysByOffset(wayWayOffsets,
                           ways)) {
        std::cout << "Error reading ways in area!" << std::endl;
        return false;
      }
    }

    waysTimer.Stop();
    waysTime=waysTimer.ResultString();

    if (parameter.IsAborted()) {
      return false;
    }

    StopClock areasTimer;

    if (!wayAreaOffsets.empty()) {
      if (!GetWaysByOffset(wayAreaOffsets,
                           areas)) {
        std::cout << "Error reading areas in area!" << std::endl;
        return false;
      }
    }

    areasTimer.Stop();
    areasTime=areasTimer.ResultString();

    if (parameter.IsAborted()) {
      return false;
    }

    return true;
  }

  bool Database::GetObjectsWaysAndAreasRel(const AreaSearchParameter& parameter,
                                           const std::vector<FileOffset>& relationWayOffsets,
                                           const std::vector<FileOffset>& relationAreaOffsets,
                                           std::string& relationWaysTime,
                                           std::string& relationAreasTime,
                                           std::vector<RelationRef>& relationWays,
                                           std::vector<RelationRef>& relationAreas) const
  {
    if (parameter.IsAborted()) {
      return false;
    }

    StopClock relationWaysTimer;

    if (!relationWayOffsets.empty()) {
      if (!GetRelationsByOffset(relationWayOffsets,
                                relationWays)) {
        std::cout << "Error reading relation ways in area!" << std::endl;
        return false;
      }
    }

    relationWaysTimer.Stop();
    relationWaysTime=relationWaysTimer.ResultString();

    if (parameter.IsAborted()) {
      return false;
    }

    StopClock relationAreasTimer;

    if (!relationAreaOffsets.empty()) {
      if (!GetRelationsByOffset(relationAreaOffsets,
                                relationAreas)) {
        std::cerr << "Error reading relation areas in area!" << std::endl;
        return false;
      }
    }

    relationAreasTimer.Stop();
    relationAreasTime=relationAreasTimer.ResultString();

    if (parameter.IsAborted()) {
      return false;
    }

    return true;
  }

  bool Database::GetObjects(const TypeSet &nodeTypes,
                            const std::vector<TypeSet>& wayTypes,
                            const TypeSet& areaTypes,
                            double lonMin, double latMin,
                            double lonMax, double latMax,
                            const Magnification& magnification,
                            const AreaSearchParameter& parameter,
                            std::vector<NodeRef>& nodes,
                            std::vector<WayRef>& ways,
                            std::vector<WayRef>& areas,
                            std::vector<RelationRef>& relationWays,
                            std::vector<RelationRef>& relationAreas) const
  {
    std::vector<FileOffset> wayWayOffsets;
    std::vector<FileOffset> relationWayOffsets;
    std::vector<FileOffset> wayAreaOffsets;
    std::vector<FileOffset> relationAreaOffsets;

    std::string             nodeIndexTime;
    std::string             nodesTime;

    std::string             wayOptimizedTime;
    std::string             wayIndexTime;
    std::string             waysTime;
    std::string             relationWaysTime;

    std::string             areaIndexTime;
    std::string             areasTime;
    std::string             relationAreasTime;

    if (!IsOpen()) {
      return false;
    }

    {
      ways.clear();
      areas.clear();
      relationWays.clear();
      relationAreas.clear();
    }

    if (parameter.IsAborted()) {
      return false;
    }

    bool nodesSuccess;
    bool wayOffsetsSuccess;
    bool areaOffsetsSuccess;

#pragma omp parallel if(parameter.GetUseMultithreading())
#pragma omp sections
    {
#pragma omp section
      nodesSuccess=GetObjectsNodes(parameter,
                                   nodeTypes,
                                   lonMin,
                                   latMin,
                                   lonMax,
                                   latMax,
                                   nodeIndexTime,
                                   nodesTime,
                                   nodes);

#pragma omp section
      wayOffsetsSuccess=GetObjectsWayOffsets(parameter,
                                             wayTypes,
                                             magnification,
                                             lonMin,
                                             latMin,
                                             lonMax,
                                             latMax,
                                             wayOptimizedTime,
                                             wayIndexTime,
                                             wayWayOffsets,
                                             relationWayOffsets,
                                             ways);

#pragma omp section
      areaOffsetsSuccess=GetObjectsAreaOffsets(parameter,
                                               areaTypes,
                                               magnification,
                                               lonMin,
                                               latMin,
                                               lonMax,
                                               latMax,
                                               areaIndexTime,
                                               wayAreaOffsets,
                                               relationAreaOffsets);
    }

    if (!nodesSuccess ||
        !wayOffsetsSuccess ||
        !areaOffsetsSuccess) {
      return false;
    }

    bool waSuccess;
    bool warSuccess;

#pragma omp parallel if(parameter.GetUseMultithreading())
#pragma omp sections
    {

#pragma omp section
      waSuccess=GetObjectsWaysAndAreas(parameter,
                                      wayWayOffsets,
                                      wayAreaOffsets,
                                      waysTime,
                                      areasTime,
                                      ways,
                                      areas);

#pragma omp section
      warSuccess=GetObjectsWaysAndAreasRel(parameter,
                                          relationWayOffsets,
                                          relationAreaOffsets,
                                          relationWaysTime,
                                          relationAreasTime,
                                          relationWays,
                                          relationAreas);
    }

    if (!waSuccess ||
        !warSuccess) {
      return false;
    }

    if (debugPerformance) {
      std::cout << "Query: ";
      std::cout << "n " << nodeIndexTime << " ";
      std::cout << "w " << wayIndexTime << " ";
      std::cout << "a " << areaIndexTime << std::endl;

      std::cout << "Load: ";
      std::cout << "n " << nodesTime << " ";
      std::cout << "w " << wayOptimizedTime << "/" << waysTime << "/" << relationWaysTime << " ";
      std::cout << "a " << areasTime << "/" << relationAreasTime;
      std::cout << std::endl;
    }

    return true;
  }

  bool Database::GetObjects(double lonMin, double latMin,
                            double lonMax, double latMax,
                            const TypeSet& types,
                            std::vector<NodeRef>& nodes,
                            std::vector<WayRef>& ways,
                            std::vector<WayRef>& areas,
                            std::vector<RelationRef>& relationWays,
                            std::vector<RelationRef>& relationAreas) const
  {
    if (!IsOpen()) {
      return false;
    }

    std::vector<TypeSet>    wayTypes;
    std::vector<FileOffset> nodeOffsets;
    std::vector<FileOffset> wayWayOffsets;
    std::vector<FileOffset> relationWayOffsets;
    std::vector<FileOffset> wayAreaOffsets;
    std::vector<FileOffset> relationAreaOffsets;

    nodes.clear();
    ways.clear();
    areas.clear();
    relationWays.clear();
    relationAreas.clear();

    wayTypes.push_back(types);;

    StopClock nodeIndexTimer;

    if (!areaNodeIndex.GetOffsets(lonMin,latMin,lonMax,latMax,
                                  types,
                                  std::numeric_limits<size_t>::max(),
                                  nodeOffsets)) {
      std::cout << "Error getting nodes from area node index!" << std::endl;
      return false;
    }

    nodeIndexTimer.Stop();

    StopClock wayIndexTimer;

    if (!areaWayIndex.GetOffsets(lonMin,
                                 latMin,
                                 lonMax,
                                 latMax,
                                 wayTypes,
                                 std::numeric_limits<size_t>::max(),
                                 wayWayOffsets,
                                 relationWayOffsets)) {
      std::cout << "Error getting ways and relations from area way index!" << std::endl;
    }

    wayIndexTimer.Stop();

    StopClock areaAreaIndexTimer;

    if (!areaAreaIndex.GetOffsets(lonMin,
                                  latMin,
                                  lonMax,
                                  latMax,
                                  std::numeric_limits<size_t>::max(),
                                  types,
                                  std::numeric_limits<size_t>::max(),
                                  wayAreaOffsets,
                                  relationAreaOffsets)) {
      std::cout << "Error getting ways and relations from area index!" << std::endl;
    }

    areaAreaIndexTimer.Stop();

    StopClock sortTimer;

    std::sort(nodeOffsets.begin(),nodeOffsets.end());
    std::sort(wayWayOffsets.begin(),wayWayOffsets.end());
    std::sort(wayAreaOffsets.begin(),wayAreaOffsets.end());
    std::sort(relationWayOffsets.begin(),relationWayOffsets.end());
    std::sort(relationAreaOffsets.begin(),relationAreaOffsets.end());

    sortTimer.Stop();

    StopClock nodesTimer;

    if (!GetNodesByOffset(nodeOffsets,
                  nodes)) {
      std::cout << "Error reading nodes in area!" << std::endl;
      return false;
    }

    nodesTimer.Stop();

    StopClock waysTimer;

    if (!GetWaysByOffset(wayWayOffsets,
                 ways)) {
      std::cout << "Error reading ways in area!" << std::endl;
      return false;
    }

    waysTimer.Stop();

    StopClock areasTimer;

    if (!GetWaysByOffset(wayAreaOffsets,
                 areas)) {
      std::cout << "Error reading areas in area!" << std::endl;
      return false;
    }

    areasTimer.Stop();

    StopClock relationWaysTimer;

    if (!GetRelationsByOffset(relationWayOffsets,
                      relationWays)) {
      std::cout << "Error reading relation ways in area!" << std::endl;
      return false;
    }

    relationWaysTimer.Stop();

    StopClock relationAreasTimer;

    if (!GetRelationsByOffset(relationAreaOffsets,
                      relationAreas)) {
      std::cerr << "Error reading relation areas in area!" << std::endl;
      return false;
    }

    relationAreasTimer.Stop();

    if (debugPerformance) {
      std::cout << "I/O: ";
      std::cout << "n " << nodeIndexTimer << " ";
      std::cout << "w " << wayIndexTimer << " ";
      std::cout << "a " << areaAreaIndexTimer;
      std::cout << " - ";
      std::cout << "s "  << sortTimer;
      std::cout << " - ";
      std::cout << "n " << nodesTimer << " ";
      std::cout << "w " << waysTimer << "/" << relationWaysTimer << " ";
      std::cout << "a " << areasTimer << "/" << relationAreasTimer;
      std::cout << std::endl;
    }

    return true;
  }

  bool Database::GetGroundTiles(double lonMin, double latMin,
                                double lonMax, double latMax,
                                const Magnification& magnification,
                                std::list<GroundTile>& tiles) const
  {
    if (!IsOpen()) {
      return false;
    }

    StopClock timer;

    if (!waterIndex.GetRegions(lonMin,
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

  bool Database::GetNodeByOffset(const FileOffset& offset,
                                 NodeRef& node) const
  {
    if (!IsOpen()) {
      return false;
    }

    std::vector<FileOffset> offsets;
    std::vector<NodeRef>    nodes;

    offsets.push_back(offset);

    if (GetNodesByOffset(offsets,nodes)) {
      if (!nodes.empty()) {
        node=*nodes.begin();
        return true;
      }
    }

    return false;
  }

  bool Database::GetNodesByOffset(const std::vector<FileOffset>& offsets,
                                  std::vector<NodeRef>& nodes) const
  {
    if (!IsOpen()) {
      return false;
    }

    return nodeDataFile.GetByOffset(offsets,nodes);
  }

  bool Database::GetNodesByOffset(const std::list<FileOffset>& offsets,
                                  std::vector<NodeRef>& nodes) const
  {
    if (!IsOpen()) {
      return false;
    }

    return nodeDataFile.GetByOffset(offsets,nodes);
  }

  bool Database::GetWayByOffset(const FileOffset& offset,
                                WayRef& way) const
  {
    if (!IsOpen()) {
      return false;
    }

    std::vector<FileOffset> offsets;
    std::vector<WayRef>     ways;

    offsets.push_back(offset);

    if (GetWaysByOffset(offsets,ways)) {
      if (!ways.empty()) {
        way=*ways.begin();
        return true;
      }
    }

    return false;
  }

  bool Database::GetWaysByOffset(const std::vector<FileOffset>& offsets,
                                 std::vector<WayRef>& ways) const
  {
    if (!IsOpen()) {
      return false;
    }

    return wayDataFile.GetByOffset(offsets,ways);
  }

  bool Database::GetWaysByOffset(const std::list<FileOffset>& offsets,
                                 std::vector<WayRef>& ways) const
  {
    if (!IsOpen()) {
      return false;
    }

    return wayDataFile.GetByOffset(offsets,ways);
  }

  bool Database::GetRelationByOffset(const FileOffset& offset,
                                     RelationRef& relation) const
  {
    if (!IsOpen()) {
      return false;
    }

    std::vector<FileOffset>  offsets;
    std::vector<RelationRef> relations;

    offsets.push_back(offset);

    if (GetRelationsByOffset(offsets,relations)) {
      if (!relations.empty()) {
        relation=*relations.begin();
        return true;
      }
    }

    return false;
  }

  bool Database::GetRelationsByOffset(const std::vector<FileOffset>& offsets,
                                      std::vector<RelationRef>& relations) const
  {
    if (!IsOpen()) {
      return false;
    }

    return relationDataFile.GetByOffset(offsets,relations);
  }

  bool Database::GetRelationsByOffset(const std::list<FileOffset>& offsets,
                                      std::vector<RelationRef>& relations) const
  {
    if (!IsOpen()) {
      return false;
    }

    return relationDataFile.GetByOffset(offsets,relations);
  }

  bool Database::GetMatchingAdminRegions(const std::string& name,
                                         std::list<AdminRegion>& regions,
                                         size_t limit,
                                         bool& limitReached,
                                         bool startWith) const
  {
    if (!IsOpen()) {
      return false;
    }

    return cityStreetIndex.GetMatchingAdminRegions(name,
                                                   regions,
                                                   limit,
                                                   limitReached,
                                                   startWith);
  }

  bool Database::GetMatchingLocations(const AdminRegion& region,
                                      const std::string& name,
                                      std::list<Location>& locations,
                                      size_t limit,
                                      bool& limitReached,
                                      bool startWith) const
  {
    if (!IsOpen()) {
      return false;
    }

    return cityStreetIndex.GetMatchingLocations(region,
                                                name,
                                                locations,
                                                limit,
                                                limitReached,
                                                startWith);
  }

  void Database::DumpStatistics()
  {
    nodeDataFile.DumpStatistics();
    wayDataFile.DumpStatistics();
    relationDataFile.DumpStatistics();

    areaAreaIndex.DumpStatistics();
    areaNodeIndex.DumpStatistics();
    areaWayIndex.DumpStatistics();
    cityStreetIndex.DumpStatistics();
    waterIndex.DumpStatistics();
  }
}
