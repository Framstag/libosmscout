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

#include <vector>
#include <iostream>
#include <iomanip>

#include <osmscout/Way.h>

#include <osmscout/DataFile.h>
#include <osmscout/BoundingBoxDataFile.h>
#include <osmscout/CoordDataFile.h>
#include <osmscout/TypeFeatures.h>
#include <osmscout/WaterIndex.h>

#include <osmscout/system/Assert.h>
#include <osmscout/system/Math.h>

#include <osmscout/util/File.h>
#include <osmscout/util/FileScanner.h>
#include <osmscout/util/String.h>

#include <osmscout/import/Preprocess.h>
#include <osmscout/import/RawCoastline.h>
#include <osmscout/import/RawNode.h>
#include <osmscout/WayDataFile.h>

//#define DEBUG_COASTLINE
//#define DEBUG_TILING

namespace osmscout {

  std::string WaterIndexGenerator::StateToString(State state) const
  {
    switch (state) {
    case unknown:
      return "unknown";
    case land:
      return "land";
    case water:
      return "water";
    case coast:
      return "coast";
    default:
      return "???";
    }
  }

  GroundTile::Coord WaterIndexGenerator::Transform(const GeoCoord& point,
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
  void WaterIndexGenerator::Level::SetBox(const GeoCoord& minCoord,
                                          const GeoCoord& maxCoord,
                                          double cellWidth, double cellHeight)
  {
    indexEntryOffset=0;

    this->cellWidth=cellWidth;
    this->cellHeight=cellHeight;

    hasCellData=false;
    defaultCellData=State::unknown;

    indexDataOffset=0;

    cellXStart=(uint32_t)floor((minCoord.GetLon()+180.0)/cellWidth);
    cellXEnd=(uint32_t)floor((maxCoord.GetLon()+180.0)/cellWidth);
    cellYStart=(uint32_t)floor((minCoord.GetLat()+90.0)/cellHeight);
    cellYEnd=(uint32_t)floor((maxCoord.GetLat()+90.0)/cellHeight);

    cellXCount=cellXEnd-cellXStart+1;
    cellYCount=cellYEnd-cellYStart+1;

    uint32_t size=(cellXCount*cellYCount)/4;

    if ((cellXCount*cellYCount)%4>0) {
      size++;
    }

    area.resize(size,0x00);
  }

  bool WaterIndexGenerator::Level::IsInAbsolute(uint32_t x, uint32_t y) const
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

    //assert(index<area.size());

    return (State)((area[index] >> offset) & 3);
  }

  void WaterIndexGenerator::Level::SetState(uint32_t x, uint32_t y, State state)
  {
    uint32_t cellId=y*cellXCount+x;
    uint32_t index=cellId/4;
    uint32_t offset=2*(cellId%4);

    //assert(index<area.size());

    area[index]=(area[index] & ~(3 << offset));
    area[index]=(area[index] | (state << offset));
  }

  void WaterIndexGenerator::Level::SetStateAbsolute(uint32_t x, uint32_t y, State state)
  {
    SetState(x-cellXStart,y-cellYStart,state);
  }

  bool WaterIndexGenerator::LoadCoastlines(const ImportParameter& parameter,
                                           Progress& progress,
                                           std::list<CoastRef>& coastlines)
  {
    // We must have coastline type defined
    FileScanner                scanner;
    std::list<RawCoastlineRef> rawCoastlines;

    coastlines.clear();

    progress.SetAction("Scanning for coastlines");

    try {
      uint32_t coastlineCount=0;
      size_t   wayCoastCount=0;
      size_t   areaCoastCount=0;

      scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                   Preprocess::RAWCOASTLINE_DAT),
                   FileScanner::Sequential,
                   true);

      scanner.Read(coastlineCount);

      progress.Info(NumberToString(coastlineCount)+" coastlines");

      for (uint32_t c=1; c<=coastlineCount; c++) {
        progress.SetProgress(c,coastlineCount);

        RawCoastlineRef coastline=std::make_shared<RawCoastline>();

        coastline->Read(scanner);

        rawCoastlines.push_back(coastline);
      }

      progress.SetAction("Resolving nodes of coastline");

      CoordDataFile coordDataFile;

      if (!coordDataFile.Open(parameter.GetDestinationDirectory(),
                              parameter.GetCoordDataMemoryMaped())) {
        progress.Error("Cannot open coord file!");
        return false;
      }

      std::set<OSMId> nodeIds;

      for (const auto& coastline : rawCoastlines) {
        for (size_t n=0; n<coastline->GetNodeCount(); n++) {
          nodeIds.insert(coastline->GetNodeId(n));
        }
      }

      CoordDataFile::ResultMap coordsMap;

      if (!coordDataFile.Get(nodeIds,
                             coordsMap)) {
        progress.Error("Cannot read nodes!");
        return false;
      }

      nodeIds.clear();

      progress.SetAction("Enriching coastline with node data");

      while (!rawCoastlines.empty()) {
        RawCoastlineRef coastline=rawCoastlines.front();
        bool            processingError=false;

        rawCoastlines.pop_front();

        CoastRef coast=std::make_shared<Coast>();

        coast->id=coastline->GetId();
        coast->isArea=coastline->IsArea();

        coast->coast.resize(coastline->GetNodeCount());

        for (size_t n=0; n<coastline->GetNodeCount(); n++) {
          CoordDataFile::ResultMap::const_iterator coord=coordsMap.find(coastline->GetNodeId(n));

          if (coord==coordsMap.end()) {
            processingError=true;

            progress.Error("Cannot resolve node with id "+
                           NumberToString(coastline->GetNodeId(n))+
                           " for coastline "+
                           NumberToString(coastline->GetId()));

            break;
          }

          if (n==0) {
            coast->frontNodeId=coord->second.GetOSMScoutId();
          }

          if (n==coastline->GetNodeCount()-1) {
            coast->backNodeId=coord->second.GetOSMScoutId();
          }

          coast->coast[n].Set(coord->second.GetSerial(),
                              coord->second.GetCoord());
        }

        if (!processingError) {
          if (coast->isArea) {
            areaCoastCount++;
          }
          else {
            wayCoastCount++;
          }

          coastlines.push_back(coast);
        }
      }

      progress.Info(NumberToString(wayCoastCount)+" way coastline(s), "+NumberToString(areaCoastCount)+" area coastline(s)");

      scanner.Close();
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());
      return false;
    }

    return true;
  }

  void WaterIndexGenerator::MergeCoastlines(Progress& progress,
                                            std::list<CoastRef>& coastlines)
  {
    progress.SetAction("Merging coastlines");

    std::map<Id,CoastRef> coastStartMap;
    std::list<CoastRef>   mergedCoastlines;
    std::set<Id>          blacklist;
    size_t                wayCoastCount=0;
    size_t                areaCoastCount=0;

    std::list<CoastRef>::iterator c=coastlines.begin();
    while( c!=coastlines.end()) {
      CoastRef coast=*c;

      if (coast->isArea) {
        areaCoastCount++;
        mergedCoastlines.push_back(coast);

        c=coastlines.erase(c);
      }
      else {
        coastStartMap.insert(std::make_pair(coast->frontNodeId,coast));

        c++;
      }
    }

    bool merged=true;

    while (merged) {
      merged=false;

      for (const auto& coast : coastlines) {
        if (blacklist.find(coast->id)!=blacklist.end()) {
          continue;
        }

        std::map<Id,CoastRef>::iterator other=coastStartMap.find(coast->backNodeId);

        if (other!=coastStartMap.end() &&
            blacklist.find(other->second->id)==blacklist.end() &&
            coast->id!=other->second->id) {
          for (size_t i=1; i<other->second->coast.size(); i++) {
            coast->coast.push_back(other->second->coast[i]);
          }

          coast->backNodeId=coast->coast.back().GetId();

          // Immediately reduce memory
          other->second->coast.clear();

          blacklist.insert(other->second->id);
          coastStartMap.erase(other);

          merged=true;
        }
      }
    }

    // Gather merged coastlines
    for (const auto& coastline : coastlines) {
      if (blacklist.find(coastline->id)!=blacklist.end()) {
        continue;
      }

      if (coastline->frontNodeId==coastline->backNodeId) {
        coastline->isArea=true;
        coastline->coast.pop_back();

        areaCoastCount++;
      }
      else {
        wayCoastCount++;
      }

      if (coastline->coast.size()<=2) {
        progress.Warning("Dropping to short coastline with id "+NumberToString(coastline->id));
        continue;
      }

      mergedCoastlines.push_back(coastline);
    }

    progress.Info(NumberToString(wayCoastCount)+" way coastline(s), "+NumberToString(areaCoastCount)+" area coastline(s)");

    coastlines=mergedCoastlines;
  }


  /**
   * Markes a cell as "coast", if one of the coastlines intersects with it.
   *
   */
  void WaterIndexGenerator::MarkCoastlineCells(Progress& progress,
                                               Level& level,
                                               const Data& data)
  {
    progress.Info("Marking cells containing coastlines");

    for (const auto& coastline : data.coastlines) {
      // Marks cells on the path as coast
      std::set<Pixel> coords;

      GetCells(level,
               coastline.points,
               coords);

      for (const auto& coord : coords) {
        if (level.IsInAbsolute(coord.x,coord.y)) {
          if (level.GetState(coord.x-level.cellXStart,coord.y-level.cellYStart)==unknown) {
#if defined(DEBUG_TILING)
            std::cout << "Coastline: " << coord.x-level.cellXStart << "," << coord.y-level.cellYStart << " " << coastline.id << std::endl;
#endif
            level.SetStateAbsolute(coord.x,coord.y,coast);
          }
        }
      }

    }
  }

  /**
   * Calculate the cell type for cells directly around coast cells
   * @param progress
   * @param level
   * @param cellGroundTileMap
   */
  void WaterIndexGenerator::CalculateCoastEnvironment(Progress& progress,
                                                      Level& level,
                                                      const std::map<Pixel,std::list<GroundTile> >& cellGroundTileMap)
  {
    progress.Info("Calculate coast cell environment");

    for (const auto& tileEntry : cellGroundTileMap) {
      State  state[4];      // type of the neighbouring cells: top, right, bottom, left
      size_t coordCount[4]; // number of coords on the given line (top, right, bottom, left)

      state[0]=unknown;
      state[1]=unknown;
      state[2]=unknown;
      state[3]=unknown;

      coordCount[0]=0;
      coordCount[1]=0;
      coordCount[2]=0;
      coordCount[3]=0;

      // Preset top
      if (tileEntry.first.y<level.cellYCount-1) {
        state[0]=level.GetState(tileEntry.first.x,tileEntry.first.y+1);
      }

      // Preset right
      if (tileEntry.first.x<level.cellXCount-1) {
        state[1]=level.GetState(tileEntry.first.x+1,tileEntry.first.y);
      }

      // Preset bottom
      if (tileEntry.first.y>0) {
        state[2]=level.GetState(tileEntry.first.x,tileEntry.first.y-1);
      }

      // Preset left
      if (tileEntry.first.x>0) {
        state[3]=level.GetState(tileEntry.first.x-1,tileEntry.first.y);
      }

      // Identify 'land' cells in relation to 'coast' cells
      for (const auto& tile : tileEntry.second) {
        for (size_t c=0; c<tile.coords.size()-1;c++) {

          //
          // Count number of coords *on* the border
          //

          // top
          if (tile.coords[c].y==GroundTile::Coord::CELL_MAX) {
            coordCount[0]++;
          }
          // right
          else if (tile.coords[c].x==GroundTile::Coord::CELL_MAX) {
            coordCount[1]++;
          }
          // bottom
          else if (tile.coords[c].y==0) {
            coordCount[2]++;
          }
          // left
          else if (tile.coords[c].x==0) {
            coordCount[3]++;
          }

          //
          // Detect fills over a complete border
          //

          // line at the top from left to right => land is above current cell
          if (tile.coords[c].x==0 &&
              tile.coords[c].y==GroundTile::Coord::CELL_MAX &&
              tile.coords[c+1].x==GroundTile::Coord::CELL_MAX &&
              tile.coords[c+1].y==GroundTile::Coord::CELL_MAX) {
            if (state[0]==unknown) {
              state[0]=land;
            }
          }
          // line at the top from right to left => water is above current cell
          else if (tile.coords[c].x==GroundTile::Coord::CELL_MAX &&
                   tile.coords[c].y==GroundTile::Coord::CELL_MAX &&
                   tile.coords[c+1].x==0 &&
                   tile.coords[c+1].y==GroundTile::Coord::CELL_MAX) {
            if (state[0]==unknown) {
              state[0]=water;
            }
          }

          // Line from right top to bottom => land is right of current cell
          if (tile.coords[c].x==GroundTile::Coord::CELL_MAX &&
              tile.coords[c].y==GroundTile::Coord::CELL_MAX &&
              tile.coords[c+1].x==GroundTile::Coord::CELL_MAX &&
              tile.coords[c+1].y==0) {
            if (state[1]==unknown) {
              state[1]=land;
            }
          }
            // Line from right bottom to top => water is right of current cell
          else if (tile.coords[c].x==GroundTile::Coord::CELL_MAX &&
                   tile.coords[c].y==0 &&
                   tile.coords[c+1].x==GroundTile::Coord::CELL_MAX &&
                   tile.coords[c+1].y==GroundTile::Coord::CELL_MAX) {
            if (state[1]==unknown) {
              state[1]=water;
            }
          }

          // Line a the bottom from right to left => land is below current cell
          if (tile.coords[c].x==GroundTile::Coord::CELL_MAX &&
              tile.coords[c].y==0 &&
              tile.coords[c+1].x==0 &&
              tile.coords[c+1].y==0) {
            if (state[2]==unknown) {
              state[2]=land;
            }
          }
          // Line a the bottom from left to right => water is below current cell
          else if (tile.coords[c].x==0 &&
                   tile.coords[c].y==0 &&
                   tile.coords[c+1].x==GroundTile::Coord::CELL_MAX &&
                   tile.coords[c+1].y==0) {
            if (state[2]==unknown) {
              state[2]=water;
            }
          }

          // Line left from bottom to top => land is left of current cell
          if (tile.coords[c].x==0 &&
              tile.coords[c].y==0 &&
              tile.coords[c+1].x==0 &&
              tile.coords[c+1].y==GroundTile::Coord::CELL_MAX) {
            if (state[3]==unknown) {
              state[3]=land;
            }
          }
          // Line left from top to bottom => Water is left of current cell
          else if (tile.coords[c].x==0 &&
                   tile.coords[c].y==GroundTile::Coord::CELL_MAX &&
                   tile.coords[c+1].x==0 &&
                   tile.coords[c+1].y==0) {
            if (state[3]==unknown) {
              state[3]=water;
            }
          }
        }
      }

      // top
      if (tileEntry.first.y<level.cellYCount-1 &&
        level.GetState(tileEntry.first.x,tileEntry.first.y+1)==unknown) {
        if (state[0]!=unknown) {
#if defined(DEBUG_TILING)
          std::cout << "Assume " << StateToString(state[0]) << " above coast: " << tileEntry.first.x << "," << tileEntry.first.y+1 << std::endl;
#endif
          level.SetState(tileEntry.first.x,tileEntry.first.y+1,state[0]);
        }
        else if (coordCount[0]<=1) {
#if defined(DEBUG_TILING)
          std::cout << "Assume water* above coast: " << tileEntry.first.x << "," << tileEntry.first.y+1 << std::endl;
#endif
          level.SetState(tileEntry.first.x,tileEntry.first.y+1,water);
        }
      }

      if (tileEntry.first.x<level.cellXCount-1 &&
        level.GetState(tileEntry.first.x+1,tileEntry.first.y)==unknown) {
        if (state[1]!=unknown) {
#if defined(DEBUG_TILING)
          std::cout << "Assume " << StateToString(state[1]) << " right of coast: " << tileEntry.first.x+1 << "," << tileEntry.first.y << std::endl;
#endif
          level.SetState(tileEntry.first.x+1,tileEntry.first.y,state[1]);
        }
        else if (coordCount[1]<=1) {
#if defined(DEBUG_TILING)
          std::cout << "Assume water* right of coast: " << tileEntry.first.x+1 << "," << tileEntry.first.y << std::endl;
#endif
          level.SetState(tileEntry.first.x+1,tileEntry.first.y,water);
        }
      }

      if (tileEntry.first.y>0 &&
        level.GetState(tileEntry.first.x,tileEntry.first.y-1)==unknown) {
        if (state[2]!=unknown) {
#if defined(DEBUG_TILING)
          std::cout << "Assume " << StateToString(state[2]) << " below coast: " << tileEntry.first.x << "," << tileEntry.first.y-1 << std::endl;
#endif
          level.SetState(tileEntry.first.x,tileEntry.first.y-1,state[2]);
        }
        else if (coordCount[2]<=1) {
#if defined(DEBUG_TILING)
          std::cout << "Assume water* below coast: " << tileEntry.first.x << "," << tileEntry.first.y-1 << std::endl;
#endif
          level.SetState(tileEntry.first.x,tileEntry.first.y-1,water);
        }
      }

      if (tileEntry.first.x>0 &&
        level.GetState(tileEntry.first.x-1,tileEntry.first.y)==unknown) {
        if (state[3]!=unknown) {
#if defined(DEBUG_TILING)
          std::cout << "Assume " << StateToString(state[3]) << " left of coast: " << tileEntry.first.x-1 << "," << tileEntry.first.y << std::endl;
#endif
          level.SetState(tileEntry.first.x-1,tileEntry.first.y,state[3]);
        }
        else if (coordCount[3]<=1) {
#if defined(DEBUG_TILING)
          std::cout << "Assume water* left of coast: " << tileEntry.first.x-1 << "," << tileEntry.first.y << std::endl;
#endif
          level.SetState(tileEntry.first.x-1,tileEntry.first.y,water);
        }
      }
    }
  }

  /**
   * Assume cell type 'land' for cells that intersect with 'land' object types
   *
   * Every cell that is unknown but contains a way (that is marked
   * as "to be ignored"), must be land.
   */
  bool WaterIndexGenerator::AssumeLand(const ImportParameter& parameter,
                                       Progress& progress,
                                       const TypeConfig& typeConfig,
                                       Level& level)
  {
    progress.Info("Assume land");

    BridgeFeatureReader bridgeFeatureRader(typeConfig);
    TunnelFeatureReader tunnelFeatureRader(typeConfig);
    FileScanner         scanner;
    uint32_t            wayCount=0;

    // We do not yet know if we handle borders as ways or areas

    try {
      scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                   WayDataFile::WAYS_DAT),
                   FileScanner::Sequential,
                   parameter.GetWayDataMemoryMaped());

      scanner.Read(wayCount);

      for (uint32_t w=1; w<=wayCount; w++) {
        progress.SetProgress(w,wayCount);

        Way way;

        way.Read(typeConfig,
                 scanner);

        if (way.GetType()!=typeConfig.typeInfoIgnore &&
            !way.GetType()->GetIgnoreSeaLand() &&
            !tunnelFeatureRader.IsSet(way.GetFeatureValueBuffer()) &&
            !bridgeFeatureRader.IsSet(way.GetFeatureValueBuffer()) &&
            way.nodes.size()>=2) {
          std::set<Pixel> coords;

          GetCells(level,way.nodes,coords);

          for (const auto& coord : coords) {
            if (level.IsInAbsolute(coord.x,coord.y)) {
              if (level.GetState(coord.x-level.cellXStart,coord.y-level.cellYStart)==unknown) {
#if defined(DEBUG_TILING)
                std::cout << "Assume land: " << coord.x-level.cellXStart << "," << coord.y-level.cellYStart << " Way " << way.GetFileOffset() << " " << way.GetType()->GetName() << " is defining area as land" << std::endl;
#endif
                level.SetStateAbsolute(coord.x,coord.y,land);
              }
            }
          }
        }
      }

      scanner.Close();
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());
      return false;
    }

    return true;
  }

  /**
   * Marks all still 'unknown' cells neighbouring 'water' cells as 'water', too
   *
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

      for (uint32_t y=0; y<level.cellYCount; y++) {
        for (uint32_t x=0; x<level.cellXCount; x++) {
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
   * Marks all still 'unknown' cells between 'coast' or 'land' and 'land' cells as 'land', too
   *
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
      for (uint32_t y=0; y<level.cellYCount; y++) {
        uint32_t x=0;
        uint32_t start=0;
        uint32_t end=0;
        uint32_t state=0;

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
                  for (uint32_t i=start; i<=end; i++) {
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
      for (uint32_t x=0; x<level.cellXCount; x++) {
        uint32_t y=0;
        uint32_t start=0;
        uint32_t end=0;
        uint32_t state=0;

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
                  for (uint32_t i=start; i<=end; i++) {
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

    for (auto& level : levels) {
      level.indexEntryOffset=writer.GetPos();
      writer.Write(level.hasCellData);
      writer.Write(level.dataOffsetBytes);
      writer.Write((uint8_t)level.defaultCellData);
      writer.WriteFileOffset(level.indexDataOffset);
      writer.WriteNumber(level.cellXStart);
      writer.WriteNumber(level.cellXEnd);
      writer.WriteNumber(level.cellYStart);
      writer.WriteNumber(level.cellYEnd);
    }
  }

  /**
   * Fills coords information for cells that completely contain a coastline
   *
   * @param progress
   * @param level
   * @param data
   * @param cellGroundTileMap
   */
  void WaterIndexGenerator::HandleAreaCoastlinesCompletelyInACell(Progress& progress,
                                                                  const Level& level,
                                                                  Data& data,
                                                                  std::map<Pixel,std::list<GroundTile> >& cellGroundTileMap)
  {
    progress.Info("Handle area coastline completely in a cell");

    size_t currentCoastline=1;
    for (const auto& coastline : data.coastlines) {
      progress.SetProgress(currentCoastline,data.coastlines.size());

      currentCoastline++;

      if (!(coastline.isArea &&
            coastline.isCompletelyInCell)) {
        continue;
      }

      if (!level.IsInAbsolute(coastline.cell.x,coastline.cell.y)) {
        continue;
      }

      Pixel      coord(coastline.cell.x-level.cellXStart,coastline.cell.y-level.cellYStart);
      GroundTile groundTile(GroundTile::land);

      double cellMinLat=level.cellHeight*coastline.cell.y-90.0;
      double cellMinLon=level.cellWidth*coastline.cell.x-180.0;

      groundTile.coords.reserve(coastline.points.size());

      for (size_t p=0; p<coastline.points.size(); p++) {
        groundTile.coords.push_back(Transform(coastline.points[p],level,cellMinLat,cellMinLon,true));
      }

      if (!groundTile.coords.empty()) {
        groundTile.coords.back().coast=false;

#if defined(DEBUG_TILING)
        std::cout << "Coastline in cell: " << coord.x << "," << coord.y << std::endl;
#endif

        cellGroundTileMap[coord].push_back(groundTile);
      }
    }
  }

  static bool IsLeftOnSameBorder(size_t border, const GeoCoord& a,const GeoCoord& b)
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
                                     const GeoCoord& a,
                                     const GeoCoord& b,
                                     std::set<Pixel>& cellIntersections)
  {
    uint32_t cx1=(uint32_t)((a.GetLon()+180.0)/level.cellWidth);
    uint32_t cy1=(uint32_t)((a.GetLat()+90.0)/level.cellHeight);

    uint32_t cx2=(uint32_t)((b.GetLon()+180.0)/level.cellWidth);
    uint32_t cy2=(uint32_t)((b.GetLat()+90.0)/level.cellHeight);

    cellIntersections.insert(Pixel(cx1,cy1));

    if (cx1!=cx2 || cy1!=cy2) {
      for (uint32_t x=std::min(cx1,cx2); x<=std::max(cx1,cx2); x++) {
        for (uint32_t y=std::min(cy1,cy2); y<=std::max(cy1,cy2); y++) {

          Pixel    coord(x,y);
          GeoCoord borderPoints[5];
          double   lonMin,lonMax,latMin,latMax;

          lonMin=x*level.cellWidth-180.0;
          lonMax=lonMin+level.cellWidth;
          latMin=y*level.cellHeight-90.0;
          latMax=latMin+level.cellHeight;

          borderPoints[0].Set(latMax,lonMin); // top left
          borderPoints[1].Set(latMax,lonMax); // top right
          borderPoints[2].Set(latMin,lonMax); // bottom right
          borderPoints[3].Set(latMin,lonMin); // bottom left
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
                                     const std::vector<GeoCoord>& points,
                                     std::set<Pixel>& cellIntersections)
  {
    for (size_t p=0; p<points.size()-1; p++) {
      GetCells(level,points[p],points[p+1],cellIntersections);
    }
  }

  void WaterIndexGenerator::GetCells(const Level& level,
                                     const std::vector<Point>& points,
                                     std::set<Pixel>& cellIntersections)
  {
    for (size_t p=0; p<points.size()-1; p++) {
      GetCells(level,points[p].GetCoord(),points[p+1].GetCoord(),cellIntersections);
    }
  }

  void WaterIndexGenerator::GetCellIntersections(const Level& level,
                                                 const std::vector<GeoCoord>& points,
                                                 size_t coastline,
                                                 std::map<Pixel,std::list<Intersection> >& cellIntersections)
  {
    for (size_t p=0; p<points.size()-1; p++) {
      // Cell coordinates of the current and the next point
      uint32_t cx1=(uint32_t)((points[p].GetLon()+180.0)/level.cellWidth);
      uint32_t cy1=(uint32_t)((points[p].GetLat()+90.0)/level.cellHeight);

      uint32_t cx2=(uint32_t)((points[p+1].GetLon()+180.0)/level.cellWidth);
      uint32_t cy2=(uint32_t)((points[p+1].GetLat()+90.0)/level.cellHeight);

      if (cx1!=cx2 || cy1!=cy2) {
        for (uint32_t x=std::min(cx1,cx2); x<=std::max(cx1,cx2); x++) {
          for (uint32_t y=std::min(cy1,cy2); y<=std::max(cy1,cy2); y++) {

            if (!level.IsInAbsolute(x,y)) {
              continue;
            }

            Pixel    coord(x-level.cellXStart,y-level.cellYStart);
            GeoCoord borderPoints[5];
            double   lonMin,lonMax,latMin,latMax;

            lonMin=x*level.cellWidth-180.0;
            lonMax=(x+1)*level.cellWidth-180.0;
            latMin=y*level.cellHeight-90.0;
            latMax=(y+1)*level.cellHeight-90.0;

            borderPoints[0].Set(latMax,lonMin); // top left
            borderPoints[1].Set(latMax,lonMax); // top right
            borderPoints[2].Set(latMin,lonMax); // bottom right
            borderPoints[3].Set(latMin,lonMin); // bottom left
            borderPoints[4]=borderPoints[0];    // To avoid modula 4 on all indexes

            size_t       intersectionCount=0;
            Intersection firstIntersection;
            Intersection secondIntersection;
            size_t       corner=0;

            // Check intersection with one of the borders
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

            // Check if there is another intersection with one of the following borders
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

            // After above steps we can have 0..2 intersections

            if (x==cx1 &&
                y==cy1) {
              assert(intersectionCount==1 ||
                     intersectionCount==2);

              if (intersectionCount==1) {
                // The segment always leaves the origin cell
                firstIntersection.direction=-1;
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
            else if (x==cx2 &&
                     y==cy2) {
              assert(intersectionCount==1 ||
                     intersectionCount==2);

              if (intersectionCount==1) {
                // The segment always enters the target cell
                firstIntersection.direction=1;
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
            else {
              assert(intersectionCount==0 ||
                     intersectionCount==1 ||
                     intersectionCount==2);

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
                                             Data& data)
  {
    progress.Info("Calculate coastline data");

    data.coastlines.resize(coastlines.size());

    size_t curCoast=0;
    for (const auto& coast : coastlines) {
      progress.SetProgress(curCoast,coastlines.size());

      TransPolygon polygon;

      if (coast->isArea) {
        polygon.TransformArea(projection,
                              parameter.GetOptimizationWayMethod(),
                              coast->coast,
                              1.0);
      }
      else {
        polygon.TransformWay(projection,
                             parameter.GetOptimizationWayMethod(),
                             coast->coast,
                             1.0);
      }

      if (coast->isArea) {
        double minX=polygon.points[polygon.GetStart()].x;
        double minY=polygon.points[polygon.GetStart()].y;
        double maxX=minX;
        double maxY=minY;

        for (size_t p=polygon.GetStart()+1; p<=polygon.GetEnd(); p++) {
          if (polygon.points[p].draw) {
            minX=std::min(minX,polygon.points[p].x);
            maxX=std::max(maxX,polygon.points[p].x);
            minY=std::min(minY,polygon.points[p].y);
            maxY=std::max(maxY,polygon.points[p].y);
          }
        }

        double pixelWidth=maxX-minX;
        double pixelHeight=maxY-minY;

        // Artificial values but for drawing an area a box of at least 4x4 might make sense
        if (pixelWidth<=4.0 ||
            pixelHeight<=4.0) {
          continue;
        }
      }

      data.coastlines[curCoast].id=coast->id;
      data.coastlines[curCoast].isArea=coast->isArea;

      data.coastlines[curCoast].points.reserve(polygon.GetLength());
      for (size_t p=polygon.GetStart(); p<=polygon.GetEnd(); p++) {
        if (polygon.points[p].draw) {
          data.coastlines[curCoast].points.push_back(GeoCoord(coast->coast[p].GetLat(),
                                                              coast->coast[p].GetLon()));
        }
      }

      // Currently transformation optimization code sometimes does not correctly handle the closing point for areas
      if (coast->isArea) {
        if (data.coastlines[curCoast].points.front()!=data.coastlines[curCoast].points.back()) {
          data.coastlines[curCoast].points.push_back(data.coastlines[curCoast].points.front());
        }
      }

      GeoBox  boundingBox;

      GetBoundingBox(coast->coast,
                     boundingBox);

      uint32_t cxMin,cxMax,cyMin,cyMax;

      cxMin=(uint32_t)floor((boundingBox.GetMinLon()+180.0)/level.cellWidth);
      cxMax=(uint32_t)floor((boundingBox.GetMaxLon()+180.0)/level.cellWidth);
      cyMin=(uint32_t)floor((boundingBox.GetMinLat()+90.0)/level.cellHeight);
      cyMax=(uint32_t)floor((boundingBox.GetMaxLat()+90.0)/level.cellHeight);

      if (cxMin==cxMax &&
          cyMin==cyMax) {
        data.coastlines[curCoast].cell.x=cxMin;
        data.coastlines[curCoast].cell.y=cyMin;
        data.coastlines[curCoast].isCompletelyInCell=true;
      }
      else {
        data.coastlines[curCoast].isCompletelyInCell=false;
      }

      if (!data.coastlines[curCoast].isCompletelyInCell) {
        // Calculate all intersections for all path steps for all cells covered
        GetCellIntersections(level,
                             data.coastlines[curCoast].points,
                             curCoast,
                             data.coastlines[curCoast].cellIntersections);

        for (const auto& intersectionEntry : data.coastlines[curCoast].cellIntersections) {
          data.cellCoastlines[intersectionEntry.first].push_back(curCoast);
        }
      }

      curCoast++;
    }

    // Fix the vector size to remove unused slots (because of filtering by min area size)
    data.coastlines.resize(curCoast);
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
                                         const std::vector<GeoCoord>& points,
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

      for (size_t idx=outgoing->prevWayPointIndex;
          idx>=targetIdx;
          idx--) {
        groundTile.coords.push_back(Transform(points[idx],level,cellMinLat,cellMinLon,true));
      }

      groundTile.coords.push_back(Transform(incoming->point,level,cellMinLat,cellMinLon,false));
    }
  }

  /**
   * Fills coords information for cells that intersect a coastline
   */
  void WaterIndexGenerator::HandleCoastlinesPartiallyInACell(Progress& progress,
                                                             const std::list<CoastRef>& coastlines,
                                                             const Level& level,
                                                             std::map<Pixel,std::list<GroundTile> >& cellGroundTileMap,
                                                             Data& data)
  {
    progress.Info("Handle coastlines partially in a cell");

    // For every cell with intersections
    size_t currentCell=0;
    for (const auto& cellEntry : data.cellCoastlines) {
      progress.SetProgress(currentCell,data.cellCoastlines.size());

      currentCell++;

      std::list<IntersectionPtr>               intersectionsCW;        // Intersections in clock wise order over all coastlines
      std::list<IntersectionPtr>               intersectionsOuter;     // Only outgoing intersections
      std::vector<std::list<IntersectionPtr> > intersectionsPathOrder; // Intersections in path order for each coastline

      intersectionsPathOrder.resize(coastlines.size());

      // For every coastline by index intersecting the current cell
      for (const auto& currentCoastline : cellEntry.second) {
        std::map<Pixel,std::list<Intersection> >::iterator cellData=data.coastlines[currentCoastline].cellIntersections.find(cellEntry.first);

        assert(cellData!=data.coastlines[currentCoastline].cellIntersections.end());

        // Build list of intersections in path order and list of intersections in clock wise order
        for (std::list<Intersection>::iterator inter=cellData->second.begin();
            inter!=cellData->second.end();
            ++inter) {
          const IntersectionPtr intersection=&(*inter);

          intersectionsPathOrder[currentCoastline].push_back(intersection);
          intersectionsCW.push_back(intersection);
        }

        intersectionsPathOrder[currentCoastline].sort(IntersectionByPathComparator());

        // Fix intersection order for areas
        if (data.coastlines[currentCoastline].isArea &&
            intersectionsPathOrder[currentCoastline].front()->direction==-1) {
          intersectionsPathOrder[currentCoastline].push_back(intersectionsPathOrder[currentCoastline].front());
          intersectionsPathOrder[currentCoastline].pop_front();
        }

        for (std::list<IntersectionPtr>::reverse_iterator inter=intersectionsPathOrder[currentCoastline].rbegin();
            inter!=intersectionsPathOrder[currentCoastline].rend();
            inter++) {
          if ((*inter)->direction==-1) {
            intersectionsOuter.push_back(*inter);
          }
        }
      }

      intersectionsCW.sort(IntersectionCWComparator());

      double            lonMin,lonMax,latMin,latMax;
      Coord             borderPoints[4];
      GroundTile::Coord borderCoords[4];

      lonMin=(level.cellXStart+cellEntry.first.x)*level.cellWidth-180.0;
      lonMax=(level.cellXStart+cellEntry.first.x+1)*level.cellWidth-180.0;
      latMin=(level.cellYStart+cellEntry.first.y)*level.cellHeight-90.0;
      latMax=(level.cellYStart+cellEntry.first.y+1)*level.cellHeight-90.0;

      borderPoints[0]=Coord(0,GeoCoord(latMax,lonMin)); // top left
      borderPoints[1]=Coord(0,GeoCoord(latMax,lonMax)); // top right
      borderPoints[2]=Coord(0,GeoCoord(latMin,lonMax)); // bottom right
      borderPoints[3]=Coord(0,GeoCoord(latMin,lonMin)); // bottom left

      borderCoords[0].Set(0,GroundTile::Coord::CELL_MAX,false);                           // top left
      borderCoords[1].Set(GroundTile::Coord::CELL_MAX,GroundTile::Coord::CELL_MAX,false); // top right
      borderCoords[2].Set(GroundTile::Coord::CELL_MAX,0,false);                           // bottom right
      borderCoords[3].Set(0,0,false);                                                     // bottom left

#if defined(DEBUG_COASTLINE)
      std::cout << std::setiosflags(std::ios::fixed) << std::setprecision(6);
      std::cout << "-- Cell: " << cellEntry.first.x << "," << cellEntry.first.y << std::endl;

      for (size_t currentCoastline=0;
           currentCoastline<coastlines.size();
           currentCoastline++) {
        if (!intersectionsPathOrder[currentCoastline].empty()) {
          std::cout << "Coastline " << currentCoastline << " " << data.coastlines[currentCoastline].id << std::endl;
          for (std::list<IntersectionPtr>::const_iterator iter=intersectionsPathOrder[currentCoastline].begin();
               iter!=intersectionsPathOrder[currentCoastline].end();
               ++iter) {
            IntersectionPtr intersection=*iter;
            std::cout << "> " << data.coastlines[intersection->coastline].points[intersection->prevWayPointIndex].GetId() << " "
                      << intersection->prevWayPointIndex << " " << intersection->distanceSquare << " "
                      << intersection->point.GetLat() << "," << intersection->point.GetLon() << " "
                      << (unsigned int) intersection->borderIndex << " " << (int) intersection->direction
                      << std::endl;
          }
        }
      }

      std::cout << "-" << std::endl;
      for (std::list<IntersectionPtr>::const_iterator iter=intersectionsCW.begin();
           iter!=intersectionsCW.end();
           ++iter) {
        IntersectionPtr intersection=*iter;
        std::cout << "* " << intersection->coastline << " " << data.coastlines[intersection->coastline].id << " "
                  << data.coastlines[intersection->coastline].points[intersection->prevWayPointIndex].GetId() << " "
                  << (unsigned int) intersection->prevWayPointIndex << " " << intersection->distanceSquare << " "
                  << intersection->point.GetLat() << "," << intersection->point.GetLon() << " "
                  << (unsigned int) intersection->borderIndex << " " << (int) intersection->direction << std::endl;
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
                     data.coastlines[initialOutgoing->coastline].points,
                     data.coastlines[initialOutgoing->coastline].isArea);

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
                       data.coastlines[outgoing->coastline].points,
                       data.coastlines[outgoing->coastline].isArea);

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

#if defined(DEBUG_TILING)
          std::cout << "Coastline intersects cell: " << cellEntry.first.x << "," << cellEntry.first.y << std::endl;

          if (cellEntry.first.x==49 && cellEntry.first.y==52) {
            for (const auto& coord : groundTile.coords) {
              std::cout << "* " << coord.x << "," << coord.y << std::endl;
            }
          }
#endif

          cellGroundTileMap[cellEntry.first].push_back(groundTile);
        }
      }
    }
  }

  void WaterIndexGenerator::GetDescription(const ImportParameter& /*parameter*/,
                                              ImportModuleDescription& description) const
  {
    description.SetName("WaterIndexGenerator");
    description.SetDescription("Create index for lookup of ground/see tiles");

    description.AddRequiredFile(BoundingBoxDataFile::BOUNDINGBOX_DAT);

    description.AddRequiredFile(Preprocess::RAWCOASTLINE_DAT);

    description.AddRequiredFile(CoordDataFile::COORD_DAT);

    description.AddRequiredFile(WayDataFile::WAYS_DAT);

    description.AddProvidedFile(WaterIndex::WATER_IDX);
  }

  bool WaterIndexGenerator::Import(const TypeConfigRef& typeConfig,
                                   const ImportParameter& parameter,
                                   Progress& progress)
  {
    std::list<CoastRef> coastlines;

    FileScanner         scanner;

    GeoBox              boundingBox;
    GeoCoord            minCoord;
    GeoCoord            maxCoord;

    std::vector<Level>  levels;

    // Calculate size of tile cells for the maximum zoom level
    double              cellWidth;
    double              cellHeight;

    //
    // Read bounding box
    //

    BoundingBoxDataFile boundingBoxDataFile;

    if (!boundingBoxDataFile.Load(parameter.GetDestinationDirectory())) {
      progress.Error("Error loading file '"+boundingBoxDataFile.GetFilename()+"'");

      return false;
    }

    boundingBox=boundingBoxDataFile.GetBoundingBox();
    minCoord=boundingBox.GetMinCoord();
    maxCoord=boundingBox.GetMaxCoord();

    //
    // Initialize levels
    //

    levels.resize(parameter.GetWaterIndexMaxMag()-parameter.GetWaterIndexMinMag()+1);

    cellWidth=360.0;
    cellHeight=180.0;

    for (size_t level=0; level<=parameter.GetWaterIndexMaxMag(); level++) {
      if (level>=parameter.GetWaterIndexMinMag() &&
          level<=parameter.GetWaterIndexMaxMag()) {
        levels[level-parameter.GetWaterIndexMinMag()].SetBox(minCoord,
                                                             maxCoord,
                                                             cellWidth,cellHeight);
      }

      cellWidth=cellWidth/2;
      cellHeight=cellHeight/2;
    }

    //
    // Load and merge coastlines
    //

    if (!LoadCoastlines(parameter,
                        progress,
                        coastlines)) {
      return false;
    }

    MergeCoastlines(progress,
                    coastlines);


    progress.SetAction("Writing 'water.idx'");

    FileWriter writer;

    try {
      writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                  WaterIndex::WATER_IDX));

      DumpIndexHeader(parameter,
                      writer,
                      levels);

      for (size_t level=0; level<levels.size(); level++) {
        Magnification                          magnification;
        MercatorProjection                     projection;
        Data                                   data;
        std::map<Pixel,std::list<GroundTile> > cellGroundTileMap;

        magnification.SetLevel((uint32_t) (level+parameter.GetWaterIndexMinMag()));

        projection.Set(GeoCoord(0.0,0.0),magnification,72,640,480);

        progress.SetAction("Building tiles for level "+NumberToString(level+parameter.GetWaterIndexMinMag()));

        if (!coastlines.empty()) {
          // Collects, calculates and generates a number of data about a coastline
          GetCoastlineData(parameter,
                           progress,
                           projection,
                           levels[level],
                           coastlines,
                           data);

          // Mark cells that intersect a coastline as coast
          MarkCoastlineCells(progress,
                             levels[level],
                             data);

          // Fills coords information for cells that completely contain a coastline
          HandleAreaCoastlinesCompletelyInACell(progress,
                                                levels[level],
                                                data,
                                                cellGroundTileMap);

          // Fills coords information for cells that intersect a coastline
          HandleCoastlinesPartiallyInACell(progress,
                                           coastlines,
                                           levels[level],
                                           cellGroundTileMap,
                                           data);
        }

        // Calculate the cell type for cells directly around coast cells
        CalculateCoastEnvironment(progress,
                                  levels[level],
                                  cellGroundTileMap);

        if (parameter.GetAssumeLand()) {
          // Assume cell type 'land' for cells that intersect with 'land' object types
          AssumeLand(parameter,
                     progress,
                     *typeConfig,
                     levels[level]);
        }

        if (!coastlines.empty()) {
          // Marks all still 'unknown' cells neighbouring 'water' cells as 'water', too
          FillWater(progress,
                    levels[level],20);
        }

        // Marks all still 'unknown' cells between 'coast' or 'land' and 'land' cells as 'land', too
        FillLand(progress,
                 levels[level]);

        levels[level].hasCellData=false;
        levels[level].defaultCellData=unknown;

        if (levels[level].cellXCount>0 && levels[level].cellYCount>0) {
          levels[level].defaultCellData=levels[level].GetState(0,0);

          if (cellGroundTileMap.size()>0) {
            levels[level].hasCellData=true;
          }
          else {
            for (uint32_t y=0; y<levels[level].cellYCount; y++) {
              for (uint32_t x=0; x<levels[level].cellXCount; x++) {
                levels[level].hasCellData=levels[level].GetState(x,y)!=levels[level].defaultCellData;

                if (levels[level].hasCellData) {
                  break;
                }
              }

              if (levels[level].hasCellData) {
                break;
              }
            }
          }
        }

        if (levels[level].hasCellData) {

          //
          // Calculate size of data
          //

          size_t dataSize=4;
          char   buffer[10];

          for (const auto& coord : cellGroundTileMap) {
            // Number of ground tiles
            dataSize+=EncodeNumber(coord.second.size(),buffer);

            for (const auto& tile : coord.second) {
              // Type
              dataSize++;

              // Number of coordinates
              dataSize+=EncodeNumber(tile.coords.size(),buffer);

              // Data for coordinate pairs
              dataSize+=tile.coords.size()*2*sizeof(uint16_t);
            }
          }

          levels[level].dataOffsetBytes=BytesNeededToEncodeNumber(dataSize);

          progress.Info("Writing index for level "+
                        NumberToString(level+parameter.GetWaterIndexMinMag())+", "+
                        NumberToString(levels[level].cellXCount*levels[level].cellXCount)+" cells, "+
                        NumberToString(cellGroundTileMap.size())+" entries, "+
                        NumberToString(levels[level].dataOffsetBytes)+" bytes/entry, "+
                        ByteSizeToString(1.0*levels[level].cellXCount*levels[level].cellYCount*levels[level].dataOffsetBytes+dataSize));

          //
          // Write bitmap
          //

          levels[level].indexDataOffset=writer.GetPos();

          for (uint32_t y=0; y<levels[level].cellYCount; y++) {
            for (uint32_t x=0; x<levels[level].cellXCount; x++) {
              State state=levels[level].GetState(x,y);

              writer.WriteFileOffset((FileOffset) state,
                                     levels[level].dataOffsetBytes);
            }
          }

          //
          // Write data
          //

          FileOffset dataOffset=writer.GetPos();

          // TODO: when data format will be changing, consider usage ones (0xFF..FF) as empty placeholder
          writer.WriteFileOffset((FileOffset)0,4);

          for (const auto& coord : cellGroundTileMap) {
            FileOffset startPos=writer.GetPos();

            writer.WriteNumber((uint32_t) coord.second.size());

            for (const auto& tile : coord.second) {
              writer.Write((uint8_t) tile.type);

              writer.WriteNumber((uint32_t) tile.coords.size());

              for (size_t c=0; c<tile.coords.size(); c++) {
                if (tile.coords[c].coast) {
                  uint16_t x=tile.coords[c].x | uint16_t(1 << 15);

                  writer.Write(x);
                }
                else {
                  writer.Write(tile.coords[c].x);
                }
                writer.Write(tile.coords[c].y);
              }
            }

            FileOffset endPos;
            uint32_t   cellId=coord.first.y*levels[level].cellXCount+coord.first.x;
            size_t     index =cellId*levels[level].dataOffsetBytes;

            endPos=writer.GetPos();

            writer.SetPos(levels[level].indexDataOffset+index);
            writer.WriteFileOffset(startPos-dataOffset,
                                   levels[level].dataOffsetBytes);
            writer.SetPos(endPos);
          }
        }
        else {
          progress.Info("All cells have state '"+StateToString(levels[level].defaultCellData)+"' and no coastlines, no cell index needed");
        }

        FileOffset currentPos=writer.GetPos();

        writer.SetPos(levels[level].indexEntryOffset);
        writer.Write(levels[level].hasCellData);
        writer.Write(levels[level].dataOffsetBytes);
        writer.Write((uint8_t) levels[level].defaultCellData);
        writer.WriteFileOffset(levels[level].indexDataOffset);
        writer.SetPos(currentPos);
      }

      coastlines.clear();

      writer.Close();
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());

      writer.CloseFailsafe();

      return false;
    }

    return true;
  }
}
