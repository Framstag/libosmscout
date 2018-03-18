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

#include <osmscout/AreaWayIndex.h>

#include <algorithm>

#include <osmscout/util/File.h>
#include <osmscout/util/Geometry.h>
#include <osmscout/util/Logger.h>
#include <osmscout/util/StopClock.h>

#include <osmscout/system/Math.h>

namespace osmscout {

  const char* AreaWayIndex::AREA_WAY_IDX="areaway.idx";

  FileOffset AreaWayIndex::TypeData::GetDataOffset() const
  {
    return bitmapOffset+cellXCount*cellYCount*(FileOffset)dataOffsetBytes;
  }

  FileOffset AreaWayIndex::TypeData::GetCellOffset(size_t x, size_t y) const
  {
    return bitmapOffset+((y-cellYStart)*cellXCount+x-cellXStart)*dataOffsetBytes;
  }

  AreaWayIndex::TypeData::TypeData()
  : indexLevel(0),
    dataOffsetBytes(0),
    bitmapOffset(0),
    cellXStart(0),
    cellXEnd(0),
    cellYStart(0),
    cellYEnd(0),
    cellXCount(0),
    cellYCount(0),
    cellWidth(0.0),
    cellHeight(0.0),
    minLon(0.0),
    maxLon(0.0),
    minLat(0.0),
    maxLat(0.0)
  {}

  AreaWayIndex::AreaWayIndex()
  {
    // no code
  }

  AreaWayIndex::~AreaWayIndex()
  {
    Close();
  }

  void AreaWayIndex::Close()
  {
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

  bool AreaWayIndex::Open(const TypeConfigRef& typeConfig,
                          const std::string& path,
                          bool memoryMappedData)
  {
    datafilename=AppendFileToDir(path,AREA_WAY_IDX);

    try {
      scanner.Open(datafilename,FileScanner::FastRandom,memoryMappedData);

      uint32_t indexEntries;

      scanner.Read(indexEntries);

      wayTypeData.reserve(indexEntries);

      for (size_t i=0; i<indexEntries; i++) {
        TypeId typeId;

        scanner.ReadTypeId(typeId,
                           typeConfig->GetWayTypeIdBytes());

        TypeData data;

        data.type=typeConfig->GetWayTypeInfo(typeId);

        scanner.ReadFileOffset(data.bitmapOffset);

        if (data.bitmapOffset>0) {
          scanner.Read(data.dataOffsetBytes);

          scanner.ReadNumber(data.indexLevel);

          scanner.ReadNumber(data.cellXStart);
          scanner.ReadNumber(data.cellXEnd);
          scanner.ReadNumber(data.cellYStart);
          scanner.ReadNumber(data.cellYEnd);

          data.cellXCount=data.cellXEnd-data.cellXStart+1;
          data.cellYCount=data.cellYEnd-data.cellYStart+1;

          data.cellWidth=cellDimension[data.indexLevel].width;
          data.cellHeight=cellDimension[data.indexLevel].height;

          data.minLon=data.cellXStart*data.cellWidth-180.0;
          data.maxLon=(data.cellXEnd+1)*data.cellWidth-180.0;
          data.minLat=data.cellYStart*data.cellHeight-90.0;
          data.maxLat=(data.cellYEnd+1)*data.cellHeight-90.0;
        }

        wayTypeData.push_back(data);
      }

      return !scanner.HasError();
    }
    catch (IOException& e) {
      log.Error() << e.GetDescription();

      return false;
    }
  }

  bool AreaWayIndex::GetOffsets(const TypeData& typeData,
                                const GeoBox& boundingBox,
                                std::unordered_set<FileOffset>& offsets) const
  {
    if (typeData.bitmapOffset==0) {

      // No data for this type available
      return true;
    }

    if (boundingBox.GetMaxLon()<typeData.minLon ||
        boundingBox.GetMinLon()>=typeData.maxLon ||
        boundingBox.GetMaxLat()<typeData.minLat ||
        boundingBox.GetMinLat()>=typeData.maxLat) {

      // No data available in given bounding box
      return true;
    }

    uint32_t minxc=(uint32_t)floor((boundingBox.GetMinLon()+180.0)/typeData.cellWidth);
    uint32_t maxxc=(uint32_t)floor((boundingBox.GetMaxLon()+180.0)/typeData.cellWidth);

    uint32_t minyc=(uint32_t)floor((boundingBox.GetMinLat()+90.0)/typeData.cellHeight);
    uint32_t maxyc=(uint32_t)floor((boundingBox.GetMaxLat()+90.0)/typeData.cellHeight);

    minxc=std::max(minxc,typeData.cellXStart);
    maxxc=std::min(maxxc,typeData.cellXEnd);

    minyc=std::max(minyc,typeData.cellYStart);
    maxyc=std::min(maxyc,typeData.cellYEnd);

    FileOffset dataOffset=typeData.GetDataOffset();

    // For each row
    for (size_t y=minyc; y<=maxyc; y++) {
      std::lock_guard<std::mutex> guard(lookupMutex);
      FileOffset                  initialCellDataOffset=0;
      size_t                      cellDataOffsetCount=0;
      FileOffset                  bitmapCellOffset=typeData.GetCellOffset(minxc,y);

      scanner.SetPos(bitmapCellOffset);

      // For each column in row
      for (size_t x=minxc; x<=maxxc; x++) {
        FileOffset cellDataOffset;

        scanner.ReadFileOffset(cellDataOffset,
                               typeData.dataOffsetBytes);

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
        uint32_t   dataCount;
        FileOffset lastOffset=0;

        scanner.ReadNumber(dataCount);

        for (size_t d=0; d<dataCount; d++) {
          FileOffset objectOffset;

          scanner.ReadNumber(objectOffset);

          objectOffset+=lastOffset;

          offsets.insert(objectOffset);

          lastOffset=objectOffset;
        }
      }
    }

    return true;
  }

  bool AreaWayIndex::GetOffsets(const GeoBox& boundingBox,
                                const TypeInfoSet& types,
                                std::vector<FileOffset>& offsets,
                                TypeInfoSet& loadedTypes) const
  {
    StopClock time;

    offsets.reserve(std::min((size_t)10000,offsets.capacity()));
    loadedTypes.Clear();

    std::unordered_set<FileOffset> uniqueOffsets;

    try {
      for (const auto& data : wayTypeData) {
        if (types.IsSet(data.type)) {
          if (!GetOffsets(data,
                          boundingBox,
                          uniqueOffsets)) {
            return false;
          }

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
                 << " way offsets from area index for "
                 << boundingBox.GetDisplayText()
                 << " took " << time.ResultString();
    }

    return true;
  }
}
