/*
  This source is part of the libosmscout library
  Copyright (C) 2010  Tim Teulings

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

#include <osmscout/import/GenWaterIndex.h>

#include <cassert>
#include <vector>

#include <osmscout/Relation.h>
#include <osmscout/Way.h>

#include <osmscout/util/File.h>
#include <osmscout/util/FileScanner.h>
#include <osmscout/util/FileWriter.h>
#include <osmscout/util/Geometry.h>
#include <osmscout/util/String.h>

#include <osmscout/private/Math.h>

#include <iostream>

namespace osmscout {

  /**
   * Sets the size of the bitmap and initializes state of all tiles to "unknown"
   */
  void WaterIndexGenerator::Level::SetBox(double minLat, double maxLat,
                                          double minLon, double maxLon,
                                          double cellWidth, double cellHeight)
  {
    this->cellWidth=cellWidth;
    this->cellHeight=cellHeight;

    minLat=minLat/conversionFactor-90.0;
    maxLat=maxLat/conversionFactor-90.0;
    minLon=minLon/conversionFactor-180.0;
    maxLon=maxLon/conversionFactor-180.0;

    cellXStart=(uint32_t)floor((minLon+180.0)/cellWidth);
    cellXEnd=(uint32_t)floor((maxLon+180.0)/cellWidth);
    cellYStart=(uint32_t)floor((minLat+90.0)/cellHeight);
    cellYEnd=(uint32_t)floor((maxLat+90.0)/cellHeight);

    cellXCount=cellXEnd-cellXStart+1;
    cellYCount=cellYEnd-cellYStart+1;

    uint32_t size=cellXCount*cellYCount/4;

    if (cellXCount*cellYCount%4>0) {
      size++;
    }

    area.resize(size,0x00);
  }

  bool WaterIndexGenerator::Level::IsIn(uint32_t x, uint32_t y) const
  {
    return x>=cellXStart &&
           x<=cellXEnd &&
           y>=cellYStart &&
           y<=cellYEnd;
  }

  WaterIndexGenerator::State WaterIndexGenerator::Level::GetState(uint32_t x, uint32_t y) const
  {
    uint32_t cellId=y*cellXCount+x;
    uint32_t index=cellId/4;
    uint32_t offset=2*(cellId%4);

    return (State)((area[index] >> offset) & 3);
  }

  void WaterIndexGenerator::Level::SetState(uint32_t x, uint32_t y, State state)
  {
    uint32_t cellId=y*cellXCount+x;
    uint32_t index=cellId/4;
    uint32_t offset=2*(cellId%4);

    area[index]=(area[index] & ~(3 << offset));
    area[index]=(area[index] | (state << offset));
  }

  void WaterIndexGenerator::Level::SetStateAbsolute(uint32_t x, uint32_t y, State state)
  {
    SetState(x-cellXStart,y-cellYStart,state);
  }

  /**
   * Does a scan line conversion on all costline ways where the cell size equals the tile size.
   * All tiles hit are marked as "coast".
   *
   */
  void WaterIndexGenerator::SetCoastlineCells(Level& level)
  {

    for (std::list<Coast>::const_iterator coastline=coastlines.begin();
        coastline!=coastlines.end();
        ++coastline) {
      // Marks cells on the path as coast

      std::vector<ScanCell> cells;

      ScanConvertLine(coastline->coast,-180.0,level.cellWidth,-90.0,level.cellHeight,cells);

      for (size_t i=0; i<cells.size(); i++) {
        if (level.IsIn(cells[i].x,cells[i].y)) {
          level.SetStateAbsolute(cells[i].x,cells[i].y,coast);
        }
      }
    }
  }

  /**
   * We assume that coastlines do not intersect each other. We split each coastline into smaller line segments.
   * We sort the segments first by minimum longitude and then by slope. We than assume that left of each
   * line is either water or land, depending on the sloe direction. We then resort from right to left and do
   * the estimation again.
   */
  void WaterIndexGenerator::ScanCellsHorizontally(Level& level)
  {
    std::list<Line> lines;

    for (std::list<Coast>::const_iterator coastline=coastlines.begin();
        coastline!=coastlines.end();
        ++coastline) {
      for (size_t i=0; i<coastline->coast.size()-1; i++) {
        Line line;

        line.a=coastline->coast[i];
        line.b=coastline->coast[i+1];
        lines.push_back(line);
      }
    }

    for (std::list<Line>::iterator line=lines.begin();
        line!=lines.end();
        ++line) {
      line->sortCriteria1=std::min(line->a.GetLon(),line->b.GetLon());
      line->sortCriteria2=std::abs((line->b.GetLat()-line->a.GetLat())/(line->b.GetLon()-line->a.GetLon()));
    }

    lines.sort(SmallerLineSort());

    for (std::list<Line>::const_iterator line=lines.begin();
        line!=lines.end();
        ++line) {
      std::vector<ScanCell> cells;

      ScanConvertLine((int)((line->a.GetLon()+180.0)/level.cellWidth),
                      (int)((line->a.GetLat()+90.0)/level.cellHeight),
                      (int)((line->b.GetLon()+180.0)/level.cellWidth),
                      (int)((line->b.GetLat()+90.0)/level.cellHeight),
                      cells);

      if (line->b.GetLat()>line->a.GetLat()) {
        // up
        for (size_t i=0; i<cells.size(); i++) {
          if (level.IsIn(cells[i].x,cells[i].y)) {
            int cx=cells[i].x-level.cellXStart;
            int cy=cells[i].y-level.cellYStart;

            if (cx-1>=0 && level.GetState(cx-1,cy)==unknown) {
              level.SetState(cx-1,cy,land);
            }
          }
        }
      }
      else if (line->b.GetLat()<line->a.GetLat()) {
        // down
        for (size_t i=0; i<cells.size(); i++) {
          if (level.IsIn(cells[i].x,cells[i].y)) {
            int cx=cells[i].x-level.cellXStart;
            int cy=cells[i].y-level.cellYStart;

            if (cx-1>=0 && level.GetState(cx-1,cy)==unknown) {
              level.SetState(cx-1,cy,water);
            }
          }
        }
      }
    }

    for (std::list<Line>::iterator line=lines.begin();
        line!=lines.end();
        ++line) {
      line->sortCriteria1=std::max(line->a.GetLon(),line->b.GetLon());
      line->sortCriteria2=std::abs((line->b.GetLat()-line->a.GetLat())/(line->b.GetLon()-line->a.GetLon()));
    }

    lines.sort(BiggerLineSort());

    for (std::list<Line>::const_iterator line=lines.begin();
        line!=lines.end();
        ++line) {
      std::vector<ScanCell> cells;

      ScanConvertLine((int)((line->a.GetLon()+180.0)/level.cellWidth),
                      (int)((line->a.GetLat()+90.0)/level.cellHeight),
                      (int)((line->b.GetLon()+180.0)/level.cellWidth),
                      (int)((line->b.GetLat()+90.0)/level.cellHeight),
                      cells);
      if (line->b.GetLat()>line->a.GetLat()) {
        // up
        for (size_t i=0; i<cells.size(); i++) {
          if (level.IsIn(cells[i].x,cells[i].y)) {
            int cx=cells[i].x-level.cellXStart;
            int cy=cells[i].y-level.cellYStart;

            if (cx+1<level.cellXCount && level.GetState(cx+1,cy)==unknown) {
              level.SetState(cx+1,cy,water);
            }
          }
        }
      }
      else if (line->b.GetLat()<line->a.GetLat()) {
        // down
        for (size_t i=0; i<cells.size(); i++) {
          if (level.IsIn(cells[i].x,cells[i].y)) {
            int cx=cells[i].x-level.cellXStart;
            int cy=cells[i].y-level.cellYStart;

            if (cx+1<level.cellXCount && level.GetState(cx+1,cy)==unknown) {
              level.SetState(cx+1,cy,land);
            }
          }
        }
      }
    }
  }

  /**
   * See description for ScanCellsHorizontally()
   */
  void WaterIndexGenerator::ScanCellsVertically(Level& level)
  {
    std::list<Line> lines;

    for (std::list<Coast>::const_iterator coastline=coastlines.begin();
        coastline!=coastlines.end();
        ++coastline) {
      for (size_t i=0; i<coastline->coast.size()-1; i++) {
        Line line;

        line.a=coastline->coast[i];
        line.b=coastline->coast[i+1];
        lines.push_back(line);
      }
    }

    for (std::list<Line>::iterator line=lines.begin();
        line!=lines.end();
        ++line) {
      line->sortCriteria1=std::min(line->a.GetLat(),line->b.GetLat());
      line->sortCriteria2=std::abs((line->b.GetLon()-line->a.GetLon())/(line->b.GetLat()-line->a.GetLat()));
    }

    lines.sort(SmallerLineSort());

    for (std::list<Line>::const_iterator line=lines.begin();
        line!=lines.end();
        ++line) {
      std::vector<ScanCell> cells;

      ScanConvertLine((int)((line->a.GetLon()+180.0)/level.cellWidth),
                      (int)((line->a.GetLat()+90.0)/level.cellHeight),
                      (int)((line->b.GetLon()+180.0)/level.cellWidth),
                      (int)((line->b.GetLat()+90.0)/level.cellHeight),
                      cells);

      if (line->b.GetLon()>line->a.GetLon()) {
        // right
        for (size_t i=0; i<cells.size(); i++) {
          if (level.IsIn(cells[i].x,cells[i].y)) {
            int cx=cells[i].x-level.cellXStart;
            int cy=cells[i].y-level.cellYStart;

            if (cy-1>=0 && level.GetState(cx,cy-1)==unknown) {
              level.SetState(cx,cy-1,water);
            }
          }
        }
      }
      else if (line->b.GetLon()<line->a.GetLon()) {
        // left
        for (size_t i=0; i<cells.size(); i++) {
          if (level.IsIn(cells[i].x,cells[i].y)) {
            int cx=cells[i].x-level.cellXStart;
            int cy=cells[i].y-level.cellYStart;

            if (cy-1>=0 && level.GetState(cx,cy-1)==unknown) {
              level.SetState(cx,cy-1,land);
            }
          }
        }
      }
    }

    for (std::list<Line>::iterator line=lines.begin();
        line!=lines.end();
        ++line) {
      line->sortCriteria1=std::max(line->a.GetLat(),line->b.GetLat());
      line->sortCriteria2=std::abs((line->b.GetLon()-line->a.GetLon())/(line->b.GetLat()-line->a.GetLat()));
    }

    lines.sort(BiggerLineSort());

    for (std::list<Line>::const_iterator line=lines.begin();
        line!=lines.end();
        ++line) {
      std::vector<ScanCell> cells;

      ScanConvertLine((int)((line->a.GetLon()+180.0)/level.cellWidth),
                      (int)((line->a.GetLat()+90.0)/level.cellHeight),
                      (int)((line->b.GetLon()+180.0)/level.cellWidth),
                      (int)((line->b.GetLat()+90.0)/level.cellHeight),
                      cells);
      if (line->b.GetLon()>line->a.GetLon()) {
        // right
        for (size_t i=0; i<cells.size(); i++) {
          if (level.IsIn(cells[i].x,cells[i].y)) {
            int cx=cells[i].x-level.cellXStart;
            int cy=cells[i].y-level.cellYStart;

            if (cy+1<level.cellYCount && level.GetState(cx,cy+1)==unknown) {
              level.SetState(cx,cy+1,land);
            }
          }
        }
      }
      else if (line->b.GetLon()<line->a.GetLon()) {
        // left
        for (size_t i=0; i<cells.size(); i++) {
          if (level.IsIn(cells[i].x,cells[i].y)) {
            int cx=cells[i].x-level.cellXStart;
            int cy=cells[i].y-level.cellYStart;

            if (cy+1<level.cellYCount && level.GetState(cx,cy+1)==unknown) {
              level.SetState(cx,cy+1,water);
            }
          }
        }
      }
    }
  }

  /**
   * Every tile that is unknown but contain a way, must be land.
   */
  bool WaterIndexGenerator::AssumeLand(const ImportParameter& parameter,
                                       Progress& progress,
                                       const TypeConfig& typeConfig,
                                       Level& level)
  {
    FileScanner scanner;

    TypeId      coastlineWayId=typeConfig.GetWayTypeId("natural_coastline");
    uint32_t    wayCount=0;

    assert(coastlineWayId!=typeIgnore);

    // We do not yet know if we handle borders as ways or areas

    if (!scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                      "ways.dat"))) {
      progress.Error("Cannot open 'ways.dat'");
      return false;
    }

    if (!scanner.Read(wayCount)) {
      progress.Error("Error while reading number of data entries in file");
      return false;
    }

    for (uint32_t w=1; w<=wayCount; w++) {
      progress.SetProgress(w,wayCount);

      Way way;

      if (!way.Read(scanner)) {
        progress.Error(std::string("Error while reading data entry ")+
                       NumberToString(w)+" of "+
                       NumberToString(wayCount)+
                       " in file '"+
                       scanner.GetFilename()+"'");
        return false;
      }

      if (way.GetType()!=coastlineWayId &&
          !typeConfig.GetTypeInfo(way.GetType()).GetIgnoreSeaLand()) {
        if (!way.IsArea() &&
            way.nodes.size()>=2) {
          std::vector<ScanCell> cells;

          ScanConvertLine(way.nodes,-180.0,level.cellWidth,-90.0,level.cellHeight,cells);

          for (size_t i=0; i<cells.size(); i++) {
            if (level.IsIn(cells[i].x,cells[i].y)) {
              if (level.GetState(cells[i].x-level.cellXStart,cells[i].y-level.cellYStart)==unknown) {
                //std::cout << "Way " << way.GetId() << " " << typeConfig.GetTypeInfo(way.GetType()).GetName() << " is defining area as land" << std::endl;
                level.SetStateAbsolute(cells[i].x,cells[i].y,land);
              }
            }
          }
        }
      }
    }

    return true;
  }

  /**
   * Scanning from left to right and bottom to top: Every tile that is unknown but is places between land and coast or
   * land tiles must be land, too.
   */
  void WaterIndexGenerator::FillLand(Level& level)
  {
    bool cont=true;

    while (cont) {
      cont=false;

      // Left to right
      for (size_t y=0; y<level.cellYCount; y++) {
        int x=0;
        int start=0;
        int end=0;
        int state=0;

        while (x<level.cellXCount) {
          switch (state) {
            case 0:
              if (level.GetState(x,y)==land) {
                state=1;
              }
              x++;
              break;
            case 1:
              if (level.GetState(x,y)==unknown) {
                state=2;
                start=x;
                end=x;
                x++;
              }
              else {
                state=0;
              }
              break;
            case 2:
              if (level.GetState(x,y)==unknown) {
                end=x;
                x++;
              }
              else if (level.GetState(x,y)==coast || level.GetState(x,y)==land) {
                if (start<level.cellXCount && end<level.cellXCount && start<=end) {
                  for (size_t i=start; i<=end; i++) {
                    level.SetState(i,y,land);
                    cont=true;
                  }
                }

                state=0;
              }
              else {
                state=0;
              }
              break;
          }
        }
      }

      //Bottom Up
      for (size_t x=0; x<level.cellXCount; x++) {
        int y=0;
        int start=0;
        int end=0;
        int state=0;

        while (y<level.cellYCount) {
          switch (state) {
            case 0:
              if (level.GetState(x,y)==land) {
                state=1;
              }
              y++;
              break;
            case 1:
              if (level.GetState(x,y)==unknown) {
                state=2;
                start=y;
                end=y;
                y++;
              }
              else {
                state=0;
              }
              break;
            case 2:
              if (level.GetState(x,y)==unknown) {
                end=y;
                y++;
              }
              else if (level.GetState(x,y)==coast || level.GetState(x,y)==land) {
                if (start<level.cellYCount && end<level.cellYCount && start<=end) {
                  for (size_t i=start; i<=end; i++) {
                    level.SetState(x,i,land);
                    cont=true;
                  }
                }

                state=0;
              }
              else {
                state=0;
              }
              break;
          }
        }
      }
    }
  }

  /**
   * Converts all tiles of state "unknown" that touch a tile with state "water" to state "water", too.
   */
  void WaterIndexGenerator::FillWater(Level& level, size_t tileCount)
  {
    for (size_t i=1; i<=tileCount; i++) {

      Level newLevel(level);

      for (size_t y=0; y<level.cellYCount; y++) {
        for (size_t x=0; x<level.cellXCount; x++) {
          if (level.GetState(x,y)==water) {
            if (y>0) {
              if (level.GetState(x,y-1)==unknown) {
                newLevel.SetState(x,y-1,water);
              }
            }

            if (y<level.cellYCount-1) {
              if (level.GetState(x,y+1)==unknown) {
                newLevel.SetState(x,y+1,water);
              }
            }

            if (x>0) {
              if (level.GetState(x-1,y)==unknown) {
                newLevel.SetState(x-1,y,water);
              }
            }

            if (x<level.cellXCount-1) {
              if (level.GetState(x+1,y)==unknown) {
                newLevel.SetState(x+1,y,water);
              }
            }
          }
        }
      }

      level=newLevel;
    }
  }

  std::string WaterIndexGenerator::GetDescription() const
  {
    return "Generate 'water.idx'";
  }

  bool WaterIndexGenerator::Import(const ImportParameter& parameter,
                                  Progress& progress,
                                  const TypeConfig& typeConfig)
  {
    FileScanner scanner;

    TypeId      coastlineWayId=typeConfig.GetWayTypeId("natural_coastline");

    uint32_t    minLonDat;
    uint32_t    minLatDat;
    uint32_t    maxLonDat;
    uint32_t    maxLatDat;

    uint32_t    wayCount=0;

    // We must have coastline type defined
    assert(coastlineWayId!=typeIgnore);

    // Calculate size of tile cells for the maximum zoom level
    double cellWidth=360.0;
    double cellHeight=180.0;

    for (size_t i=1; i<=parameter.GetWaterIndexMaxMag(); i++) {
      cellWidth=cellWidth/2;
      cellHeight=cellHeight/2;
    }

    //
    // Read bounding box
    //

    if (!scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                      "bounding.dat"))) {
      progress.Error("Cannot open 'bounding.dat'");
      return false;
    }

    scanner.ReadNumber(minLatDat);
    scanner.ReadNumber(minLonDat);
    scanner.ReadNumber(maxLatDat);
    scanner.ReadNumber(maxLonDat);

    if (scanner.HasError() || !scanner.Close()) {
      progress.Error("Error while reading/closing 'bounding.dat'");
      return false;
    }

    levels.resize(parameter.GetWaterIndexMaxMag()+1);

    /*
    // In the beginning everything is undecided
    levels.back().SetBox(minLatDat,maxLatDat,
                         minLonDat,maxLonDat,
                         cellWidth,cellHeight);*/

    progress.SetAction("Scanning for coastlines ways");

    if (!scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                      "ways.dat"))) {
      progress.Error("Cannot open 'ways.dat'");
      return false;
    }

    if (!scanner.Read(wayCount)) {
      progress.Error("Error while reading number of data entries in file");
      return false;
    }

    for (uint32_t w=1; w<=wayCount; w++) {
      progress.SetProgress(w,wayCount);

      Way way;

      if (!way.Read(scanner)) {
        progress.Error(std::string("Error while reading data entry ")+
                       NumberToString(w)+" of "+
                       NumberToString(wayCount)+
                       " in file '"+
                       scanner.GetFilename()+"'");
        return false;
      }

      if (way.GetType()!=coastlineWayId) {
        continue;
      }

      Coast coast;

      coast.coast=way.nodes;

      coastlines.push_back(coast);
    }

    if (scanner.HasError() || !scanner.Close()) {
      progress.Error("Error while reading/closing 'ways.dat'");
      return false;
    }

    cellWidth=360.0;
    cellHeight=180.0;

    for (size_t level=0; level<levels.size(); level++) {
      progress.SetAction("Building tiles for level "+NumberToString(level));

      levels[level].SetBox(minLatDat,maxLatDat,
                           minLonDat,maxLonDat,
                           cellWidth,cellHeight);

      progress.SetAction("Setting coastline cells");
      SetCoastlineCells(levels[level]);

      progress.SetAction("Scan coastlines horizontally");
      ScanCellsHorizontally(levels[level]);

      progress.SetAction("Scan coastlines vertically");
      ScanCellsVertically(levels[level]);

      if (parameter.GetAssumeLand()) {
        progress.SetAction("Assume land");

        AssumeLand(parameter,
                   progress,
                   typeConfig,
                   levels[level]);
      }

      progress.SetAction("Filling land");
      FillLand(levels[level]);

      progress.SetAction("Filling water");
      FillWater(levels[level],20);

      cellWidth=cellWidth/2;
      cellHeight=cellHeight/2;
    }

    /*
    for (size_t l=1; l<levels.size(); l++) {
      size_t prev=levels.size()-l;
      size_t current=levels.size()-l-1;

      progress.SetAction("Propergating data from level "+NumberToString(prev)+" to "+NumberToString(current));

      levels[current].SetBox(minLatDat,maxLatDat,
                             minLonDat,maxLonDat,
                             levels[prev].cellWidth*2,levels[prev].cellHeight*2);

      for (size_t y=0; y<levels[current].cellYCount; y++) {
        for (size_t x=0; x<levels[current].cellXCount; x++) {
          State states[4];

          states[0]=unknown;
          states[1]=unknown;
          states[2]=unknown;
          states[3]=unknown;

          if (levels[prev].IsIn(x*2,y*2)) {
            states[0]=levels[prev].GetState(x*2,y*2);
          }

          if (levels[prev].IsIn(x*2+1,y*2)) {
            states[1]=levels[prev].GetState(x*2+1,y*2);
          }

          if (levels[prev].IsIn(x*2,y*2+1)) {
            states[2]=levels[prev].GetState(x*2,y*2+1);
          }

          if (levels[prev].IsIn(x*2+1,y*2+1)) {
            states[3]=levels[prev].GetState(x*2+1,y*2+1);
          }

          size_t unknownCount=0;
          size_t landCount=0;
          size_t coastCount=0;
          size_t waterCount=0;

          for (size_t i=0; i<4; i++) {
            switch (states[i]) {
            case unknown:
              unknownCount++;
              break;
            case land:
              landCount++;
              break;
            case coast:
              coastCount++;
              break;
            case water:
              waterCount++;
              break;
            }
          }

          if (coastCount>0) {
            levels[current].SetState(x,y,coast);
          }
          else if (landCount>0) {
            levels[current].SetState(x,y,land);
          }
          else if (waterCount>0) {
            levels[current].SetState(x,y,water);
          }
          else {
            levels[current].SetState(x,y,unknown);
          }
        }
      }
    }*/

    progress.SetAction("Writing 'water.idx'");

    FileWriter writer;

    if (!writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                     "water.idx"))) {
      progress.Error("Error while opening 'water.idx' for writing");
      return false;
    }

    writer.WriteNumber((uint32_t)parameter.GetWaterIndexMaxMag());

    for (size_t level=0; level<levels.size(); level++) {
      FileOffset offset=0;

      writer.GetPos(levels[level].indexEntryOffset);
      writer.WriteFileOffset(offset);

      writer.WriteNumber(levels[level].cellXStart);
      writer.WriteNumber(levels[level].cellXEnd);
      writer.WriteNumber(levels[level].cellYStart);
      writer.WriteNumber(levels[level].cellYEnd);
    }

    for (size_t level=0; level<levels.size(); level++) {
      FileOffset indexOffset;

      writer.GetPos(indexOffset);
      writer.SetPos(levels[level].indexEntryOffset);
      writer.WriteFileOffset(indexOffset);
      writer.SetPos(indexOffset);

      for (size_t c=0; c<levels[level].area.size(); c++) {
        writer.Write((uint8_t)levels[level].area[c]);
      }
    }

    if (writer.HasError() || !writer.Close()) {
      progress.Error("Error while closing 'water.idx'");
      return false;
    }

    return true;
  }
}
