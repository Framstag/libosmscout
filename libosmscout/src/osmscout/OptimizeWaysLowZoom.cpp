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

#include <osmscout/OptimizeWaysLowZoom.h>

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

namespace osmscout
{
  const char* OptimizeWaysLowZoom::FILE_WAYSOPT_DAT = "waysopt.dat";

  OptimizeWaysLowZoom::OptimizeWaysLowZoom()
  : magnification(0.0)
  {
    // no code
  }

  OptimizeWaysLowZoom::~OptimizeWaysLowZoom()
  {
    Close();
  }

  void OptimizeWaysLowZoom::ReadTypeData(FileScanner& scanner,
                                         OptimizeWaysLowZoom::TypeData& data)
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
  }

  bool OptimizeWaysLowZoom::Open(const TypeConfigRef& typeConfig,
                                 const std::string& path,
                                 bool memoryMappedData)
  {
    this->typeConfig=typeConfig;
    datafilename=AppendFileToDir(path,FILE_WAYSOPT_DAT);

    try {
      scanner.Open(datafilename,FileScanner::LowMemRandom,memoryMappedData);

      FileOffset indexOffset;

      scanner.ReadFileOffset(indexOffset);

      scanner.SetPos(indexOffset);

      uint32_t optimizationMaxMag;
      uint32_t wayTypeCount;

      scanner.Read(optimizationMaxMag);
      scanner.Read(wayTypeCount);

      if (scanner.HasError()) {
        return false;
      }

      magnification=pow(2.0,(int)optimizationMaxMag);

      for (size_t i=1; i<=wayTypeCount; i++) {
        TypeId typeId;

        scanner.Read(typeId);

        TypeInfoRef type=typeConfig->GetWayTypeInfo(typeId);

        TypeData typeData;

        ReadTypeData(scanner,
                     typeData);

        wayTypesData[type].push_back(typeData);
      }

      return !scanner.HasError();
    }
    catch (IOException& e) {
      log.Error() << e.GetDescription();
      return false;
    }
  }

  bool OptimizeWaysLowZoom::Close()
  {
    try  {
      if (scanner.IsOpen()) {
        scanner.Close();
      }
    }
    catch (IOException& e) {
      log.Error() << e.GetDescription();
      scanner.CloseFailsafe();
      return false;
    }

    return true;
  }

  bool OptimizeWaysLowZoom::HasOptimizations(double magnification) const
  {
    return magnification<=this->magnification;
  }

  void OptimizeWaysLowZoom::GetOffsets(const TypeData& typeData,
                                       const GeoBox& boundingBox,
                                       std::set<FileOffset>& offsets) const
  {
    std::lock_guard<std::mutex> guard(lookupMutex);

    if (typeData.bitmapOffset==0) {
      // No data for this type available
      return;
    }

    if (boundingBox.GetMaxLon()<typeData.minLon ||
        boundingBox.GetMinLon()>=typeData.maxLon ||
        boundingBox.GetMaxLat()<typeData.minLat ||
        boundingBox.GetMinLat()>=typeData.maxLat) {
      // No data available in given bounding box
      return;
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

      scanner.SetPos(cellIndexOffset);

      // For each column in row
      for (size_t x=minxc; x<=maxxc; x++) {
        FileOffset cellDataOffset;

        scanner.ReadFileOffset(cellDataOffset,
                               typeData.dataOffsetBytes);

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

      scanner.SetPos(initialCellDataOffset);

      // For each data cell in row found
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
  }

  void OptimizeWaysLowZoom::LoadData(std::set<FileOffset>& offsets,
                                     std::vector<WayRef>& ways) const
  {
    std::lock_guard<std::mutex> guard(lookupMutex);

    for (const auto& offset : offsets) {
      scanner.SetPos(offset);

      WayRef way=std::make_shared<Way>();

      way->ReadOptimized(*typeConfig,
                         scanner);

      ways.push_back(way);
    }
  }

  /**
   * Returns the subset of types of wayTypes that can get retrieved from this index.
   */
  void OptimizeWaysLowZoom::GetTypes(const Magnification& magnification,
                                     const TypeInfoSet& wayTypes,
                                     TypeInfoSet& availableWayTypes) const
  {
    availableWayTypes.Clear();

    for (const auto& type : wayTypesData) {
      if (wayTypes.IsSet(type.first)) {
        for (const auto& typeData : type.second) {
          if (typeData.optLevel==magnification.GetLevel()) {
            availableWayTypes.Set(type.first);
            break;
          }
        }
      }
    }
  }

  bool OptimizeWaysLowZoom::GetWays(const GeoBox& boundingBox,
                                    const Magnification& magnification,
                                    const TypeInfoSet& wayTypes,
                                    std::vector<WayRef>& ways,
                                    TypeInfoSet& loadedWayTypes) const
  {
    StopClock            time;
    std::set<FileOffset> offsets;

    loadedWayTypes.Clear();

    try {
      for (std::map<TypeInfoRef,std::list<TypeData> >::const_iterator type=wayTypesData.begin();
           type!=wayTypesData.end();
           ++type) {
        if (wayTypes.IsSet(type->first)) {
          std::list<TypeData>::const_iterator match=type->second.end();

          for (std::list<TypeData>::const_iterator typeData=type->second.begin();
               typeData!=type->second.end();
               ++typeData) {
            if (typeData->optLevel==magnification.GetLevel()) {
              match=typeData;
              break;
            }
          }

          if (match!=type->second.end()) {
            if (match->bitmapOffset!=0) {
              GetOffsets(*match,
                         boundingBox,
                         offsets);
            }

            // Successfully loaded type data
            loadedWayTypes.Set(type->first);
          }
        }
      }

      LoadData(offsets,
               ways);
    }
    catch (IOException& e) {
      log.Error() << e.GetDescription();
      return false;
    }

    time.Stop();

    if (time.GetMilliseconds()>100) {
      log.Warn() << "Retrieving " << ways.size() << " optimized ways from area index took " << time.ResultString();
    }

    return true;
  }
}

