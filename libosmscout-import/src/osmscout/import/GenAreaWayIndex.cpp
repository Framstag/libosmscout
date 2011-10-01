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

#include <osmscout/Util.h>

#include <osmscout/util/FileWriter.h>
#include <osmscout/util/FileScanner.h>
#include <osmscout/util/Geometry.h>
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
  }

  std::string AreaWayIndexGenerator::GetDescription() const
  {
    return "Generate 'areaway.idx'";
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
                                         "ways.dat"))) {
      progress.Error("Cannot open 'ways.dat'");
      return false;
    }

    if (!relScanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                         "relations.dat"))) {
      progress.Error("Cannot open 'relations.dat'");
      return false;
    }

    //
    // Scanning distribution
    //

    progress.SetAction("Scannning level distribution of way types");

    for (size_t i=0; i<typeConfig.GetTypes().size(); i++) {
      if (typeConfig.GetTypeInfo(i).CanBeWay() &&
          !typeConfig.GetTypeInfo(i).GetIgnore()) {
        remainingWayTypes.insert(i);
      }
    }

    level=0;
    while (!remainingWayTypes.empty()) {
      uint32_t         wayCount=0;
      std::set<TypeId> currentWayTypes(remainingWayTypes);
      double           cellWidth=360.0/pow(2.0,(int)level);
      double           cellHeight=180.0/pow(2.0,(int)level);
      std::vector<std::map<Coord,size_t> > cellFillCount;

      cellFillCount.resize(typeConfig.GetTypes().size());

      progress.Info("Scanning Level "+NumberToString(level)+" ("+NumberToString(remainingWayTypes.size())+" types remaining)");

      wayScanner.GotoBegin();

      if (!wayScanner.Read(wayCount)) {
        progress.Error("Error while reading number of data entries in file");
        return false;
      }

      for (uint32_t w=1; w<=wayCount; w++) {
        progress.SetProgress(w,wayCount);

        FileOffset offset;
        Way        way;

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

        if (currentWayTypes.find(way.GetType())!=currentWayTypes.end()) {
          double minLon;
          double maxLon;
          double minLat;
          double maxLat;

          way.GetBoundingBox(minLon,maxLon,minLat,maxLat);

          //
          // Calculate minumum and maximum tile ids that are covered
          // by the way
          // Renormated coordinate space (everything is >=0)
          //
          size_t minxc=(size_t)floor((minLon+180.0)/cellWidth);
          size_t maxxc=(size_t)floor((maxLon+180.0)/cellWidth);
          size_t minyc=(size_t)floor((minLat+90.0)/cellHeight);
          size_t maxyc=(size_t)floor((maxLat+90.0)/cellHeight);

          for (size_t y=minyc; y<=maxyc; y++) {
            for (size_t x=minxc; x<=maxxc; x++) {
              cellFillCount[way.GetType()][Coord(x,y)]++;
            }
          }
        }
      }

      for (size_t i=0; i<typeConfig.GetTypes().size(); i++) {
        if (currentWayTypes.find(i)!=currentWayTypes.end()) {
          size_t entryCount=0;
          size_t max=0;

          for (std::map<Coord,size_t>::const_iterator cell=cellFillCount[i].begin();
               cell!=cellFillCount[i].end();
               ++cell) {
            entryCount+=cell->second;
            max=std::max(max,cell->second);
          }

          double average=entryCount*1.0/cellFillCount[i].size();

          if (!cellFillCount[i].empty() &&
              (max>parameter.GetAreaWayIndexCellSizeMax() ||
               average>parameter.GetAreaWayIndexCellSizeAverage())) {
            currentWayTypes.erase(i);
          }
        }
      }

      for (std::set<TypeId>::const_iterator cwt=currentWayTypes.begin();
           cwt!=currentWayTypes.end();
           cwt++) {
        maxLevel=std::max(maxLevel,level);

        wayTypeData[*cwt].indexLevel=level;
        wayTypeData[*cwt].indexCells=cellFillCount[*cwt].size();
        wayTypeData[*cwt].indexEntries=0;

        if (!cellFillCount[*cwt].empty()) {
          wayTypeData[*cwt].cellXStart=cellFillCount[*cwt].begin()->first.x;
          wayTypeData[*cwt].cellYStart=cellFillCount[*cwt].begin()->first.y;

          wayTypeData[*cwt].cellXEnd=wayTypeData[*cwt].cellXStart;
          wayTypeData[*cwt].cellYEnd=wayTypeData[*cwt].cellYStart;

          for (std::map<Coord,size_t>::const_iterator cell=cellFillCount[*cwt].begin();
               cell!=cellFillCount[*cwt].end();
               ++cell) {
            wayTypeData[*cwt].indexEntries+=cell->second;

            wayTypeData[*cwt].cellXStart=std::min(wayTypeData[*cwt].cellXStart,cell->first.x);
            wayTypeData[*cwt].cellXEnd=std::max(wayTypeData[*cwt].cellXEnd,cell->first.x);

            wayTypeData[*cwt].cellYStart=std::min(wayTypeData[*cwt].cellYStart,cell->first.y);
            wayTypeData[*cwt].cellYEnd=std::max(wayTypeData[*cwt].cellYEnd,cell->first.y);
          }
        }

        wayTypeData[*cwt].cellXCount=wayTypeData[*cwt].cellXEnd-wayTypeData[*cwt].cellXStart+1;
        wayTypeData[*cwt].cellYCount=wayTypeData[*cwt].cellYEnd-wayTypeData[*cwt].cellYStart+1;

        progress.Info("Type "+typeConfig.GetTypeInfo(*cwt).GetName()+"(" + NumberToString(*cwt)+"), "+NumberToString(wayTypeData[*cwt].indexCells)+" cells, "+NumberToString(wayTypeData[*cwt].indexEntries)+" objects");

        remainingWayTypes.erase(*cwt);
      }

      level++;
    }

    progress.SetAction("Scannning level distribution of relation types");

    for (size_t i=0; i<typeConfig.GetTypes().size(); i++) {
      if (typeConfig.GetTypeInfo(i).CanBeRelation() &&
          !typeConfig.GetTypeInfo(i).GetIgnore()) {
        remainingRelTypes.insert(i);
      }
    }

    level=0;
    while (!remainingRelTypes.empty())  {
      uint32_t         relCount=0;
      std::set<TypeId> currentRelTypes(remainingRelTypes);
      double           cellWidth=360.0/pow(2.0,(int)level);
      double           cellHeight=180.0/pow(2.0,(int)level);
      std::vector<std::map<Coord,size_t> > cellFillCount;

      cellFillCount.resize(typeConfig.GetTypes().size());

      progress.Info("Scanning Level "+NumberToString(level)+" ("+NumberToString(remainingRelTypes.size())+" types remaining)");

      relScanner.GotoBegin();

      if (!relScanner.Read(relCount)) {
        progress.Error("Error while reading number of data entries in file");
        return false;
      }

      for (uint32_t r=1; r<=relCount; r++) {
        progress.SetProgress(r,relCount);

        FileOffset offset;
        Relation   rel;

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

        if (currentRelTypes.find(rel.GetType())!=currentRelTypes.end()) {
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
              cellFillCount[rel.GetType()][Coord(x,y)]++;
            }
          }
        }
      }

      for (size_t i=0; i<typeConfig.GetTypes().size(); i++) {
        if (currentRelTypes.find(i)!=currentRelTypes.end()) {
          size_t entryCount=0;
          size_t max=0;

          for (std::map<Coord,size_t>::const_iterator cell=cellFillCount[i].begin();
               cell!=cellFillCount[i].end();
               ++cell) {
            entryCount+=cell->second;
            max=std::max(max,cell->second);
          }

          double average=entryCount*1.0/cellFillCount[i].size();

          if (!cellFillCount[i].empty() &&
              (max>parameter.GetAreaWayIndexCellSizeMax() ||
               average>parameter.GetAreaWayIndexCellSizeAverage())) {
            currentRelTypes.erase(i);
          }
        }
      }

      for (std::set<TypeId>::const_iterator crt=currentRelTypes.begin();
           crt!=currentRelTypes.end();
           crt++) {
        maxLevel=std::max(maxLevel,level);

        relTypeData[*crt].indexLevel=level;
        relTypeData[*crt].indexCells=cellFillCount[*crt].size();
        relTypeData[*crt].indexEntries=0;

        if (!cellFillCount[*crt].empty()) {
          relTypeData[*crt].cellXStart=cellFillCount[*crt].begin()->first.x;
          relTypeData[*crt].cellYStart=cellFillCount[*crt].begin()->first.y;

          relTypeData[*crt].cellXEnd=relTypeData[*crt].cellXStart;
          relTypeData[*crt].cellYEnd=relTypeData[*crt].cellYStart;

          for (std::map<Coord,size_t>::const_iterator cell=cellFillCount[*crt].begin();
               cell!=cellFillCount[*crt].end();
               ++cell) {
            relTypeData[*crt].indexEntries+=cell->second;

            relTypeData[*crt].cellXStart=std::min(relTypeData[*crt].cellXStart,cell->first.x);
            relTypeData[*crt].cellXEnd=std::max(relTypeData[*crt].cellXEnd,cell->first.x);

            relTypeData[*crt].cellYStart=std::min(relTypeData[*crt].cellYStart,cell->first.y);
            relTypeData[*crt].cellYEnd=std::max(relTypeData[*crt].cellYEnd,cell->first.y);
          }
        }

        relTypeData[*crt].cellXCount=relTypeData[*crt].cellXEnd-relTypeData[*crt].cellXStart+1;
        relTypeData[*crt].cellYCount=relTypeData[*crt].cellYEnd-relTypeData[*crt].cellYStart+1;

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
           wayTypeData[i].indexCells>0 &&
           wayTypeData[i].indexEntries>0) ||
          (typeConfig.GetTypeInfo(i).CanBeRelation() &&
           relTypeData[i].indexCells>0 &&
           relTypeData[i].indexEntries>0)) {
        indexEntries++;
      }
    }

    writer.Write(indexEntries);

    for (size_t i=0; i<typeConfig.GetTypes().size(); i++)
    {
      if ((typeConfig.GetTypeInfo(i).CanBeWay() &&
           wayTypeData[i].indexCells>0 &&
           wayTypeData[i].indexEntries>0) ||
          (typeConfig.GetTypeInfo(i).CanBeRelation() &&
           relTypeData[i].indexCells>0 &&
           relTypeData[i].indexEntries>0)) {
        FileOffset bitmapOffset=0;

        writer.WriteNumber((uint32_t)i);

        writer.GetPos(wayTypeData[i].indexOffset);

        writer.Write(bitmapOffset);

        if (wayTypeData[i].indexCells>0 &&
            wayTypeData[i].indexEntries>0) {
          writer.WriteNumber(wayTypeData[i].indexLevel);
          writer.WriteNumber(wayTypeData[i].cellXStart);
          writer.WriteNumber(wayTypeData[i].cellXEnd);
          writer.WriteNumber(wayTypeData[i].cellYStart);
          writer.WriteNumber(wayTypeData[i].cellYEnd);
        }

        writer.GetPos(relTypeData[i].indexOffset);

        writer.Write(bitmapOffset);

        if (relTypeData[i].indexCells>0 &&
            relTypeData[i].indexEntries>0) {
          writer.WriteNumber(relTypeData[i].indexLevel);
          writer.WriteNumber(relTypeData[i].cellXStart);
          writer.WriteNumber(relTypeData[i].cellXEnd);
          writer.WriteNumber(relTypeData[i].cellYStart);
          writer.WriteNumber(relTypeData[i].cellYEnd);
        }
      }
    }

    for (size_t l=0; l<=maxLevel; l++) {
      std::set<TypeId> indexTypes;
      uint32_t         wayCount;
      double           cellWidth=360.0/pow(2.0,(int)l);
      double           cellHeight=180.0/pow(2.0,(int)l);

      for (size_t i=0; i<typeConfig.GetTypes().size(); i++) {
        if (typeConfig.GetTypeInfo(i).CanBeWay() &&
            wayTypeData[i].indexCells>0 &&
            wayTypeData[i].indexEntries>0 &&
            wayTypeData[i].indexLevel==l) {
          indexTypes.insert(i);
        }
      }

      if (indexTypes.empty()) {
        continue;
      }

      progress.Info("Scanning ways for index level "+NumberToString(l));

      std::vector<std::map<Coord,std::list<FileOffset> > > typeCellOffsets;

      typeCellOffsets.resize(typeConfig.GetTypes().size());

      wayScanner.GotoBegin();

      if (!wayScanner.Read(wayCount)) {
        progress.Error("Error while reading number of data entries in file");
        return false;
      }

      for (uint32_t w=1; w<=wayCount; w++) {
        progress.SetProgress(w,wayCount);

        FileOffset offset;
        Way        way;

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

        if (indexTypes.find(way.GetType())!=indexTypes.end()) {
          double minLon;
          double maxLon;
          double minLat;
          double maxLat;

          way.GetBoundingBox(minLon,maxLon,minLat,maxLat);

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
              typeCellOffsets[way.GetType()][Coord(x,y)].push_back(offset);
            }
          }
        }
      }

      for (std::set<TypeId>::const_iterator type=indexTypes.begin();
           type!=indexTypes.end();
           ++type) {
        size_t   indexEntries=0;
        for (std::map<Coord,std::list<FileOffset> >::const_iterator cell=typeCellOffsets[*type].begin();
             cell!=typeCellOffsets[*type].end();
             ++cell) {
          indexEntries+=cell->second.size();
        }

        progress.Info("Writing way bitmap for type "+
                      typeConfig.GetTypeInfo(*type).GetName()+" ("+NumberToString(*type)+") "+
                      NumberToString(typeCellOffsets[*type].size())+" "+
                      NumberToString(indexEntries)+" "+
                      NumberToString(wayTypeData[*type].cellXCount*wayTypeData[*type].cellYCount));

        FileOffset bitmapOffset;

        if (!writer.GetPos(bitmapOffset)) {
          progress.Error("Cannot get type index start position in file");
          return false;
        }

        assert(wayTypeData[*type].indexOffset!=0);

        if (!writer.SetPos(wayTypeData[*type].indexOffset)) {
          progress.Error("Cannot go to type index offset in file");
          return false;
        }

        writer.Write(bitmapOffset);

        if (!writer.SetPos(bitmapOffset)) {
          progress.Error("Cannot go to type index start position in file");
          return false;
        }

        // Write the bitmap with offsets for each cell
        // We prefill with zero and only overrite cells that have data
        // So zero means "no data for this cell"
        for (size_t i=0; i<wayTypeData[*type].cellXCount*wayTypeData[*type].cellYCount; i++) {
          FileOffset cellOffset=0;

          writer.Write(cellOffset);
        }

        // Now write the list of offsets of objects for every cell with content
        for (std::map<Coord,std::list<FileOffset> >::const_iterator cell=typeCellOffsets[*type].begin();
             cell!=typeCellOffsets[*type].end();
             ++cell) {
          FileOffset bitmapCellOffset=bitmapOffset+
                                      ((cell->first.y-wayTypeData[*type].cellYStart)*wayTypeData[*type].cellXCount+
                                       cell->first.x-wayTypeData[*type].cellXStart)*sizeof(FileOffset);
          FileOffset previousOffset=0;
          FileOffset cellOffset;

          if (!writer.GetPos(cellOffset)) {
            progress.Error("Cannot get cell start position in file");
            return false;
          }

          if (!writer.SetPos(bitmapCellOffset)) {
            progress.Error("Cannot go to cell start position in file");
            return false;
          }

          writer.Write(cellOffset);

          if (!writer.SetPos(cellOffset)) {
            progress.Error("Cannot go back to cell start position in file");
            return false;
          }

          writer.WriteNumber((uint32_t)cell->second.size());

          for (std::list<FileOffset>::const_iterator offset=cell->second.begin();
               offset!=cell->second.end();
               ++offset) {
            writer.WriteNumber(*offset-previousOffset);

            previousOffset=*offset;
          }
        }
      }
    }

    for (size_t l=0; l<=maxLevel; l++) {
      std::set<TypeId> indexTypes;
      uint32_t         relCount;
      double           cellWidth=360.0/pow(2.0,(int)l);
      double           cellHeight=180.0/pow(2.0,(int)l);

      for (size_t i=0; i<typeConfig.GetTypes().size(); i++) {
        if (typeConfig.GetTypeInfo(i).CanBeRelation() &&
            relTypeData[i].indexCells>0 &&
            relTypeData[i].indexEntries>0 &&
            relTypeData[i].indexLevel==l) {
          indexTypes.insert(i);
        }
      }

      if (indexTypes.empty()) {
        continue;
      }

      progress.Info("Scanning relations for index level "+NumberToString(l));

      std::vector<std::map<Coord,std::list<FileOffset> > > typeCellOffsets;

      typeCellOffsets.clear();
      typeCellOffsets.resize(typeConfig.GetTypes().size());

      relScanner.GotoBegin();

      if (!relScanner.Read(relCount)) {
        progress.Error("Error while reading number of data entries in file");
        return false;
      }

      for (uint32_t r=1; r<=relCount; r++) {
        progress.SetProgress(r,relCount);

        FileOffset offset;
        Relation   rel;

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

        if (indexTypes.find(rel.GetType())!=indexTypes.end()) {
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
              typeCellOffsets[rel.GetType()][Coord(x,y)].push_back(offset);
            }
          }
        }
      }

      for (std::set<TypeId>::const_iterator type=indexTypes.begin();
           type!=indexTypes.end();
           ++type) {
        size_t   indexEntries=0;
        for (std::map<Coord,std::list<FileOffset> >::const_iterator cell=typeCellOffsets[*type].begin();
             cell!=typeCellOffsets[*type].end();
             ++cell) {
          indexEntries+=cell->second.size();
        }

        progress.Info("Writing relation bitmap for type "+
                      typeConfig.GetTypeInfo(*type).GetName()+" ("+NumberToString(*type)+") "+
                      NumberToString(typeCellOffsets[*type].size())+" "+
                      NumberToString(indexEntries)+" "+
                      NumberToString(relTypeData[*type].cellXCount*relTypeData[*type].cellYCount));

        FileOffset bitmapOffset;

        if (!writer.GetPos(bitmapOffset)) {
          progress.Error("Cannot get type index start position in file");
          return false;
        }

        assert(relTypeData[*type].indexOffset!=0);

        if (!writer.SetPos(relTypeData[*type].indexOffset)) {
          progress.Error("Cannot go to type index offset in file");
          return false;
        }

        writer.Write(bitmapOffset);

        if (!writer.SetPos(bitmapOffset)) {
          progress.Error("Cannot go to type index start position in file");
          return false;
        }

        // Write the bitmap with offsets for each cell
        // We prefill with zero and only overrite cells that have data
        // So zero means "no data for this cell"
        for (size_t i=0; i<relTypeData[*type].cellXCount*relTypeData[*type].cellYCount; i++) {
          FileOffset cellOffset=0;

          writer.Write(cellOffset);
        }

        // Now write the list of offsets of objects for every cell with content
        for (std::map<Coord,std::list<FileOffset> >::const_iterator cell=typeCellOffsets[*type].begin();
             cell!=typeCellOffsets[*type].end();
             ++cell) {
          FileOffset bitmapCellOffset=bitmapOffset+
                                      ((cell->first.y-relTypeData[*type].cellYStart)*relTypeData[*type].cellXCount+
                                       cell->first.x-relTypeData[*type].cellXStart)*sizeof(FileOffset);
          FileOffset previousOffset=0;
          FileOffset cellOffset;

          if (!writer.GetPos(cellOffset)) {
            progress.Error("Cannot get cell start position in file");
            return false;
          }

          if (!writer.SetPos(bitmapCellOffset)) {
            progress.Error("Cannot go to cell start position in file");
            return false;
          }

          writer.Write(cellOffset);

          if (!writer.SetPos(cellOffset)) {
            progress.Error("Cannot go back to cell start position in file");
            return false;
          }

          writer.WriteNumber((uint32_t)cell->second.size());

          for (std::list<FileOffset>::const_iterator offset=cell->second.begin();
               offset!=cell->second.end();
               ++offset) {
            writer.WriteNumber(*offset-previousOffset);

            previousOffset=*offset;
          }
        }
      }
    }

    return !writer.HasError() && writer.Close();
  }
}

