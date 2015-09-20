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

#include <osmscout/OptimizeAreasLowZoom.h>

#include <osmscout/Way.h>

#include <osmscout/system/Assert.h>
#include <osmscout/system/Math.h>

#include <osmscout/util/File.h>
#include <osmscout/util/Geometry.h>
#include <osmscout/util/Logger.h>
#include <osmscout/util/Projection.h>
#include <osmscout/util/StopClock.h>
#include <osmscout/util/String.h>
#include <osmscout/util/Transformation.h>

#include <iostream>

namespace osmscout
{
  OptimizeAreasLowZoom::OptimizeAreasLowZoom()
  : datafile("areasopt.dat"),
    magnification(0.0)
  {
    // no code
  }

  OptimizeAreasLowZoom::~OptimizeAreasLowZoom()
  {
    if (scanner.IsOpen()) {
      Close();
    }
  }

  bool OptimizeAreasLowZoom::ReadTypeData(FileScanner& scanner,
                                          OptimizeAreasLowZoom::TypeData& data)
  {
    scanner.Read(data.optLevel);
    scanner.Read(data.indexLevel);
    scanner.Read(data.cellXStart);
    scanner.Read(data.cellXEnd);
    scanner.Read(data.cellYStart);
    scanner.Read(data.cellYEnd);

    scanner.ReadFileOffset(data.bitmapOffset);
    scanner.Read(data.dataOffsetBytes);

    data.cellXCount=data.cellXEnd-data.cellXStart+1;
    data.cellYCount=data.cellYEnd-data.cellYStart+1;

    data.cellWidth=cellDimension[data.indexLevel].width;
    data.cellHeight=cellDimension[data.indexLevel].height;

    data.minLon=data.cellXStart*data.cellWidth-180.0;
    data.maxLon=(data.cellXEnd+1)*data.cellWidth-180.0;
    data.minLat=data.cellYStart*data.cellHeight-90.0;
    data.maxLat=(data.cellYEnd+1)*data.cellHeight-90.0;

    return !scanner.HasError();
  }

  bool OptimizeAreasLowZoom::Open(const TypeConfigRef& typeConfig,
                                  const std::string& path)
  {
    this->typeConfig=typeConfig;
    datafilename=AppendFileToDir(path,datafile);

    if (!scanner.Open(datafilename,FileScanner::LowMemRandom,true)) {
      log.Error() << "Cannot open file '" << scanner.GetFilename() << "'!";
      return false;
    }

    FileOffset indexOffset;

    if (!scanner.ReadFileOffset(indexOffset)) {
      log.Error() << "Cannot read index offset from file '" << scanner.GetFilename() << "'";
      return false;
    }

    if (!scanner.SetPos(indexOffset)) {
      log.Error() << "Cannot goto to start of index at position " << indexOffset << " in file '" << scanner.GetFilename() << "'";
      return false;
    }

    uint32_t optimizationMaxMag;
    uint32_t areaTypeCount;

    scanner.Read(optimizationMaxMag);
    scanner.Read(areaTypeCount);

    if (scanner.HasError()) {
      return false;
    }

    magnification=pow(2.0,(int)optimizationMaxMag);

    for (size_t i=1; i<=areaTypeCount; i++) {
      TypeId typeId;

      scanner.Read(typeId);

      TypeData typeData;

      if (!ReadTypeData(scanner,
                        typeData)) {
        return false;
      }

      areaTypesData[typeId].push_back(typeData);
    }

    return !scanner.HasError();
  }

  bool OptimizeAreasLowZoom::Close()
  {
    bool success=true;

    if (scanner.IsOpen()) {
      if (!scanner.Close()) {
        success=false;
      }
    }

    return success;
  }

  bool OptimizeAreasLowZoom::HasOptimizations(double magnification) const
  {
    return magnification<=this->magnification;
  }

  bool OptimizeAreasLowZoom::GetOffsets(const TypeData& typeData,
                                        const GeoBox& boundingBox,
                                        std::vector<FileOffset>& offsets) const
  {
    std::set<FileOffset> newOffsets;

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
      FileOffset cellIndexOffset=typeData.bitmapOffset+
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

  bool OptimizeAreasLowZoom::GetAreas(const GeoBox& boundingBox,
                                      const Magnification& magnification,
                                      TypeSet& areaTypes,
                                      std::vector<AreaRef>& areas,
                                      TypeInfoSet& loadedAreaTypes) const
  {
    StopClock time;

    std::vector<FileOffset> offsets;

    loadedAreaTypes.Clear();

    if (!scanner.IsOpen()) {
      if (!scanner.Open(datafilename,FileScanner::LowMemRandom,true)) {
        log.Error() << "Error while opening file '" << scanner.GetFilename() << "' for reading!";
        return false;
      }
    }

    offsets.reserve(20000);

    for (std::map<TypeId,std::list<TypeData> >::const_iterator type=areaTypesData.begin();
        type!=areaTypesData.end();
        ++type) {
      if (areaTypes.IsTypeSet(type->first)) {
        std::list<TypeData>::const_iterator match=type->second.end();

        for (std::list<TypeData>::const_iterator typeData=type->second.begin();
            typeData!=type->second.end();
            ++typeData) {
          if (typeData->optLevel==magnification.GetLevel()) {
            match=typeData;
          }
        }

        if (match!=type->second.end()) {
          if (match->bitmapOffset!=0) {
            if (!GetOffsets(*match,
                            boundingBox,
                            offsets)) {
              return false;
            }

            loadedAreaTypes.Set(typeConfig->GetAreaTypeInfo(type->first));

            for (const auto& offset : offsets) {
              if (!scanner.SetPos(offset)) {
                log.Error() << "Error while positioning in file '" << scanner.GetFilename()  << "'";
                return false;
              }

              AreaRef area=std::make_shared<Area>();

              if (!area->ReadOptimized(*typeConfig,
                                       scanner)) {
                log.Error() << "Error while reading area at offset " << offset << " from file '" << scanner.GetFilename()  << "'";
                return false;
              }

              areas.push_back(area);
            }

            offsets.clear();
          }
        }

        if (match!=type->second.end()) {
          areaTypes.UnsetType(type->first);
        }
      }
    }

    time.Stop();

    if (time.GetMilliseconds()>100) {
      log.Warn() << "Retrieving " << areas.size() << " optimized areas from area index took " << time.ResultString();
    }

    return true;
  }
}

