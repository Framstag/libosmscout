/*
  This source is part of the libosmscout library
  Copyright (C) 2013  Tim Teulings

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

#include <osmscout/DebugDatabase.h>

#include <algorithm>
#include <cassert>
#include <iostream>

#include <osmscout/TypeConfigLoader.h>

#include <osmscout/util/HashMap.h>
#include <osmscout/util/StopClock.h>

#include <osmscout/private/Math.h>

namespace osmscout {

  DebugDatabaseParameter::DebugDatabaseParameter()
  : nodeIndexCacheSize(1000),
    nodeCacheSize(1000),
    wayCacheSize(8000),
    relationCacheSize(1000)
  {
    // no code
  }

  void DebugDatabaseParameter::SetNodeIndexCacheSize(unsigned long nodeIndexCacheSize)
  {
    this->nodeIndexCacheSize=nodeIndexCacheSize;
  }

  void DebugDatabaseParameter::SetNodeCacheSize(unsigned long nodeCacheSize)
  {
    this->nodeCacheSize=nodeCacheSize;
  }

  void DebugDatabaseParameter::SetWayCacheSize(unsigned long wayCacheSize)
  {
    this->wayCacheSize=wayCacheSize;
  }

  void DebugDatabaseParameter::SetRelationCacheSize(unsigned long relationCacheSize)
  {
    this->relationCacheSize=relationCacheSize;
  }

  unsigned long DebugDatabaseParameter::GetNodeIndexCacheSize() const
  {
    return nodeIndexCacheSize;
  }

  unsigned long DebugDatabaseParameter::GetNodeCacheSize() const
  {
    return nodeCacheSize;
  }

  unsigned long DebugDatabaseParameter::GetWayCacheSize() const
  {
    return wayCacheSize;
  }

  unsigned long DebugDatabaseParameter::GetRelationCacheSize() const
  {
    return relationCacheSize;
  }

  DebugDatabase::DebugDatabase(const DebugDatabaseParameter& parameter)
   : isOpen(false),
     nodeDataFile("nodes.dat",
                  "node.idx",
                  parameter.GetNodeCacheSize(),
                  parameter.GetNodeIndexCacheSize()),
     typeConfig(NULL)
  {
    // no code
  }

  DebugDatabase::~DebugDatabase()
  {
    delete typeConfig;
  }

  bool DebugDatabase::Open(const std::string& path)
  {
    assert(!path.empty());

    this->path=path;

    typeConfig=new TypeConfig();

    if (!LoadTypeData(path,*typeConfig)) {
      std::cerr << "Cannot load 'types.dat'!" << std::endl;
      delete typeConfig;
      typeConfig=NULL;
      return false;
    }

    if (!nodeDataFile.Open(path,
                           FileScanner::LowMemRandom,true,
                           FileScanner::LowMemRandom,true)) {
      std::cerr << "Cannot open 'nodes.dat'!" << std::endl;
      delete typeConfig;
      typeConfig=NULL;
      return false;
    }

    isOpen=true;

    return true;
  }

  bool DebugDatabase::IsOpen() const
  {
    return isOpen;
  }


  void DebugDatabase::Close()
  {
    nodeDataFile.Close();

    isOpen=false;
  }

  TypeConfig* DebugDatabase::GetTypeConfig() const
  {
    return typeConfig;
  }

  bool DebugDatabase::GetCoords(const std::vector<Id>& ids,
                                std::vector<Point>& coords) const
  {
    osmscout::FileScanner           coordDataFile;
    uint32_t                        coordPageSize;
    OSMSCOUT_HASHMAP<Id,FileOffset> coordPageIdOffsetMap;

    if (!coordDataFile.Open(osmscout::AppendFileToDir(path,"coord.dat"),
                            osmscout::FileScanner::LowMemRandom,
                            false)) {
      return false;

    }

    osmscout::FileOffset mapFileOffset;

    if (!coordDataFile.Read(coordPageSize))
    {
      std::cerr << "Error while reading page size from coord.dat" << std::endl;
      return 1;
    }

    if (!coordDataFile.Read(mapFileOffset)) {
      std::cerr << "Error while reading map offset from coord.dat" << std::endl;
      return 1;
    }

    if (!coordDataFile.SetPos(mapFileOffset)) {
      return false;
    }

    uint32_t mapSize;

    if (!coordDataFile.Read(mapSize)) {
      std::cerr << "Error while reading map size from coord.dat" << std::endl;
      return 1;
    }

    for (size_t i=1; i<=mapSize; i++) {
      osmscout::Id         pageId;
      osmscout::FileOffset pageOffset;

      if (!coordDataFile.Read(pageId)) {
        std::cerr << "Error while reading pageId from coord.dat" << std::endl;
        return 1;
      }
      if (!coordDataFile.Read(pageOffset)) {
        std::cerr << "Error while reading pageOffset from coord.dat" << std::endl;
        return 1;
      }

      coordPageIdOffsetMap[pageId]=pageOffset;
    }

    for (size_t i=0; i<ids.size(); i++) {
      Id                                              coordPageId=ids[i]/coordPageSize;
      OSMSCOUT_HASHMAP<Id,FileOffset>::const_iterator pageOffset=coordPageIdOffsetMap.find(coordPageId);

      if (pageOffset==coordPageIdOffsetMap.end()) {
        continue;
      }

      coordDataFile.SetPos(pageOffset->second+(ids[i]%coordPageSize)*2*sizeof(uint32_t));

      uint32_t latDat;
      uint32_t lonDat;

      coordDataFile.Read(latDat);
      coordDataFile.Read(lonDat);

      if (latDat==0xffffffff || lonDat==0xffffffff) {
        std::cerr << "Cannot load coord " << ids[i] << std::endl;
        continue;
      }

      if (coordDataFile.HasError()) {
        std::cerr << "Error while reading data from offset " << pageOffset->second << " of file " << coordDataFile.GetFilename() << "!" << std::endl;
        continue;
      }

      coords.push_back(Point(ids[i],
                             latDat/osmscout::conversionFactor-90.0,
                             lonDat/osmscout::conversionFactor-180.0));
    }

    return true;
  }

  bool DebugDatabase::GetNodes(const std::vector<Id>& ids,
                               std::vector<NodeRef>& nodes) const
  {
    if (!IsOpen()) {
      return false;
    }

    return nodeDataFile.Get(ids,nodes);
  }

  bool DebugDatabase::ResolveWayIdsAndOffsets(const std::set<Id>& ids,
                                              std::map<Id,FileOffset>& idFileOffsetMap,
                                              const std::set<FileOffset>& fileOffsets,
                                              std::map<FileOffset,Id>& fileOffsetIdMap)
  {
    FileScanner scanner;
    uint32_t    entryCount;
    std::string filename=AppendFileToDir(path,"way.idmap");

    if (!scanner.Open(filename,FileScanner::LowMemRandom,false)) {
      return false;
    }

    if (!scanner.Read(entryCount)) {
      return false;
    }

    for (size_t i=1; i<=entryCount; i++) {
      Id         id;
      FileOffset fileOffset;

      if (!scanner.Read(id)) {
        return false;
      }

      if (!scanner.ReadFileOffset(fileOffset)) {
        return false;
      }

      if (ids.find(id)!=ids.end()) {
        idFileOffsetMap.insert(std::make_pair(id,fileOffset));
        fileOffsetIdMap.insert(std::make_pair(fileOffset,id));
      }
      else if (fileOffsets.find(fileOffset)!=fileOffsets.end()) {
        idFileOffsetMap.insert(std::make_pair(id,fileOffset));
        fileOffsetIdMap.insert(std::make_pair(fileOffset,id));
      }
    }

    return scanner.Close();
  }

  bool DebugDatabase::ResolveRelationIdsAndOffsets(const std::set<Id>& ids,
                                                   std::map<Id,FileOffset>& idFileOffsetMap,
                                                   const std::set<FileOffset>& fileOffsets,
                                                   std::map<FileOffset,Id>& fileOffsetIdMap)
  {
    FileScanner scanner;
    uint32_t    entryCount;
    std::string filename=AppendFileToDir(path,"relation.idmap");

    if (!scanner.Open(filename,FileScanner::LowMemRandom,false)) {
      return false;
    }

    if (!scanner.Read(entryCount)) {
      return false;
    }

    for (size_t i=1; i<=entryCount; i++) {
      Id         id;
      FileOffset fileOffset;

      if (!scanner.Read(id)) {
        return false;
      }

      if (!scanner.ReadFileOffset(fileOffset)) {
        return false;
      }

      if (ids.find(id)!=ids.end()) {
        idFileOffsetMap.insert(std::make_pair(id,fileOffset));
        fileOffsetIdMap.insert(std::make_pair(fileOffset,id));
      }
      else if (fileOffsets.find(fileOffset)!=fileOffsets.end()) {
        idFileOffsetMap.insert(std::make_pair(id,fileOffset));
        fileOffsetIdMap.insert(std::make_pair(fileOffset,id));
      }
    }

    return scanner.Close();
  }
}
