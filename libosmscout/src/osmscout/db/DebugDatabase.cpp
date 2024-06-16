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

#include <osmscout/db/DebugDatabase.h>

#include <osmscout/system/Assert.h>

#include <osmscout/io/File.h>
#include <osmscout/io/FileScanner.h>

#include <osmscout/log/Logger.h>

#include <osmscout/ObjectRef.h>

namespace osmscout {

  DebugDatabase::DebugDatabase(const DebugDatabaseParameter& /*parameter*/)
  : isOpen(false)
  {
    // no code
  }

  bool DebugDatabase::Open(const std::string& path)
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

  bool DebugDatabase::IsOpen() const
  {
    return isOpen;
  }

  void DebugDatabase::Close()
  {
    isOpen=false;
  }

  TypeConfigRef DebugDatabase::GetTypeConfig() const
  {
    return typeConfig;
  }

  bool DebugDatabase::GetCoords(const std::set<OSMId>& ids,
                                CoordDataFile::ResultMap& coordsMap) const
  {
    CoordDataFile dataFile;

    if (!dataFile.Open(path,
                       true)) {
      return false;
    }

    if (!dataFile.Get(ids,
                      coordsMap)) {
      return false;
    }

    return dataFile.Close();
  }

  bool DebugDatabase::ResolveReferences(const std::string& mapName,
                                        RefType fileType,
                                        const std::set<ObjectOSMRef>& ids,
                                        const std::set<ObjectFileRef>& fileOffsets,
                                        std::multimap<ObjectOSMRef,ObjectFileRef>& idFileOffsetMap,
                                        std::map<ObjectFileRef,ObjectOSMRef>& fileOffsetIdMap)
  {
    FileScanner scanner;
    std::string filename=AppendFileToDir(path,mapName);

    try {

      scanner.Open(filename,FileScanner::LowMemRandom,false);

      uint32_t    entryCount=scanner.ReadUInt32();

      for (uint32_t i=1; i<=entryCount; i++) {
        Id         id=scanner.ReadUInt64();
        uint8_t    typeByte=scanner.ReadUInt8();
        FileOffset fileOffset=scanner.ReadFileOffset();

        ObjectOSMRef  osmRef(id,(OSMRefType)typeByte);
        ObjectFileRef fileRef(fileOffset,fileType);

        if (ids.contains(osmRef) ||
            fileOffsets.contains(fileRef)) {
          idFileOffsetMap.emplace(osmRef,fileRef);
          fileOffsetIdMap.emplace(fileRef,osmRef);
        }
      }

      scanner.Close();

      return true;
    }
    catch (const IOException& e) {
      log.Error() << e.GetDescription();
      scanner.CloseFailsafe();

      return false;
    }
  }

  bool DebugDatabase::ResolveReferences(const std::set<ObjectOSMRef>& ids,
                                        const std::set<ObjectFileRef>& fileOffsets,
                                        std::multimap<ObjectOSMRef,ObjectFileRef>& idFileOffsetMap,
                                        std::map<ObjectFileRef,ObjectOSMRef>& fileOffsetIdMap)
  {
    bool haveToScanNodes=false;
    bool haveToScanAreas=false;
    bool haveToScanWays=false;

    for (const auto& id : ids) {
      if (haveToScanNodes && haveToScanAreas && haveToScanWays) {
        break;
      }

      switch (id.GetType()) {
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

    for (const auto& fileOffset : fileOffsets) {
      if (haveToScanNodes && haveToScanAreas && haveToScanWays) {
        break;
      }

      switch (fileOffset.GetType()) {
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
