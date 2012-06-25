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
  void WaterIndexGenerator::Area::SetSize(uint32_t cellXCount, uint32_t cellYCount)
  {
    this->cellXCount=cellXCount;
    this->cellYCount=cellYCount;

    uint32_t size=cellXCount*cellYCount/4;

    if (cellXCount*cellYCount%4>0) {
      size++;
    }

    area.resize(size,0x00);
  }

  WaterIndexGenerator::State WaterIndexGenerator::Area::GetState(uint32_t x, uint32_t y) const
  {
    uint32_t cellId=y*cellXCount+x;
    uint32_t index=cellId/4;
    uint32_t offset=2*(cellId%4);

    return (State)((area[index] >> offset) & 3);
  }

  void WaterIndexGenerator::Area::SetState(uint32_t x, uint32_t y, State state)
  {
    uint32_t cellId=y*cellXCount+x;
    uint32_t index=cellId/4;
    uint32_t offset=2*(cellId%4);

    area[index]=(area[index] & ~(3 << offset));
    area[index]=(area[index] | (state << offset));
  }

  /**
   * Does a scan line conversion on all costline ways where the cell size equals the tile size.
   * All tiles hit are marked as "coast".
   *
   */
  void WaterIndexGenerator::SetCoastlineCells()
  {
    for (std::list<Coast>::const_iterator coastline=coastlines.begin();
        coastline!=coastlines.end();
        ++coastline) {
      // Marks cells on the path as coast

      std::vector<ScanCell> cells;

      ScanConvertLine(coastline->coast,-180.0,cellWidth,-90.0,cellHeight,cells);

      for (size_t i=0; i<cells.size(); i++) {
        if (cells[i].x>=cellXStart &&
            cells[i].x<=cellXEnd &&
            cells[i].y>=cellYStart &&
            cells[i].y<=cellYEnd) {
          area.SetState(cells[i].x-cellXStart,cells[i].y-cellYStart,coast);
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
      line->sortCriteria1=std::min(line->a.GetLon(),line->b.GetLon());
      line->sortCriteria2=std::abs((line->b.GetLat()-line->a.GetLat())/(line->b.GetLon()-line->a.GetLon()));
    }

    lines.sort(SmallerLineSort());

    for (std::list<Line>::const_iterator line=lines.begin();
        line!=lines.end();
        ++line) {
      std::vector<ScanCell> cells;

      ScanConvertLine((int)((line->a.GetLon()+180.0)/cellWidth),
                      (int)((line->a.GetLat()+90.0)/cellHeight),
                      (int)((line->b.GetLon()+180.0)/cellWidth),
                      (int)((line->b.GetLat()+90.0)/cellHeight),
                      cells);

      if (line->b.GetLat()>line->a.GetLat()) {
        // up
        for (size_t i=0; i<cells.size(); i++) {
          if (cells[i].x>=cellXStart &&
              cells[i].x<=cellXEnd &&
              cells[i].y>=cellYStart &&
              cells[i].y<=cellYEnd) {
            int cx=cells[i].x-cellXStart;
            int cy=cells[i].y-cellYStart;

            if (cx-1>=0 && area.GetState(cx-1,cy)==unknown) {
              area.SetState(cx-1,cy,land);
            }
          }
        }
      }
      else if (line->b.GetLat()<line->a.GetLat()) {
        // down
        for (size_t i=0; i<cells.size(); i++) {
          if (cells[i].x>=cellXStart &&
              cells[i].x<=cellXEnd &&
              cells[i].y>=cellYStart &&
              cells[i].y<=cellYEnd) {
            int cx=cells[i].x-cellXStart;
            int cy=cells[i].y-cellYStart;

            if (cx-1>=0 && area.GetState(cx-1,cy)==unknown) {
              area.SetState(cx-1,cy,water);
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

      ScanConvertLine((int)((line->a.GetLon()+180.0)/cellWidth),
                      (int)((line->a.GetLat()+90.0)/cellHeight),
                      (int)((line->b.GetLon()+180.0)/cellWidth),
                      (int)((line->b.GetLat()+90.0)/cellHeight),
                      cells);
      if (line->b.GetLat()>line->a.GetLat()) {
        // up
        for (size_t i=0; i<cells.size(); i++) {
          if (cells[i].x>=cellXStart &&
              cells[i].x<=cellXEnd &&
              cells[i].y>=cellYStart &&
              cells[i].y<=cellYEnd) {
            int cx=cells[i].x-cellXStart;
            int cy=cells[i].y-cellYStart;

            if (cx+1<cellXCount && area.GetState(cx+1,cy)==unknown) {
              area.SetState(cx+1,cy,water);
            }
          }
        }
      }
      else if (line->b.GetLat()<line->a.GetLat()) {
        // down
        for (size_t i=0; i<cells.size(); i++) {
          if (cells[i].x>=cellXStart &&
              cells[i].x<=cellXEnd &&
              cells[i].y>=cellYStart &&
              cells[i].y<=cellYEnd) {
            int cx=cells[i].x-cellXStart;
            int cy=cells[i].y-cellYStart;

            if (cx+1<cellXCount && area.GetState(cx+1,cy)==unknown) {
              area.SetState(cx+1,cy,land);
            }
          }
        }
      }
    }
  }

  /**
   * See description for ScanCellsHorizontally()
   */
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
      line->sortCriteria1=std::min(line->a.GetLat(),line->b.GetLat());
      line->sortCriteria2=std::abs((line->b.GetLon()-line->a.GetLon())/(line->b.GetLat()-line->a.GetLat()));
    }

    lines.sort(SmallerLineSort());

    for (std::list<Line>::const_iterator line=lines.begin();
        line!=lines.end();
        ++line) {
      std::vector<ScanCell> cells;

      ScanConvertLine((int)((line->a.GetLon()+180.0)/cellWidth),
                      (int)((line->a.GetLat()+90.0)/cellHeight),
                      (int)((line->b.GetLon()+180.0)/cellWidth),
                      (int)((line->b.GetLat()+90.0)/cellHeight),
                      cells);

      if (line->b.GetLon()>line->a.GetLon()) {
        // right
        for (size_t i=0; i<cells.size(); i++) {
          if (cells[i].x>=cellXStart &&
              cells[i].x<=cellXEnd &&
              cells[i].y>=cellYStart &&
              cells[i].y<=cellYEnd) {
            int cx=cells[i].x-cellXStart;
            int cy=cells[i].y-cellYStart;

            if (cy-1>=0 && area.GetState(cx,cy-1)==unknown) {
              area.SetState(cx,cy-1,water);
            }
          }
        }
      }
      else if (line->b.GetLon()<line->a.GetLon()) {
        // left
        for (size_t i=0; i<cells.size(); i++) {
          if (cells[i].x>=cellXStart &&
              cells[i].x<=cellXEnd &&
              cells[i].y>=cellYStart &&
              cells[i].y<=cellYEnd) {
            int cx=cells[i].x-cellXStart;
            int cy=cells[i].y-cellYStart;

            if (cy-1>=0 && area.GetState(cx,cy-1)==unknown) {
              area.SetState(cx,cy-1,land);
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

      ScanConvertLine((int)((line->a.GetLon()+180.0)/cellWidth),
                      (int)((line->a.GetLat()+90.0)/cellHeight),
                      (int)((line->b.GetLon()+180.0)/cellWidth),
                      (int)((line->b.GetLat()+90.0)/cellHeight),
                      cells);
      if (line->b.GetLon()>line->a.GetLon()) {
        // right
        for (size_t i=0; i<cells.size(); i++) {
          if (cells[i].x>=cellXStart &&
              cells[i].x<=cellXEnd &&
              cells[i].y>=cellYStart &&
              cells[i].y<=cellYEnd) {
            int cx=cells[i].x-cellXStart;
            int cy=cells[i].y-cellYStart;

            if (cy+1<cellYCount && area.GetState(cx,cy+1)==unknown) {
            area.SetState(cx,cy+1,land);
            }
          }
        }
      }
      else if (line->b.GetLon()<line->a.GetLon()) {
        // left
        for (size_t i=0; i<cells.size(); i++) {
          if (cells[i].x>=cellXStart &&
              cells[i].x<=cellXEnd &&
              cells[i].y>=cellYStart &&
              cells[i].y<=cellYEnd) {
            int cx=cells[i].x-cellXStart;
            int cy=cells[i].y-cellYStart;

            if (cy+1<cellYCount && area.GetState(cx,cy+1)==unknown) {
              area.SetState(cx,cy+1,water);
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
                                       const TypeConfig& typeConfig)
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

          ScanConvertLine(way.nodes,-180.0,cellWidth,-90.0,cellHeight,cells);

          for (size_t i=0; i<cells.size(); i++) {
            if (cells[i].x>=cellXStart &&
                cells[i].x<=cellXEnd &&
                cells[i].y>=cellYStart &&
                cells[i].y<=cellYEnd) {
              if (area.GetState(cells[i].x-cellXStart,cells[i].y-cellYStart)==unknown) {
                //std::cout << "Way " << way.GetId() << " " << typeConfig.GetTypeInfo(way.GetType()).GetName() << " is defining area as land" << std::endl;
                area.SetState(cells[i].x-cellXStart,cells[i].y-cellYStart,land);
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

  /**
   * Converts all tiles of state "unknown" that touch a tile with state "water" to state "water", too.
   */
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

    TypeId      coastlineWayId=typeConfig.GetWayTypeId("natural_coastline");

    uint32_t    minLonDat;
    uint32_t    minLatDat;
    uint32_t    maxLonDat;
    uint32_t    maxLatDat;

    uint32_t    wayCount=0;

    progress.SetAction("Setup");

    // We do not yet know if we handle borders as ways or areas
    assert(coastlineWayId!=typeIgnore);

    // Calculate size of tile cells for the maximum zoom level
    cellWidth=360.0;
    cellHeight=180.0;

    for (size_t i=2; i<=parameter.GetWaterIndexMaxMag(); i++) {
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

    minLat=minLatDat/conversionFactor-90.0;
    minLon=minLonDat/conversionFactor-180.0;
    maxLat=maxLatDat/conversionFactor-90.0;
    maxLon=maxLonDat/conversionFactor-180.0;

    cellXStart=(uint32_t)floor((minLon+180.0)/cellWidth);
    cellXEnd=(uint32_t)floor((maxLon+180.0)/cellWidth);
    cellYStart=(uint32_t)floor((minLat+90.0)/cellHeight);
    cellYEnd=(uint32_t)floor((maxLat+90.0)/cellHeight);

    cellXCount=cellXEnd-cellXStart+1;
    cellYCount=cellYEnd-cellYStart+1;

    progress.Info("Size of sea/land bitmap for zoom level "+NumberToString(parameter.GetWaterIndexMaxMag())+
        " is "+NumberToString(cellXCount*cellYCount/4/1024)+"kb");

    // In the beginning everything is undecided
    area.SetSize(cellXCount,cellYCount);

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

    progress.SetAction("Setting coastline cells");
    SetCoastlineCells();

    progress.SetAction("Scan coastlines horizontally");
    ScanCellsHorizontally();

    progress.SetAction("Scan coastlines vertically");
    ScanCellsVertically();

    if (parameter.GetAssumeLand()) {
      progress.SetAction("Assume land");

      AssumeLand(parameter,
                 progress,
                 typeConfig);
    }

    progress.SetAction("Filling land");
    FillLand();

    progress.SetAction("Filling water");
    FillWater();

    progress.SetAction("Writing 'water.idx'");

    FileWriter writer;

    if (!writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                     "water.idx"))) {
      progress.Error("Error while opening 'water.idx' for writing");
      return false;
    }

    writer.WriteNumber((uint32_t)parameter.GetWaterIndexMaxMag());
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
