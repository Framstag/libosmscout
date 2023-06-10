/*
  This source is part of the libosmscout library
  Copyright (C) 2011  Tim Teulings

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

#include <osmscout/db/AreaNodeIndex.h>

#include <algorithm>

#include <osmscout/util/File.h>
#include <osmscout/util/Logger.h>
#include <osmscout/util/StopClock.h>
#include <osmscout/util/TileId.h>

namespace osmscout {

  const char* const AreaNodeIndex::AREA_NODE_IDX="areanode.idx";

  void AreaNodeIndex::Close()
  {
    try {
      if (scanner.IsOpen()) {
        scanner.Close();
      }
    }
    catch (const IOException& e) {
      log.Error() << e.GetDescription();
      scanner.CloseFailsafe();
    }
  }

  bool AreaNodeIndex::Open(const std::string& path,
                           bool memoryMappedData)
  {
    std::string datafilename=AppendFileToDir(path,AREA_NODE_IDX);

    try {
      scanner.Open(datafilename,
                   FileScanner::FastRandom,
                   memoryMappedData);

      uint32_t gridMag=scanner.ReadUInt32();

      this->gridMag=MagnificationLevel(gridMag);

      uint16_t indexEntryCount=scanner.ReadUInt16();

      for (uint16_t i=1; i<=indexEntryCount; i++) {
        TypeId typeId=scanner.ReadUInt16Number();

        if (typeId>=nodeTypeData.size()) {
          nodeTypeData.resize(typeId+1);
        }

        nodeTypeData[typeId].isComplex=scanner.ReadBool();

        GeoCoord minCoord=scanner.ReadCoord();
        GeoCoord maxCoord=scanner.ReadCoord();

        TypeData& entry=nodeTypeData[typeId];

        entry.boundingBox.Set(minCoord,maxCoord);

        if (!entry.isComplex) {
          entry.indexOffset=scanner.ReadFileOffset();
          entry.entryCount=scanner.ReadUInt16();
        }
      }

      uint32_t tileEntryCount=scanner.ReadUInt32();

      for (uint32_t i=1; i<=tileEntryCount; i++) {
        TypeId typeId=scanner.ReadUInt16Number();

        if (typeId>=nodeTypeData.size()) {
          nodeTypeData.resize(typeId+1);
        }

        uint32_t x=scanner.ReadUInt32();
        uint32_t y=scanner.ReadUInt32();

        ListTile& entry=nodeTypeData[typeId].listTiles[TileId(x,y)];

        entry.fileOffset=scanner.ReadFileOffset();
        entry.entryCount=scanner.ReadUInt16();
        entry.storeGeoCoord=scanner.ReadBool();
      }

      uint32_t bitmapEntryCount=scanner.ReadUInt32();

      for (uint32_t i=1; i<=bitmapEntryCount; i++) {
        TypeId typeId=scanner.ReadUInt16Number();

        if (typeId>=nodeTypeData.size()) {
          nodeTypeData.resize(typeId+1);
        }

        uint8_t  magnification;

        uint32_t x=scanner.ReadUInt32();
        uint32_t y=scanner.ReadUInt32();

        BitmapTile& entry=nodeTypeData[typeId].bitmapTiles[TileId(x,y)];

        entry.fileOffset=scanner.ReadFileOffset();
        entry.dataOffsetBytes=scanner.ReadUInt8();

        magnification=scanner.ReadUInt8();

        entry.magnification=MagnificationLevel(magnification);
      }

      return !scanner.HasError();
    }
    catch (const IOException& e) {
      log.Error() << e.GetDescription();

      return false;
    }
  }

  bool AreaNodeIndex::GetOffsetsList(const TypeData& typeData,
                                     const GeoBox& boundingBox,
                                     std::vector<FileOffset>& offsets) const
  {
    std::scoped_lock<std::mutex> guard(lookupMutex);

    scanner.SetPos(typeData.indexOffset);

    FileOffset previousOffset=0;

    for (auto i=1; i<=typeData.entryCount; i++) {
      GeoCoord   coord;
      FileOffset fileOffset;

      coord=scanner.ReadCoord();
      fileOffset=scanner.ReadUInt64Number();

      fileOffset+=previousOffset;

      previousOffset=fileOffset;

      if (boundingBox.Includes(coord)) {
        offsets.push_back(fileOffset);
      }
    }

    return true;
  }

  bool AreaNodeIndex::GetOffsetsTileList(const TypeData& typeData,
                                         const GeoBox& boundingBox,
                                         std::vector<FileOffset>& offsets) const
  {
    std::scoped_lock<std::mutex> guard(lookupMutex);

    TileIdBox tileBox(TileId::GetTile(gridMag,boundingBox.GetMinCoord()),
                      TileId::GetTile(gridMag,boundingBox.GetMaxCoord()));

    for (const auto& tileId : tileBox) {
      auto tile=typeData.listTiles.find(tileId);

      if (tile!=typeData.listTiles.end()) {
        scanner.SetPos(tile->second.fileOffset);

        FileOffset previousOffset=0;

        if (tile->second.storeGeoCoord) {
          for (auto i=1; i<=tile->second.entryCount; i++) {
            GeoCoord   coord;
            FileOffset fileOffset;

            coord=scanner.ReadCoord();
            fileOffset=scanner.ReadUInt64Number();

            fileOffset+=previousOffset;

            previousOffset=fileOffset;

            if (boundingBox.Includes(coord)) {
              offsets.push_back(fileOffset);
            }
          }
        }
        else {
          for (auto i=1; i<=tile->second.entryCount; i++) {
            FileOffset fileOffset;

            fileOffset=scanner.ReadUInt64Number();

            fileOffset+=previousOffset;

            previousOffset=fileOffset;

            offsets.push_back(fileOffset);
          }
        }
      }
    }

    return true;
  }

  bool AreaNodeIndex::GetOffsetsBitmap(const TypeData& typeData,
                                       const GeoBox& boundingBox,
                                       std::vector<FileOffset>& offsets) const
  {
    TileIdBox searchTileBox(TileId::GetTile(gridMag,
                                            boundingBox.GetMinCoord()),
                            TileId::GetTile(gridMag,
                                            boundingBox.GetMaxCoord()));

    for (const auto& tileId : searchTileBox) {
      auto tileBitmap=typeData.bitmapTiles.find(tileId);

      if (tileBitmap!=typeData.bitmapTiles.end()) {
        MagnificationLevel magnification(tileBitmap->second.magnification);
        GeoBox             box=tileBitmap->first.GetBoundingBox(gridMag);
        TileIdBox          bitmapTileBox(TileId::GetTile(magnification,
                                                         box.GetMinCoord()),
                                         TileId::GetTile(magnification,
                                                         box.GetMaxCoord()));
        TileIdBox          boundingBoxTileBox(TileId::GetTile(magnification,
                                                              boundingBox.GetMinCoord()),
                                         TileId::GetTile(magnification,
                                                              boundingBox.GetMaxCoord()));

        // Offset of the data behind the bitmap
        FileOffset dataOffset=tileBitmap->second.fileOffset+bitmapTileBox.GetCount()*tileBitmap->second.dataOffsetBytes;

        auto minxc=std::max(bitmapTileBox.GetMinX(),boundingBoxTileBox.GetMinX());
        auto maxxc=std::min(bitmapTileBox.GetMaxX(),boundingBoxTileBox.GetMaxX());

        auto minyc=std::max(bitmapTileBox.GetMinY(),boundingBoxTileBox.GetMinY());
        auto maxyc=std::min(bitmapTileBox.GetMaxY(),boundingBoxTileBox.GetMaxY());

        // For each row
        for (auto y=minyc; y<=maxyc; y++) {
          std::scoped_lock<std::mutex> guard(lookupMutex);
          FileOffset                  initialCellDataOffset=0;
          size_t                      cellDataOffsetCount=0;
          FileOffset                  cellIndexOffset=tileBitmap->second.fileOffset+
                                                      ((y-bitmapTileBox.GetMinY())*bitmapTileBox.GetWidth()+
                                                       minxc-bitmapTileBox.GetMinX())*tileBitmap->second.dataOffsetBytes;

          scanner.SetPos(cellIndexOffset);

          // For each column in row
          for (size_t x=minxc; x<=maxxc; x++) {
            FileOffset cellDataOffset=scanner.ReadFileOffset(tileBitmap->second.dataOffsetBytes);

            if (cellDataOffset==0) {
              continue;
            }

            // We added +1 during import and now substract it again
            cellDataOffset--;

            if (initialCellDataOffset==0) {
              initialCellDataOffset=dataOffset+cellDataOffset;
            }

            cellDataOffsetCount++;
          }

          if (cellDataOffsetCount==0) {
            continue;
          }

          assert(initialCellDataOffset>=cellIndexOffset);

          scanner.SetPos(initialCellDataOffset);

          // For each data cell in row found
          for (size_t i=0; i<cellDataOffsetCount; i++) {
            FileOffset previousOffset=0;
            uint32_t   dataCount=scanner.ReadUInt32Number();

            for (uint32_t d=0; d<dataCount; d++) {
              FileOffset fileOffset=scanner.ReadUInt64Number();

              fileOffset+=previousOffset;

              offsets.push_back(fileOffset);

              previousOffset=fileOffset;
            }
          }
        }
      }
    }

    return true;
  }

  bool AreaNodeIndex::GetOffsets(const GeoBox& boundingBox,
                                 const TypeInfoSet& requestedTypes,
                                 std::vector<FileOffset>& offsets,
                                 TypeInfoSet& loadedTypes) const
  {

    StopClock time;

    loadedTypes.Clear();

    offsets.reserve(std::min((size_t)10000,offsets.capacity()));

    try {
      for (const TypeInfoRef& type : requestedTypes) {
        if (type->IsInternal()) {
          continue;
        }

        auto index=type->GetNodeId();
        if (index<nodeTypeData.size()) {
          if (!nodeTypeData[index].boundingBox.Intersects(boundingBox)) {
            // No data available in given bounding box
            continue;
          }

          if (!nodeTypeData[index].isComplex &&
              nodeTypeData[index].indexOffset!=0 &&
              nodeTypeData[index].entryCount!=0) {
            if (!GetOffsetsList(nodeTypeData[index],boundingBox,offsets)) {
              return false;
            }
          }
          else if (nodeTypeData[index].isComplex) {
            if (!GetOffsetsTileList(nodeTypeData[index],boundingBox,offsets)) {
              return false;
            }
            if (!GetOffsetsBitmap(nodeTypeData[index],boundingBox,offsets)) {
              return false;
            }
          }
          else {
            continue;
          }
        }

        loadedTypes.Set(type);
      }
    }
    catch (const IOException& e) {
      log.Error() << e.GetDescription();

      return false;
    }

    time.Stop();

    if (time.GetMilliseconds()>100) {
      log.Warn() << "Retrieving " << offsets.size()
                 << " node offsets from area index for "
                 << boundingBox.GetDisplayText()
                 << " took " << time.ResultString();
    }

    return true;
  }
}
