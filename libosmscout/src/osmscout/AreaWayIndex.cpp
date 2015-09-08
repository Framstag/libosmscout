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

#include <osmscout/util/Geometry.h>
#include <osmscout/util/Logger.h>
#include <osmscout/util/StopClock.h>

#include <osmscout/system/Math.h>

namespace osmscout {

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
  {
  }

  AreaWayIndex::AreaWayIndex()
  : filepart("areaway.idx")
  {
    // no code
  }

  void AreaWayIndex::Close()
  {
    if (scanner.IsOpen()) {
      scanner.Close();
    }
  }

  bool AreaWayIndex::Load(const TypeConfigRef& typeConfig,
                          const std::string& path)
  {
    datafilename=path+"/"+filepart;

    if (!scanner.Open(datafilename,FileScanner::LowMemRandom,true)) {
      log.Error() << "Cannot open file '" << scanner.GetFilename() << "'";
      return false;
    }

    uint32_t indexEntries;

    scanner.Read(indexEntries);

    for (size_t i=0; i<indexEntries; i++) {
      TypeId type;

      scanner.ReadTypeId(type,
                         typeConfig->GetWayTypeIdBytes());

      if (type>=wayTypeData.size()) {
        wayTypeData.resize(type+1);
      }

      scanner.ReadFileOffset(wayTypeData[type].bitmapOffset);

      if (wayTypeData[type].bitmapOffset>0) {
        scanner.Read(wayTypeData[type].dataOffsetBytes);

        scanner.ReadNumber(wayTypeData[type].indexLevel);

        scanner.ReadNumber(wayTypeData[type].cellXStart);
        scanner.ReadNumber(wayTypeData[type].cellXEnd);
        scanner.ReadNumber(wayTypeData[type].cellYStart);
        scanner.ReadNumber(wayTypeData[type].cellYEnd);

        wayTypeData[type].cellXCount=wayTypeData[type].cellXEnd-wayTypeData[type].cellXStart+1;
        wayTypeData[type].cellYCount=wayTypeData[type].cellYEnd-wayTypeData[type].cellYStart+1;

        wayTypeData[type].cellWidth=cellDimension[wayTypeData[type].indexLevel].width;
        wayTypeData[type].cellHeight=cellDimension[wayTypeData[type].indexLevel].height;

        wayTypeData[type].minLon=wayTypeData[type].cellXStart*wayTypeData[type].cellWidth-180.0;
        wayTypeData[type].maxLon=(wayTypeData[type].cellXEnd+1)*wayTypeData[type].cellWidth-180.0;
        wayTypeData[type].minLat=wayTypeData[type].cellYStart*wayTypeData[type].cellHeight-90.0;
        wayTypeData[type].maxLat=(wayTypeData[type].cellYEnd+1)*wayTypeData[type].cellHeight-90.0;
      }
    }

    return !scanner.HasError() && scanner.Close();
  }

  bool AreaWayIndex::GetOffsets(const TypeData& typeData,
                                const GeoBox& boundingBox,
                                size_t maxWayCount,
                                std::unordered_set<FileOffset>& offsets,
                                size_t currentSize,
                                bool& sizeExceeded) const
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

    FileOffset dataOffset=typeData.bitmapOffset+
                          typeData.cellXCount*typeData.cellYCount*(FileOffset)typeData.dataOffsetBytes;

    // For each row
    for (size_t y=minyc; y<=maxyc; y++) {
      FileOffset initialCellDataOffset=0;
      size_t     cellDataOffsetCount=0;
      FileOffset bitmapCellOffset=typeData.bitmapOffset+
                                  ((y-typeData.cellYStart)*typeData.cellXCount+
                                   minxc-typeData.cellXStart)*(FileOffset)typeData.dataOffsetBytes;

      if (!scanner.SetPos(bitmapCellOffset)) {
        log.Error() << "Cannot go to type cell index position " << bitmapCellOffset << " in file '" << scanner.GetFilename() << "'";
        return false;
      }

      // For each column in row
      for (size_t x=minxc; x<=maxxc; x++) {
        FileOffset cellDataOffset;

        if (!scanner.ReadFileOffset(cellDataOffset,
                                    typeData.dataOffsetBytes)) {
          log.Error() << "Cannot read cell data position in file '" << scanner.GetFilename() << "'";
          return false;
        }

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
      if (!scanner.SetPos(initialCellDataOffset)) {
        log.Error() << "Cannot go to cell data position " << initialCellDataOffset  << " in file '" << scanner.GetFilename() << "'";
        return false;
      }

      // For each data cell (in range) in row found
      for (size_t i=0; i<cellDataOffsetCount; i++) {
        uint32_t   dataCount;
        FileOffset lastOffset=0;


        if (!scanner.ReadNumber(dataCount)) {
          log.Error() << "Cannot read cell data count"  << " in file '" << scanner.GetFilename() << "'";
          return false;
        }

        if (currentSize+offsets.size()+dataCount>maxWayCount) {
          //std::cout << currentSize<< "+" << newOffsets.size() << "+" << dataCount << ">" << maxWayCount << std::endl;
          sizeExceeded=true;
          return true;
        }

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
                                const std::vector<TypeSet>& wayTypes,
                                size_t maxWayCount,
                                std::vector<FileOffset>& offsets) const
  {
    StopClock time;

    if (!scanner.IsOpen()) {
      if (!scanner.Open(datafilename,FileScanner::LowMemRandom,true)) {
        log.Error() << "Error while opening " << scanner.GetFilename() << " for reading!";
        return false;
      }
    }

    bool                           sizeExceeded=false;
    std::unordered_set<FileOffset> newOffsets;

    offsets.reserve(std::min(10000u,(uint32_t)maxWayCount));
    newOffsets.reserve(std::min(10000u,(uint32_t)maxWayCount));

    for (size_t i=0; i<wayTypes.size(); i++) {
      newOffsets.clear();

      for (TypeId type=0;
          type<wayTypeData.size();
          ++type) {
        if (wayTypes[i].IsTypeSet(type)) {
          if (!GetOffsets(wayTypeData[type],
                          boundingBox,
                          maxWayCount,
                          newOffsets,
                          offsets.size(),
                          sizeExceeded)) {
            return false;
          }

          if (sizeExceeded) {
            return true;
          }
        }
      }

      // Copy data from temporary set to final vector

      offsets.insert(offsets.end(),newOffsets.begin(),newOffsets.end());
    }

    //std::cout << "Found " << wayWayOffsets.size() << "+" << relationWayOffsets.size()<< " offsets in 'areaway.idx'" << std::endl;

    time.Stop();

    if (time.GetMilliseconds()>100) {
      log.Warn() << "Retrieving " << offsets.size() << " way offsets from area index took " << time.ResultString();
    }

    return true;
  }

  void AreaWayIndex::DumpStatistics()
  {
  }
}

