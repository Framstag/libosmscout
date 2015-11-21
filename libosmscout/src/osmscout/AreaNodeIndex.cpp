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

#include <osmscout/AreaNodeIndex.h>

#include <algorithm>

#include <osmscout/util/Geometry.h>
#include <osmscout/util/Logger.h>
#include <osmscout/util/StopClock.h>

#include <osmscout/system/Math.h>

namespace osmscout {

  AreaNodeIndex::TypeData::TypeData()
  : indexLevel(0),
    indexOffset(0),
    dataOffsetBytes(0),
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

  AreaNodeIndex::AreaNodeIndex()
  : filepart("areanode.idx")
  {
    // no code
  }

  void AreaNodeIndex::Close()
  {
    if (scanner.IsOpen()) {
      scanner.Close();
    }
  }

  bool AreaNodeIndex::Load(const std::string& path)
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

      scanner.ReadNumber(type);

      if (type>=nodeTypeData.size()) {
        nodeTypeData.resize(type+1);
      }

      scanner.ReadFileOffset(nodeTypeData[type].indexOffset);
      scanner.Read(nodeTypeData[type].dataOffsetBytes);

      scanner.ReadNumber(nodeTypeData[type].indexLevel);

      scanner.ReadNumber(nodeTypeData[type].cellXStart);
      scanner.ReadNumber(nodeTypeData[type].cellXEnd);
      scanner.ReadNumber(nodeTypeData[type].cellYStart);
      scanner.ReadNumber(nodeTypeData[type].cellYEnd);

      nodeTypeData[type].cellXCount=nodeTypeData[type].cellXEnd-nodeTypeData[type].cellXStart+1;
      nodeTypeData[type].cellYCount=nodeTypeData[type].cellYEnd-nodeTypeData[type].cellYStart+1;

      nodeTypeData[type].cellWidth=cellDimension[nodeTypeData[type].indexLevel].width;
      nodeTypeData[type].cellHeight=cellDimension[nodeTypeData[type].indexLevel].height;

      nodeTypeData[type].minLon=nodeTypeData[type].cellXStart*nodeTypeData[type].cellWidth-180.0;
      nodeTypeData[type].maxLon=(nodeTypeData[type].cellXEnd+1)*nodeTypeData[type].cellWidth-180.0;
      nodeTypeData[type].minLat=nodeTypeData[type].cellYStart*nodeTypeData[type].cellHeight-90.0;
      nodeTypeData[type].maxLat=(nodeTypeData[type].cellYEnd+1)*nodeTypeData[type].cellHeight-90.0;
    }

    return !scanner.HasError() && scanner.Close();
  }

  bool AreaNodeIndex::GetOffsets(const TypeData& typeData,
                                 const GeoBox& boundingBox,
                                 size_t maxNodeCount,
                                 std::vector<FileOffset>& offsets,
                                 size_t currentSize,
                                 bool& sizeExceeded) const
  {
    if (typeData.indexOffset==0) {
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

    std::unordered_set<FileOffset> newOffsets;

    uint32_t             minxc=(uint32_t)floor((boundingBox.GetMinLon()+180.0)/typeData.cellWidth);
    uint32_t             maxxc=(uint32_t)floor((boundingBox.GetMaxLon()+180.0)/typeData.cellWidth);

    uint32_t             minyc=(uint32_t)floor((boundingBox.GetMinLat()+90.0)/typeData.cellHeight);
    uint32_t             maxyc=(uint32_t)floor((boundingBox.GetMaxLat()+90.0)/typeData.cellHeight);

    minxc=std::max(minxc,typeData.cellXStart);
    maxxc=std::min(maxxc,typeData.cellXEnd);

    minyc=std::max(minyc,typeData.cellYStart);
    maxyc=std::min(maxyc,typeData.cellYEnd);

    FileOffset dataOffset=typeData.indexOffset+
                          typeData.cellXCount*typeData.cellYCount*(FileOffset)typeData.dataOffsetBytes;

    // For each row
    for (size_t y=minyc; y<=maxyc; y++) {
      FileOffset initialCellDataOffset=0;
      size_t     cellDataOffsetCount=0;
      FileOffset cellIndexOffset=typeData.indexOffset+
                                 ((y-typeData.cellYStart)*typeData.cellXCount+
                                  minxc-typeData.cellXStart)*typeData.dataOffsetBytes;

      if (!scanner.SetPos(cellIndexOffset)) {
        log.Error() << "Cannot go to type cell index position " << cellIndexOffset << " in file '" << scanner.GetFilename() << "'";
        return false;
      }

      // For each column in row
      for (size_t x=minxc; x<=maxxc; x++) {
        FileOffset cellDataOffset;

        if (!scanner.ReadFileOffset(cellDataOffset,
                                    typeData.dataOffsetBytes)) {
          log.Error() << "Cannot read cell data position from file '" << scanner.GetFilename() << "'";
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

      if (cellDataOffsetCount==0) {
        continue;
      }

      assert(initialCellDataOffset>=cellIndexOffset);

      if (!scanner.SetPos(initialCellDataOffset)) {
        log.Error() << "Cannot go to cell data position " << initialCellDataOffset << " in file '" << scanner.GetFilename() << "'";
        return false;
      }

      // For each data cell in row found
      for (size_t i=0; i<cellDataOffsetCount; i++) {
        uint32_t   dataCount;
        FileOffset lastOffset=0;


        if (!scanner.ReadNumber(dataCount)) {
          log.Error() << "Cannot read cell data count from file '" << scanner.GetFilename() << "'";
          return false;
        }

        if (currentSize+newOffsets.size()+dataCount>maxNodeCount) {
          sizeExceeded=true;
          return true;
        }

        for (size_t d=0; d<dataCount; d++) {
          FileOffset objectOffset;

          scanner.ReadNumber(objectOffset);

          objectOffset+=lastOffset;

          newOffsets.insert(objectOffset);

          lastOffset=objectOffset;
        }
      }
    }

    offsets.insert(offsets.end(),newOffsets.begin(),newOffsets.end());

    return true;
  }

  bool AreaNodeIndex::GetOffsets(const GeoBox& boundingBox,
                                 const TypeInfoSet& requestedTypes,
                                 size_t maxNodeCount,
                                 std::vector<FileOffset>& offsets,
                                 TypeInfoSet& loadedTypes) const
  {
    StopClock time;

    if (!scanner.IsOpen()) {
      if (!scanner.Open(datafilename,FileScanner::LowMemRandom,true)) {
        log.Error() << "Error while opening file '" << scanner.GetFilename() << "' for reading!";
        return false;
      }
    }

    bool sizeExceeded=false;

    loadedTypes.Clear();

    offsets.reserve(std::min(10000u,(uint32_t)maxNodeCount));

    for (TypeInfoRef type : requestedTypes) {
      if (!GetOffsets(nodeTypeData[type->GetNodeId()],
                      boundingBox,
                      maxNodeCount,
                      offsets,
                      offsets.size(),
                      sizeExceeded)) {
        return false;
      }

      if (sizeExceeded) {
        break;
      }

      loadedTypes.Set(type);
    }

    time.Stop();

    if (time.GetMilliseconds()>100) {
      log.Warn() << "Retrieving " << offsets.size() << " node offsets from area index took " << time.ResultString();
    }

    return true;
  }

  void AreaNodeIndex::DumpStatistics()
  {
  }
}

