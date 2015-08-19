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

namespace osmscout {

  DatabaseParameter::DatabaseParameter()
  : areaAreaIndexCacheSize(1000),
    areaNodeIndexCacheSize(1000),
    nodeCacheSize(1000),
    wayCacheSize(4000),
    areaCacheSize(4000)
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

    FileScanner scanner;

    if (!scanner.Open(AppendFileToDir(path,"bounding.dat"),
                      FileScanner::Normal,
                      false)) {
      log.Error() << "Cannot open '" << scanner.GetFilename() << "'";
      return false;
    }

    if (!scanner.ReadBox(boundingBox)) {
      log.Error() << "Error while reading '" << scanner.GetFilename() << "'";
    }

    log.Debug() << "BoundingBox: " << boundingBox.GetDisplayText();

    if (scanner.HasError() || !scanner.Close()) {
      log.Error() << "Cannot while reading/closing '" << scanner.GetFilename() << "'";
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

  void Database::FlushCache()
  {
    if (nodeDataFile) {
      nodeDataFile->FlushCache();
    }

    if (areaDataFile) {
      areaDataFile->FlushCache();
    }

    if (wayDataFile) {
      wayDataFile->FlushCache();
    }
  }

  std::string Database::GetPath() const
  {
    return path;
  }

  TypeConfigRef Database::GetTypeConfig() const
  {
    return typeConfig;
  }

  NodeDataFileRef Database::GetNodeDataFile() const
  {
    if (!IsOpen()) {
      return NULL;
    }

    if (!nodeDataFile) {
      nodeDataFile=std::make_shared<NodeDataFile>(parameter.GetNodeCacheSize());
    }

    if (!nodeDataFile->IsOpen()) {
      StopClock timer;

      if (!nodeDataFile->Open(typeConfig,
                              path,
                              FileScanner::LowMemRandom,
                              true)) {
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
    if (!IsOpen()) {
      return NULL;
    }

    if (!areaDataFile) {
      areaDataFile=std::make_shared<AreaDataFile>("areas.dat",
                                                  parameter.GetAreaCacheSize());
    }

    if (!areaDataFile->IsOpen()) {
      StopClock timer;

      if (!areaDataFile->Open(typeConfig,
                              path,
                              FileScanner::LowMemRandom,
                              true)) {
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
    if (!IsOpen()) {
      return NULL;
    }

    if (!wayDataFile) {
      wayDataFile=std::make_shared<WayDataFile>("ways.dat",
                                                parameter.GetWayCacheSize());
    }

    if (!wayDataFile->IsOpen()) {
      StopClock timer;

      if (!wayDataFile->Open(typeConfig,
                             path,
                             FileScanner::LowMemRandom,
                             true)) {
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
    if (!IsOpen()) {
      return NULL;
    }

    if (!areaNodeIndex) {
      areaNodeIndex=std::make_shared<AreaNodeIndex>(/*parameter.GetAreaNodeIndexCacheSize()*/);

      StopClock timer;

      if (!areaNodeIndex->Load(path)) {
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
    if (!IsOpen()) {
      return NULL;
    }

    if (!areaAreaIndex) {
      areaAreaIndex=std::make_shared<AreaAreaIndex>(parameter.GetAreaAreaIndexCacheSize());

      StopClock timer;

      if (!areaAreaIndex->Load(path)) {
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
    if (!IsOpen()) {
      return NULL;
    }

    if (!areaWayIndex) {
      areaWayIndex=std::make_shared<AreaWayIndex>();

      StopClock timer;

      if (!areaWayIndex->Load(typeConfig,
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
    if (!IsOpen()) {
      return NULL;
    }

    if (!waterIndex) {
      waterIndex=std::make_shared<WaterIndex>();

      StopClock timer;

      if (!waterIndex->Load(path)) {
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
    if (!IsOpen()) {
      return false;
    }

    boundingBox=this->boundingBox;

    return true;
  }

  bool Database::GetNodeByOffset(const FileOffset& offset,
                                 NodeRef& node) const
  {
    NodeDataFileRef nodeDataFile=GetNodeDataFile();

    if (!nodeDataFile) {
      return false;
    }

    if (nodeDataFile->GetByOffset(offset,node)) {
      return true;
    }

    return false;
  }

  bool Database::GetNodesByOffset(const std::vector<FileOffset>& offsets,
                                  std::vector<NodeRef>& nodes) const
  {
    NodeDataFileRef nodeDataFile=GetNodeDataFile();

    if (!nodeDataFile) {
      return false;
    }

    return nodeDataFile->GetByOffset(offsets,nodes);
  }

  bool Database::GetNodesByOffset(const std::set<FileOffset>& offsets,
                                  std::vector<NodeRef>& nodes) const
  {
    NodeDataFileRef nodeDataFile=GetNodeDataFile();

    if (!nodeDataFile) {
      return false;
    }

    return nodeDataFile->GetByOffset(offsets,nodes);
  }

  bool Database::GetNodesByOffset(const std::list<FileOffset>& offsets,
                                  std::vector<NodeRef>& nodes) const
  {
    NodeDataFileRef nodeDataFile=GetNodeDataFile();

    if (!nodeDataFile) {
      return false;
    }

    return nodeDataFile->GetByOffset(offsets,nodes);
  }

  bool Database::GetNodesByOffset(const std::set<FileOffset>& offsets,
                                  std::unordered_map<FileOffset,NodeRef>& dataMap) const
  {
    NodeDataFileRef nodeDataFile=GetNodeDataFile();

    if (!nodeDataFile) {
      return false;
    }

    return nodeDataFile->GetByOffset(offsets,dataMap);
  }

  bool Database::GetAreaByOffset(const FileOffset& offset,
                                 AreaRef& area) const
  {
    AreaDataFileRef areaDataFile=GetAreaDataFile();

    if (!areaDataFile) {
      return false;
    }

    if (areaDataFile->GetByOffset(offset,area)) {
      return true;
    }

    return false;
  }

  bool Database::GetAreasByOffset(const std::vector<FileOffset>& offsets,
                                  std::vector<AreaRef>& areas) const
  {
    AreaDataFileRef areaDataFile=GetAreaDataFile();

    if (!areaDataFile) {
      return false;
    }

    return areaDataFile->GetByOffset(offsets,areas);
  }

  bool Database::GetAreasByOffset(const std::set<FileOffset>& offsets,
                                  std::vector<AreaRef>& areas) const
  {
    AreaDataFileRef areaDataFile=GetAreaDataFile();

    if (!areaDataFile) {
      return false;
    }

    return areaDataFile->GetByOffset(offsets,areas);
  }

  bool Database::GetAreasByOffset(const std::list<FileOffset>& offsets,
                                  std::vector<AreaRef>& areas) const
  {
    AreaDataFileRef areaDataFile=GetAreaDataFile();

    if (!areaDataFile) {
      return false;
    }

    return areaDataFile->GetByOffset(offsets,areas);
  }

  bool Database::GetAreasByOffset(const std::set<FileOffset>& offsets,
                                  std::unordered_map<FileOffset,AreaRef>& dataMap) const
  {
    AreaDataFileRef areaDataFile=GetAreaDataFile();

    if (!areaDataFile) {
      return false;
    }

    return areaDataFile->GetByOffset(offsets,dataMap);
  }

  bool Database::GetWayByOffset(const FileOffset& offset,
                                WayRef& way) const
  {
    WayDataFileRef wayDataFile=GetWayDataFile();

    if (!wayDataFile) {
      return false;
    }

    return wayDataFile->GetByOffset(offset,way);
  }

  bool Database::GetWaysByOffset(const std::vector<FileOffset>& offsets,
                                 std::vector<WayRef>& ways) const
  {
    WayDataFileRef wayDataFile=GetWayDataFile();

    if (!wayDataFile) {
      return false;
    }

    return wayDataFile->GetByOffset(offsets,ways);
  }

  bool Database::GetWaysByOffset(const std::set<FileOffset>& offsets,
                                 std::vector<WayRef>& ways) const
  {
    WayDataFileRef wayDataFile=GetWayDataFile();

    if (!wayDataFile) {
      return false;
    }

    return wayDataFile->GetByOffset(offsets,ways);
  }

  bool Database::GetWaysByOffset(const std::list<FileOffset>& offsets,
                                 std::vector<WayRef>& ways) const
  {
    WayDataFileRef wayDataFile=GetWayDataFile();

    if (!wayDataFile) {
      return false;
    }

    return wayDataFile->GetByOffset(offsets,ways);
  }

  bool Database::GetWaysByOffset(const std::set<FileOffset>& offsets,
                                 std::unordered_map<FileOffset,WayRef>& dataMap) const
  {
    WayDataFileRef wayDataFile=GetWayDataFile();

    if (!wayDataFile) {
      return false;
    }

    return wayDataFile->GetByOffset(offsets,dataMap);
  }

  void Database::DumpStatistics()
  {
    if (nodeDataFile) {
      nodeDataFile->DumpStatistics();
    }

    if (areaDataFile) {
      areaDataFile->DumpStatistics();
    }

    if (wayDataFile) {
      wayDataFile->DumpStatistics();
    }

    if (areaNodeIndex) {
      areaNodeIndex->DumpStatistics();
    }

    if (areaAreaIndex) {
      areaAreaIndex->DumpStatistics();
    }

    if (areaWayIndex) {
      areaWayIndex->DumpStatistics();
    }

    if (locationIndex) {
      locationIndex->DumpStatistics();
    }

    if (waterIndex) {
      waterIndex->DumpStatistics();
    }
  }
}
