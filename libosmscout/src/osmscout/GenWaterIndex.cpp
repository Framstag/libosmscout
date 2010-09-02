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

#include <osmscout/GenWaterIndex.h>

#include <cassert>
#include <cmath>
#include <vector>

#include <osmscout/FileScanner.h>
#include <osmscout/FileWriter.h>

#include <osmscout/Relation.h>
#include <osmscout/Way.h>

#include <osmscout/Util.h>

#include <iostream>

namespace osmscout {

  void WaterIndexGenerator::Area::SetSize(size_t cellXCount, size_t cellYCount)
  {
    this->cellXCount=cellXCount;
    this->cellYCount=cellYCount;

    area.resize(cellXCount*cellYCount/4,0x00);
  }

  WaterIndexGenerator::State WaterIndexGenerator::Area::GetState(size_t x, size_t y) const
  {
    size_t cellId=y*cellXCount+x;
    size_t index=cellId/4;
    size_t offset=2*(cellId%4);

    return (State)((area[index] >> offset) & 3);
  }

  void WaterIndexGenerator::Area::SetState(size_t x, size_t y, State state)
  {
    size_t cellId=y*cellXCount+x;
    size_t index=cellId/4;
    size_t offset=2*(cellId%4);

    area[index]=(area[index] & ~(3 << offset));
    area[index]=(area[index] | (state << offset));
  }

  void WaterIndexGenerator::SetCoastlineCells()
  {
    for (std::list<Coast>::const_iterator coastline=coastlines.begin();
        coastline!=coastlines.end();
        ++coastline) {
      // Marks cells on the path as coast

      std::vector<ScanCell> cells;

      ScanConvertLine(coastline->coast,-180.0,cellWidth,-90.0,cellHeight,cells);

      for (size_t i=0; i<cells.size(); i++) {
        area.SetState(cells[i].x-cellXStart,cells[i].y-cellYStart,coast);
      }
    }
  }

  void WaterIndexGenerator::ScanCellsHorizontally()
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
      line->sortCriteria1=std::min(line->a.lon,line->b.lon);
      line->sortCriteria2=std::abs((line->b.lat-line->a.lat)/(line->b.lon-line->a.lon));
    }

    lines.sort(SmallerLineSort());

    for (std::list<Line>::const_iterator line=lines.begin();
        line!=lines.end();
        ++line) {
      std::vector<ScanCell> cells;

      ScanConvertLine((line->a.lon+180.0)/cellWidth,
                      (line->a.lat+90.0)/cellHeight,
                      (line->b.lon+180.0)/cellWidth,
                      (line->b.lat+90.0)/cellHeight,
                      cells);

      if (line->b.lat>line->a.lat) {
        // up
        for (size_t i=0; i<cells.size(); i++) {
          int cx=cells[i].x-cellXStart;
          int cy=cells[i].y-cellYStart;

          if (cx-1>=0 && area.GetState(cx-1,cy)==unknown) {
            area.SetState(cx-1,cy,land);
          }
        }
      }
      else if (line->b.lat<line->a.lat) {
        // down
        for (size_t i=0; i<cells.size(); i++) {
          int cx=cells[i].x-cellXStart;
          int cy=cells[i].y-cellYStart;

          if (cx-1>=0 && area.GetState(cx-1,cy)==unknown) {
            area.SetState(cx-1,cy,water);
          }
        }
      }
    }

    for (std::list<Line>::iterator line=lines.begin();
        line!=lines.end();
        ++line) {
      line->sortCriteria1=std::max(line->a.lon,line->b.lon);
      line->sortCriteria2=std::abs((line->b.lat-line->a.lat)/(line->b.lon-line->a.lon));
    }

    lines.sort(BiggerLineSort());

    for (std::list<Line>::const_iterator line=lines.begin();
        line!=lines.end();
        ++line) {
      std::vector<ScanCell> cells;

      ScanConvertLine((line->a.lon+180.0)/cellWidth,
                      (line->a.lat+90.0)/cellHeight,
                      (line->b.lon+180.0)/cellWidth,
                      (line->b.lat+90.0)/cellHeight,
                      cells);
      if (line->b.lat>line->a.lat) {
        // up
        for (size_t i=0; i<cells.size(); i++) {
          int cx=cells[i].x-cellXStart;
          int cy=cells[i].y-cellYStart;

          if (cx+1<cellXCount && area.GetState(cx+1,cy)==unknown) {
            area.SetState(cx+1,cy,water);
          }
        }
      }
      else if (line->b.lat<line->a.lat) {
        // down
        for (size_t i=0; i<cells.size(); i++) {
          int cx=cells[i].x-cellXStart;
          int cy=cells[i].y-cellYStart;

          if (cx+1<cellXCount && area.GetState(cx+1,cy)==unknown) {
            area.SetState(cx+1,cy,land);
          }
        }
      }
    }
  }

  void WaterIndexGenerator::ScanCellsVertically()
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
      line->sortCriteria1=std::min(line->a.lat,line->b.lat);
      line->sortCriteria2=std::abs((line->b.lon-line->a.lon)/(line->b.lat-line->a.lat));
    }

    lines.sort(SmallerLineSort());

    for (std::list<Line>::const_iterator line=lines.begin();
        line!=lines.end();
        ++line) {
      std::vector<ScanCell> cells;

      ScanConvertLine((line->a.lon+180.0)/cellWidth,
                      (line->a.lat+90.0)/cellHeight,
                      (line->b.lon+180.0)/cellWidth,
                      (line->b.lat+90.0)/cellHeight,
                      cells);

      if (line->b.lon>line->a.lon) {
        // right
        for (size_t i=0; i<cells.size(); i++) {
          int cx=cells[i].x-cellXStart;
          int cy=cells[i].y-cellYStart;

          if (cy-1>=0 && area.GetState(cx,cy-1)==unknown) {
            area.SetState(cx,cy-1,water);
          }
        }
      }
      else if (line->b.lon<line->a.lon) {
        // left
        for (size_t i=0; i<cells.size(); i++) {
          int cx=cells[i].x-cellXStart;
          int cy=cells[i].y-cellYStart;

          if (cy-1>=0 && area.GetState(cx,cy-1)==unknown) {
            area.SetState(cx,cy-1,land);
          }
        }
      }
    }

    for (std::list<Line>::iterator line=lines.begin();
        line!=lines.end();
        ++line) {
      line->sortCriteria1=std::max(line->a.lat,line->b.lat);
      line->sortCriteria2=std::abs((line->b.lon-line->a.lon)/(line->b.lat-line->a.lat));
    }

    lines.sort(BiggerLineSort());

    for (std::list<Line>::const_iterator line=lines.begin();
        line!=lines.end();
        ++line) {
      std::vector<ScanCell> cells;

      ScanConvertLine((line->a.lon+180.0)/cellWidth,
                      (line->a.lat+90.0)/cellHeight,
                      (line->b.lon+180.0)/cellWidth,
                      (line->b.lat+90.0)/cellHeight,
                      cells);
      if (line->b.lon>line->a.lon) {
        // right
        for (size_t i=0; i<cells.size(); i++) {
          int cx=cells[i].x-cellXStart;
          int cy=cells[i].y-cellYStart;

          if (cy+1<cellYCount && area.GetState(cx,cy+1)==unknown) {
            area.SetState(cx,cy+1,land);
          }
        }
      }
      else if (line->b.lon<line->a.lon) {
        // left
        for (size_t i=0; i<cells.size(); i++) {
          int cx=cells[i].x-cellXStart;
          int cy=cells[i].y-cellYStart;

          if (cy+1<cellYCount && area.GetState(cx,cy+1)==unknown) {
            area.SetState(cx,cy+1,water);
          }
        }
      }
    }
  }

  void WaterIndexGenerator::FillLand()
  {
    bool cont=true;

    while (cont) {
      cont=false;

      // Left to right
      for (size_t y=0; y<cellYCount; y++) {
        int x=0;
        int start=0;
        int end=0;
        int state=0;

        while (x<cellXCount) {
          switch (state) {
            case 0:
              if (area.GetState(x,y)==land) {
                state=1;
              }
              x++;
              break;
            case 1:
              if (area.GetState(x,y)==unknown) {
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
              if (area.GetState(x,y)==unknown) {
                end=x;
                x++;
              }
              else if (area.GetState(x,y)==coast || area.GetState(x,y)==land) {
                if (start<cellXCount && end<cellXCount && start<=end) {
                  for (size_t i=start; i<=end; i++) {
                    area.SetState(i,y,land);
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
      for (size_t x=0; x<cellXCount; x++) {
        int y=0;
        int start=0;
        int end=0;
        int state=0;

        while (y<cellYCount) {
          switch (state) {
            case 0:
              if (area.GetState(x,y)==land) {
                state=1;
              }
              y++;
              break;
            case 1:
              if (area.GetState(x,y)==unknown) {
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
              if (area.GetState(x,y)==unknown) {
                end=y;
                y++;
              }
              else if (area.GetState(x,y)==coast || area.GetState(x,y)==land) {
                if (start<cellYCount && end<cellYCount && start<=end) {
                  for (size_t i=start; i<=end; i++) {
                    area.SetState(x,i,land);
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

  bool WaterIndexGenerator::AssumeLand(const ImportParameter& parameter,
                                       Progress& progress,
                                       const TypeConfig& typeConfig)
  {
    FileScanner scanner;

    TypeId      coastlineWayId;
    uint32_t    wayCount=0;

    // We do not yet know if we handle borders as ways or areas
    coastlineWayId=typeConfig.GetWayTypeId(tagNatural,"coastline");
    assert(coastlineWayId!=typeIgnore);

    if (!scanner.Open("ways.dat")) {
      progress.Error("Cannot open 'ways.dat'");
      return false;
    }

    if (!scanner.Read(wayCount)) {
      progress.Error("Error while reading number of data entries in file");
      return false;
    }

    for (uint32_t w=1; w<=wayCount; w++) {
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
        if (!way.IsArea() && way.nodes.size()>=2) {
          std::vector<ScanCell> cells;

          ScanConvertLine(way.nodes,-180.0,cellWidth,-90.0,cellHeight,cells);

          for (size_t i=0; i<cells.size(); i++) {
            if (area.GetState(cells[i].x-cellXStart,cells[i].y-cellYStart)==unknown) {
              area.SetState(cells[i].x-cellXStart,cells[i].y-cellYStart,land);
            }
          }
       }
      }
    }

    return true;
  }

  void WaterIndexGenerator::FillWater()
  {
    for (size_t i=1; i<=3; i++) {

      Area newArea(area);

      for (size_t y=0; y<cellYCount; y++) {
        for (size_t x=0; x<cellXCount; x++) {
          if (area.GetState(x,y)==water) {
            if (y>0) {
              if (area.GetState(x,y-1)==unknown) {
                newArea.SetState(x,y-1,water);
              }
            }

            if (y<cellYCount-1) {
              if (area.GetState(x,y+1)==unknown) {
                newArea.SetState(x,y+1,water);
              }
            }

            if (x>0) {
              if (area.GetState(x-1,y)==unknown) {
                newArea.SetState(x-1,y,water);
              }
            }

            if (x<cellXCount-1) {
              if (area.GetState(x+1,y)==unknown) {
                newArea.SetState(x+1,y,water);
              }
            }
          }
        }
      }

      area=newArea;
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

    TypeId      coastlineWayId;

    uint32_t    minLonDat;
    uint32_t    minLatDat;
    uint32_t    maxLonDat;
    uint32_t    maxLatDat;

    uint32_t    wayCount=0;

    progress.SetAction("Setup");

    // We do not yet know if we handle borders as ways or areas
    coastlineWayId=typeConfig.GetWayTypeId(tagNatural,"coastline");
    assert(coastlineWayId!=typeIgnore);

    cellWidth=360.0;
    cellHeight=180.0;

    for (size_t i=2; i<=parameter.GetWaterIndexMaxMag(); i++) {
      cellWidth=cellWidth/2;
      cellHeight=cellHeight/2;
    }

    std::cout << "Cell dimension: " << cellWidth << "x" << cellHeight << std::endl;

    if (!scanner.Open("bounding.dat")) {
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

    minLat=minLatDat/conversionFactor-90.0;
    minLon=minLonDat/conversionFactor-180.0;
    maxLat=maxLatDat/conversionFactor-90.0;
    maxLon=maxLonDat/conversionFactor-180.0;

    std::cout << "Data bounding box: [" << minLon << "," << minLat << "] - [" << maxLon << "," << maxLat << "]" << std::endl;

    cellXStart=floor((minLon+180.0)/cellWidth);
    cellXEnd=floor((maxLon+180.0)/cellWidth);
    cellYStart=floor((minLat+90.0)/cellHeight);
    cellYEnd=floor((maxLat+90.0)/cellHeight);

    cellXCount=cellXEnd-cellXStart+1;
    cellYCount=cellYEnd-cellYStart+1;

    std::cout << "Cell rectangle: [" << cellXStart << "," << cellYStart << "]x[" << cellXEnd << "," << cellYEnd << "]";
    std::cout <<  " => " << cellXCount << "x" << cellYCount << std::endl;

    std::cout << "Array size: " << cellXCount*cellYCount/4/1024 << "kb" << std::endl;

    // In the beginning everything is undecided
    area.SetSize(cellXCount,cellYCount);

    progress.SetAction("Scanning ways");

    if (!scanner.Open("ways.dat")) {
      progress.Error("Cannot open 'ways.dat'");
      return false;
    }

    if (!scanner.Read(wayCount)) {
      progress.Error("Error while reading number of data entries in file");
      return false;
    }

    for (uint32_t w=1; w<=wayCount; w++) {
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

    progress.SetAction("Setting coastline cells");
    SetCoastlineCells();

    progress.SetAction("Scan coastlines horizontally");
    ScanCellsHorizontally();

    progress.SetAction("Scan coastlines vertically");
    ScanCellsVertically();

    progress.SetAction("Assume land");
    AssumeLand(parameter,
               progress,
               typeConfig);

    progress.SetAction("Filling land");
    FillLand();

    progress.SetAction("Filling water");
    FillWater();

    progress.SetAction("Writing 'water.idx'");

    FileWriter writer;

    if (!writer.Open("water.idx")) {
      progress.Error("Error while opening 'water.idx' for writing");
      return false;
    }

    writer.WriteNumber(parameter.GetWaterIndexMaxMag());
    writer.WriteNumber(cellXStart);
    writer.WriteNumber(cellXEnd);
    writer.WriteNumber(cellYStart);
    writer.WriteNumber(cellYEnd);

    for (size_t i=0; i<area.area.size(); i++) {
      writer.Write(area.area[i]);
    }

    if (writer.HasError() || !writer.Close()) {
      progress.Error("Error while closing 'water.idx'");
      return false;
    }

    return true;
  }
}
