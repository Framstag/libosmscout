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

#include <cassert>
#include <vector>

#include <osmscout/Relation.h>
#include <osmscout/Way.h>

#include <osmscout/util/File.h>
#include <osmscout/util/FileScanner.h>
#include <osmscout/util/Number.h>
#include <osmscout/util/String.h>

#include <osmscout/private/Math.h>

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

  void AreaWayIndexGenerator::CalculateStatistics(size_t level,
                                                  TypeData& typeData,
                                                  const CoordCountMap& cellFillCount)
  {
    typeData.indexLevel=level;
    typeData.indexCells=cellFillCount.size();
    typeData.indexEntries=0;

    // If we do not have any entries, we store it now
    if (cellFillCount.empty()) {
      return;
    }

    typeData.cellXStart=cellFillCount.begin()->first.x;
    typeData.cellYStart=cellFillCount.begin()->first.y;

    typeData.cellXEnd=typeData.cellXStart;
    typeData.cellYEnd=typeData.cellYStart;

    for (CoordCountMap::const_iterator cell=cellFillCount.begin();
         cell!=cellFillCount.end();
         ++cell) {
      typeData.indexEntries+=cell->second;

      typeData.cellXStart=std::min(typeData.cellXStart,cell->first.x);
      typeData.cellXEnd=std::max(typeData.cellXEnd,cell->first.x);

      typeData.cellYStart=std::min(typeData.cellYStart,cell->first.y);
      typeData.cellYEnd=std::max(typeData.cellYEnd,cell->first.y);
    }

    typeData.cellXCount=typeData.cellXEnd-typeData.cellXStart+1;
    typeData.cellYCount=typeData.cellYEnd-typeData.cellYStart+1;
  }

  bool AreaWayIndexGenerator::FitsIndexCriteria(const ImportParameter& parameter,
                                                Progress& progress,
                                                const TypeInfo& typeInfo,
                                                const TypeData& typeData,
                                                const CoordCountMap& cellFillCount)
  {
    if (typeData.indexCells==0) {
      return true;
    }

    size_t entryCount=0;
    size_t max=0;

    for (CoordCountMap::const_iterator cell=cellFillCount.begin();
         cell!=cellFillCount.end();
         ++cell) {
      entryCount+=cell->second;
      max=std::max(max,cell->second);
    }

    // Average number of entries per tile cell
    double average=entryCount*1.0/cellFillCount.size();

    // If the fill rate of the index is too low, we use this index level anyway
    if (typeData.indexCells/(1.0*typeData.cellXCount*typeData.cellYCount)<=
        parameter.GetAreaWayIndexMinFillRate()) {
      progress.Warning(typeInfo.GetName()+" ("+NumberToString(typeInfo.GetId())+") is not well distributed");
      return true;
    }

    // If average fill size and max fill size for tile cells
    // is within limits, store it now.
    if (max<=parameter.GetAreaWayIndexCellSizeMax() &&
        average<=parameter.GetAreaWayIndexCellSizeAverage()) {
      return true;
    }

    return false;
  }

  bool AreaWayIndexGenerator::WriteBitmap(Progress& progress,
                                          FileWriter& writer,
                                          const TypeInfo& typeInfo,
                                          const TypeData& typeData,
                                          const CoordOffsetsMap& typeCellOffsets)
  {
    size_t indexEntries=0;
    size_t dataSize=0;
    char   buffer[10];

    for (CoordOffsetsMap::const_iterator cell=typeCellOffsets.begin();
         cell!=typeCellOffsets.end();
         ++cell) {
      indexEntries+=cell->second.size();

      dataSize+=EncodeNumber(cell->second.size(),buffer);

      FileOffset previousOffset=0;
      for (std::list<FileOffset>::const_iterator offset=cell->second.begin();
           offset!=cell->second.end();
           ++offset) {
        FileOffset data=*offset-previousOffset;

        dataSize+=EncodeNumber(data,buffer);

        previousOffset=*offset;
      }
    }

    uint8_t dataOffsetBytes=BytesNeeededToAddressFileData(dataSize);

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
      FileOffset cellOffset=0;

      writer.WriteFileOffset(cellOffset,
                             dataOffsetBytes);
    }

    FileOffset dataStartOffset;

    if (!writer.GetPos(dataStartOffset)) {
      progress.Error("Cannot get start of data section after bitmap");
      return false;
    }

    // Now write the list of offsets of objects for every cell with content
    for (CoordOffsetsMap::const_iterator cell=typeCellOffsets.begin();
         cell!=typeCellOffsets.end();
         ++cell) {
      FileOffset bitmapCellOffset=bitmapOffset+
                                  ((cell->first.y-typeData.cellYStart)*typeData.cellXCount+
                                    cell->first.x-typeData.cellXStart)*(FileOffset)dataOffsetBytes;
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

      writer.WriteFileOffset(cellOffset-dataStartOffset,dataOffsetBytes);

      if (!writer.SetPos(cellOffset)) {
        progress.Error("Cannot go back to cell start position in file");
        return false;
      }

      writer.WriteNumber((uint32_t)cell->second.size());

      // FileOffsets are already in increasing order, since
      // File is scanned from start to end
      for (std::list<FileOffset>::const_iterator offset=cell->second.begin();
           offset!=cell->second.end();
           ++offset) {
        writer.WriteNumber((FileOffset)(*offset-previousOffset));

        previousOffset=*offset;
      }
    }

    return true;
  }

  bool AreaWayIndexGenerator::Import(const ImportParameter& parameter,
                                     Progress& progress,
                                     const TypeConfig& typeConfig)
  {
    FileScanner           wayScanner;
    FileScanner           relScanner;
    FileWriter            writer;
    std::set<TypeId>      remainingWayTypes;
    std::set<TypeId>      remainingRelTypes;
    std::vector<TypeData> wayTypeData;
    std::vector<TypeData> relTypeData;
    size_t                level;
    size_t                maxLevel=0;

    wayTypeData.resize(typeConfig.GetTypes().size());
    relTypeData.resize(typeConfig.GetTypes().size());

    if (!wayScanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                         "ways.dat"),
                         FileScanner::Sequential,
                         parameter.GetWayDataMemoryMaped())) {
      progress.Error("Cannot open 'ways.dat'");
      return false;
    }

    if (!relScanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                         "relations.dat"),
                         FileScanner::Sequential,
                         true)) {
      progress.Error("Cannot open 'relations.dat'");
      return false;
    }

    //
    // Scanning distribution
    //

    progress.SetAction("Scanning level distribution of way types");

    for (size_t i=0; i<typeConfig.GetTypes().size(); i++) {
      if (typeConfig.GetTypeInfo(i).CanBeWay() &&
          !typeConfig.GetTypeInfo(i).GetIgnore()) {
        remainingWayTypes.insert(i);
      }
    }

    level=parameter.GetAreaWayMinMag();
    while (!remainingWayTypes.empty()) {
      uint32_t                   wayCount=0;
      std::set<TypeId>           currentWayTypes(remainingWayTypes);
      double                     cellWidth=360.0/pow(2.0,(int)level);
      double                     cellHeight=180.0/pow(2.0,(int)level);
      std::vector<CoordCountMap> cellFillCount(typeConfig.GetTypes().size());

      progress.Info("Scanning Level "+NumberToString(level)+" ("+NumberToString(remainingWayTypes.size())+" types remaining)");

      wayScanner.GotoBegin();

      if (!wayScanner.Read(wayCount)) {
        progress.Error("Error while reading number of data entries in file");
        return false;
      }

      Way way;

      for (uint32_t w=1; w<=wayCount; w++) {
        progress.SetProgress(w,wayCount);

        if (!way.Read(wayScanner)) {
          progress.Error(std::string("Error while reading data entry ")+
                         NumberToString(w)+" of "+
                         NumberToString(wayCount)+
                         " in file '"+
                         wayScanner.GetFilename()+"'");
          return false;
        }

        if (way.IsArea()) {
          continue;
        }

        // Count number of entries per current type and coordinate
        if (currentWayTypes.find(way.GetType())==currentWayTypes.end()) {
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
        // Renormated coordinate space (everything is >=0)
        //
        size_t minxc=(size_t)floor((minLon+180.0)/cellWidth);
        size_t maxxc=(size_t)floor((maxLon+180.0)/cellWidth);
        size_t minyc=(size_t)floor((minLat+90.0)/cellHeight);
        size_t maxyc=(size_t)floor((maxLat+90.0)/cellHeight);

        for (size_t y=minyc; y<=maxyc; y++) {
          for (size_t x=minxc; x<=maxxc; x++) {
            cellFillCount[way.GetType()][Pixel(x,y)]++;
          }
        }
      }

      // Check if cell fill for current type is in defined limits
      for (size_t i=0; i<typeConfig.GetTypes().size(); i++) {
        if (currentWayTypes.find(i)!=currentWayTypes.end()) {
          CalculateStatistics(level,wayTypeData[i],cellFillCount[i]);

          if (!FitsIndexCriteria(parameter,
                                 progress,
                                 typeConfig.GetTypeInfo(i),
                                 wayTypeData[i],
                                 cellFillCount[i])) {
            currentWayTypes.erase(i);
          }
        }
      }

      for (std::set<TypeId>::const_iterator cwt=currentWayTypes.begin();
           cwt!=currentWayTypes.end();
           cwt++) {
        maxLevel=std::max(maxLevel,level);

        progress.Info("Type "+typeConfig.GetTypeInfo(*cwt).GetName()+"(" + NumberToString(*cwt)+"), "+NumberToString(wayTypeData[*cwt].indexCells)+" cells, "+NumberToString(wayTypeData[*cwt].indexEntries)+" objects");

        remainingWayTypes.erase(*cwt);
      }

      level++;
    }

    progress.SetAction("Scanning level distribution of relation types");

    for (size_t i=0; i<typeConfig.GetTypes().size(); i++) {
      if (typeConfig.GetTypeInfo(i).CanBeRelation() &&
          !typeConfig.GetTypeInfo(i).GetIgnore()) {
        remainingRelTypes.insert(i);
      }
    }

    level=parameter.GetAreaWayMinMag();
    while (!remainingRelTypes.empty())  {
      uint32_t                   relCount=0;
      std::set<TypeId>           currentRelTypes(remainingRelTypes);
      double                     cellWidth=360.0/pow(2.0,(int)level);
      double                     cellHeight=180.0/pow(2.0,(int)level);
      std::vector<CoordCountMap> cellFillCount(typeConfig.GetTypes().size());

      progress.Info("Scanning Level "+NumberToString(level)+" ("+NumberToString(remainingRelTypes.size())+" types remaining)");

      relScanner.GotoBegin();

      if (!relScanner.Read(relCount)) {
        progress.Error("Error while reading number of data entries in file");
        return false;
      }

      Relation rel;

      for (uint32_t r=1; r<=relCount; r++) {
        progress.SetProgress(r,relCount);

        if (!rel.Read(relScanner)) {
          progress.Error(std::string("Error while reading data entry ")+
                         NumberToString(r)+" of "+
                         NumberToString(relCount)+
                         " in file '"+
                         relScanner.GetFilename()+"'");
          return false;
        }

        if (rel.IsArea()) {
          continue;
        }

        // Count number of entries per current type and coordinate
        if (currentRelTypes.find(rel.GetType())==currentRelTypes.end()) {
          continue;
        }

        double minLon;
        double maxLon;
        double minLat;
        double maxLat;

        rel.GetBoundingBox(minLon,maxLon,minLat,maxLat);

        //
        // Calculate minimum and maximum tile ids that are covered
        // by the way
        // Renormated coordinate space (everything is >=0)
        //
        size_t minxc=(size_t)floor((minLon+180.0)/cellWidth);
        size_t maxxc=(size_t)floor((maxLon+180.0)/cellWidth);
        size_t minyc=(size_t)floor((minLat+90.0)/cellHeight);
        size_t maxyc=(size_t)floor((maxLat+90.0)/cellHeight);

        for (size_t y=minyc; y<=maxyc; y++) {
          for (size_t x=minxc; x<=maxxc; x++) {
            cellFillCount[rel.GetType()][Pixel(x,y)]++;
          }
        }
      }

      // Check if cell fill for current type is in defined limits
      for (size_t i=0; i<typeConfig.GetTypes().size(); i++) {
        if (currentRelTypes.find(i)!=currentRelTypes.end()) {
          CalculateStatistics(level,relTypeData[i],cellFillCount[i]);

          if (!FitsIndexCriteria(parameter,
                                 progress,
                                 typeConfig.GetTypeInfo(i),
                                 relTypeData[i],
                                 cellFillCount[i])) {
            currentRelTypes.erase(i);
          }
        }
      }

      for (std::set<TypeId>::const_iterator crt=currentRelTypes.begin();
           crt!=currentRelTypes.end();
           crt++) {
        maxLevel=std::max(maxLevel,level);

        progress.Info("Type "+typeConfig.GetTypeInfo(*crt).GetName()+"(" + NumberToString(*crt)+"), "+NumberToString(relTypeData[*crt].indexCells)+" cells, "+NumberToString(relTypeData[*crt].indexEntries)+" objects");

        remainingRelTypes.erase(*crt);
      }

      level++;
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

    uint32_t indexEntries=0;

    for (size_t i=0; i<typeConfig.GetTypes().size(); i++)
    {
      if ((typeConfig.GetTypeInfo(i).CanBeWay() &&
           wayTypeData[i].HasEntries()) ||
          (typeConfig.GetTypeInfo(i).CanBeRelation() &&
           relTypeData[i].HasEntries())) {
        indexEntries++;
      }
    }

    writer.Write(indexEntries);

    for (size_t i=0; i<typeConfig.GetTypes().size(); i++)
    {
      if ((typeConfig.GetTypeInfo(i).CanBeWay() &&
           wayTypeData[i].HasEntries()) ||
          (typeConfig.GetTypeInfo(i).CanBeRelation() &&
           relTypeData[i].HasEntries())) {
        uint8_t    dataOffsetBytes=0;
        FileOffset bitmapOffset=0;

        writer.WriteNumber(typeConfig.GetTypeInfo(i).GetId());

        writer.GetPos(wayTypeData[i].indexOffset);

        writer.WriteFileOffset(bitmapOffset);

        if (wayTypeData[i].HasEntries()) {
          writer.Write(dataOffsetBytes);
          writer.WriteNumber(wayTypeData[i].indexLevel);
          writer.WriteNumber(wayTypeData[i].cellXStart);
          writer.WriteNumber(wayTypeData[i].cellXEnd);
          writer.WriteNumber(wayTypeData[i].cellYStart);
          writer.WriteNumber(wayTypeData[i].cellYEnd);
        }

        writer.GetPos(relTypeData[i].indexOffset);

        writer.WriteFileOffset(bitmapOffset);

        if (relTypeData[i].HasEntries()) {
          writer.Write(dataOffsetBytes);
          writer.WriteNumber(relTypeData[i].indexLevel);
          writer.WriteNumber(relTypeData[i].cellXStart);
          writer.WriteNumber(relTypeData[i].cellXEnd);
          writer.WriteNumber(relTypeData[i].cellYStart);
          writer.WriteNumber(relTypeData[i].cellYEnd);
        }
      }
    }

    for (size_t l=parameter.GetAreaWayMinMag(); l<=maxLevel; l++) {
      std::set<TypeId> indexTypes;
      uint32_t         wayCount;
      double           cellWidth=360.0/pow(2.0,(int)l);
      double           cellHeight=180.0/pow(2.0,(int)l);

      for (size_t i=0; i<typeConfig.GetTypes().size(); i++) {
        if (typeConfig.GetTypeInfo(i).CanBeWay() &&
            wayTypeData[i].HasEntries() &&
            wayTypeData[i].indexLevel==l) {
          indexTypes.insert(i);
        }
      }

      if (indexTypes.empty()) {
        continue;
      }

      progress.Info("Scanning ways for index level "+NumberToString(l));

      std::vector<CoordOffsetsMap> typeCellOffsets(typeConfig.GetTypes().size());

      wayScanner.GotoBegin();

      if (!wayScanner.Read(wayCount)) {
        progress.Error("Error while reading number of data entries in file");
        return false;
      }

      Way way;

      for (uint32_t w=1; w<=wayCount; w++) {
        progress.SetProgress(w,wayCount);

        FileOffset offset;

        wayScanner.GetPos(offset);

        if (!way.Read(wayScanner)) {
          progress.Error(std::string("Error while reading data entry ")+
                         NumberToString(w)+" of "+
                         NumberToString(wayCount)+
                         " in file '"+
                         wayScanner.GetFilename()+"'");
          return false;
        }

        if (way.IsArea()) {
          continue;
        }

        if (indexTypes.find(way.GetType())==indexTypes.end()) {
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
        // Renormated coordinate space (everything is >=0)
        //
        size_t minxc=(size_t)floor((minLon+180.0)/cellWidth);
        size_t maxxc=(size_t)floor((maxLon+180.0)/cellWidth);
        size_t minyc=(size_t)floor((minLat+90.0)/cellHeight);
        size_t maxyc=(size_t)floor((maxLat+90.0)/cellHeight);

        for (size_t y=minyc; y<=maxyc; y++) {
          for (size_t x=minxc; x<=maxxc; x++) {
            typeCellOffsets[way.GetType()][Pixel(x,y)].push_back(offset);
          }
        }
      }

      for (std::set<TypeId>::const_iterator type=indexTypes.begin();
           type!=indexTypes.end();
           ++type) {
        if (!WriteBitmap(progress,
                         writer,
                         typeConfig.GetTypeInfo(*type),
                         wayTypeData[*type],
                         typeCellOffsets[*type])) {
          return false;
        }
      }
    }

    for (size_t l=parameter.GetAreaWayMinMag(); l<=maxLevel; l++) {
      std::set<TypeId> indexTypes;
      uint32_t         relCount;
      double           cellWidth=360.0/pow(2.0,(int)l);
      double           cellHeight=180.0/pow(2.0,(int)l);

      for (size_t i=0; i<typeConfig.GetTypes().size(); i++) {
        if (typeConfig.GetTypeInfo(i).CanBeRelation() &&
            relTypeData[i].HasEntries() &&
            relTypeData[i].indexLevel==l) {
          indexTypes.insert(i);
        }
      }

      if (indexTypes.empty()) {
        continue;
      }

      progress.Info("Scanning relations for index level "+NumberToString(l));

      std::vector<CoordOffsetsMap> typeCellOffsets(typeConfig.GetTypes().size());

      relScanner.GotoBegin();

      if (!relScanner.Read(relCount)) {
        progress.Error("Error while reading number of data entries in file");
        return false;
      }

      Relation rel;

      for (uint32_t r=1; r<=relCount; r++) {
        progress.SetProgress(r,relCount);

        FileOffset offset;

        relScanner.GetPos(offset);

        if (!rel.Read(relScanner)) {
          progress.Error(std::string("Error while reading data entry ")+
                         NumberToString(r)+" of "+
                         NumberToString(relCount)+
                         " in file '"+
                         relScanner.GetFilename()+"'");
          return false;
        }

        if (rel.IsArea()) {
          continue;
        }

        if (indexTypes.find(rel.GetType())==indexTypes.end()) {
          continue;
        }

        double minLon;
        double maxLon;
        double minLat;
        double maxLat;

        rel.GetBoundingBox(minLon,maxLon,minLat,maxLat);

        //
        // Calculate minum and maximum tile ids that are covered
        // by the way
        // Renormated coordinate space (everything is >=0)
        //
        size_t minxc=(size_t)floor((minLon+180.0)/cellWidth);
        size_t maxxc=(size_t)floor((maxLon+180.0)/cellWidth);
        size_t minyc=(size_t)floor((minLat+90.0)/cellHeight);
        size_t maxyc=(size_t)floor((maxLat+90.0)/cellHeight);

        for (size_t y=minyc; y<=maxyc; y++) {
          for (size_t x=minxc; x<=maxxc; x++) {
            typeCellOffsets[rel.GetType()][Pixel(x,y)].push_back(offset);
          }
        }
      }

      for (std::set<TypeId>::const_iterator type=indexTypes.begin();
           type!=indexTypes.end();
           ++type) {
        if (!WriteBitmap(progress,
                         writer,
                         typeConfig.GetTypeInfo(*type),
                         relTypeData[*type],
                         typeCellOffsets[*type])) {
          return false;
        }
      }
    }

    return !writer.HasError() && writer.Close();
  }
}

