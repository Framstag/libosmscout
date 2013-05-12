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
#include <iostream>

#include <osmscout/TypeConfigLoader.h>

#include <osmscout/system/Assert.h>
#include <osmscout/system/Math.h>

#include <osmscout/util/File.h>
#include <osmscout/util/FileScanner.h>
#include <osmscout/util/HashMap.h>
#include <osmscout/util/StopClock.h>

#include "osmscout/ObjectRef.h"

namespace osmscout {

  DebugDatabaseParameter::DebugDatabaseParameter()
  {
    // no code
  }

  DebugDatabase::DebugDatabase(const DebugDatabaseParameter& parameter)
   : isOpen(false),
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

    isOpen=true;

    return true;
  }

  bool DebugDatabase::IsOpen() const
  {
    return isOpen;
  }


  void DebugDatabase::Close()
  {
    isOpen=false;
  }

  TypeConfig* DebugDatabase::GetTypeConfig() const
  {
    return typeConfig;
  }

  bool DebugDatabase::GetCoords(const std::vector<Id>& ids,
                                std::vector<Point>& coords) const
  {
    FileScanner                     coordDataFile;
    uint32_t                        coordPageSize;
    OSMSCOUT_HASHMAP<Id,FileOffset> coordPageIdOffsetMap;

    if (!coordDataFile.Open(osmscout::AppendFileToDir(path,"coord.dat"),
                            osmscout::FileScanner::LowMemRandom,
                            false)) {
      return false;

    }

    FileOffset mapFileOffset;

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

  bool DebugDatabase::ResolveReferences(const std::string& mapName,
                                        RefType fileType,
                                        const std::set<ObjectOSMRef>& ids,
                                        const std::set<ObjectFileRef>& fileOffsets,
                                        std::map<ObjectOSMRef,ObjectFileRef>& idFileOffsetMap,
                                        std::map<ObjectFileRef,ObjectOSMRef>& fileOffsetIdMap)
  {
    FileScanner scanner;
    uint32_t    entryCount;
    std::string filename=AppendFileToDir(path,mapName);

    if (!scanner.Open(filename,FileScanner::LowMemRandom,false)) {
      std::cerr << "Cannot open file '" << scanner.GetFilename() << "'!" << std::endl;
      return false;
    }

    if (!scanner.Read(entryCount)) {
      return false;
    }

    for (size_t i=1; i<=entryCount; i++) {
      Id         id;
      uint8_t    typeByte;
      OSMRefType osmType;
      FileOffset fileOffset;

      if (!scanner.Read(id)) {
        return false;
      }

      if (!scanner.Read(typeByte)) {
        return false;
      }

      osmType=(OSMRefType)typeByte;

      if (!scanner.ReadFileOffset(fileOffset)) {
        return false;
      }

      ObjectOSMRef  osmRef(id,osmType);
      ObjectFileRef fileRef(fileOffset,fileType);

      if (ids.find(osmRef)!=ids.end() ||
          fileOffsets.find(fileRef)!=fileOffsets.end()) {
        idFileOffsetMap.insert(std::make_pair(osmRef,fileRef));
        fileOffsetIdMap.insert(std::make_pair(fileRef,osmRef));
      }
    }

    return scanner.Close();
  }

  bool DebugDatabase::ResolveReferences(const std::set<ObjectOSMRef>& ids,
                                        const std::set<ObjectFileRef>& fileOffsets,
                                        std::map<ObjectOSMRef,ObjectFileRef>& idFileOffsetMap,
                                        std::map<ObjectFileRef,ObjectOSMRef>& fileOffsetIdMap)
  {
    bool haveToScanNodes=false;
    bool haveToScanAreas=false;
    bool haveToScanWays=false;

    for (std::set<ObjectOSMRef>::const_iterator ref=ids.begin();
         ref!=ids.end();
         ++ref) {
      if (haveToScanNodes && haveToScanAreas && haveToScanWays) {
        break;
      }

      switch (ref->GetType()) {
      case osmRefNone:
        break;
      case osmRefNode:
        haveToScanNodes=true;
        break;
      case osmRefWay:
        haveToScanAreas=true;
        haveToScanWays=true;
        break;
      case osmRefRelation:
        haveToScanAreas=true;
        break;
      }

    }

    for (std::set<ObjectFileRef>::const_iterator ref=fileOffsets.begin();
         ref!=fileOffsets.end();
         ++ref) {
      if (haveToScanNodes && haveToScanAreas && haveToScanWays) {
        break;
      }

      switch (ref->GetType()) {
      case refNone:
        break;
      case refNode:
        haveToScanNodes=true;
        break;
      case refWay:
        haveToScanWays=true;
        break;
      case refArea:
        haveToScanAreas=true;
        break;
      }
    }

    if (haveToScanNodes) {
      if (!ResolveReferences("nodes.idmap",
                             refNode,
                             ids,
                             fileOffsets,
                             idFileOffsetMap,
                             fileOffsetIdMap)) {
        return false;
      }
    }

    if (haveToScanAreas) {
      if (!ResolveReferences("areas.idmap",
                             refArea,
                             ids,
                             fileOffsets,
                             idFileOffsetMap,
                             fileOffsetIdMap)) {
        return false;
      }
    }

    if (haveToScanWays) {
      if (!ResolveReferences("ways.idmap",
                             refWay,
                             ids,
                             fileOffsets,
                             idFileOffsetMap,
                             fileOffsetIdMap)) {
        return false;
      }
    }

    return true;
  }
}
