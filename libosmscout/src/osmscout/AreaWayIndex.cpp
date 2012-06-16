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

#include <iostream>

#include <osmscout/private/Math.h>

namespace osmscout {

  AreaWayIndex::TypeData::TypeData()
  : indexLevel(0),
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

  bool AreaWayIndex::Load(const std::string& path)
  {
    datafilename=path+"/"+filepart;

    if (!scanner.Open(datafilename)) {
      std::cerr << "Cannot open file '" << datafilename << "'" << std::endl;
      return false;
    }

    uint32_t indexEntries;

    scanner.Read(indexEntries);

    for (size_t i=0; i<indexEntries; i++) {
      TypeId type;

      scanner.ReadNumber(type);

      if (type>=wayTypeData.size()) {
        wayTypeData.resize(type+1);
      }

      scanner.ReadFileOffset(wayTypeData[type].bitmapOffset);

      if (wayTypeData[type].bitmapOffset>0) {
        scanner.ReadNumber(wayTypeData[type].indexLevel);

        scanner.ReadNumber(wayTypeData[type].cellXStart);
        scanner.ReadNumber(wayTypeData[type].cellXEnd);
        scanner.ReadNumber(wayTypeData[type].cellYStart);
        scanner.ReadNumber(wayTypeData[type].cellYEnd);

        wayTypeData[type].cellXCount=wayTypeData[type].cellXEnd-wayTypeData[type].cellXStart+1;
        wayTypeData[type].cellYCount=wayTypeData[type].cellYEnd-wayTypeData[type].cellYStart+1;

        wayTypeData[type].cellWidth=360.0/pow(2.0,(int)wayTypeData[type].indexLevel);
        wayTypeData[type].cellHeight=180.0/pow(2.0,(int)wayTypeData[type].indexLevel);

        wayTypeData[type].minLon=wayTypeData[type].cellXStart*wayTypeData[type].cellWidth-180.0;
        wayTypeData[type].maxLon=(wayTypeData[type].cellXEnd+1)*wayTypeData[type].cellWidth-180.0;
        wayTypeData[type].minLat=wayTypeData[type].cellYStart*wayTypeData[type].cellHeight-90.0;
        wayTypeData[type].maxLat=(wayTypeData[type].cellYEnd+1)*wayTypeData[type].cellHeight-90.0;
      }

      if (type>=relTypeData.size()) {
        relTypeData.resize(type+1);
      }

      scanner.ReadFileOffset(relTypeData[type].bitmapOffset);

      if (relTypeData[type].bitmapOffset>0) {
        scanner.ReadNumber(relTypeData[type].indexLevel);

        scanner.ReadNumber(relTypeData[type].cellXStart);
        scanner.ReadNumber(relTypeData[type].cellXEnd);
        scanner.ReadNumber(relTypeData[type].cellYStart);
        scanner.ReadNumber(relTypeData[type].cellYEnd);

        relTypeData[type].cellXCount=relTypeData[type].cellXEnd-relTypeData[type].cellXStart+1;
        relTypeData[type].cellYCount=relTypeData[type].cellYEnd-relTypeData[type].cellYStart+1;

        relTypeData[type].cellWidth=360.0/pow(2.0,(int)relTypeData[type].indexLevel);
        relTypeData[type].cellHeight=180.0/pow(2.0,(int)relTypeData[type].indexLevel);

        relTypeData[type].minLon=relTypeData[type].cellXStart*relTypeData[type].cellWidth-180.0;
        relTypeData[type].maxLon=(relTypeData[type].cellXEnd+1)*relTypeData[type].cellWidth-180.0;
        relTypeData[type].minLat=relTypeData[type].cellYStart*relTypeData[type].cellHeight-90.0;
        relTypeData[type].maxLat=(relTypeData[type].cellYEnd+1)*relTypeData[type].cellHeight-90.0;
      }
    }

    return !scanner.HasError() && scanner.Close();
  }

  bool AreaWayIndex::GetOffsets(const TypeData& typeData,
                                double minlon,
                                double minlat,
                                double maxlon,
                                double maxlat,
                                size_t maxWayCount,
                                OSMSCOUT_HASHSET<FileOffset>& offsets,
                                size_t currentSize,
                                bool& sizeExceeded) const
  {
    if (typeData.bitmapOffset==0) {
      // No data for this type available
      return true;
    }

    if (maxlon<typeData.minLon ||
        minlon>=typeData.maxLon ||
        maxlat<typeData.minLat ||
        minlat>=typeData.maxLat) {
      // No data available in given bounding box
      return true;
    }

    uint32_t minxc=(uint32_t)floor((minlon+180.0)/typeData.cellWidth);
    uint32_t maxxc=(uint32_t)floor((maxlon+180.0)/typeData.cellWidth);

    uint32_t minyc=(uint32_t)floor((minlat+90.0)/typeData.cellHeight);
    uint32_t maxyc=(uint32_t)floor((maxlat+90.0)/typeData.cellHeight);

    minxc=std::max(minxc,typeData.cellXStart);
    maxxc=std::min(maxxc,typeData.cellXEnd);

    minyc=std::max(minyc,typeData.cellYStart);
    maxyc=std::min(maxyc,typeData.cellYEnd);

    std::vector<FileOffset> cellDataOffsets;

    cellDataOffsets.reserve(maxxc-minxc+1);

    // For each row
    for (size_t y=minyc; y<=maxyc; y++) {
      FileOffset bitmapCellOffset=typeData.bitmapOffset+
                                  ((y-typeData.cellYStart)*typeData.cellXCount+
                                   minxc-typeData.cellXStart)*(FileOffset)8;

      cellDataOffsets.clear();

      if (!scanner.SetPos(bitmapCellOffset)) {
        std::cerr << "Cannot go to type cell index position " << bitmapCellOffset << std::endl;
        return false;
      }

      // For each column in row
      for (size_t x=minxc; x<=maxxc; x++) {
        FileOffset cellDataOffset;

        if (!scanner.ReadFileOffset(cellDataOffset)) {
          std::cerr << "Cannot read cell data position" << std::endl;
          return false;
        }

        if (cellDataOffset==0) {
          continue;
        }

        cellDataOffsets.push_back(cellDataOffset);
      }

      if (cellDataOffsets.empty()) {
        continue;
      }

      FileOffset offset=cellDataOffsets.front();

      assert(offset>bitmapCellOffset);

      if (!scanner.SetPos(offset)) {
        std::cerr << "Cannot go to cell data position " << offset << std::endl;
        return false;
      }

      // For each data cell in row found
      for (size_t i=0; i<cellDataOffsets.size(); i++) {
        uint32_t   dataCount;
        FileOffset lastOffset=0;


        if (!scanner.ReadNumber(dataCount)) {
          std::cerr << "Cannot read cell data count" << std::endl;
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

  bool AreaWayIndex::GetOffsets(double minlon,
                                double minlat,
                                double maxlon,
                                double maxlat,
                                const std::vector<TypeSet>& wayTypes,
                                size_t maxWayCount,
                                std::vector<FileOffset>& wayWayOffsets,
                                std::vector<FileOffset>& relationWayOffsets) const
  {
    if (!scanner.IsOpen()) {
      if (!scanner.Open(datafilename)) {
        std::cerr << "Error while opening " << datafilename << " for reading!" << std::endl;
        return false;
      }
    }

    bool                         sizeExceeded=false;
    OSMSCOUT_HASHSET<FileOffset> newWayWayOffsets;
    OSMSCOUT_HASHSET<FileOffset> newRelationWayOffsets;

    wayWayOffsets.reserve(std::min(100000u,(uint32_t)maxWayCount));
    relationWayOffsets.reserve(std::min(100000u,(uint32_t)maxWayCount));

#if defined(OSMSCOUT_HASHSET_HAS_RESERVE)
    newWayWayOffsets.reserve(std::min(100000u,(uint32_t)maxWayCount));
    newRelationWayOffsets.reserve(std::min(100000u,(uint32_t)maxWayCount));
#endif

    for (size_t i=0; i<wayTypes.size(); i++) {
      newWayWayOffsets.clear();
      newRelationWayOffsets.clear();

      for (size_t type=0;
          type<wayTypeData.size();
          ++type) {
        if (wayTypes[i].IsTypeSet(type)) {
          if (!GetOffsets(wayTypeData[type],
                          minlon,
                          minlat,
                          maxlon,
                          maxlat,
                          maxWayCount,
                          newWayWayOffsets,
                          wayWayOffsets.size()+
                          relationWayOffsets.size(),
                          sizeExceeded)) {
            return false;
          }

          if (sizeExceeded) {
            return true;
          }

          if (!GetOffsets(relTypeData[type],
                          minlon,
                          minlat,
                          maxlon,
                          maxlat,
                          maxWayCount,
                          newRelationWayOffsets,
                          wayWayOffsets.size()+relationWayOffsets.size(),
                          sizeExceeded)) {
            return false;
          }

          if (sizeExceeded) {
            return true;
          }
        }
      }

      // Copy data from temporary set to final vector

      wayWayOffsets.insert(wayWayOffsets.end(),newWayWayOffsets.begin(),newWayWayOffsets.end());
      relationWayOffsets.insert(relationWayOffsets.end(),newRelationWayOffsets.begin(),newRelationWayOffsets.end());
    }

    //std::cout << "Found " << wayWayOffsets.size() << "+" << relationWayOffsets.size()<< " offsets in 'areaway.idx'" << std::endl;

    return true;
  }

  void AreaWayIndex::DumpStatistics()
  {
  }
}

