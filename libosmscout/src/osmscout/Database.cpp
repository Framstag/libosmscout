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
    waysDataMMap(true)
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
    boundingBoxDataFile=NULL;

    if (nodeDataFile &&
        nodeDataFile->IsOpen()) {
      nodeDataFile->Close();
      nodeDataFile=NULL;
    }

    if (areaDataFile &&
        areaDataFile->IsOpen()) {
      areaDataFile->Close();
      areaDataFile=NULL;
    }

    if (wayDataFile &&
        wayDataFile->IsOpen()) {
      wayDataFile->Close();
      wayDataFile=NULL;
    }

    if (areaNodeIndex) {
      areaNodeIndex->Close();
      areaNodeIndex=NULL;
    }

    if (areaAreaIndex) {
      areaAreaIndex->Close();
      areaAreaIndex=NULL;
    }

    if (areaWayIndex) {
      areaWayIndex->Close();
      areaWayIndex=NULL;
    }

    if (locationIndex) {
      locationIndex=NULL;
    }

    if (waterIndex) {
      waterIndex->Close();
      waterIndex=NULL;
    }

    if (optimizeWaysLowZoom) {
      optimizeWaysLowZoom->Close();
      optimizeWaysLowZoom=NULL;
    }

    if (optimizeAreasLowZoom) {
      optimizeAreasLowZoom->Close();
      optimizeAreasLowZoom=NULL;
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
      return NULL;
    }

    if (!boundingBoxDataFile) {
      boundingBoxDataFile=std::make_shared<BoundingBoxDataFile>();
    }

    if (!boundingBoxDataFile->IsLoaded()) {
      StopClock timer;

      if (!boundingBoxDataFile->Load(path)) {
        log.Error() << "Cannot open '" << BoundingBoxDataFile::BOUNDINGBOX_DAT << "'!";
        return NULL;
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
      return NULL;
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
        return NULL;
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
      return NULL;
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
        return NULL;
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
      return NULL;
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
        return NULL;
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
      return NULL;
    }

    if (!areaNodeIndex) {
      areaNodeIndex=std::make_shared<AreaNodeIndex>();

      StopClock timer;

      if (!areaNodeIndex->Open(path)) {
        log.Error() << "Cannot load area node index!";
        areaNodeIndex=NULL;

        return NULL;
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
      return NULL;
    }

    if (!areaAreaIndex) {
      areaAreaIndex=std::make_shared<AreaAreaIndex>(parameter.GetAreaAreaIndexCacheSize());

      StopClock timer;

      if (!areaAreaIndex->Open(path)) {
        log.Error() << "Cannot load area area index!";
        areaAreaIndex=NULL;

        return NULL;
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
      return NULL;
    }

    if (!areaWayIndex) {
      areaWayIndex=std::make_shared<AreaWayIndex>();

      StopClock timer;

      if (!areaWayIndex->Open(typeConfig,
                              path)) {
        log.Error() << "Cannot load area way index!";
        areaWayIndex=NULL;

        return NULL;
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
      return NULL;
    }

    if (!locationIndex) {
      locationIndex=std::make_shared<LocationIndex>();

      StopClock timer;

      if (!locationIndex->Load(path)) {
        log.Error() << "Cannot load location index!";
        locationIndex=NULL;

        return NULL;
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
      return NULL;
    }

    if (!waterIndex) {
      waterIndex=std::make_shared<WaterIndex>();

      StopClock timer;

      if (!waterIndex->Open(path)) {
        log.Error() << "Cannot load water index!";
        waterIndex=NULL;

        return NULL;
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
      return NULL;
    }

    if (!optimizeAreasLowZoom) {
      optimizeAreasLowZoom=std::make_shared<OptimizeAreasLowZoom>();

      StopClock timer;

      if (!optimizeAreasLowZoom->Open(typeConfig,
                                      path)) {
        log.Error() << "Cannot load optimize areas low zoom index!";
        optimizeAreasLowZoom=NULL;

        return NULL;
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
                                     path)) {
        log.Error() << "Cannot load optimize areas low zoom index!";
        optimizeWaysLowZoom=NULL;

        return NULL;
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
}
