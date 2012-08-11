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

#include <osmscout/Point.h>
#include <osmscout/Relation.h>
#include <osmscout/Way.h>

#include <osmscout/util/File.h>
#include <osmscout/util/FileScanner.h>
#include <osmscout/util/String.h>

#include <osmscout/private/Math.h>

#include <iostream>
#include <iomanip>

//#define DEBUG_COASTLINE
//#define DEBUG_TILING

namespace osmscout {

  GroundTile::Coord WaterIndexGenerator::Transform(const Point& point,
                                                   const Level& level,
                                                   double cellMinLat,
                                                   double cellMinLon,
                                                   bool coast)
  {
    GroundTile::Coord coord(floor((point.GetLon()-cellMinLon)/level.cellWidth*GroundTile::Coord::CELL_MAX+0.5),
                            floor((point.GetLat()-cellMinLat)/level.cellHeight*GroundTile::Coord::CELL_MAX+0.5),
                            coast);

    return coord;
  }

  /**
   * Sets the size of the bitmap and initializes state of all tiles to "unknown"
   */
  void WaterIndexGenerator::Level::SetBox(uint32_t minLat, uint32_t maxLat,
                                          uint32_t minLon, uint32_t maxLon,
                                          double cellWidth, double cellHeight)
  {
    this->cellWidth=cellWidth;
    this->cellHeight=cellHeight;

    // Convert to the real double values
    this->minLat=minLat/conversionFactor-90.0;
    this->maxLat=maxLat/conversionFactor-90.0;
    this->minLon=minLon/conversionFactor-180.0;
    this->maxLon=maxLon/conversionFactor-180.0;

    cellXStart=(uint32_t)floor((this->minLon+180.0)/cellWidth);
    cellXEnd=(uint32_t)floor((this->maxLon+180.0)/cellWidth);
    cellYStart=(uint32_t)floor((this->minLat+90.0)/cellHeight);
    cellYEnd=(uint32_t)floor((this->maxLat+90.0)/cellHeight);

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

  WaterIndexGenerator::State WaterIndexGenerator::Level::GetStateAbsolute(uint32_t x, uint32_t y) const
  {
    return GetState(x+cellXStart,y+cellYStart);
  }

  void WaterIndexGenerator::Level::SetStateAbsolute(uint32_t x, uint32_t y, State state)
  {
    SetState(x-cellXStart,y-cellYStart,state);
  }

  bool WaterIndexGenerator::LoadCoastlines(const ImportParameter& parameter,
                                           Progress& progress,
                                           const TypeConfig& typeConfig)
  {
    // We must have coastline type defined
    TypeId      coastlineWayId=typeConfig.GetWayTypeId("natural_coastline");
    TypeId      coastlineAreaId=typeConfig.GetAreaTypeId("natural_coastline");
    FileScanner scanner;
    uint32_t    wayCount=0;

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

      if (way.GetType()!=coastlineWayId &&
          way.GetType()!=coastlineAreaId) {
        continue;
      }

      CoastRef coast=new Coast();

      coast->id=way.GetId();
      coast->coast=way.nodes;

      coastlines.push_back(coast);
    }

    if (!scanner.Close()) {
      progress.Error("Error while reading/closing 'ways.dat'");
      return false;
    }

    return true;
  }

  void WaterIndexGenerator::MergeCoastlines(Progress& progress)
  {
    progress.SetAction("Merging coastlines");

    progress.Info("Initial coastline count: "+NumberToString(coastlines.size()));

    std::map<Id,CoastRef> coastStartMap;
    std::list<CoastRef>   mergedCoastlines;
    std::set<Id>          blacklist;

    std::list<CoastRef>::iterator c=coastlines.begin();
    while( c!=coastlines.end()) {
      CoastRef coast=*c;

      if (coast->coast.front().GetId()!=coast->coast.back().GetId()) {
        coastStartMap.insert(std::make_pair(coast->coast.front().GetId(),coast));

        c++;
      }
      else {
        mergedCoastlines.push_back(coast);

        c=coastlines.erase(c);
      }
    }

    bool merged=true;

    while (merged) {
      merged=false;

      for (std::list<CoastRef>::iterator c=coastlines.begin();
          c!=coastlines.end();
          ++c) {
        if (blacklist.find((*c)->id)!=blacklist.end()) {
          continue;
        }

        CoastRef coast=*c;

        std::map<Id,CoastRef>::iterator other=coastStartMap.find(coast->coast.back().GetId());

        if (other!=coastStartMap.end() &&
            blacklist.find(other->second->id)==blacklist.end() &&
            coast->id!=other->second->id) {
          for (size_t i=1; i<other->second->coast.size(); i++) {
            coast->coast.push_back(other->second->coast[i]);
          }
          other->second->coast.clear();

          blacklist.insert(other->second->id);
          coastStartMap.erase(other);

          merged=true;
        }
      }
    }

    // Gather merged coastlines
    for (std::list<CoastRef>::iterator c=coastlines.begin();
        c!=coastlines.end();
        ++c) {
      if (blacklist.find((*c)->id)!=blacklist.end()) {
        continue;
      }

      mergedCoastlines.push_back(*c);
    }

    progress.Info("Final coastline count: "+NumberToString(mergedCoastlines.size()));

    coastlines=mergedCoastlines;
  }


  /**
   * Markes as cells as "coast", if one of the coastlines intersects with it..
   *
   */
  void WaterIndexGenerator::MarkCoastlineCells(Progress& progress,
                                               Level& level)
  {
    progress.Info("Marking cells containing coastlines");

    for (std::list<CoastRef>::const_iterator c=coastlines.begin();
        c!=coastlines.end();
        ++c) {
      const CoastRef& coastline=*c;

      // Marks cells on the path as coast

      std::set<Coord> coords;

      GetCells(level,coastline->coast,coords);

      for (std::set<Coord>::const_iterator coord=coords.begin();
          coord!=coords.end();
          ++coord) {
        if (level.IsIn(coord->x,coord->y)) {
          if (level.GetState(coord->x-level.cellXStart,coord->y-level.cellYStart)==unknown) {
#if defined(DEBUG_TILING)
            std::cout << "Coastline: " << coord->x-level.cellXStart << "," << coord->y-level.cellYStart << std::endl;
#endif
            level.SetStateAbsolute(coord->x,coord->y,coast);
          }
        }
      }

    }
  }

  void WaterIndexGenerator::CalculateLandCells(Progress& progress,
                                               Level& level,
                                               const CoastlineData& data,
                                               const std::map<Coord,std::list<GroundTile> >& cellGroundTileMap)
  {
    progress.Info("Calculate land cells");

    for (std::map<Coord,std::list<GroundTile> >::const_iterator coord=cellGroundTileMap.begin();
        coord!=cellGroundTileMap.end();
        ++coord) {
      State state[4]; // top, right, bottom, left

      state[0]=unknown;
      state[1]=unknown;
      state[2]=unknown;
      state[3]=unknown;

      // Preset top
      if (coord->first.y<level.cellYCount-1) {
        state[0]=level.GetState(coord->first.x,coord->first.y+1);
      }

      // Preset right
      if (coord->first.x<level.cellXCount-1) {
        state[1]=level.GetState(coord->first.x+1,coord->first.y);
      }

      // Preset bottom
      if (coord->first.y>0) {
        state[2]=level.GetState(coord->first.x,coord->first.y-1);
      }

      // Preset left
      if (coord->first.x>0) {
        state[3]=level.GetState(coord->first.x-1,coord->first.y);
      }

      for (std::list<GroundTile>::const_iterator tile=coord->second.begin();
           tile!=coord->second.end();
           ++tile) {
        for (size_t c=0; c<tile->coords.size()-1;c++) {
          // Top
          if (tile->coords[c].x==0 &&
              tile->coords[c].y==GroundTile::Coord::CELL_MAX &&
              tile->coords[c+1].x==GroundTile::Coord::CELL_MAX &&
              tile->coords[c+1].y==GroundTile::Coord::CELL_MAX) {
            if (state[0]!=unknown) {
              continue;
            }

            state[0]=land;
          }
          else if (tile->coords[c].y==GroundTile::Coord::CELL_MAX &&
                   tile->coords[c].x!=0 &&
                   tile->coords[c].x!=GroundTile::Coord::CELL_MAX) {
            if (state[0]!=unknown) {
              continue;
            }

            state[0]=coast;
          }

          // Right
          if (tile->coords[c].x==GroundTile::Coord::CELL_MAX &&
              tile->coords[c].y==GroundTile::Coord::CELL_MAX &&
              tile->coords[c+1].x==GroundTile::Coord::CELL_MAX &&
              tile->coords[c+1].y==0) {
            if (state[1]!=unknown) {
              continue;
            }

            state[1]=land;
          }
          else if (tile->coords[c].x==GroundTile::Coord::CELL_MAX &&
                   tile->coords[c].y!=0 &&
                   tile->coords[c].y!=GroundTile::Coord::CELL_MAX) {
            if (state[1]!=unknown) {
              continue;
            }

            state[1]=coast;
          }

          // Below
          if (tile->coords[c].x==GroundTile::Coord::CELL_MAX &&
              tile->coords[c].y==0 &&
              tile->coords[c+1].x==0 &&
              tile->coords[c+1].y==0) {
            if (state[2]!=unknown) {
              continue;
            }

            state[2]=land;
          }
          else if (tile->coords[c].y==0 &&
                   tile->coords[c].x!=0 &&
                   tile->coords[c].x!=GroundTile::Coord::CELL_MAX) {
            if (state[2]!=unknown) {
              continue;
            }

            state[2]=coast;
          }

          // left
          if (tile->coords[c].x==0 &&
              tile->coords[c].y==0 &&
              tile->coords[c+1].x==0 &&
              tile->coords[c+1].y==GroundTile::Coord::CELL_MAX) {
            if (state[3]!=unknown) {
              continue;
            }

            state[3]=land;
          }
          else if (tile->coords[c].x==0 &&
                   tile->coords[c].y!=0 &&
                   tile->coords[c].y!=GroundTile::Coord::CELL_MAX) {
            if (state[3]!=unknown) {
              continue;
            }

            state[3]=coast;
          }
        }
      }

      if (coord->first.y<level.cellYCount-1) {
        if (state[0]!=unknown) {
          level.SetState(coord->first.x,coord->first.y+1,state[0]);
        }
        else {
          level.SetState(coord->first.x,coord->first.y+1,water);
        }
      }

      if (coord->first.x<level.cellXCount-1) {
        if (state[1]!=unknown) {
          level.SetState(coord->first.x+1,coord->first.y,state[1]);
        }
        else {
          level.SetState(coord->first.x+1,coord->first.y,water);
        }
      }

      if (coord->first.y>0) {
        if (state[2]!=unknown) {
          level.SetState(coord->first.x,coord->first.y-1,state[2]);
        }
        else {
          level.SetState(coord->first.x,coord->first.y-1,water);
        }
      }

      if (coord->first.x>0) {
        if (state[3]!=unknown) {
          level.SetState(coord->first.x-1,coord->first.y,state[3]);
        }
        else {
          level.SetState(coord->first.x-1,coord->first.y,water);
        }
      }
    }
  }

  /**
   * Every cell that is unknown but contains a way (that is marked
   * as "to be ignored"), must be land.
   */
  bool WaterIndexGenerator::AssumeLand(const ImportParameter& parameter,
                                       Progress& progress,
                                       const TypeConfig& typeConfig,
                                       Level& level)
  {
    progress.Info("Assume land");

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
          std::set<Coord> coords;

          GetCells(level,way.nodes,coords);

          for (std::set<Coord>::const_iterator coord=coords.begin();
              coord!=coords.end();
              ++coord) {
            if (level.IsIn(coord->x,coord->y)) {
              if (level.GetState(coord->x-level.cellXStart,coord->y-level.cellYStart)==unknown) {
#if defined(DEBUG_TILING)
          std::cout << "Assume land: " << coord->x-level.cellXStart << "," << coord->y-level.cellYStart << " Way " << way.GetId() << " " << typeConfig.GetTypeInfo(way.GetType()).GetName() << " is defining area as land" << std::endl;
#endif
                level.SetStateAbsolute(coord->x,coord->y,land);
              }
            }
          }
        }
      }
    }

    return true;
  }

  /**
   * Converts all cells of state "unknown" that touch a tile with state
   * "water" to state "water", too.
   */
  void WaterIndexGenerator::FillWater(Progress& progress,
                                      Level& level,
                                      size_t tileCount)
  {
    progress.Info("Filling water");

    for (size_t i=1; i<=tileCount; i++) {

      Level newLevel(level);

      for (size_t y=0; y<level.cellYCount; y++) {
        for (size_t x=0; x<level.cellXCount; x++) {
          if (level.GetState(x,y)==water) {
            if (y>0) {
              if (level.GetState(x,y-1)==unknown) {
#if defined(DEBUG_TILING)
                std::cout << "Water below water: " << x << "," << y-1 << std::endl;
#endif
                newLevel.SetState(x,y-1,water);
              }
            }

            if (y<level.cellYCount-1) {
              if (level.GetState(x,y+1)==unknown) {
#if defined(DEBUG_TILING)
                std::cout << "Water above water: " << x << "," << y+1 << std::endl;
#endif
                newLevel.SetState(x,y+1,water);
              }
            }

            if (x>0) {
              if (level.GetState(x-1,y)==unknown) {
#if defined(DEBUG_TILING)
                std::cout << "Water left of water: " << x-1 << "," << y << std::endl;
#endif
                newLevel.SetState(x-1,y,water);
              }
            }

            if (x<level.cellXCount-1) {
              if (level.GetState(x+1,y)==unknown) {
#if defined(DEBUG_TILING)
                std::cout << "Water right of water: " << x+1 << "," << y << std::endl;
#endif
                newLevel.SetState(x+1,y,water);
              }
            }
          }
        }
      }

      level=newLevel;
    }
  }

  /**
   * Scanning from left to right and bottom to top: Every tile that is unknown
   * but is placed between land and coast or land cells must be land, too.
   */
  void WaterIndexGenerator::FillLand(Progress& progress,
                                     Level& level)
  {
    progress.Info("Filling land");

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
#if defined(DEBUG_TILING)
                    std::cout << "Land between: " << i << "," << y << std::endl;
#endif
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
#if defined(DEBUG_TILING)
                    std::cout << "Land between: " << x << "," << i << std::endl;
#endif
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

  void WaterIndexGenerator::DumpIndexHeader(const ImportParameter& parameter,
                                            FileWriter& writer,
                                            std::vector<Level>& levels)
  {
    writer.WriteNumber((uint32_t)(parameter.GetWaterIndexMinMag()));
    writer.WriteNumber((uint32_t)(parameter.GetWaterIndexMaxMag()));

    for (size_t level=0; level<levels.size(); level++) {
      FileOffset offset=0;
      writer.GetPos(levels[level].indexEntryOffset);
      writer.WriteFileOffset(offset);
      writer.WriteNumber(levels[level].cellXStart);
      writer.WriteNumber(levels[level].cellXEnd);
      writer.WriteNumber(levels[level].cellYStart);
      writer.WriteNumber(levels[level].cellYEnd);
    }
  }

  void WaterIndexGenerator::HandleAreaCoastlinesCompletelyInACell(const ImportParameter& parameter,
                                                                  Progress& progress,
                                                                  Projection& projection,
                                                                  const Level& level,
                                                                  const std::list<CoastRef>& coastlines,
                                                                  std::map<Coord,std::list<GroundTile> >& cellGroundTileMap)
  {
    progress.Info("Handle area coastline completely in a cell");

    size_t currentCoastline=0;
    for (std::list<CoastRef>::const_iterator c=coastlines.begin();
        c!=coastlines.end();
        ++c) {
      const  CoastRef& coast=*c;

      currentCoastline++;
      progress.SetProgress(currentCoastline,coastlines.size());

      if (coast->coast.front().GetId()==coast->coast.back().GetId()) {
        double   minLat,maxLat,minLon,maxLon;
        uint32_t cx1,cx2,cy1,cy2;

        minLat=coast->coast[0].GetLat();
        maxLat=minLat;
        minLon=coast->coast[0].GetLon();
        maxLon=minLon;

        for (size_t p=1; p<coast->coast.size()-1; p++) {
          minLat=std::min(minLat,coast->coast[p].GetLat());
          maxLat=std::max(maxLat,coast->coast[p].GetLat());

          minLon=std::min(minLon,coast->coast[p].GetLon());
          maxLon=std::max(maxLon,coast->coast[p].GetLon());
        }

        cx1=(uint32_t)floor((minLon+180.0)/level.cellWidth);
        cx2=(uint32_t)floor((maxLon+180.0)/level.cellWidth);
        cy1=(uint32_t)floor((minLat+90.0)/level.cellHeight);
        cy2=(uint32_t)floor((maxLat+90.0)/level.cellHeight);

        double cellMinLat=level.cellHeight*cy1-90.0;
        double cellMinLon=level.cellWidth*cx1-180.0;

        if (cx1==cx2 && cy1==cy2) {
          Coord        coord(cx1-level.cellXStart,cy1-level.cellYStart);
          TransPolygon polygon;
          double       minX;
          double       maxX;
          double       minY;
          double       maxY;
          GroundTile   groundTile(GroundTile::land);

          polygon.TransformArea(projection,parameter.GetOptimizationWayMethod(),coast->coast, 1.0);

          polygon.GetBoundingBox(minX,minY,maxX,maxY);

          if (maxX-minX<=1.0 && maxY-minY<=1.0) {
            continue;
          }

          groundTile.coords.reserve(polygon.GetLength());

          for (size_t p=polygon.GetStart(); p<=polygon.GetEnd(); p++) {
            if (polygon.points[p].draw) {
              GroundTile::Coord coord;

              if (p==polygon.GetEnd()) {
                coord=Transform(coast->coast[polygon.GetStart()],level,cellMinLat,cellMinLon,false);
              }
              else {
                coord=Transform(coast->coast[p],level,cellMinLat,cellMinLon,true);
              }

              groundTile.coords.push_back(coord);
            }
          }

          cellGroundTileMap[coord].push_back(groundTile);
        }
      }
    }
  }

  static bool IsLeftOnSameBorder(size_t border, const Point& a,const Point& b)
  {
    switch (border) {
    case 0:
      return b.GetLon()>=a.GetLon();
    case 1:
      return b.GetLat()<=a.GetLat();
    case 2:
      return b.GetLon()<=a.GetLon();
    case 3:
      return b.GetLat()>=a.GetLat();
    }

    assert(false);

    return false;
  }

  void WaterIndexGenerator::GetCells(const Level& level,
                                     const Point& a,
                                     const Point& b,
                                     std::set<Coord>& cellIntersections)
  {
    uint32_t cx1=(uint32_t)((a.GetLon()+180.0)/level.cellWidth);
    uint32_t cy1=(uint32_t)((a.GetLat()+90.0)/level.cellHeight);

    uint32_t cx2=(uint32_t)((b.GetLon()+180.0)/level.cellWidth);
    uint32_t cy2=(uint32_t)((b.GetLat()+90.0)/level.cellHeight);

    cellIntersections.insert(Coord(cx1,cy1));

    if (cx1!=cx2 || cy1!=cy2) {
      for (size_t x=std::min(cx1,cx2); x<=std::max(cx1,cx2); x++) {
        for (size_t y=std::min(cy1,cy2); y<=std::max(cy1,cy2); y++) {

          Coord  coord(x,y);
          Point  borderPoints[5];
          double lonMin,lonMax,latMin,latMax;

          lonMin=x*level.cellWidth-180.0;
          lonMax=lonMin+level.cellWidth;
          latMin=y*level.cellHeight-90.0;
          latMax=latMin+level.cellHeight;

          borderPoints[0]=Point(1,latMax,lonMin); // top left
          borderPoints[1]=Point(2,latMax,lonMax); // top right
          borderPoints[2]=Point(3,latMin,lonMax); // bottom right
          borderPoints[3]=Point(4,latMin,lonMin); // bottom left
          borderPoints[4]=borderPoints[0];    // To avoid "% 4" on all indexes

          size_t corner=0;

          while (corner<4) {
            if (LinesIntersect(a,
                               b,
                               borderPoints[corner],
                               borderPoints[corner+1])) {
              cellIntersections.insert(coord);

              break;
            }

            corner++;
          }
        }
      }
    }
  }

  void WaterIndexGenerator::GetCells(const Level& level,
                                     const std::vector<Point>& points,
                                     std::set<Coord>& cellIntersections)
  {
    for (size_t p=0; p<points.size()-1; p++) {
      GetCells(level,points[p],points[p+1],cellIntersections);
    }
  }

  void WaterIndexGenerator::GetCellIntersections(const Level& level,
                                                 const std::vector<Point>& points,
                                                 size_t coastline,
                                                 std::map<Coord,std::list<Intersection> >& cellIntersections)
  {
    for (size_t p=0; p<points.size()-1; p++) {
      uint32_t cx1=(uint32_t)((points[p].GetLon()+180.0)/level.cellWidth);
      uint32_t cy1=(uint32_t)((points[p].GetLat()+90.0)/level.cellHeight);

      uint32_t cx2=(uint32_t)((points[p+1].GetLon()+180.0)/level.cellWidth);
      uint32_t cy2=(uint32_t)((points[p+1].GetLat()+90.0)/level.cellHeight);

      if (cx1!=cx2 || cy1!=cy2) {
        for (size_t x=std::min(cx1,cx2); x<=std::max(cx1,cx2); x++) {
          for (size_t y=std::min(cy1,cy2); y<=std::max(cy1,cy2); y++) {

            Coord              coord(x-level.cellXStart,y-level.cellYStart);
            Point              borderPoints[5];
            double             lonMin,lonMax,latMin,latMax;

            lonMin=x*level.cellWidth-180.0;
            lonMax=(x+1)*level.cellWidth-180.0;
            latMin=y*level.cellHeight-90.0;
            latMax=(y+1)*level.cellHeight-90.0;

            borderPoints[0]=Point(1,latMax,lonMin); // top left
            borderPoints[1]=Point(2,latMax,lonMax); // top right
            borderPoints[2]=Point(3,latMin,lonMax); // bottom right
            borderPoints[3]=Point(4,latMin,lonMin); // bottom left
            borderPoints[4]=borderPoints[0];    // To avoid modula 4 on all indexes

            size_t       intersectionCount=0;
            Intersection firstIntersection;
            Intersection secondIntersection;
            size_t       corner=0;

            while (corner<4) {
              if (GetLineIntersection(points[p],
                                      points[p+1],
                                      borderPoints[corner],
                                      borderPoints[corner+1],
                                      firstIntersection.point)) {
                intersectionCount++;

                firstIntersection.coastline=coastline;
                firstIntersection.prevWayPointIndex=p;
                firstIntersection.distanceSquare=DistanceSquare(points[p],firstIntersection.point);
                firstIntersection.borderIndex=corner;

                corner++;
                break;
              }

              corner++;
            }

            while (corner<4) {
              if (GetLineIntersection(points[p],
                                      points[p+1],
                                      borderPoints[corner],
                                      borderPoints[corner+1],
                                      secondIntersection.point)) {
                intersectionCount++;

                secondIntersection.coastline=coastline;
                secondIntersection.prevWayPointIndex=p;
                secondIntersection.distanceSquare=DistanceSquare(points[p],secondIntersection.point);
                secondIntersection.borderIndex=corner;

                corner++;
                break;
              }

              corner++;
            }

            if (x==cx1 && y==cy1) {
              // The segment always leaves the origin cell

              assert(intersectionCount==1);

              firstIntersection.direction=-1;
              cellIntersections[coord].push_back(firstIntersection);
            }
            else if (x==cx2 && y==cy2) {
              // The segment always enteres the detsination cell

              assert(intersectionCount==1);

              firstIntersection.direction=1;
              cellIntersections[coord].push_back(firstIntersection);
            }
            else {
              assert(intersectionCount==0 || intersectionCount==1 || intersectionCount==2);

              if (intersectionCount==1) {
                // If we only have one intersection with borders of cells between the starting borderPoints and the
                // target borderPoints then this is a "touch"
                firstIntersection.direction=0;
                cellIntersections[coord].push_back(firstIntersection);
              }
              else if (intersectionCount==2) {
                // If we have two intersections with borders of cells between the starting cell and the
                // target cell then the one closer to the starting point is the incoming and the one farer
                // away is the leaving one
                double firstLength=DistanceSquare(firstIntersection.point,points[p]);
                double secondLength=DistanceSquare(secondIntersection.point,points[p]);

                if (firstLength<=secondLength) {
                  firstIntersection.direction=1; // Enter
                  cellIntersections[coord].push_back(firstIntersection);

                  secondIntersection.direction=-1; // Leave
                  cellIntersections[coord].push_back(secondIntersection);
                }
                else {
                  secondIntersection.direction=1; // Enter
                  cellIntersections[coord].push_back(secondIntersection);

                  firstIntersection.direction=-1; // Leave
                  cellIntersections[coord].push_back(firstIntersection);

                }
              }
            }
          }
        }
      }
    }
  }

  /**
   * Collects, calculates and generates a number of data about a coastline.
   */
  void WaterIndexGenerator::GetCoastlineData(const ImportParameter& parameter,
                                             Progress& progress,
                                             Projection& projection,
                                             const Level& level,
                                             const std::list<CoastRef>& coastlines,
                                             CoastlineData& data)
  {
    progress.Info("Calculate coastline data");

    data.isArea.resize(coastlines.size());
    data.isCompletelyInCell.resize(coastlines.size(),false);
    data.cell.resize(coastlines.size(),Coord(0,0));
    data.points.resize(coastlines.size());
    data.cellIntersections.resize(coastlines.size());

    size_t curCoast=0;
    for (std::list<CoastRef>::const_iterator c=coastlines.begin();
        c!=coastlines.end();
        ++c) {
      const  CoastRef& coast=*c;
      BoundingBox      boundingBox;

      progress.SetProgress(curCoast,coastlines.size());

      data.isArea[curCoast]=coast->coast.front().GetId()==coast->coast.back().GetId();

      boundingBox.minLat=coast->coast[0].GetLat();
      boundingBox.maxLat=boundingBox.minLat;
      boundingBox.minLon=coast->coast[0].GetLon();
      boundingBox.maxLon=boundingBox.minLon;

      for (size_t p=1; p<coast->coast.size(); p++) {
        boundingBox.minLat=std::min(boundingBox.minLat,coast->coast[p].GetLat());
        boundingBox.maxLat=std::max(boundingBox.maxLat,coast->coast[p].GetLat());

        boundingBox.minLon=std::min(boundingBox.minLon,coast->coast[p].GetLon());
        boundingBox.maxLon=std::max(boundingBox.maxLon,coast->coast[p].GetLon());
      }

      uint32_t cx1,cx2,cy1,cy2;

      cx1=(uint32_t)floor((boundingBox.minLon+180.0)/level.cellWidth);
      cx2=(uint32_t)floor((boundingBox.maxLon+180.0)/level.cellWidth);
      cy1=(uint32_t)floor((boundingBox.minLat+90.0)/level.cellHeight);
      cy2=(uint32_t)floor((boundingBox.maxLat+90.0)/level.cellHeight);

      if (cx1==cx2 && cy1==cy2) {
        data.cell[curCoast].x=cx1;
        data.cell[curCoast].y=cy1;
        data.isCompletelyInCell[curCoast]=true;
      }

      TransPolygon polygon;

      if (data.isArea[curCoast]) {
        // TODO: We already handled areas that are completely within one cell, we should skip them here
        polygon.TransformArea(projection,parameter.GetOptimizationWayMethod(),coast->coast, 1.0);
      }
      else {
        polygon.TransformWay(projection,parameter.GetOptimizationWayMethod(),coast->coast, 1.0);
      }

      data.points.reserve(coast->coast.size());
      for (size_t p=polygon.GetStart(); p<=polygon.GetEnd(); p++) {
        if (polygon.points[p].draw) {
          data.points[curCoast].push_back(coast->coast[p]);
        }
      }

      // Currently transformation optimization code sometimes does not correctly handle the closing point for areas
      if (data.isArea[curCoast] &&
          data.points[curCoast].front().GetId()!=data.points[curCoast].back().GetId()) {
        data.points[curCoast].push_back(data.points[curCoast].front());
      }

      // Calculate all intersections for all path steps for all cells covered
      GetCellIntersections(level,
                           data.points[curCoast],
                           curCoast,
                           data.cellIntersections[curCoast]);

      for (std::map<Coord,std::list<Intersection> >::iterator cell=data.cellIntersections[curCoast].begin();
          cell!=data.cellIntersections[curCoast].end();
          ++cell) {
        data.cellCoastlines[cell->first].push_back(curCoast);
      }

      curCoast++;
    }
  }

  WaterIndexGenerator::IntersectionPtr WaterIndexGenerator::GetPreviousIntersection(std::list<IntersectionPtr>& intersectionsPathOrder,
                                                                                    const IntersectionPtr& current)
  {
    std::list<IntersectionPtr>::iterator currentIter=intersectionsPathOrder.begin();

    while (currentIter!=intersectionsPathOrder.end() &&
           (*currentIter)!=current) {
      ++currentIter;
    }

    if (currentIter==intersectionsPathOrder.end()) {
      return NULL;
    }

    if (currentIter==intersectionsPathOrder.begin()) {
      return NULL;
    }

    currentIter--;

    return *currentIter;
  }

  /**
   * Closes the sling from the incoming intersection to the outgoing intersection traveling clock wise around the cell
   * border.
   */
  void WaterIndexGenerator::WalkBorderCW(GroundTile& groundTile,
                                         const Level& level,
                                         double cellMinLat,
                                         double cellMinLon,
                                         const IntersectionPtr& incoming,
                                         const IntersectionPtr& outgoing,
                                         const GroundTile::Coord borderCoords[])
  {

    if (outgoing->borderIndex!=incoming->borderIndex ||
        !IsLeftOnSameBorder(incoming->borderIndex,incoming->point,outgoing->point)) {
      size_t borderPoint=(incoming->borderIndex+1)%4;
      size_t endBorderPoint=outgoing->borderIndex;

      while (borderPoint!=endBorderPoint) {
        groundTile.coords.push_back(borderCoords[borderPoint]);

        if (borderPoint==3) {
          borderPoint=0;
        }
        else {
          borderPoint++;
        }
      }

      groundTile.coords.push_back(borderCoords[borderPoint]);
    }

    groundTile.coords.push_back(Transform(outgoing->point,level,cellMinLat,cellMinLon,false));
  }


  WaterIndexGenerator::IntersectionPtr WaterIndexGenerator::GetNextCW(const std::list<IntersectionPtr>& intersectionsCW,
                                                                      const IntersectionPtr& current) const
  {
    std::list<IntersectionPtr>::const_iterator next=intersectionsCW.begin();

    while (next!=intersectionsCW.end() &&
           (*next)!=current) {
      next++;
    }

    assert(next!=intersectionsCW.end());

    next++;

    if (next==intersectionsCW.end()) {
      next=intersectionsCW.begin();
    }

    assert(next!=intersectionsCW.end());

    return *next;
  }

  void WaterIndexGenerator::WalkPathBack(GroundTile& groundTile,
                                         const Level& level,
                                         double cellMinLat,
                                         double cellMinLon,
                                         const IntersectionPtr& outgoing,
                                         const IntersectionPtr& incoming,
                                         const std::vector<Point>& points,
                                         bool isArea)
  {
    groundTile.coords.back().coast=true;

    if (isArea) {
      if (outgoing->prevWayPointIndex==incoming->prevWayPointIndex &&
          outgoing->distanceSquare>incoming->distanceSquare) {
        groundTile.coords.push_back(Transform(incoming->point,level,cellMinLat,cellMinLon,false));
      }
      else {
        size_t idx=outgoing->prevWayPointIndex;
        size_t targetIdx=incoming->prevWayPointIndex+1;

        if (targetIdx==points.size()) {
          targetIdx=0;
        }

        while (idx!=targetIdx) {
          groundTile.coords.push_back(Transform(points[idx],level,cellMinLat,cellMinLon,true));

          if (idx>0) {
            idx--;
          }
          else {
            idx=points.size()-1;
          }
        }

        groundTile.coords.push_back(Transform(points[idx],level,cellMinLat,cellMinLon,true));

        groundTile.coords.push_back(Transform(incoming->point,level,cellMinLat,cellMinLon,false));
      }
    }
    else {
      size_t targetIdx=incoming->prevWayPointIndex+1;

      for (int idx=outgoing->prevWayPointIndex;
          idx>=targetIdx;
          idx--) {
        groundTile.coords.push_back(Transform(points[idx],level,cellMinLat,cellMinLon,true));
      }

      groundTile.coords.push_back(Transform(incoming->point,level,cellMinLat,cellMinLon,false));
    }
  }

  /**
   * The algorithm is as following:
   * TODO
   */
  void WaterIndexGenerator::HandleCoastlinesPartiallyInACell(const ImportParameter& parameter,
                                                             Progress& progress,
                                                             Projection& projection,
                                                             const Level& level,
                                                             std::map<Coord,std::list<GroundTile> >& cellGroundTileMap,
                                                             CoastlineData& data)
  {
    progress.Info("Handle coastlines partially in a cell");

    // For every cell with intersections
    size_t currentCell=0;
    for (std::map<Coord,std::list<size_t> >::const_iterator cell=data.cellCoastlines.begin();
         cell!=data.cellCoastlines.end();
        ++cell) {
      progress.SetProgress(currentCell,data.cellCoastlines.size());

      currentCell++;

      std::list<IntersectionPtr>               intersectionsCW;
      std::list<IntersectionPtr>               intersectionsOuter;
      std::vector<std::list<IntersectionPtr> > intersectionsPathOrder;

      intersectionsPathOrder.resize(coastlines.size());

      for (std::list<size_t>::const_iterator currentCoastline=cell->second.begin();
          currentCoastline!=cell->second.end();
          ++currentCoastline) {
        std::map<Coord,std::list<Intersection> >::iterator cellData=data.cellIntersections[*currentCoastline].find(cell->first);

        if (cellData==data.cellIntersections[*currentCoastline].end()) {
          continue;
        }

        // Build list of intersections in path order and list of intersections in clock wise order
        for (std::list<Intersection>::iterator inter=cellData->second.begin();
            inter!=cellData->second.end();
            ++inter) {
          const IntersectionPtr intersection=&(*inter);

          intersectionsPathOrder[*currentCoastline].push_back(intersection);
          intersectionsCW.push_back(intersection);
        }

        intersectionsPathOrder[*currentCoastline].sort(IntersectionByPathComparator());

        // Fix intersection order for areas
        if (data.isArea[*currentCoastline] &&
            intersectionsPathOrder[*currentCoastline].front()->direction==-1) {
          intersectionsPathOrder[*currentCoastline].push_back(intersectionsPathOrder[*currentCoastline].front());
          intersectionsPathOrder[*currentCoastline].pop_front();
        }

        for (std::list<IntersectionPtr>::reverse_iterator inter=intersectionsPathOrder[*currentCoastline].rbegin();
            inter!=intersectionsPathOrder[*currentCoastline].rend();
            inter++) {
          if ((*inter)->direction==-1) {
            intersectionsOuter.push_back(*inter);
          }
        }
      }


      intersectionsCW.sort(IntersectionCWComparator());

      double    lonMin,lonMax,latMin,latMax;
      Point     borderPoints[4];
      GroundTile::Coord borderCoords[4];

      lonMin=(level.cellXStart+cell->first.x)*level.cellWidth-180.0;
      lonMax=(level.cellXStart+cell->first.x+1)*level.cellWidth-180.0;
      latMin=(level.cellYStart+cell->first.y)*level.cellHeight-90.0;
      latMax=(level.cellYStart+cell->first.y+1)*level.cellHeight-90.0;

      borderPoints[0]=Point(1,latMax,lonMin); // top left
      borderPoints[1]=Point(2,latMax,lonMax); // top right
      borderPoints[2]=Point(3,latMin,lonMax); // bottom right
      borderPoints[3]=Point(4,latMin,lonMin); // bottom left

      borderCoords[0].Set(0,GroundTile::Coord::CELL_MAX,false);                   // top left
      borderCoords[1].Set(GroundTile::Coord::CELL_MAX,GroundTile::Coord::CELL_MAX,false); // top right
      borderCoords[2].Set(GroundTile::Coord::CELL_MAX,0,false);                   // bottom right
      borderCoords[3].Set(0,0,false);                                     // bottom left

#if defined(DEBUG_COASTLINE)
      std::cout << std::setiosflags(std::ios::fixed) << std::setprecision(6);
      std::cout << "-- Cell: " << cell->first.x << "," << cell->first.y << std::endl;

      for (size_t currentCoastline=0;
          currentCoastline<coastlines.size();
          currentCoastline++) {
        if (!intersectionsPathOrder[currentCoastline].empty()) {
          std::cout << "Coastline " << currentCoastline << std::endl;
          for (std::list<IntersectionPtr>::const_iterator iter=intersectionsPathOrder[currentCoastline].begin();
              iter!=intersectionsPathOrder[currentCoastline].end();
              ++iter) {
            IntersectionPtr intersection=*iter;
            std::cout <<"> "  << intersection->coastline << " " << points[intersection->coastline][intersection->prevWayPointIndex].GetId() << " " << intersection->prevWayPointIndex << " " << intersection->distanceSquare << " " << intersection->point.GetLat() << "," << intersection->point.GetLon() << " " << (unsigned int)intersection->borderIndex << " " << (int)intersection->direction << std::endl;
          }
        }
      }

      std::cout << "-" << std::endl;
      for (std::list<IntersectionPtr>::const_iterator iter=intersectionsCW.begin();
          iter!=intersectionsCW.end();
          ++iter) {
        IntersectionPtr intersection=*iter;
        std::cout <<"* "  << intersection->coastline << " " << points[intersection->coastline][intersection->prevWayPointIndex].GetId() << " " << (unsigned int)intersection->prevWayPointIndex << " " << intersection->distanceSquare << " " << intersection->point.GetLat() << "," << intersection->point.GetLon() << " " << (unsigned int)intersection->borderIndex << " " << (int)intersection->direction << std::endl;
      }
#endif

      while (!intersectionsOuter.empty()) {
        GroundTile      groundTile(GroundTile::land);
        IntersectionPtr initialOutgoing;

        // Take an unused outgoing intersection as far possible down the path
        initialOutgoing=intersectionsOuter.front();
        intersectionsOuter.pop_front();

#if defined(DEBUG_COASTLINE)
        std::cout << "Outgoing: " << initialOutgoing->coastline << " " << initialOutgoing->prevWayPointIndex << " " << initialOutgoing->distanceSquare << " " << isArea[initialOutgoing->coastline] << std::endl;
#endif

        groundTile.coords.push_back(Transform(initialOutgoing->point,level,latMin,lonMin,false));


        IntersectionPtr incoming=GetPreviousIntersection(intersectionsPathOrder[initialOutgoing->coastline],
                                                         initialOutgoing);

        if (incoming==NULL) {
#if defined(DEBUG_COASTLINE)
          std::cerr << "Polygon is not closed, but cannot find incoming" << std::endl;
#endif

          continue;
        }

#if defined(DEBUG_COASTLINE)
        std::cout << "Incoming: " << incoming->coastline << " " << incoming->prevWayPointIndex << " " << incoming->distanceSquare << std::endl;
#endif

        if (incoming->direction!=1) {
#if defined(DEBUG_COASTLINE)
          std::cerr << "The intersection before the outgoing intersection is not incoming as expected" << std::endl;
#endif

          continue;
        }

        WalkPathBack(groundTile,
                     level,
                     latMin,
                     lonMin,
                     initialOutgoing,
                     incoming,
                     data.points[initialOutgoing->coastline],
                     data.isArea[initialOutgoing->coastline]);

        IntersectionPtr nextCWIter=GetNextCW(intersectionsCW,
                                             incoming);

        bool error=false;

        while (!error &&
               nextCWIter!=initialOutgoing) {
#if defined(DEBUG_COASTLINE)
          std::cout << "Next CW: " << nextCWIter->coastline << " " << nextCWIter->prevWayPointIndex << " " << nextCWIter->distanceSquare << std::endl;
#endif

          IntersectionPtr outgoing=nextCWIter;

#if defined(DEBUG_COASTLINE)
          std::cout << "Outgoing: " << outgoing->coastline << " " << outgoing->prevWayPointIndex << " " << outgoing->distanceSquare << std::endl;
#endif

          if (outgoing->direction!=-1) {
#if defined(DEBUG_COASTLINE)
            std::cerr << "We expect an outgoing intersection" << std::endl;
#endif

            error=true;
            continue;
          }

          intersectionsOuter.remove(outgoing);

          WalkBorderCW(groundTile,
                       level,
                       latMin,
                       lonMin,
                       incoming,
                       outgoing,
                       borderCoords);

          incoming=GetPreviousIntersection(intersectionsPathOrder[outgoing->coastline],
                                           outgoing);

          if (incoming==NULL) {
#if defined(DEBUG_COASTLINE)
            std::cerr << "Polygon is not closed, but there are no intersections left" << std::endl;
#endif

            error=true;
            continue;
          }

#if defined(DEBUG_COASTLINE)
          std::cout << "Incoming: " << incoming->coastline << " " << incoming->prevWayPointIndex << " " << incoming->distanceSquare << std::endl;
#endif

          if (incoming->direction!=1) {
#if defined(DEBUG_COASTLINE)
            std::cerr << "We expect an incoming intersection" << std::endl;
#endif

            error=true;
            continue;
          }

          WalkPathBack(groundTile,
                       level,
                       latMin,
                       lonMin,
                       outgoing,
                       incoming,
                       data.points[outgoing->coastline],
                       data.isArea[outgoing->coastline]);

          nextCWIter=GetNextCW(intersectionsCW,
                               incoming);
        }

        if (error) {
          continue;
        }

        if (!groundTile.coords.empty()) {
#if defined(DEBUG_COASTLINE)
        std::cout << "Polygon closed!" << std::endl;
#endif

          WalkBorderCW(groundTile,
                       level,
                       latMin,
                       lonMin,
                       incoming,
                       initialOutgoing,
                       borderCoords);

          cellGroundTileMap[cell->first].push_back(groundTile);
        }
      }
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
    FileScanner        scanner;

    uint32_t           minLonDat;
    uint32_t           minLatDat;
    uint32_t           maxLonDat;
    uint32_t           maxLatDat;

    std::vector<Level> levels;

    // Calculate size of tile cells for the maximum zoom level
    double             cellWidth;
    double             cellHeight;

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

    //
    // Initialize levels
    //

    levels.resize(parameter.GetWaterIndexMaxMag()-parameter.GetWaterIndexMinMag()+1);

    cellWidth=360.0;
    cellHeight=180.0;

    for (size_t level=0; level<=parameter.GetWaterIndexMaxMag(); level++) {
      if (level>=parameter.GetWaterIndexMinMag() &&
          level<=parameter.GetWaterIndexMaxMag()) {
        levels[level-parameter.GetWaterIndexMinMag()].SetBox(minLatDat,maxLatDat,
                                                             minLonDat,maxLonDat,
                                                             cellWidth,cellHeight);
      }

      cellWidth=cellWidth/2;
      cellHeight=cellHeight/2;
    }

    //
    // Load and merge coastlines
    //

    LoadCoastlines(parameter,
                   progress,
                   typeConfig);

    MergeCoastlines(progress);


    progress.SetAction("Writing 'water.idx'");

    FileWriter writer;

    if (!writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                     "water.idx"))) {
      progress.Error("Error while opening 'water.idx' for writing");
      return false;
    }

    DumpIndexHeader(parameter,
                    writer,
                    levels);

    for (size_t level=0; level<levels.size(); level++) {
      FileOffset         indexOffset;
      MercatorProjection projection;
      CoastlineData      data;

      projection.Set(0,0,pow(2.0,(double)(level+parameter.GetWaterIndexMinMag())),640,480);

      progress.SetAction("Building tiles for level "+NumberToString(level+parameter.GetWaterIndexMinMag()));

      writer.GetPos(indexOffset);
      writer.SetPos(levels[level].indexEntryOffset);
      writer.WriteFileOffset(indexOffset);
      writer.SetPos(indexOffset);

      MarkCoastlineCells(progress,
                         levels[level]);

      GetCoastlineData(parameter,
                       progress,
                       projection,
                       levels[level],
                       coastlines,
                       data);

      std::map<Coord,std::list<GroundTile> > cellGroundTileMap;

      HandleAreaCoastlinesCompletelyInACell(parameter,
                                            progress,
                                            projection,
                                            levels[level],
                                            coastlines,
                                            cellGroundTileMap);

      HandleCoastlinesPartiallyInACell(parameter,
                                       progress,
                                       projection,
                                       levels[level],
                                       cellGroundTileMap,
                                       data);

      CalculateLandCells(progress,
                         levels[level],
                         data,
                         cellGroundTileMap);

      if (parameter.GetAssumeLand()) {
        AssumeLand(parameter,
                   progress,
                   typeConfig,
                   levels[level]);
      }

      FillWater(progress,
                levels[level],20);

      FillLand(progress,
               levels[level]);

      for (size_t y=0; y<levels[level].cellYCount; y++) {
        for (size_t x=0; x<levels[level].cellXCount; x++) {
          State state=levels[level].GetState(x,y);

          writer.WriteFileOffset((FileOffset)state);
        }
      }

      for (std::map<Coord,std::list<GroundTile> >::const_iterator coord=cellGroundTileMap.begin();
          coord!=cellGroundTileMap.end();
          ++coord) {
        FileOffset startPos;

        writer.GetPos(startPos);

        writer.WriteNumber((uint32_t)coord->second.size());

        for (std::list<GroundTile>::const_iterator tile=coord->second.begin();
             tile!=coord->second.end();
             ++tile) {
          writer.Write((uint8_t)tile->type);

          writer.WriteNumber((uint32_t)tile->coords.size());

          for (size_t c=0; c<tile->coords.size(); c++) {
            if (tile->coords[c].coast) {
              uint16_t x=tile->coords[c].x | uint16_t(1 << 15);

              writer.Write(x);
            }
            else {
              writer.Write(tile->coords[c].x);
            }
            writer.Write(tile->coords[c].y);
          }
        }

        FileOffset endPos;
        uint32_t   cellId=coord->first.y*levels[level].cellXCount+coord->first.x;
        uint32_t   index=cellId*8;

        writer.GetPos(endPos);

        writer.SetPos(indexOffset+index);
        writer.WriteFileOffset(startPos);

        writer.SetPos(endPos);
      }
    }

    coastlines.clear();

    if (writer.HasError() || !writer.Close()) {
      progress.Error("Error while closing 'water.idx'");
      return false;
    }

    return true;
  }
}
