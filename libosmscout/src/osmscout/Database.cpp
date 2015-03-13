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

#include <osmscout/system/Assert.h>
#include <osmscout/system/Math.h>

#include <osmscout/util/Geometry.h>

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

  Database::Database(const DatabaseParameter& parameter)
   : parameter(parameter),
     isOpen(false)
  {
    // no code
  }

  Database::~Database()
  {
    if (IsOpen()) {
      Close();
    }
  }

  bool Database::Open(const std::string& path)
  {
    assert(!path.empty());

    this->path=path;

    typeConfig=new TypeConfig();

    if (!typeConfig->LoadFromDataFile(path)) {
      std::cerr << "Cannot load 'types.dat'!" << std::endl;
      return false;
    }

    FileScanner scanner;
    std::string file=AppendFileToDir(path,"bounding.dat");

    if (!scanner.Open(file,FileScanner::Normal,true)) {
      std::cerr << "Cannot open 'bounding.dat'" << std::endl;
      return false;
    }

    if (!scanner.ReadBox(boundingBox)) {
      std::cerr << "Error while reading from '" << file << "'" << std::endl;
    }

    std::cout << "BoundingBox: " << boundingBox.GetDisplayText() << std::endl;

    if (scanner.HasError() || !scanner.Close()) {
      std::cerr << "Error while reading/closing '" << file << "'" << std::endl;
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
    if (nodeDataFile.Valid() &&
        nodeDataFile->IsOpen()) {
      nodeDataFile->Close();
      nodeDataFile=NULL;
    }

    if (areaDataFile.Valid() &&
        areaDataFile->IsOpen()) {
      areaDataFile->Close();
      areaDataFile=NULL;
    }

    if (wayDataFile.Valid() &&
        wayDataFile->IsOpen()) {
      wayDataFile->Close();
      wayDataFile=NULL;
    }

    if (areaNodeIndex.Valid()) {
      areaNodeIndex->Close();
      areaNodeIndex=NULL;
    }

    if (areaAreaIndex.Valid()) {
      areaAreaIndex->Close();
      areaAreaIndex=NULL;
    }

    if (areaWayIndex.Valid()) {
      areaWayIndex->Close();
      areaWayIndex=NULL;
    }

    if (optimizeWaysLowZoom.Valid()) {
      optimizeWaysLowZoom->Close();
      optimizeWaysLowZoom=NULL;
    }

    if (optimizeAreasLowZoom.Valid()) {
      optimizeAreasLowZoom->Close();
      optimizeAreasLowZoom=NULL;
    }

    isOpen=false;
  }

  void Database::FlushCache()
  {
    if (nodeDataFile.Valid()) {
      nodeDataFile->FlushCache();
    }

    if (areaDataFile.Valid()) {
      areaDataFile->FlushCache();
    }

    if (wayDataFile.Valid()) {
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

    if (nodeDataFile.Invalid()) {
      nodeDataFile=new NodeDataFile(parameter.GetNodeCacheSize());
    }

    if (!nodeDataFile->IsOpen()) {
      if (!nodeDataFile->Open(typeConfig,
                              path,
                              FileScanner::LowMemRandom,
                              true)) {
        std::cerr << "Cannot open 'nodes.dat'!" << std::endl;
        return NULL;
      }
    }

    return nodeDataFile;
  }

  AreaDataFileRef Database::GetAreaDataFile() const
  {
    if (!IsOpen()) {
      return NULL;
    }

    if (areaDataFile.Invalid()) {
      areaDataFile=new AreaDataFile("areas.dat",
                                    parameter.GetAreaCacheSize());
    }

    if (!areaDataFile->IsOpen()) {
      if (!areaDataFile->Open(typeConfig,
                              path,
                              FileScanner::LowMemRandom,
                              true)) {
        std::cerr << "Cannot open 'areas.dat'!" << std::endl;
        return NULL;
      }
    }

    return areaDataFile;
  }

  WayDataFileRef Database::GetWayDataFile() const
  {
    if (!IsOpen()) {
      return NULL;
    }

    if (wayDataFile.Invalid()) {
      wayDataFile=new WayDataFile("ways.dat",
                                  parameter.GetWayCacheSize());
    }

    if (!wayDataFile->IsOpen()) {
      if (!wayDataFile->Open(typeConfig,
                             path,
                             FileScanner::LowMemRandom,
                             true)) {
        std::cerr << "Cannot open 'ways.dat'!" << std::endl;
        return NULL;
      }
    }

    return wayDataFile;
  }

  AreaNodeIndexRef Database::GetAreaNodeIndex() const
  {
    if (!IsOpen()) {
      return NULL;
    }

    if (areaNodeIndex.Invalid()) {
      areaNodeIndex=new AreaNodeIndex(/*parameter.GetAreaNodeIndexCacheSize()*/);

      if (!areaNodeIndex->Load(path)) {
        std::cerr << "Cannot load area node index!" << std::endl;
        areaNodeIndex=NULL;

        return NULL;
      }
    }

    return areaNodeIndex;
  }

  AreaAreaIndexRef Database::GetAreaAreaIndex() const
  {
    if (!IsOpen()) {
      return NULL;
    }

    if (areaAreaIndex.Invalid()) {
      areaAreaIndex=new AreaAreaIndex(parameter.GetAreaAreaIndexCacheSize());

      if (!areaAreaIndex->Load(path)) {
        std::cerr << "Cannot load area area index!" << std::endl;
        areaAreaIndex=NULL;

        return NULL;
      }
    }

    return areaAreaIndex;
  }

  AreaWayIndexRef Database::GetAreaWayIndex() const
  {
    if (!IsOpen()) {
      return NULL;
    }

    if (areaWayIndex.Invalid()) {
      areaWayIndex=new AreaWayIndex();

      if (!areaWayIndex->Load(typeConfig,
                              path)) {
        std::cerr << "Cannot load area way index!" << std::endl;
        areaWayIndex=NULL;

        return NULL;
      }
    }

    return areaWayIndex;
  }

  LocationIndexRef Database::GetLocationIndex() const
  {
    if (!IsOpen()) {
      return NULL;
    }

    if (locationIndex.Invalid()) {
      locationIndex=new LocationIndex();

      if (!locationIndex->Load(path)) {
        std::cerr << "Cannot load location index!" << std::endl;
        locationIndex=NULL;

        return NULL;
      }
    }

    return locationIndex;
  }

  WaterIndexRef Database::GetWaterIndex() const
  {
    if (!IsOpen()) {
      return NULL;
    }

    if (waterIndex.Invalid()) {
      waterIndex=new WaterIndex();

      if (!waterIndex->Load(path)) {
        std::cerr << "Cannot load water index!" << std::endl;
        waterIndex=NULL;

        return NULL;
      }
    }

    return waterIndex;
  }

  OptimizeAreasLowZoomRef Database::GetOptimizeAreasLowZoom() const
  {
    if (!IsOpen()) {
      return NULL;
    }

    if (optimizeAreasLowZoom.Invalid()) {
      optimizeAreasLowZoom=new OptimizeAreasLowZoom();

      if (!optimizeAreasLowZoom->Open(typeConfig,
                                      path)) {
        std::cerr << "Cannot load optimize areas low zoom index!" << std::endl;
        optimizeAreasLowZoom=NULL;

        return NULL;
      }
    }

    return optimizeAreasLowZoom;
  }

  OptimizeWaysLowZoomRef Database::GetOptimizeWaysLowZoom() const
  {
    if (optimizeWaysLowZoom.Invalid()) {
      optimizeWaysLowZoom=new OptimizeWaysLowZoom();

      if (!optimizeWaysLowZoom->Open(typeConfig,
                                     path)) {
        std::cerr << "Cannot load optimize areas low zoom index!" << std::endl;
        optimizeWaysLowZoom=NULL;

        return NULL;
      }
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

    if (nodeDataFile.Invalid()) {
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

    if (nodeDataFile.Invalid()) {
      return false;
    }

    return nodeDataFile->GetByOffset(offsets,nodes);
  }

  bool Database::GetNodesByOffset(const std::set<FileOffset>& offsets,
                                  std::vector<NodeRef>& nodes) const
  {
    NodeDataFileRef nodeDataFile=GetNodeDataFile();

    if (nodeDataFile.Invalid()) {
      return false;
    }

    return nodeDataFile->GetByOffset(offsets,nodes);
  }

  bool Database::GetNodesByOffset(const std::list<FileOffset>& offsets,
                                  std::vector<NodeRef>& nodes) const
  {
    NodeDataFileRef nodeDataFile=GetNodeDataFile();

    if (nodeDataFile.Invalid()) {
      return false;
    }

    return nodeDataFile->GetByOffset(offsets,nodes);
  }

  bool Database::GetNodesByOffset(const std::set<FileOffset>& offsets,
                                  OSMSCOUT_HASHMAP<FileOffset,NodeRef>& dataMap) const
  {
    NodeDataFileRef nodeDataFile=GetNodeDataFile();

    if (nodeDataFile.Invalid()) {
      return false;
    }

    return nodeDataFile->GetByOffset(offsets,dataMap);
  }

  bool Database::GetAreaByOffset(const FileOffset& offset,
                                 AreaRef& area) const
  {
    AreaDataFileRef areaDataFile=GetAreaDataFile();

    if (areaDataFile.Invalid()) {
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

    if (areaDataFile.Invalid()) {
      return false;
    }

    return areaDataFile->GetByOffset(offsets,areas);
  }

  bool Database::GetAreasByOffset(const std::set<FileOffset>& offsets,
                                  std::vector<AreaRef>& areas) const
  {
    AreaDataFileRef areaDataFile=GetAreaDataFile();

    if (areaDataFile.Invalid()) {
      return false;
    }

    return areaDataFile->GetByOffset(offsets,areas);
  }

  bool Database::GetAreasByOffset(const std::list<FileOffset>& offsets,
                                  std::vector<AreaRef>& areas) const
  {
    AreaDataFileRef areaDataFile=GetAreaDataFile();

    if (areaDataFile.Invalid()) {
      return false;
    }

    return areaDataFile->GetByOffset(offsets,areas);
  }

  bool Database::GetAreasByOffset(const std::set<FileOffset>& offsets,
                                  OSMSCOUT_HASHMAP<FileOffset,AreaRef>& dataMap) const
  {
    AreaDataFileRef areaDataFile=GetAreaDataFile();

    if (areaDataFile.Invalid()) {
      return false;
    }

    return areaDataFile->GetByOffset(offsets,dataMap);
  }

  bool Database::GetWayByOffset(const FileOffset& offset,
                                WayRef& way) const
  {
    WayDataFileRef wayDataFile=GetWayDataFile();

    if (wayDataFile.Invalid()) {
      return false;
    }

    if (wayDataFile->GetByOffset(offset,way)) {
      return true;
    }

    return false;
  }

  bool Database::GetWaysByOffset(const std::vector<FileOffset>& offsets,
                                 std::vector<WayRef>& ways) const
  {
    WayDataFileRef wayDataFile=GetWayDataFile();

    if (wayDataFile.Invalid()) {
      return false;
    }

    return wayDataFile->GetByOffset(offsets,ways);
  }

  bool Database::GetWaysByOffset(const std::set<FileOffset>& offsets,
                                 std::vector<WayRef>& ways) const
  {
    WayDataFileRef wayDataFile=GetWayDataFile();

    if (wayDataFile.Invalid()) {
      return false;
    }

    return wayDataFile->GetByOffset(offsets,ways);
  }

  bool Database::GetWaysByOffset(const std::list<FileOffset>& offsets,
                                 std::vector<WayRef>& ways) const
  {
    WayDataFileRef wayDataFile=GetWayDataFile();

    if (wayDataFile.Invalid()) {
      return false;
    }

    return wayDataFile->GetByOffset(offsets,ways);
  }

  bool Database::GetWaysByOffset(const std::set<FileOffset>& offsets,
                                 OSMSCOUT_HASHMAP<FileOffset,WayRef>& dataMap) const
  {
    WayDataFileRef wayDataFile=GetWayDataFile();

    if (wayDataFile.Invalid()) {
      return false;
    }

    return wayDataFile->GetByOffset(offsets,dataMap);
  }

  void Database::DumpStatistics()
  {
    if (nodeDataFile.Valid()) {
      nodeDataFile->DumpStatistics();
    }

    if (areaDataFile.Valid()) {
      areaDataFile->DumpStatistics();
    }

    if (wayDataFile.Valid()) {
      wayDataFile->DumpStatistics();
    }

    if (areaNodeIndex.Valid()) {
      areaNodeIndex->DumpStatistics();
    }

    if (areaAreaIndex.Valid()) {
      areaAreaIndex->DumpStatistics();
    }

    if (areaWayIndex.Valid()) {
      areaWayIndex->DumpStatistics();
    }

    if (locationIndex.Valid()) {
      locationIndex->DumpStatistics();
    }

    if (waterIndex.Valid()) {
      waterIndex->DumpStatistics();
    }
  }
}
