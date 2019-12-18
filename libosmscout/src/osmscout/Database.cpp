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

#if _OPENMP
#include <omp.h>
#endif

#include <osmscout/system/Assert.h>
#include <osmscout/system/Math.h>

#include <osmscout/util/Geometry.h>
#include <osmscout/util/Logger.h>
#include <osmscout/util/StopClock.h>

namespace osmscout {

  DatabaseParameter::DatabaseParameter()
  : areaAreaIndexCacheSize(5000),
    nodeDataCacheSize(5000),
    wayDataCacheSize(10000),
    areaDataCacheSize(5000),
    routerDataMMap(true),
    nodesDataMMap(true),
    areasDataMMap(true),
    waysDataMMap(true),
    optimizeLowZoomMMap(true),
    indexMMap(true)
  {
    // no code
  }

  void DatabaseParameter::SetAreaAreaIndexCacheSize(unsigned long areaAreaIndexCacheSize)
  {
    this->areaAreaIndexCacheSize=areaAreaIndexCacheSize;
  }

  void DatabaseParameter::SetNodeDataCacheSize(unsigned long size)
  {
    this->nodeDataCacheSize=size;
  }

  void DatabaseParameter::SetWayDataCacheSize(unsigned long  size)
  {
    this->wayDataCacheSize=size;
  }

  void DatabaseParameter::SetAreaDataCacheSize(unsigned long  size)
  {
    this->areaDataCacheSize=size;
  }

  void DatabaseParameter::SetRouterDataMMap(bool mmap)
  {
    routerDataMMap=mmap;
  }

  void DatabaseParameter::SetNodesDataMMap(bool mmap)
  {
    nodesDataMMap=mmap;
  }

  void DatabaseParameter::SetAreasDataMMap(bool mmap)
  {
    areasDataMMap=mmap;
  }

  void DatabaseParameter::SetWaysDataMMap(bool mmap)
  {
    waysDataMMap=mmap;
  }

  void DatabaseParameter::SetOptimizeLowZoomMMap(bool mmap)
  {
    optimizeLowZoomMMap=mmap;
  }

  void DatabaseParameter::SetIndexMMap(bool mmap)
  {
    indexMMap=mmap;
  }

  unsigned long DatabaseParameter::GetAreaAreaIndexCacheSize() const
  {
    return areaAreaIndexCacheSize;
  }

  unsigned long DatabaseParameter::GetNodeDataCacheSize() const
  {
    return nodeDataCacheSize;
  }

  unsigned long DatabaseParameter::GetWayDataCacheSize() const
  {
    return wayDataCacheSize;
  }

  unsigned long DatabaseParameter::GetAreaDataCacheSize() const
  {
    return areaDataCacheSize;
  }

  bool DatabaseParameter::GetRouterDataMMap() const
  {
    return routerDataMMap;
  }

  bool DatabaseParameter::GetNodesDataMMap() const
  {
    return nodesDataMMap;
  }

  bool DatabaseParameter::GetAreasDataMMap() const
  {
    return areasDataMMap;
  }

  bool DatabaseParameter::GetWaysDataMMap() const
  {
    return waysDataMMap;
  }

  bool DatabaseParameter::GetOptimizeLowZoomMMap() const
  {
    return optimizeLowZoomMMap;
  }

  bool DatabaseParameter::GetIndexMMap() const
  {
    return indexMMap;
  }

  NodeRegionSearchResultEntry::NodeRegionSearchResultEntry(const NodeRef &node,
                                                           const Distance &distance)
  : node(node),
    distance(distance)
  {
  }

  WayRegionSearchResultEntry::WayRegionSearchResultEntry(const WayRef &way,
                                                         const Distance &distance,
                                                         const GeoCoord &closestPoint)
  : way(way),
    distance(distance),
    closestPoint(closestPoint)
  {
  }

  AreaRegionSearchResultEntry::AreaRegionSearchResultEntry(const AreaRef& area,
                                                           const Distance &distance,
                                                           const GeoCoord& closestPoint,
                                                           bool inArea)
  : area(area),
    distance(distance),
    closestPoint(closestPoint),
    inArea(inArea)
  {
  }

  Database::Database(const DatabaseParameter& parameter)
   : parameter(parameter),
     isOpen(false)
  {
    log.Debug() << "Database::Database()";
  }

  Database::~Database()
  {
    log.Debug() << "Database::~Database()";

    if (IsOpen()) {
      Close();
    }
  }

  bool Database::Open(const std::string& path)
  {
    assert(!path.empty());

    this->path=path;

    typeConfig=std::make_shared<TypeConfig>();

    if (!typeConfig->LoadFromDataFile(path)) {
      log.Error() << "Cannot load 'types.dat'!";
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
    boundingBoxDataFile=nullptr;

    if (nodeDataFile &&
        nodeDataFile->IsOpen()) {
      nodeDataFile->Close();
      nodeDataFile=nullptr;
    }

    if (areaDataFile &&
        areaDataFile->IsOpen()) {
      areaDataFile->Close();
      areaDataFile=nullptr;
    }

    if (wayDataFile &&
        wayDataFile->IsOpen()) {
      wayDataFile->Close();
      wayDataFile=nullptr;
    }

    if (areaNodeIndex) {
      areaNodeIndex->Close();
      areaNodeIndex=nullptr;
    }

    if (areaAreaIndex) {
      areaAreaIndex->Close();
      areaAreaIndex=nullptr;
    }

    if (areaWayIndex) {
      areaWayIndex->Close();
      areaWayIndex=nullptr;
    }

    if (locationIndex) {
      locationIndex=nullptr;
    }

    if (waterIndex) {
      waterIndex->Close();
      waterIndex=nullptr;
    }

    if (optimizeWaysLowZoom) {
      optimizeWaysLowZoom->Close();
      optimizeWaysLowZoom=nullptr;
    }

    if (optimizeAreasLowZoom) {
      optimizeAreasLowZoom->Close();
      optimizeAreasLowZoom=nullptr;
    }

    isOpen=false;
  }

  std::string Database::GetPath() const
  {
    return path;
  }

  TypeConfigRef Database::GetTypeConfig() const
  {
    return typeConfig;
  }

  BoundingBoxDataFileRef Database::GetBoundingBoxDataFile() const
  {
    std::lock_guard<std::mutex> guard(boundingBoxDataFileMutex);

    if (!IsOpen()) {
      return nullptr;
    }

    if (!boundingBoxDataFile) {
      boundingBoxDataFile=std::make_shared<BoundingBoxDataFile>();
    }

    if (!boundingBoxDataFile->IsLoaded()) {
      StopClock timer;

      if (!boundingBoxDataFile->Load(path)) {
        log.Error() << "Cannot open '" << BoundingBoxDataFile::BOUNDINGBOX_DAT << "'!";
        return nullptr;
      }

      timer.Stop();

      log.Debug() << "Opening BoundingBoxDataFile: " << timer.ResultString();
    }

    return boundingBoxDataFile;
  }

  NodeDataFileRef Database::GetNodeDataFile() const
  {
    std::lock_guard<std::mutex> guard(nodeDataFileMutex);

    if (!IsOpen()) {
      return nullptr;
    }

    if (!nodeDataFile) {
      nodeDataFile=std::make_shared<NodeDataFile>(parameter.GetNodeDataCacheSize());
    }

    if (!nodeDataFile->IsOpen()) {
      StopClock timer;

      if (!nodeDataFile->Open(typeConfig,
                              path,
                              parameter.GetNodesDataMMap())) {
        log.Error() << "Cannot open 'nodes.dat'!";
        return nullptr;
      }

      timer.Stop();

      log.Debug() << "Opening NodeDataFile: " << timer.ResultString();
    }

    return nodeDataFile;
  }

  AreaDataFileRef Database::GetAreaDataFile() const
  {
    std::lock_guard<std::mutex> guard(areaDataFileMutex);

    if (!IsOpen()) {
      return nullptr;
    }

    if (!areaDataFile) {
      areaDataFile=std::make_shared<AreaDataFile>(parameter.GetAreaDataCacheSize());
    }

    if (!areaDataFile->IsOpen()) {
      StopClock timer;

      if (!areaDataFile->Open(typeConfig,
                              path,
                              parameter.GetAreasDataMMap())) {
        log.Error() << "Cannot open 'areas.dat'!";
        return nullptr;
      }

      timer.Stop();

      log.Debug() << "Opening AreaDataFile: " << timer.ResultString();
    }

    return areaDataFile;
  }

  WayDataFileRef Database::GetWayDataFile() const
  {
    std::lock_guard<std::mutex> guard(wayDataFileMutex);

    if (!IsOpen()) {
      return nullptr;
    }

    if (!wayDataFile) {
      wayDataFile=std::make_shared<WayDataFile>(parameter.GetWayDataCacheSize());
    }

    if (!wayDataFile->IsOpen()) {
      StopClock timer;

      if (!wayDataFile->Open(typeConfig,
                             path,
                             parameter.GetWaysDataMMap())) {
        log.Error() << "Cannot open 'ways.dat'!";
        return nullptr;
      }

      timer.Stop();

      log.Debug() << "Opening WayDataFile: " << timer.ResultString();
    }

    return wayDataFile;
  }

  AreaNodeIndexRef Database::GetAreaNodeIndex() const
  {
    std::lock_guard<std::mutex> guard(areaNodeIndexMutex);

    if (!IsOpen()) {
      return nullptr;
    }

    if (!areaNodeIndex) {
      areaNodeIndex=std::make_shared<AreaNodeIndex>();

      StopClock timer;

      if (!areaNodeIndex->Open(path, parameter.GetIndexMMap())) {
        log.Error() << "Cannot load area node index!";
        areaNodeIndex=nullptr;

        return nullptr;
      }

      timer.Stop();

      log.Debug() << "Opening AreaNodeIndex: " << timer.ResultString();
    }

    return areaNodeIndex;
  }

  AreaAreaIndexRef Database::GetAreaAreaIndex() const
  {
    std::lock_guard<std::mutex> guard(areaAreaIndexMutex);

    if (!IsOpen()) {
      return nullptr;
    }

    if (!areaAreaIndex) {
      areaAreaIndex=std::make_shared<AreaAreaIndex>(parameter.GetAreaAreaIndexCacheSize());

      StopClock timer;

      if (!areaAreaIndex->Open(path, parameter.GetIndexMMap())) {
        log.Error() << "Cannot load area area index!";
        areaAreaIndex=nullptr;

        return nullptr;
      }

      timer.Stop();

      log.Debug() << "Opening AreaAreaIndex: " << timer.ResultString();
    }

    return areaAreaIndex;
  }

  AreaWayIndexRef Database::GetAreaWayIndex() const
  {
    std::lock_guard<std::mutex> guard(areaWayIndexMutex);

    if (!IsOpen()) {
      return nullptr;
    }

    if (!areaWayIndex) {
      areaWayIndex=std::make_shared<AreaWayIndex>();

      StopClock timer;

      if (!areaWayIndex->Open(typeConfig,
                              path,
                              parameter.GetIndexMMap())) {
        log.Error() << "Cannot load area way index!";
        areaWayIndex=nullptr;

        return nullptr;
      }

      timer.Stop();

      log.Debug() << "Opening AreaWayIndex: " << timer.ResultString();
    }

    return areaWayIndex;
  }

  LocationIndexRef Database::GetLocationIndex() const
  {
    std::lock_guard<std::mutex> guard(locationIndexMutex);

    if (!IsOpen()) {
      return nullptr;
    }

    if (!locationIndex) {
      locationIndex=std::make_shared<LocationIndex>();

      StopClock timer;

      if (!locationIndex->Load(path, parameter.GetIndexMMap())) {
        log.Error() << "Cannot load location index!";
        locationIndex=nullptr;

        return nullptr;
      }

      timer.Stop();

      log.Debug() << "Opening LocationIndex: " << timer.ResultString();
    }

    return locationIndex;
  }

  WaterIndexRef Database::GetWaterIndex() const
  {
    std::lock_guard<std::mutex> guard(waterIndexMutex);

    if (!IsOpen()) {
      return nullptr;
    }

    if (!waterIndex) {
      waterIndex=std::make_shared<WaterIndex>();

      StopClock timer;

      if (!waterIndex->Open(path, parameter.GetIndexMMap())) {
        log.Error() << "Cannot load water index!";
        waterIndex=nullptr;

        return nullptr;
      }

      timer.Stop();

      log.Debug() << "Opening WaterIndex: " << timer.ResultString();
    }

    return waterIndex;
  }

  OptimizeAreasLowZoomRef Database::GetOptimizeAreasLowZoom() const
  {
    std::lock_guard<std::mutex> guard(optimizeAreasMutex);

    if (!IsOpen()) {
      return nullptr;
    }

    if (!optimizeAreasLowZoom) {
      optimizeAreasLowZoom=std::make_shared<OptimizeAreasLowZoom>();

      StopClock timer;

      if (!optimizeAreasLowZoom->Open(typeConfig,
                                      path,
                                      parameter.GetOptimizeLowZoomMMap())) {
        log.Error() << "Cannot load optimize areas low zoom index!";
        optimizeAreasLowZoom=nullptr;

        return nullptr;
      }


      timer.Stop();

      log.Debug() << "Opening OptimizeAreasLowZoom: " << timer.ResultString();
    }

    return optimizeAreasLowZoom;
  }

  OptimizeWaysLowZoomRef Database::GetOptimizeWaysLowZoom() const
  {
    std::lock_guard<std::mutex> guard(optimizeWaysMutex);

    if (!optimizeWaysLowZoom) {
      optimizeWaysLowZoom=std::make_shared<OptimizeWaysLowZoom>();

      StopClock timer;

      if (!optimizeWaysLowZoom->Open(typeConfig,
                                     path,
                                     parameter.GetOptimizeLowZoomMMap())) {
        log.Error() << "Cannot load optimize areas low zoom index!";
        optimizeWaysLowZoom=nullptr;

        return nullptr;
      }

      timer.Stop();

      log.Debug() << "Opening OptimizeWaysLowZoom: " << timer.ResultString();
    }

    return optimizeWaysLowZoom;
  }

  bool Database::GetBoundingBox(GeoBox& boundingBox) const
  {
    BoundingBoxDataFileRef boundingBoxDataFile=GetBoundingBoxDataFile();

    if (!boundingBoxDataFile) {
      return false;
    }

    boundingBox=boundingBoxDataFile->GetBoundingBox();

    return true;
  }

  bool Database::GetNodeByOffset(const FileOffset& offset,
                                 NodeRef& node) const
  {
    NodeDataFileRef nodeDataFile=GetNodeDataFile();

    if (!nodeDataFile) {
      return false;
    }

    StopClock time;

    bool result=nodeDataFile->GetByOffset(offset,node);

    time.Stop();

    if (time.GetMilliseconds()>100) {
      log.Warn() << "Retrieving 1 node by offset took " << time.ResultString();
    }

    return result;
  }

  bool Database::GetNodesByOffset(const std::vector<FileOffset>& offsets,
                                  std::vector<NodeRef>& nodes) const
  {
    NodeDataFileRef nodeDataFile=GetNodeDataFile();

    if (!nodeDataFile) {
      return false;
    }

    StopClock time;

    bool result=nodeDataFile->GetByOffset(offsets.begin(),offsets.end(),offsets.size(),nodes);

    time.Stop();

    if (time.GetMilliseconds()>100) {
      log.Warn() << "Retrieving " << nodes.size() << " nodes by offset took " << time.ResultString();
    }

    return result;
  }

  bool Database::GetNodesByOffset(const std::vector<FileOffset>& offsets,
                                  const GeoBox& boundingBox,
                                  std::vector<NodeRef>& nodes) const
  {
    NodeDataFileRef nodeDataFile=GetNodeDataFile();

    if (!nodeDataFile) {
      return false;
    }

    StopClock time;

    bool result=nodeDataFile->GetByOffset(offsets.begin(),
                                          offsets.end(),
                                          offsets.size(),
                                          boundingBox,
                                          nodes);

    time.Stop();

    if (time.GetMilliseconds()>100) {
      log.Warn() << "Retrieving " << nodes.size() << " nodes by offset took " << time.ResultString();
    }

    return result;
  }

  bool Database::GetNodesByOffset(const std::set<FileOffset>& offsets,
                                  std::vector<NodeRef>& nodes) const
  {
    NodeDataFileRef nodeDataFile=GetNodeDataFile();

    if (!nodeDataFile) {
      return false;
    }

    StopClock time;

    bool result=nodeDataFile->GetByOffset(offsets.begin(),offsets.end(),offsets.size(),nodes);

    time.Stop();

    if (time.GetMilliseconds()>100) {
      log.Warn() << "Retrieving " << nodes.size() << " nodes by offset took " << time.ResultString();
    }

    return result;
  }

  bool Database::GetNodesByOffset(const std::list<FileOffset>& offsets,
                                  std::vector<NodeRef>& nodes) const
  {
    NodeDataFileRef nodeDataFile=GetNodeDataFile();

    if (!nodeDataFile) {
      return false;
    }

    StopClock time;

    bool result=nodeDataFile->GetByOffset(offsets.begin(),offsets.end(),offsets.size(),nodes);

    time.Stop();

    if (time.GetMilliseconds()>100) {
      log.Warn() << "Retrieving " << nodes.size() << " nodes by offset took " << time.ResultString();
    }

    return result;
  }

  bool Database::GetNodesByOffset(const std::set<FileOffset>& offsets,
                                  std::unordered_map<FileOffset,NodeRef>& dataMap) const
  {
    NodeDataFileRef nodeDataFile=GetNodeDataFile();

    if (!nodeDataFile) {
      return false;
    }

    StopClock time;

    bool result=nodeDataFile->GetByOffset(offsets.begin(),
                                          offsets.end(),
                                          offsets.size(),
                                          dataMap);

    time.Stop();

    if (time.GetMilliseconds()>100) {
      log.Warn() << "Retrieving " << dataMap.size() << " nodes by offset took " << time.ResultString();
    }

    return result;
  }

  bool Database::GetAreaByOffset(const FileOffset& offset,
                                 AreaRef& area) const
  {
    AreaDataFileRef areaDataFile=GetAreaDataFile();

    if (!areaDataFile) {
      return false;
    }

    StopClock time;

    bool result=areaDataFile->GetByOffset(offset,area);

    time.Stop();

    if (time.GetMilliseconds()>100) {
      log.Warn() << "Retrieving 1 area by offset took " << time.ResultString();
    }

    return result;
  }

  bool Database::GetAreasByOffset(const std::vector<FileOffset>& offsets,
                                  std::vector<AreaRef>& areas) const
  {
    AreaDataFileRef areaDataFile=GetAreaDataFile();

    if (!areaDataFile) {
      return false;
    }

    StopClock time;

    bool result=areaDataFile->GetByOffset(offsets.begin(),offsets.end(),offsets.size(),areas);

    if (time.GetMilliseconds()>100) {
      log.Warn() << "Retrieving " << areas.size() << " areas by offset took " << time.ResultString();
    }

    return result;
  }

  bool Database::GetAreasByOffset(const std::set<FileOffset>& offsets,
                                  std::vector<AreaRef>& areas) const
  {
    AreaDataFileRef areaDataFile=GetAreaDataFile();

    if (!areaDataFile) {
      return false;
    }

    StopClock time;

    bool result=areaDataFile->GetByOffset(offsets.begin(),offsets.end(),offsets.size(),areas);

    if (time.GetMilliseconds()>100) {
      log.Warn() << "Retrieving " << areas.size() << " areas by offset took " << time.ResultString();
    }

    return result;
  }

  bool Database::GetAreasByOffset(const std::list<FileOffset>& offsets,
                                  std::vector<AreaRef>& areas) const
  {
    AreaDataFileRef areaDataFile=GetAreaDataFile();

    if (!areaDataFile) {
      return false;
    }

    StopClock time;

    bool result=areaDataFile->GetByOffset(offsets.begin(),offsets.end(),offsets.size(),areas);

    if (time.GetMilliseconds()>100) {
      log.Warn() << "Retrieving " << areas.size() << " areas by offset took " << time.ResultString();
    }

    return result;
  }

  bool Database::GetAreasByOffset(const std::set<FileOffset>& offsets,
                                  std::unordered_map<FileOffset,AreaRef>& dataMap) const
  {
    AreaDataFileRef areaDataFile=GetAreaDataFile();

    if (!areaDataFile) {
      return false;
    }

    StopClock time;

    bool result=areaDataFile->GetByOffset(offsets.begin(),
                                          offsets.end(),
                                          offsets.size(),
                                          dataMap);

    if (time.GetMilliseconds()>100) {
      log.Warn() << "Retrieving " << dataMap.size() << " areas by offset took " << time.ResultString();
    }

    return result;
  }

  bool Database::GetAreasByBlockSpan(const DataBlockSpan& span,
                                     std::vector<AreaRef>& area) const
  {
    AreaDataFileRef areaDataFile=GetAreaDataFile();

    if (!areaDataFile) {
      return false;
    }

    return areaDataFile->GetByBlockSpan(span,area);
  }

  bool Database::GetAreasByBlockSpans(const std::vector<DataBlockSpan>& spans,
                                      std::vector<AreaRef>& areas) const
  {
    AreaDataFileRef areaDataFile=GetAreaDataFile();

    if (!areaDataFile) {
      return false;
    }

    return areaDataFile->GetByBlockSpans(spans.begin(),
                                         spans.end(),
                                         areas);
  }

  bool Database::GetWayByOffset(const FileOffset& offset,
                                WayRef& way) const
  {
    WayDataFileRef wayDataFile=GetWayDataFile();

    if (!wayDataFile) {
      return false;
    }

    StopClock time;

    bool result=wayDataFile->GetByOffset(offset,way);

    if (time.GetMilliseconds()>100) {
      log.Warn() << "Retrieving 1 way by offset took " << time.ResultString();
    }

    return result;
  }

  bool Database::GetWaysByOffset(const std::vector<FileOffset>& offsets,
                                 std::vector<WayRef>& ways) const
  {
    WayDataFileRef wayDataFile=GetWayDataFile();

    if (!wayDataFile) {
      return false;
    }

    StopClock time;

    bool result=wayDataFile->GetByOffset(offsets.begin(),offsets.end(),offsets.size(),ways);

    if (time.GetMilliseconds()>100) {
      log.Warn() << "Retrieving " << ways.size() << " ways by offset took " << time.ResultString();
    }

    return result;
  }

  bool Database::GetWaysByOffset(const std::set<FileOffset>& offsets,
                                 std::vector<WayRef>& ways) const
  {
    WayDataFileRef wayDataFile=GetWayDataFile();

    if (!wayDataFile) {
      return false;
    }

    StopClock time;

    bool result=wayDataFile->GetByOffset(offsets.begin(),offsets.end(),offsets.size(),ways);

    if (time.GetMilliseconds()>100) {
      log.Warn() << "Retrieving " << ways.size() << " ways by offset took " << time.ResultString();
    }

    return result;
  }

  bool Database::GetWaysByOffset(const std::list<FileOffset>& offsets,
                                 std::vector<WayRef>& ways) const
  {
    WayDataFileRef wayDataFile=GetWayDataFile();

    if (!wayDataFile) {
      return false;
    }

    StopClock time;

    bool result=wayDataFile->GetByOffset(offsets.begin(),offsets.end(),offsets.size(),ways);

    if (time.GetMilliseconds()>100) {
      log.Warn() << "Retrieving " << ways.size() << " ways by offset took " << time.ResultString();
    }

    return result;
  }

  bool Database::GetWaysByOffset(const std::set<FileOffset>& offsets,
                                 std::unordered_map<FileOffset,WayRef>& dataMap) const
  {
    WayDataFileRef wayDataFile=GetWayDataFile();

    if (!wayDataFile) {
      return false;
    }

    StopClock time;

    bool result=wayDataFile->GetByOffset(offsets.begin(),
                                         offsets.end(),
                                         offsets.size(),
                                         dataMap);

    if (time.GetMilliseconds()>100) {
      log.Warn() << "Retrieving " << dataMap.size() << " ways by offset took " << time.ResultString();
    }

    return result;
  }

  void Database::DumpStatistics()
  {
    if (areaAreaIndex) {
      areaAreaIndex->DumpStatistics();
    }

    if (locationIndex) {
      locationIndex->DumpStatistics();
    }

    if (waterIndex) {
      waterIndex->DumpStatistics();
    }
  }

  NodeRegionSearchResult Database::LoadNodesInRadius(const GeoCoord& location,
                                                     const TypeInfoSet& types,
                                                     Distance maxDistance)
  {
    AreaNodeIndexRef areaNodeIndex=GetAreaNodeIndex();

    if (!areaNodeIndex) {
      throw UninitializedException("AreaNodeIndex");
    }

    NodeRegionSearchResult  result;
    GeoBox                  box=GeoBox::BoxByCenterAndRadius(location,
                                                             maxDistance);
    std::vector<FileOffset> offsets;
    TypeInfoSet             loadedAddressTypes;

    if (!areaNodeIndex->GetOffsets(box,
                                   types,
                                   offsets,
                                   loadedAddressTypes)) {
      throw IOException(areaNodeIndex->GetFilename(),
                        "Error while reading offsets");
    }

    if (offsets.empty()) {
      return result;
    }

    std::vector<NodeRef> nodes;

    if (!GetNodesByOffset(offsets,
                          nodes)) {
      throw IOException(nodeDataFile->GetFilename(),
                        "Error while reading nodes");
    }

    if (nodes.empty()) {
      return result;
    }

    for (const auto& node : nodes) {
      Distance distance=GetEllipsoidalDistance(location,
                                               node.get()->GetCoords());
      if (distance<=maxDistance) {
        result.nodeResults.push_back(NodeRegionSearchResultEntry(node,
                                                                 distance));
      }
    }

    return result;
  }

  WayRegionSearchResult Database::LoadWaysInRadius(const GeoCoord& location,
                                                   const TypeInfoSet& types,
                                                   Distance maxDistance)
  {
    AreaWayIndexRef areaWayIndex=GetAreaWayIndex();

    if (!areaWayIndex) {
      throw UninitializedException("AreaWayIndex");
    }

    WayRegionSearchResult   result;
    GeoBox                  box=GeoBox::BoxByCenterAndRadius(location,
                                                             maxDistance);
    std::vector<FileOffset> offsets;
    TypeInfoSet             loadedAddressTypes;

    if (!areaWayIndex->GetOffsets(box,
                                  types,
                                  offsets,
                                  loadedAddressTypes)) {
      throw IOException(areaWayIndex->GetFilename(),
                        "Error while reading offsets");
    }

    if (offsets.empty()) {
      return result;
    }

    std::vector<WayRef> ways;

    if (!GetWaysByOffset(offsets,
                         ways)) {
      throw IOException(areaDataFile->GetFilename(),
                        "Error while reading ways");
    }

    if (ways.empty()) {
      return result;
    }

    for (const auto& way : ways) {
      Distance distance=Distance::Max();
      GeoCoord closestPoint;

      for (size_t i=1; i<way->nodes.size(); i++) {
        Distance currentDistance;
        GeoCoord a;
        GeoCoord b;
        GeoCoord intersection;

        a=way->nodes[i-1].GetCoord();
        b=way->nodes[i].GetCoord();

        double newDistance=CalculateDistancePointToLineSegment(location,
                                                               a,
                                                               b,
                                                               intersection);

        if (!std::isfinite(newDistance)) {
          continue;
        }

        currentDistance=GetEllipsoidalDistance(location,
                                               intersection);

        if (currentDistance<distance) {
          distance=currentDistance;
          closestPoint=intersection;
        }
      }

      if (distance<=maxDistance) {
        result.wayResults.push_back(WayRegionSearchResultEntry(way,
                                                               distance,
                                                               closestPoint));
      }
    }

    return result;
  }

  AreaRegionSearchResult Database::LoadAreasInRadius(const GeoCoord& location,
                                                     const TypeInfoSet& types,
                                                     Distance maxDistance)
  {
    AreaAreaIndexRef areaAreaIndex=GetAreaAreaIndex();

    if (!areaAreaIndex) {
      throw UninitializedException("AreaAreaIndex");
    }

    AreaRegionSearchResult     result;
    GeoBox                     box=GeoBox::BoxByCenterAndRadius(location,
                                                                maxDistance);
    std::vector<DataBlockSpan> areaSpans;
    TypeInfoSet                loadedTypes;

    if (!areaAreaIndex->GetAreasInArea(*typeConfig,
                                       box,
                                       std::numeric_limits<size_t>::max(),
                                       types,
                                       areaSpans,
                                       loadedTypes)) {
      throw IOException(areaAreaIndex->GetFilename(),
                        "Error while reading offsets");
    }

    if (areaSpans.empty()) {
      return result;
    }

    std::vector<AreaRef> areas;

    if (!GetAreasByBlockSpans(areaSpans,
                              areas)) {
      throw IOException(areaDataFile->GetFilename(),
                        "Error while reading areas");
    }

    if (areas.empty()) {
      return result;
    }

    for (const auto& area : areas) {
      Distance distance=Distance::Max();
      GeoCoord closestPoint(0.0,0.0);
      bool     stop=false;
      bool     inArea=false;

      for (const auto& ring : area->rings) {
        if (stop) {
          break;
        }

        if (ring.IsTopOuter()) {
          if (IsCoordInArea(location,
                            ring.nodes)) {
            distance=Distance::Of<Meter>(0.0);
            inArea=true;
            stop=true;
            break;
          }

          for (size_t i=0; i<ring.nodes.size(); i++) {
            Distance currentDistance;
            GeoCoord a;
            GeoCoord b;
            GeoCoord intersection;

            if (i>0) {
              a=ring.nodes[i-1].GetCoord();
              b=ring.nodes[i].GetCoord();
            }
            else {
              a=ring.nodes[ring.nodes.size()-1].GetCoord();
              b=ring.nodes[i].GetCoord();
            }

            double newDistance=CalculateDistancePointToLineSegment(location,
                                                                   a,
                                                                   b,
                                                                   intersection);

            if (!std::isfinite(newDistance)) {
              continue;
            }

            currentDistance=GetEllipsoidalDistance(location,
                                                   intersection);

            if (currentDistance<distance) {
              distance=currentDistance;
              closestPoint=intersection;
            }
          }
        }
      }

      if (distance<=maxDistance){
        result.areaResults.push_back(AreaRegionSearchResultEntry(area,
                                                                 distance,
                                                                 closestPoint,
                                                                 inArea));
      }
    }

    return result;
  }

  NodeRegionSearchResult Database::LoadNodesInArea(const TypeInfoSet& types,
                                                   const GeoBox& boundingBox)
  {
    AreaNodeIndexRef areaNodeIndex=GetAreaNodeIndex();

    if (!areaNodeIndex) {
      throw UninitializedException("AreaNodeIndex");
    }

    NodeRegionSearchResult  result;
    std::vector<FileOffset> offsets;
    TypeInfoSet             loadedAddressTypes;
    GeoCoord                center=boundingBox.GetCenter();

    if (!areaNodeIndex->GetOffsets(boundingBox,
                                   types,
                                   offsets,
                                   loadedAddressTypes)) {
      throw IOException(areaNodeIndex->GetFilename(),
                        "Error while reading offsets");
    }

    if (offsets.empty()) {
      return result;
    }

    std::vector<NodeRef> nodes;

    if (!GetNodesByOffset(offsets,
                          nodes)) {
      throw IOException(nodeDataFile->GetFilename(),
                        "Error while reading nodes");
    }

    if (nodes.empty()) {
      return result;
    }

    for (const auto& node : nodes) {
      Distance distance=GetEllipsoidalDistance(center,
                                               node.get()->GetCoords());

      result.nodeResults.push_back(NodeRegionSearchResultEntry(node,
                                                               distance));
    }

    return result;
  }

  WayRegionSearchResult Database::LoadWaysInArea(const TypeInfoSet& types,
                                                 const GeoBox& boundingBox)
  {
    AreaWayIndexRef areaWayIndex=GetAreaWayIndex();

    if (!areaWayIndex) {
      throw UninitializedException("AreaWayIndex");
    }

    WayRegionSearchResult   result;
    std::vector<FileOffset> offsets;
    TypeInfoSet             loadedAddressTypes;
    GeoCoord                center=boundingBox.GetCenter();

    if (!areaWayIndex->GetOffsets(boundingBox,
                                  types,
                                  offsets,
                                  loadedAddressTypes)) {
      throw IOException(areaWayIndex->GetFilename(),
                        "Error while reading offsets");
    }

    if (offsets.empty()) {
      return result;
    }

    std::vector<WayRef> ways;

    if (!GetWaysByOffset(offsets,
                         ways)) {
      throw IOException(areaDataFile->GetFilename(),
                        "Error while reading ways");
    }

    if (ways.empty()) {
      return result;
    }

    for (const auto& way : ways) {
      Distance distance=Distance::Max();
      GeoCoord closestPoint;

      for (size_t i=1; i<way->nodes.size(); i++) {
        Distance currentDistance;
        GeoCoord a;
        GeoCoord b;
        GeoCoord intersection;

        a=way->nodes[i-1].GetCoord();
        b=way->nodes[i].GetCoord();

        double newDistance=CalculateDistancePointToLineSegment(center,
                                                               a,
                                                               b,
                                                               intersection);

        if (!std::isfinite(newDistance)) {
          continue;
        }

        currentDistance=GetEllipsoidalDistance(center,
                                               intersection);

        if (currentDistance<distance) {
          distance=currentDistance;
          closestPoint=intersection;
        }
      }

      result.wayResults.push_back(WayRegionSearchResultEntry(way,
                                                             distance,
                                                             closestPoint));
    }

    return result;
  }

  AreaRegionSearchResult Database::LoadAreasInArea(const TypeInfoSet& types,
                                                   const GeoBox& boundingBox)
  {
    AreaAreaIndexRef areaAreaIndex=GetAreaAreaIndex();

    if (!areaAreaIndex) {
      throw UninitializedException("AreaAreaIndex");
    }

    AreaRegionSearchResult     result;
    std::vector<DataBlockSpan> areaSpans;
    TypeInfoSet                loadedTypes;
    GeoCoord                   center=boundingBox.GetCenter();

    if (!areaAreaIndex->GetAreasInArea(*typeConfig,
                                       boundingBox,
                                       std::numeric_limits<size_t>::max(),
                                       types,
                                       areaSpans,
                                       loadedTypes)) {
      throw IOException(areaAreaIndex->GetFilename(),
                        "Error while reading offsets");
    }

    if (areaSpans.empty()) {
      return result;
    }

    std::vector<AreaRef> areas;

    if (!GetAreasByBlockSpans(areaSpans,
                              areas)) {
      throw IOException(areaDataFile->GetFilename(),
                        "Error while reading areas");
    }

    if (areas.empty()) {
      return result;
    }

    for (const auto& area : areas) {
      Distance distance=Distance::Max();
      GeoCoord closestPoint;
      bool     stop=false;
      bool     inArea=false;

      for (const auto& ring : area->rings) {
        if (stop) {
          break;
        }

        if (ring.IsTopOuter()) {
          if (IsCoordInArea(center,
                            ring.nodes)) {
            distance=Distance::Of<Meter>(0.0);
            inArea=true;
            stop=true;
            break;
          }

          for (size_t i=0; i<ring.nodes.size(); i++) {
            Distance currentDistance;
            GeoCoord a;
            GeoCoord b;
            GeoCoord intersection;

            if (i>0) {
              a=ring.nodes[i-1].GetCoord();
              b=ring.nodes[i].GetCoord();
            }
            else {
              a=ring.nodes[ring.nodes.size()-1].GetCoord();
              b=ring.nodes[i].GetCoord();
            }

            double newDistance=CalculateDistancePointToLineSegment(center,
                                                                   a,
                                                                   b,
                                                                   intersection);

            if (!std::isfinite(newDistance)) {
              continue;
            }

            currentDistance=GetEllipsoidalDistance(center,
                                                   intersection);

            if (currentDistance<distance) {
              distance=currentDistance;
              closestPoint=intersection;
            }
          }
        }
      }

      result.areaResults.push_back(AreaRegionSearchResultEntry(area,
                                                               distance,
                                                               closestPoint,
                                                               inArea));
    }

    return result;
  }
}
