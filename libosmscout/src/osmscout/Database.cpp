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
    wayCacheSize(4000),
    areaCacheSize(4000),
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

  void DatabaseParameter::SetAreaCacheSize(unsigned long areaCacheSize)
  {
    this->areaCacheSize=areaCacheSize;
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

  unsigned long DatabaseParameter::GetAreaCacheSize() const
  {
    return areaCacheSize;
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
     minLon(0.0),
     minLat(0.0),
     maxLon(0.0),
     maxLat(0.0),
     areaNodeIndex(/*parameter.GetAreaNodeIndexCacheSize()*/),
     areaWayIndex(),
     areaAreaIndex(parameter.GetAreaAreaIndexCacheSize()),
     nodeDataFile("nodes.dat",
                  parameter.GetNodeCacheSize()),
     areaDataFile("areas.dat",
                  parameter.GetAreaCacheSize()),
     wayDataFile("ways.dat",
                  parameter.GetWayCacheSize()),
     typeConfig(NULL),
     hashFunction(NULL)
  {
    // no code
  }

  Database::~Database()
  {
    delete typeConfig;
  }

  bool Database::Open(const std::string& path)
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

    if (!areaDataFile.Open(path,FileScanner::LowMemRandom,true)) {
      std::cerr << "Cannot open 'areas.dat'!" << std::endl;
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

    if (!optimizeAreasLowZoom.Open(path)) {
      std::cerr << "Cannot load area low zoom optimizations!" << std::endl;
      delete typeConfig;
      typeConfig=NULL;
      return false;
    }


    if (!optimizeWaysLowZoom.Open(path)) {
      std::cerr << "Cannot load ways low zoom optimizations!" << std::endl;
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

    if (!waterIndex.Load(path)) {
      std::cerr << "Cannot load water index!" << std::endl;
      delete typeConfig;
      typeConfig=NULL;
      return false;
    }

    if (!cityStreetIndex.Load(path)) {
      std::cerr << "Cannot load city street index!" << std::endl;
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
    areaDataFile.FlushCache();
    wayDataFile.FlushCache();
  }

  std::string Database::GetPath() const
  {
    return path;
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

    if (parameter.IsAborted()) {
      return false;
    }

    nodes.clear();

    if (parameter.IsAborted()) {
      return false;
    }

    std::vector<FileOffset> nodeOffsets;
    StopClock               nodeIndexTimer;

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

  bool Database::GetObjectsAreas(const AreaSearchParameter& parameter,
                                 const TypeSet& areaTypes,
                                 const Magnification& magnification,
                                 double lonMin, double latMin,
                                 double lonMax, double latMax,
                                 std::string& areaOptimizedTime,
                                 std::string& areaIndexTime,
                                 std::string& areasTime,
                                 std::vector<AreaRef>& areas) const
  {
    TypeSet internalAreaTypes(areaTypes);

    if (parameter.IsAborted()) {
      return false;
    }

    std::vector<FileOffset> areaOffsets;
    StopClock               areaOptimizedTimer;

    if (internalAreaTypes.HasTypes()) {
      if (parameter.GetUseLowZoomOptimization() &&
          optimizeAreasLowZoom.HasOptimizations(magnification.GetMagnification())) {
        optimizeAreasLowZoom.GetAreas(lonMin,
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
      if (!areaAreaIndex.GetOffsets(lonMin,
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

    std::sort(offsets.begin(),offsets.end());

    if (parameter.IsAborted()) {
      return false;
    }

    StopClock areasTimer;

    if (!offsets.empty()) {
      if (!GetAreasByOffset(offsets,
                            areas)) {
        std::cout << "Error reading areas in area!" << std::endl;
        return false;
      }
    }

    areasTimer.Stop();
    areasTime=areasTimer.ResultString();

    return !parameter.IsAborted();
  }

  bool Database::GetObjectsWays(const AreaSearchParameter& parameter,
                                const std::vector<TypeSet>& wayTypes,
                                const Magnification& magnification,
                                double lonMin, double latMin,
                                double lonMax, double latMax,
                                std::string& wayOptimizedTime,
                                std::string& wayIndexTime,
                                std::string& waysTime,
                                std::vector<WayRef>& ways) const
  {
    std::vector<TypeSet> internalWayTypes(wayTypes);

    if (parameter.IsAborted()) {
      return false;
    }

    std::vector<FileOffset> offsets;
    StopClock               wayOptimizedTimer;

    if (!internalWayTypes.empty()) {
      if (parameter.GetUseLowZoomOptimization() &&
          optimizeWaysLowZoom.HasOptimizations(magnification.GetMagnification())) {
        optimizeWaysLowZoom.GetWays(lonMin,
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

    StopClock wayIndexTimer;

    if (!internalWayTypes.empty()) {
      if (!areaWayIndex.GetOffsets(lonMin,
                                   latMin,
                                   lonMax,
                                   latMax,
                                   internalWayTypes,
                                   parameter.GetMaximumWays(),
                                   offsets)) {
        std::cout << "Error getting ways Glations from area way index!" << std::endl;
        return false;
      }
    }

    wayIndexTimer.Stop();
    wayIndexTime=wayIndexTimer.ResultString();

    if (parameter.IsAborted()) {
      return false;
    }

    std::sort(offsets.begin(),offsets.end());

    if (parameter.IsAborted()) {
      return false;
    }

    StopClock waysTimer;

    if (!offsets.empty()) {
      if (!GetWaysByOffset(offsets,
                           ways)) {
        std::cout << "Error reading ways in area!" << std::endl;
        return false;
      }
    }

    waysTimer.Stop();
    waysTime=waysTimer.ResultString();

    return !parameter.IsAborted();
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

    if (!IsOpen()) {
      return false;
    }

    nodes.clear();
    ways.clear();
    areas.clear();

    if (parameter.IsAborted()) {
      return false;
    }

    bool nodesSuccess;
    bool waysSuccess;
    bool areasSuccess;

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
      waysSuccess=GetObjectsWays(parameter,
                                 wayTypes,
                                 magnification,
                                 lonMin,
                                 latMin,
                                 lonMax,
                                 latMax,
                                 wayOptimizedTime,
                                 wayIndexTime,
                                 waysTime,
                                 ways);

#pragma omp section
      areasSuccess=GetObjectsAreas(parameter,
                                   areaTypes,
                                   magnification,
                                   lonMin,
                                   latMin,
                                   lonMax,
                                   latMax,
                                   areaOptimizedTime,
                                   areaIndexTime,
                                   areasTime,
                                   areas);
    }

    if (!nodesSuccess ||
        !waysSuccess ||
        !areasSuccess) {
      return false;
    }

    if (debugPerformance) {
      std::cout << "Query: ";
      std::cout << "n " << nodeIndexTime << " ";
      std::cout << "w " << wayIndexTime << " ";
      std::cout << "a " << areaIndexTime << std::endl;

      std::cout << "Load: ";
      std::cout << "n " << nodesTime << " ";
      std::cout << "w " << wayOptimizedTime << "/" << waysTime << " ";
      std::cout << "a " << areaOptimizedTime << "/" << areasTime;
      std::cout << std::endl;
    }

    return true;
  }

  bool Database::GetObjects(double lonMin, double latMin,
                            double lonMax, double latMax,
                            const TypeSet& types,
                            std::vector<NodeRef>& nodes,
                            std::vector<WayRef>& ways,
                            std::vector<AreaRef>& areas) const
  {
    if (!IsOpen()) {
      return false;
    }

    std::vector<TypeSet>    wayTypes;
    std::vector<FileOffset> nodeOffsets;
    std::vector<FileOffset> wayWayOffsets;
    std::vector<FileOffset> wayAreaOffsets;

    nodes.clear();
    ways.clear();
    areas.clear();

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
                                 wayWayOffsets)) {
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
                                  wayAreaOffsets)) {
      std::cout << "Error getting ways and relations from area index!" << std::endl;
    }

    areaAreaIndexTimer.Stop();

    StopClock sortTimer;

    std::sort(nodeOffsets.begin(),nodeOffsets.end());
    std::sort(wayWayOffsets.begin(),wayWayOffsets.end());
    std::sort(wayAreaOffsets.begin(),wayAreaOffsets.end());

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

    if (!GetAreasByOffset(wayAreaOffsets,
                          areas)) {
      std::cout << "Error reading areas in area!" << std::endl;
      return false;
    }

    areasTimer.Stop();

    if (debugPerformance) {
      std::cout << "I/O: ";
      std::cout << "n " << nodeIndexTimer << " ";
      std::cout << "w " << wayIndexTimer << " ";
      std::cout << "a " << areaAreaIndexTimer;
      std::cout << " - ";
      std::cout << "s "  << sortTimer;
      std::cout << " - ";
      std::cout << "n " << nodesTimer << " ";
      std::cout << "w " << waysTimer << " ";
      std::cout << "a " << areasTimer;
      std::cout << std::endl;
    }

    return true;
  }

  bool Database::GetObjects(const std::set<ObjectFileRef>& objects,
                            OSMSCOUT_HASHMAP<FileOffset,NodeRef>& nodesMap,
                            OSMSCOUT_HASHMAP<FileOffset,AreaRef>& areasMap,
                            OSMSCOUT_HASHMAP<FileOffset,WayRef>& waysMap) const
  {
    if (!IsOpen()) {
      return false;
    }

    std::set<FileOffset> nodeOffsets;
    std::set<FileOffset> areaOffsets;
    std::set<FileOffset> wayOffsets;

    for (std::set<ObjectFileRef>::const_iterator o=objects.begin();
        o!=objects.end();
        ++o) {
      ObjectFileRef object(*o);

      switch (object.GetType()) {
      case osmscout::refNode:
        nodeOffsets.insert(object.GetFileOffset());
        break;
      case osmscout::refArea:
        areaOffsets.insert(object.GetFileOffset());
        break;
      case osmscout::refWay:
        wayOffsets.insert(object.GetFileOffset());
        break;
      default:
        break;
      }
    }

    if (!GetNodesByOffset(nodeOffsets,nodesMap) ||
        !GetAreasByOffset(areaOffsets,areasMap) ||
        !GetWaysByOffset(wayOffsets,waysMap)) {
      std::cerr << "Error while resolving locations" << std::endl;
      return false;
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

  bool Database::GetNodesByOffset(const std::set<FileOffset>& offsets,
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

  bool Database::GetNodesByOffset(const std::set<FileOffset>& offsets,
                                  OSMSCOUT_HASHMAP<FileOffset,NodeRef>& dataMap) const
  {
    if (!IsOpen()) {
      return false;
    }

    return nodeDataFile.GetByOffset(offsets,dataMap);
  }

  bool Database::GetAreaByOffset(const FileOffset& offset,
                                 AreaRef& area) const
  {
    if (!IsOpen()) {
      return false;
    }

    std::vector<FileOffset>  offsets;
    std::vector<AreaRef> areas;

    offsets.push_back(offset);

    if (GetAreasByOffset(offsets,areas)) {
      if (!areas.empty()) {
        area=*areas.begin();
        return true;
      }
    }

    return false;
  }

  bool Database::GetAreasByOffset(const std::vector<FileOffset>& offsets,
                                  std::vector<AreaRef>& areas) const
  {
    if (!IsOpen()) {
      return false;
    }

    return areaDataFile.GetByOffset(offsets,areas);
  }

  bool Database::GetAreasByOffset(const std::set<FileOffset>& offsets,
                                  std::vector<AreaRef>& areas) const
  {
    if (!IsOpen()) {
      return false;
    }

    return areaDataFile.GetByOffset(offsets,areas);
  }

  bool Database::GetAreasByOffset(const std::list<FileOffset>& offsets,
                                  std::vector<AreaRef>& areas) const
  {
    if (!IsOpen()) {
      return false;
    }

    return areaDataFile.GetByOffset(offsets,areas);
  }

  bool Database::GetAreasByOffset(const std::set<FileOffset>& offsets,
                                  OSMSCOUT_HASHMAP<FileOffset,AreaRef>& dataMap) const
  {
    if (!IsOpen()) {
      return false;
    }

    return areaDataFile.GetByOffset(offsets,dataMap);
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

  bool Database::GetWaysByOffset(const std::set<FileOffset>& offsets,
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

  bool Database::GetWaysByOffset(const std::set<FileOffset>& offsets,
                                 OSMSCOUT_HASHMAP<FileOffset,WayRef>& dataMap) const
  {
    if (!IsOpen()) {
      return false;
    }

    return wayDataFile.GetByOffset(offsets,dataMap);
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
    areaDataFile.DumpStatistics();
    wayDataFile.DumpStatistics();

    areaAreaIndex.DumpStatistics();
    areaNodeIndex.DumpStatistics();
    areaWayIndex.DumpStatistics();
    cityStreetIndex.DumpStatistics();
    waterIndex.DumpStatistics();
  }
}
