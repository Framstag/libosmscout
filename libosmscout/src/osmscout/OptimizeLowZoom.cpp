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

#include <osmscout/OptimizeLowZoom.h>

#include <cassert>

#include <osmscout/Way.h>

#include <osmscout/Util.h>

#include <osmscout/util/Projection.h>
#include <osmscout/util/StopClock.h>
#include <osmscout/util/String.h>
#include <osmscout/util/Transformation.h>

#include <osmscout/private/Math.h>

#include <iostream>

namespace osmscout
{
  OptimizeLowZoom::OptimizeLowZoom()
  : datafile("optimized.dat"),
    magnification(0.0)
  {
    // no code
  }

  OptimizeLowZoom::~OptimizeLowZoom()
  {
    if (scanner.IsOpen()) {
      Close();
    }
  }

  bool OptimizeLowZoom::Open(const std::string& path)
  {
    datafilename=AppendFileToDir(path,datafile);

    if (!scanner.Open(datafilename)) {
      return false;
    }

    uint32_t optimizationMaxMag;
    uint32_t typeCount;

    scanner.Read(optimizationMaxMag);
    scanner.Read(typeCount);

    if (scanner.HasError()) {
      return false;
    }

    magnification=pow(2.0,(int)optimizationMaxMag);

    for (size_t i=1; i<=typeCount; i++) {
      TypeId   typeId;
      TypeData data;

      scanner.Read(typeId);

      scanner.Read(data.indexLevel);
      scanner.Read(data.cellXStart);
      scanner.Read(data.cellXEnd);
      scanner.Read(data.cellYStart);
      scanner.Read(data.cellYEnd);
      scanner.Read(data.bitmapOffset);

      data.cellXCount=data.cellXEnd-data.cellXStart+1;
      data.cellYCount=data.cellYEnd-data.cellYStart+1;

      data.cellWidth=360.0/pow(2.0,(int)data.indexLevel);
      data.cellHeight=180.0/pow(2.0,(int)data.indexLevel);

      data.minLon=data.cellXStart*data.cellWidth-180.0;
      data.maxLon=(data.cellXEnd+1)*data.cellWidth-180.0;
      data.minLat=data.cellYStart*data.cellHeight-90.0;
      data.maxLat=(data.cellYEnd+1)*data.cellHeight-90.0;

      typesData[typeId]=data;
    }

    return !scanner.HasError();
  }

  bool OptimizeLowZoom::Close()
  {
    bool success=true;

    if (scanner.IsOpen()) {
      if (!scanner.Close()) {
        success=false;
      }
    }

    return success;
  }

  bool OptimizeLowZoom::HasOptimizations(double magnification) const
  {
    return magnification<=this->magnification;
  }

  bool OptimizeLowZoom::GetOffsets(TypeId type,
                                   const TypeData& typeData,
                                   double minlon,
                                   double minlat,
                                   double maxlon,
                                   double maxlat,
                                   std::vector<FileOffset>& offsets) const
  {
    std::set<FileOffset> newOffsets;

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
      FileOffset cellIndexOffset=typeData.bitmapOffset+
                                 ((y-typeData.cellYStart)*typeData.cellXCount+
                                  minxc-typeData.cellXStart)*sizeof(FileOffset);

      cellDataOffsets.clear();

      if (!scanner.SetPos(cellIndexOffset)) {
        std::cerr << "Cannot go to type cell index position " << cellIndexOffset << std::endl;
        return false;
      }

      // For each column in row
      for (size_t x=minxc; x<=maxxc; x++) {
        FileOffset cellDataOffset;

        if (!scanner.Read(cellDataOffset)) {
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

      assert(offset>cellIndexOffset);

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

        for (size_t d=0; d<dataCount; d++) {
          FileOffset objectOffset;

          scanner.ReadNumber(objectOffset);

          objectOffset+=lastOffset;

          newOffsets.insert(objectOffset);

          lastOffset=objectOffset;
        }
      }
    }

    for (std::set<FileOffset>::const_iterator offset=newOffsets.begin();
         offset!=newOffsets.end();
         ++offset) {
      offsets.push_back(*offset);
    }

    return true;
  }

  bool OptimizeLowZoom::GetWays(double lonMin, double latMin,
                                double lonMax, double latMax,
                                size_t maxWayCount,
                                std::vector<TypeId>& wayTypes,
                                std::vector<WayRef>& ways) const
  {
    std::vector<TypeId>::iterator type=wayTypes.begin();
    std::vector<FileOffset>       offsets;

    if (!scanner.IsOpen()) {
      if (!scanner.Open(datafilename)) {
        std::cerr << "Error while opening " << datafilename << " for reading!" << std::endl;
        return false;
      }
    }

    offsets.reserve(20000);

    while (type!=wayTypes.end()) {
      std::map<TypeId, TypeData>::const_iterator data=typesData.find(*type);

      if (data!=typesData.end()) {
        if (!GetOffsets(*type,
                        data->second,
                        lonMin,
                        latMin,
                        lonMax,
                        latMax,
                        offsets)) {
          return false;
        }

        for (std::vector<FileOffset>::const_iterator offset=offsets.begin();
            offset!=offsets.end();
            ++offset) {
          if (!scanner.SetPos(*offset)) {
            std::cerr << "Error while positioning in file " << datafilename  << std::endl;
            type++;
            continue;
          }

          WayRef way=new Way();

          if (!way->Read(scanner)) {
            std::cerr << "Error while reading data entry of type " << *type << " from file " << datafilename  << std::endl;
            continue;
          }

          ways.push_back(way);
        }

        offsets.clear();
        type=wayTypes.erase(type);
      }
      else {
        type++;
      }
    }

    return true;
  }
}

