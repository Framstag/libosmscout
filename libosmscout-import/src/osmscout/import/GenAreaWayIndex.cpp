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

#include <osmscout/import/GenAreaWayIndex.h>

#include <vector>

#include <osmscout/Way.h>

#include <osmscout/system/Assert.h>
#include <osmscout/system/Math.h>

#include <osmscout/util/File.h>
#include <osmscout/util/FileScanner.h>
#include <osmscout/util/Number.h>
#include <osmscout/util/String.h>

#include <iostream>
namespace osmscout {

  AreaWayIndexGenerator::TypeData::TypeData()
  : indexLevel(0),
    indexCells(0),
    indexEntries(0),
    cellXStart(0),
    cellXEnd(0),
    cellYStart(0),
    cellYEnd(0),
    cellXCount(0),
    cellYCount(0),
    indexOffset(0)
  {
    // no code
  }

  std::string AreaWayIndexGenerator::GetDescription() const
  {
    return "Generate 'areaway.idx'";
  }

  bool AreaWayIndexGenerator::FitsIndexCriteria(const ImportParameter& /*parameter*/,
                                                Progress& progress,
                                                const TypeInfo& typeInfo,
                                                const TypeData& typeData,
                                                const CoordCountMap& cellFillCount) const
  {
    if (typeData.indexCells==0) {
      return true;
    }

    size_t overallCount=0;
    size_t maxCellCount=0;

    for (const auto& cell : cellFillCount) {
      overallCount+=cell.second;
      maxCellCount=std::max(maxCellCount,cell.second);
    }

    // Average number of entries per tile cell
    double average=overallCount*1.0/cellFillCount.size();

    size_t emptyCount=0;
    size_t toLowCount=0;
    size_t toHighCount=0;
    size_t inCount=0;
    size_t allCount=0;

    for (const auto& cell : cellFillCount) {
      if (cell.second==0) {
        emptyCount++;
      }
      else if (cell.second<0.4*average) {
        toLowCount++;
      }
      else if (cell.second>128){
        toHighCount++;
      }
      else {
        inCount++;
      }

      allCount++;
    }

    if (toHighCount*1.0/allCount>=0.05) {
      return false;
    }

    if (toLowCount*1.0/allCount>=0.2) {
      progress.Warning(typeInfo.GetName()+" has more than 20% cells with <40% of average filling ("+NumberToString(toLowCount)+"/"+NumberToString(allCount)+")");
    }

    /*
    // If the fill rate of the index is too low, we use this index level anyway
    if (fillRate<parameter.GetAreaWayIndexMinFillRate()) {
      progress.Warning(typeInfo.GetName()+" is not well distributed");
      return true;
    }

    // If average fill size and max fill size for tile cells
    // is within limits, store it now.
    if (maxCellCount<=parameter.GetAreaWayIndexCellSizeMax() &&
        average<=parameter.GetAreaWayIndexCellSizeAverage()) {
      return true;
    }*/

    return true;
  }

  /**
   * Calculate internal statistics that are the base for deciding if the given way type is
   * indexed at the given index level.
   * @param level
   * @param typeData
   * @param cellFillCount
   */
  void AreaWayIndexGenerator::CalculateStatistics(size_t level,
                                                  TypeData& typeData,
                                                  const CoordCountMap& cellFillCount) const
  {
    // Initialize/reset data structure
    typeData.indexLevel=(uint32_t)level;
    typeData.indexCells=cellFillCount.size();
    typeData.indexEntries=0;

    // If we do not have any entries, we are done ;-)
    if (cellFillCount.empty()) {
      return;
    }

    typeData.cellXStart=cellFillCount.begin()->first.x;
    typeData.cellYStart=cellFillCount.begin()->first.y;

    typeData.cellXEnd=typeData.cellXStart;
    typeData.cellYEnd=typeData.cellYStart;

    for (const auto& cell : cellFillCount) {
      typeData.indexEntries+=cell.second;

      typeData.cellXStart=std::min(typeData.cellXStart,cell.first.x);
      typeData.cellXEnd=std::max(typeData.cellXEnd,cell.first.x);

      typeData.cellYStart=std::min(typeData.cellYStart,cell.first.y);
      typeData.cellYEnd=std::max(typeData.cellYEnd,cell.first.y);
    }

    typeData.cellXCount=typeData.cellXEnd-typeData.cellXStart+1;
    typeData.cellYCount=typeData.cellYEnd-typeData.cellYStart+1;
  }

  bool AreaWayIndexGenerator::CalculateDistribution(const TypeConfigRef& typeConfig,
                                                    const ImportParameter& parameter,
                                                    Progress& progress,
                                                    std::vector<TypeData>& wayTypeData,
                                                    size_t& maxLevel) const
  {
    FileScanner wayScanner;
    TypeInfoSet remainingWayTypes;
    size_t      level;

    maxLevel=0;
    wayTypeData.resize(typeConfig->GetTypeCount());

    if (!wayScanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                         "ways.dat"),
                         FileScanner::Sequential,
                         parameter.GetWayDataMemoryMaped())) {
      progress.Error("Cannot open 'ways.dat'");
      return false;
    }

    remainingWayTypes.Set(typeConfig->GetWayTypes());

    level=parameter.GetAreaWayMinMag();
    while (!remainingWayTypes.Empty()) {
      uint32_t                   wayCount=0;
      TypeInfoSet                currentWayTypes(remainingWayTypes);
      double                     cellWidth=360.0/pow(2.0,(int)level);
      double                     cellHeight=180.0/pow(2.0,(int)level);
      std::vector<CoordCountMap> cellFillCount(typeConfig->GetTypeCount());

      progress.Info("Scanning Level "+NumberToString(level)+" ("+NumberToString(remainingWayTypes.Size())+" types remaining)");

      wayScanner.GotoBegin();

      if (!wayScanner.Read(wayCount)) {
        progress.Error("Error while reading number of data entries in file");
        return false;
      }

      Way way;

      for (uint32_t w=1; w<=wayCount; w++) {
        progress.SetProgress(w,wayCount);

        if (!way.Read(typeConfig,
                      wayScanner)) {
          progress.Error(std::string("Error while reading data entry ")+
                         NumberToString(w)+" of "+
                         NumberToString(wayCount)+
                         " in file '"+
                         wayScanner.GetFilename()+"'");
          return false;
        }

        // Count number of entries per current type and coordinate
        if (!currentWayTypes.IsSet(way.GetType())) {
          continue;
        }

        double minLon;
        double maxLon;
        double minLat;
        double maxLat;

        way.GetBoundingBox(minLon,maxLon,minLat,maxLat);

        //
        // Calculate minimum and maximum tile ids that are covered
        // by the way
        // Renormalized coordinate space (everything is >=0)
        //
        uint32_t minxc=(uint32_t)floor((minLon+180.0)/cellWidth);
        uint32_t maxxc=(uint32_t)floor((maxLon+180.0)/cellWidth);
        uint32_t minyc=(uint32_t)floor((minLat+90.0)/cellHeight);
        uint32_t maxyc=(uint32_t)floor((maxLat+90.0)/cellHeight);

        for (uint32_t y=minyc; y<=maxyc; y++) {
          for (uint32_t x=minxc; x<=maxxc; x++) {
            cellFillCount[way.GetType()->GetIndex()][Pixel(x,y)]++;
          }
        }
      }

      // Check if cell fill for current type is in defined limits
      for (auto &type : currentWayTypes) {
        size_t i=type->GetIndex();

        CalculateStatistics(level,
                            wayTypeData[i],
                            cellFillCount[i]);

        if (!FitsIndexCriteria(parameter,
                               progress,
                               typeConfig->GetTypeInfo(i),
                               wayTypeData[i],
                               cellFillCount[i])) {
          currentWayTypes.Remove(type);
        }
      }

      for (const auto &type : currentWayTypes) {
        maxLevel=std::max(maxLevel,level);

        progress.Info("Type "+type->GetName()+", "+NumberToString(wayTypeData[type->GetIndex()].indexCells)+" cells, "+NumberToString(wayTypeData[type->GetIndex()].indexEntries)+" objects");

        remainingWayTypes.Remove(type);
      }

      level++;
    }

    return !wayScanner.HasError() && wayScanner.Close();
  }

  /**
   * For each cell we store a file offset to the bitmap data or 0, if there is no data for the cell. The bitmap entry itself
   * contains the number of offsets followed by the offsets themselves (delta-encoded).
   *
   *
   * @param progress
   * @param writer
   * @param typeInfo
   * @param typeData
   * @param typeCellOffsets
   * @return
   */
  bool AreaWayIndexGenerator::WriteBitmap(Progress& progress,
                                          FileWriter& writer,
                                          const TypeInfo& typeInfo,
                                          const TypeData& typeData,
                                          const CoordOffsetsMap& typeCellOffsets)
  {
    size_t indexEntries=0;
    size_t dataSize=0;
    char   buffer[10];

    //
    // Calculate the number of entries and the overall size of the data in the bitmap entries
    // We need the overall size of the bitmap entry data, because we would store the file offset only with
    // that much bytes we need to address the last data entry.

    for (const auto& cell : typeCellOffsets) {
      indexEntries+=cell.second.size();

      dataSize+=EncodeNumber(cell.second.size(),
                             buffer);

      FileOffset previousOffset=0;

      for (const auto& offset : cell.second) {
        FileOffset data=offset-previousOffset;

        dataSize+=EncodeNumber(data,buffer);

        previousOffset=offset;
      }
    }

    // "+1" because we add +1 to every offset, to generate offset > 0
    uint8_t dataOffsetBytes=BytesNeededToAddressFileData(dataSize+1);

    progress.Info("Writing map for "+
                  typeInfo.GetName()+" , "+
                  ByteSizeToString(1.0*dataOffsetBytes*typeData.cellXCount*typeData.cellYCount+dataSize));

    FileOffset bitmapOffset;

    if (!writer.GetPos(bitmapOffset)) {
      progress.Error("Cannot get type index start position in file");
      return false;
    }

    assert(typeData.indexOffset!=0);

    if (!writer.SetPos(typeData.indexOffset)) {
      progress.Error("Cannot go to type index offset in file");
      return false;
    }

    writer.WriteFileOffset(bitmapOffset);
    writer.Write(dataOffsetBytes);

    if (!writer.SetPos(bitmapOffset)) {
      progress.Error("Cannot go to type index start position in file");
      return false;
    }

    // Write the bitmap with offsets for each cell
    // We prefill with zero and only overwrite cells that have data
    // So zero means "no data for this cell"
    for (size_t i=0; i<typeData.cellXCount*typeData.cellYCount; i++) {
      writer.WriteFileOffset(0,
                             dataOffsetBytes);
    }

    FileOffset dataStartOffset;

    if (!writer.GetPos(dataStartOffset)) {
      progress.Error("Cannot get start of data section after bitmap");
      return false;
    }

    // Now write the list of offsets of objects for every cell with content
    for (const auto& cell : typeCellOffsets) {
      FileOffset bitmapCellOffset=bitmapOffset+
                                  ((cell.first.y-typeData.cellYStart)*typeData.cellXCount+
                                    cell.first.x-typeData.cellXStart)*(FileOffset)dataOffsetBytes;
      FileOffset previousOffset=0;
      FileOffset cellOffset;

      assert(bitmapCellOffset>=bitmapOffset);

      if (!writer.GetPos(cellOffset)) {
        progress.Error("Cannot get cell start position in file");
        return false;
      }

      if (!writer.SetPos(bitmapCellOffset)) {
        progress.Error("Cannot go to cell start position in file");
        return false;
      }

      assert(cellOffset>bitmapCellOffset);

      // We add +1 to make sure, that we can differentiate between "0" as "no entry" and "0" as first data entry.
      writer.WriteFileOffset(cellOffset-dataStartOffset+1,dataOffsetBytes);

      if (!writer.SetPos(cellOffset)) {
        progress.Error("Cannot go back to cell start position in file");
        return false;
      }

      writer.WriteNumber((uint32_t)cell.second.size());

      // FileOffsets are already in increasing order, since
      // File is scanned from start to end
      for (const auto& offset : cell.second) {
        assert(offset>previousOffset);

        writer.WriteNumber((FileOffset)(offset-previousOffset));

        previousOffset=offset;
      }
    }

    return true;
  }

  bool AreaWayIndexGenerator::Import(const TypeConfigRef& typeConfig,
                                     const ImportParameter& parameter,
                                     Progress& progress)
  {
    FileScanner           wayScanner;
    FileWriter            writer;
    std::vector<TypeData> wayTypeData;
    size_t                maxLevel;

    progress.Info("Minimum magnification: "+NumberToString(parameter.GetAreaWayMinMag()));

    //
    // Scanning distribution
    //

    progress.SetAction("Scanning level distribution of way types");

    if (!CalculateDistribution(typeConfig,
                               parameter,
                               progress,
                               wayTypeData,
                               maxLevel)) {
      return false;
    }

    // Calculate number of types which have data

    uint32_t indexEntries=0;

    for (const auto& type : typeConfig->GetWayTypes())
    {
      if (wayTypeData[type->GetIndex()].HasEntries()) {
        indexEntries++;
      }
    }

    //
    // Writing index file
    //

    progress.SetAction("Generating 'areaway.idx'");

    if (!writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                     "areaway.idx"))) {
      progress.Error("Cannot create 'areaway.idx'");
      return false;
    }

    writer.Write(indexEntries);

    for (const auto &type : typeConfig->GetWayTypes()) {
      size_t i=type->GetIndex();

      if (wayTypeData[i].HasEntries()) {
        uint8_t    dataOffsetBytes=0;
        FileOffset bitmapOffset=0;

        writer.WriteTypeId(type->GetWayId(),
                           typeConfig->GetWayTypeIdBytes());

        writer.GetPos(wayTypeData[i].indexOffset);

        writer.WriteFileOffset(bitmapOffset);
        writer.Write(dataOffsetBytes);
        writer.WriteNumber(wayTypeData[i].indexLevel);
        writer.WriteNumber(wayTypeData[i].cellXStart);
        writer.WriteNumber(wayTypeData[i].cellXEnd);
        writer.WriteNumber(wayTypeData[i].cellYStart);
        writer.WriteNumber(wayTypeData[i].cellYEnd);
      }
    }

    if (!wayScanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                         "ways.dat"),
                         FileScanner::Sequential,
                         parameter.GetWayDataMemoryMaped())) {
      progress.Error("Cannot open 'ways.dat'");
      return false;
    }

    for (size_t l=parameter.GetAreaWayMinMag(); l<=maxLevel; l++) {
      TypeInfoSet indexTypes(typeConfig);
      uint32_t    wayCount;
      double      cellWidth=360.0/pow(2.0,(int)l);
      double      cellHeight=180.0/pow(2.0,(int)l);

      wayScanner.GotoBegin();

      for (const auto &type : typeConfig->GetWayTypes()) {
        if (wayTypeData[type->GetIndex()].HasEntries() &&
            wayTypeData[type->GetIndex()].indexLevel==l) {
          indexTypes.Set(type);
        }
      }

      if (indexTypes.Empty()) {
        continue;
      }

      progress.Info("Scanning ways for index level "+NumberToString(l));

      std::vector<CoordOffsetsMap> typeCellOffsets(typeConfig->GetTypeCount());

      if (!wayScanner.Read(wayCount)) {
        progress.Error("Error while reading number of data entries in file");
        return false;
      }

      Way way;

      for (uint32_t w=1; w<=wayCount; w++) {
        progress.SetProgress(w,wayCount);

        FileOffset offset;

        wayScanner.GetPos(offset);

        if (!way.Read(typeConfig,
                      wayScanner)) {
          progress.Error(std::string("Error while reading data entry ")+
                         NumberToString(w)+" of "+
                         NumberToString(wayCount)+
                         " in file '"+
                         wayScanner.GetFilename()+"'");
          return false;
        }

        if (!indexTypes.IsSet(way.GetType())) {
          continue;
        }

        double minLon;
        double maxLon;
        double minLat;
        double maxLat;

        way.GetBoundingBox(minLon,maxLon,minLat,maxLat);

        //
        // Calculate minimum and maximum tile ids that are covered
        // by the way
        // Renormalized coordinate space (everything is >=0)
        //
        uint32_t minxc=(uint32_t)floor((minLon+180.0)/cellWidth);
        uint32_t maxxc=(uint32_t)floor((maxLon+180.0)/cellWidth);
        uint32_t minyc=(uint32_t)floor((minLat+90.0)/cellHeight);
        uint32_t maxyc=(uint32_t)floor((maxLat+90.0)/cellHeight);

        for (uint32_t y=minyc; y<=maxyc; y++) {
          for (uint32_t x=minxc; x<=maxxc; x++) {
            typeCellOffsets[way.GetType()->GetIndex()][Pixel(x,y)].push_back(offset);
          }
        }
      }

      for (const auto &type : indexTypes) {
        size_t index=type->GetIndex();

        if (!WriteBitmap(progress,
                         writer,
                         typeConfig->GetTypeInfo(index),
                         wayTypeData[index],
                         typeCellOffsets[index])) {
          return false;
        }
      }
    }

    return !writer.HasError() && writer.Close();
  }
}

