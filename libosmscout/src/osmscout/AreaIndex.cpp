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

#include <osmscout/AreaIndex.h>

#include <algorithm>

#include <osmscout/util/File.h>
#include <osmscout/util/Geometry.h>
#include <osmscout/util/Logger.h>
#include <osmscout/util/StopClock.h>

#include <osmscout/system/Math.h>

namespace osmscout {

  FileOffset AreaIndex::TypeData::GetDataOffset() const
  {
    return bitmapOffset+tileBox.GetCount()*(FileOffset)dataOffsetBytes;
  }

  FileOffset AreaIndex::TypeData::GetCellOffset(size_t x, size_t y) const
  {
    return bitmapOffset+((y-tileBox.GetMinY())*tileBox.GetWidth()+x-tileBox.GetMinX())*dataOffsetBytes;
  }

  AreaIndex::TypeData::TypeData()
  : indexLevel(0),
    dataOffsetBytes(0),
    bitmapOffset(0),
    tileBox(TileId(0,0),
            TileId(0,0))
  {}

  AreaIndex::AreaIndex(const std::string &indexFileName):
    indexFileName(indexFileName)
  {}

  AreaIndex::~AreaIndex()
  {
    Close();
  }

  void AreaIndex::Close()
  {
    typeData.clear();
    try {
      if (scanner.IsOpen()) {
        scanner.Close();
      }
    }
    catch (IOException& e) {
      log.Error() << e.GetDescription();
      scanner.CloseFailsafe();
    }
  }

  bool AreaIndex::Open(const TypeConfigRef& typeConfig,
                       const std::string& path,
                       bool memoryMappedData)
  {
    fullIndexFileName=AppendFileToDir(path,indexFileName);

    try {
      scanner.Open(fullIndexFileName,FileScanner::FastRandom,memoryMappedData);

      uint32_t indexEntries=scanner.ReadUInt32();

      typeData.reserve(indexEntries);

      for (size_t i=0; i<indexEntries; i++) {
        TypeData data;
        ReadTypeData(typeConfig, data);

        data.bitmapOffset=scanner.ReadFileOffset();

        if (data.bitmapOffset>0) {
          data.dataOffsetBytes=scanner.ReadUInt8();

          uint32_t indexLevel=scanner.ReadUInt32Number();
          data.indexLevel=MagnificationLevel(indexLevel);

          uint32_t minX=scanner.ReadUInt32Number();
          uint32_t maxX=scanner.ReadUInt32Number();
          uint32_t minY=scanner.ReadUInt32Number();
          uint32_t maxY=scanner.ReadUInt32Number();

          data.tileBox=TileIdBox(TileId(minX,minY),
                                 TileId(maxX,maxY));

          data.boundingBox=data.tileBox.GetBoundingBox(Magnification(data.indexLevel));
        }

        typeData.push_back(data);
      }

      return !scanner.HasError();
    }
    catch (IOException& e) {
      log.Error() << e.GetDescription();

      return false;
    }
  }

  void AreaIndex::GetOffsets(const TypeData& typeData,
                             const GeoBox& boundingBox,
                             std::unordered_set<FileOffset>& offsets) const
  {
    if (typeData.bitmapOffset==0) {

      // No data for this type available
      return;
    }

    if (!boundingBox.Intersects(typeData.boundingBox)) {

      // No data available in given bounding box
      return;
    }

    TileIdBox boundingTileBox(Magnification(typeData.indexLevel),
                              boundingBox);

    if (!boundingTileBox.Intersects(typeData.tileBox)) {
      // No data available in given bounding box
      return;
    }

    boundingTileBox=boundingTileBox.Intersection(typeData.tileBox);

    FileOffset dataOffset=typeData.GetDataOffset();

    // For each row
    for (size_t y=boundingTileBox.GetMinY(); y<=boundingTileBox.GetMaxY(); y++) {
      std::lock_guard<std::mutex> guard(lookupMutex);
      FileOffset                  initialCellDataOffset=0;
      size_t                      cellDataOffsetCount=0;
      FileOffset                  bitmapCellOffset=typeData.GetCellOffset(boundingTileBox.GetMinX(),y);

      scanner.SetPos(bitmapCellOffset);

      // For each column in row
      for (size_t x=boundingTileBox.GetMinX(); x<=boundingTileBox.GetMaxX(); x++) {
        FileOffset cellDataOffset=scanner.ReadFileOffset(typeData.dataOffsetBytes);

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

      // We did not find any cells in the current row
      if (cellDataOffsetCount==0) {
        continue;
      }

      // The first data entry must be positioned behind the bitmap
      assert(initialCellDataOffset>=bitmapCellOffset);

      // first data entry in the row
      scanner.SetPos(initialCellDataOffset);

      // For each data cell (in range) in row found
      for (size_t i=0; i<cellDataOffsetCount; i++) {
        uint32_t   dataCount=scanner.ReadUInt32Number();
        FileOffset lastOffset=0;

        for (size_t d=0; d<dataCount; d++) {
          FileOffset objectOffset=scanner.ReadUInt64Number();

          objectOffset+=lastOffset;

          offsets.insert(objectOffset);

          lastOffset=objectOffset;
        }
      }
    }
  }

  bool AreaIndex::GetOffsets(const GeoBox& boundingBox,
                             const TypeInfoSet& types,
                             std::vector<FileOffset>& offsets,
                             TypeInfoSet& loadedTypes) const
  {
    StopClock time;

    offsets.reserve(std::min((size_t)10000,offsets.capacity()));
    loadedTypes.Clear();

    std::unordered_set<FileOffset> uniqueOffsets;

    try {
      for (const auto& data : typeData) {
        if (types.IsSet(data.type)) {
          GetOffsets(data,
                     boundingBox,
                     uniqueOffsets);

          loadedTypes.Set(data.type);
        }
      }
    }
    catch (IOException& e) {
      log.Error() << e.GetDescription();

      return false;
    }

    offsets.insert(offsets.end(),uniqueOffsets.begin(),uniqueOffsets.end());

    //std::cout << "Found " << wayWayOffsets.size() << "+" << relationWayOffsets.size()<< " offsets in 'areaway.idx'" << std::endl;

    time.Stop();

    if (time.GetMilliseconds()>100) {
      log.Warn() << "Retrieving " << offsets.size()
                 << " offsets from area index for "
                 << boundingBox.GetDisplayText()
                 << " took " << time.ResultString();
    }

    return true;
  }
}
