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
#include <cassert>
#include <iostream>

#include <osmscout/TypeConfigLoader.h>

#include <osmscout/util/StopClock.h>

#include <osmscout/private/Math.h>

namespace osmscout {

  DatabaseParameter::DatabaseParameter()
  : areaAreaIndexCacheSize(1000),
    areaNodeIndexCacheSize(1000),
    nodeIndexCacheSize(1000),
    nodeCacheSize(1000),
    wayIndexCacheSize(2000),
    wayCacheSize(8000),
    relationIndexCacheSize(1000),
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

  void DatabaseParameter::SetNodeIndexCacheSize(unsigned long nodeIndexCacheSize)
  {
    this->nodeIndexCacheSize=nodeIndexCacheSize;
  }

  void DatabaseParameter::SetNodeCacheSize(unsigned long nodeCacheSize)
  {
    this->nodeCacheSize=nodeCacheSize;
  }

  void DatabaseParameter::SetWayIndexCacheSize(unsigned long wayIndexCacheSize)
  {
    this->wayIndexCacheSize=wayIndexCacheSize;
  }

  void DatabaseParameter::SetWayCacheSize(unsigned long wayCacheSize)
  {
    this->wayCacheSize=wayCacheSize;
  }

  void DatabaseParameter::SetRelationIndexCacheSize(unsigned long relationIndexCacheSize)
  {
    this->relationIndexCacheSize=relationIndexCacheSize;
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

  unsigned long DatabaseParameter::GetNodeIndexCacheSize() const
  {
    return nodeIndexCacheSize;
  }

  unsigned long DatabaseParameter::GetNodeCacheSize() const
  {
    return nodeCacheSize;
  }

  unsigned long DatabaseParameter::GetWayIndexCacheSize() const
  {
    return wayIndexCacheSize;
  }

  unsigned long DatabaseParameter::GetWayCacheSize() const
  {
    return wayCacheSize;
  }

  unsigned long DatabaseParameter::GetRelationIndexCacheSize() const
  {
    return relationIndexCacheSize;
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
    useLowZoomOptimization(true)
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
     areaAreaIndex(parameter.GetAreaAreaIndexCacheSize()),
     areaNodeIndex(/*parameter.GetAreaNodeIndexCacheSize()*/),
     areaWayIndex(),
     nodeDataFile("nodes.dat",
                  "node.idx",
                  parameter.GetNodeCacheSize(),
                  parameter.GetNodeIndexCacheSize()),
     relationDataFile("relations.dat",
                      "relation.idx",
                      parameter.GetRelationCacheSize(),
                      parameter.GetRelationIndexCacheSize()),
     wayDataFile("ways.dat",
                 "way.idx",
                  parameter.GetWayCacheSize(),
                  parameter.GetWayIndexCacheSize()),
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

    if (!nodeDataFile.Open(path,FileScanner::LowMemRandom,true,FileScanner::LowMemRandom,true)) {
      std::cerr << "Cannot open 'nodes.dat'!" << std::endl;
      delete typeConfig;
      typeConfig=NULL;
      return false;
    }

    if (!wayDataFile.Open(path,FileScanner::LowMemRandom,true,FileScanner::LowMemRandom,true)) {
      std::cerr << "Cannot open 'ways.dat'!" << std::endl;
      delete typeConfig;
      typeConfig=NULL;
      return false;
    }

    if (!relationDataFile.Open(path,FileScanner::LowMemRandom,true,FileScanner::LowMemRandom,true)) {
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
    if (!IsOpen()) {
      return false;
    }

    if (parameter.IsAborted()) {
      return false;
    }

    std::vector<TypeSet>    internalWayTypes(wayTypes);
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

    if (parameter.IsAborted()) {
      return false;
    }

    StopClock nodeIndexTimer;

    if (!areaNodeIndex.GetOffsets(lonMin,latMin,lonMax,latMax,
                                  nodeTypes,
                                  parameter.GetMaximumNodes(),
                                  nodeOffsets)) {
      std::cout << "Error getting nodes from area node index!" << std::endl;
      return false;
    }

    nodeIndexTimer.Stop();

    if (parameter.IsAborted()) {
      return false;
    }

    StopClock wayIndexTimer;

    if (parameter.GetUseLowZoomOptimization() &&
        optimizeLowZoom.HasOptimizations(magnification.GetMagnification())) {
      optimizeLowZoom.GetWays(lonMin,
                              latMin,
                              lonMax,
                              latMax,
                              parameter.GetMaximumWays(),
                              internalWayTypes,
                              ways);

      /* TODO:
      for (size_t i=0; i<wayTypes.size(); i++) {
        std::cout << "Warning: Loading type " << typeConfig->GetTypeInfo(wayTypes[i]).GetName() << " via normal index" << std::endl;
      }*/
    }

    if (parameter.IsAborted()) {
      return false;
    }

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

    if (parameter.IsAborted()) {
      return false;
    }

    wayIndexTimer.Stop();

    StopClock areaAreaIndexTimer;

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

    areaAreaIndexTimer.Stop();

    if (parameter.IsAborted()) {
      return false;
    }

    StopClock nodesTimer;

    std::sort(nodeOffsets.begin(),nodeOffsets.end());
    std::sort(wayWayOffsets.begin(),wayWayOffsets.end());
    std::sort(wayAreaOffsets.begin(),wayAreaOffsets.end());
    std::sort(relationWayOffsets.begin(),relationWayOffsets.end());
    std::sort(relationAreaOffsets.begin(),relationAreaOffsets.end());

    if (!GetNodesByOffset(nodeOffsets,
                          nodes)) {
      std::cout << "Error reading nodes in area!" << std::endl;
      return false;
    }

    nodesTimer.Stop();

    if (parameter.IsAborted()) {
      return false;
    }

    StopClock waysTimer;

    if (!GetWaysByOffset(wayWayOffsets,
                         ways)) {
      std::cout << "Error reading ways in area!" << std::endl;
      return false;
    }

    waysTimer.Stop();

    if (parameter.IsAborted()) {
      return false;
    }

    StopClock areasTimer;

    if (!GetWaysByOffset(wayAreaOffsets,
                         areas)) {
      std::cout << "Error reading areas in area!" << std::endl;
      return false;
    }

    areasTimer.Stop();

    if (parameter.IsAborted()) {
      return false;
    }

    StopClock relationWaysTimer;

    if (!GetRelationsByOffset(relationWayOffsets,
                              relationWays)) {
      std::cout << "Error reading relation ways in area!" << std::endl;
      return false;
    }

    relationWaysTimer.Stop();

    if (parameter.IsAborted()) {
      return false;
    }

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
      std::cout << "n " << nodesTimer << " ";
      std::cout << "w " << waysTimer << "/" << relationWaysTimer << " ";
      std::cout << "a " << areasTimer << "/" << relationAreasTimer;
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

  bool Database::GetNode(const Id& id,
                         NodeRef& node) const
  {
    if (!IsOpen()) {
      return false;
    }

    std::vector<Id>      ids;
    std::vector<NodeRef> nodes;

    ids.push_back(id);

    if (GetNodes(ids,nodes)) {
      if (!nodes.empty()) {
        node=*nodes.begin();
        return true;
      }
    }

    return false;
  }

  bool Database::GetNodes(const std::vector<Id>& ids,
                          std::vector<NodeRef>& nodes) const
  {
    if (!IsOpen()) {
      return false;
    }

    return nodeDataFile.Get(ids,nodes);
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

  bool Database::GetWay(const Id& id,
                        WayRef& way) const
  {
    if (!IsOpen()) {
      return false;
    }

    std::vector<Id>     ids;
    std::vector<WayRef> ways;

    ids.push_back(id);

    if (GetWays(ids,ways)) {
      if (!ways.empty()) {
        way=*ways.begin();
        return true;
      }
    }

    return false;
  }

  bool Database::GetWays(const std::vector<Id>& ids,
                         std::vector<WayRef>& ways) const
  {
    if (!IsOpen()) {
      return false;
    }

    return wayDataFile.Get(ids,ways);
  }

  bool Database::GetWays(const std::set<Id>& ids,
                         std::vector<WayRef>& ways) const
  {
    if (!IsOpen()) {
      return false;
    }

    return wayDataFile.Get(ids,ways);
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

  bool Database::GetRelation(const Id& id,
                             RelationRef& relation) const
  {
    if (!IsOpen()) {
      return false;
    }

    std::vector<Id>          ids;
    std::vector<RelationRef> relations;

    ids.push_back(id);

    if (GetRelations(ids,relations)) {
      if (!relations.empty()) {
        relation=*relations.begin();
        return true;
      }
    }

    return false;
  }

  bool Database::GetRelations(const std::vector<Id>& ids,
                              std::vector<RelationRef>& relations) const
  {
    if (!IsOpen()) {
      return false;
    }

    return relationDataFile.Get(ids,relations);
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
