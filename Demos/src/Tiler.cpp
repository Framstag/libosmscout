/*
  Tiles - a demo program for libosmscout
  Copyright (C) 2011  Tim Teulings

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <iostream>
#include <iomanip>
#include <limits>

#include <osmscout/Database.h>
#include <osmscout/MapService.h>

#include <osmscout/MapPainterAgg.h>

#include <osmscout/util/StopClock.h>
#include <osmscout/util/Tiling.h>

/*
  Example for the nordrhein-westfalen.osm (to be executed in the Demos top
  level directory), drawing the "Ruhrgebiet":

  src/Tiler ../maps/nordrhein-westfalen ../stylesheets/standard.oss 51.2 6.5 51.7 8 10 13
*/

static const unsigned int tileWidth=256;
static const unsigned int tileHeight=256;
static const double       DPI=96.0;
static const int          tileRingSize=1;

bool write_ppm(const agg::rendering_buffer& buffer,
               const char* file_name)
{
  FILE* fd=fopen(file_name, "wb");

  if (fd) {
    fprintf(fd,"P6 %d %d 255\n", buffer.width(),buffer.height());

    for (size_t y=0; y<buffer.height();y++)
    {
      const unsigned char* row=buffer.row_ptr(y);

      fwrite(row,1,buffer.width()*3,fd);
    }

    fclose(fd);
    return true;
  }

  return false;
}

void MergeTilesToMapData(const std::list<osmscout::TileRef>& centerTiles,
                         const osmscout::MapService::TypeDefinition& ringTypeDefinition,
                         const std::list<osmscout::TileRef>& ringTiles,
                         osmscout::MapData& data)
{
  std::unordered_map<osmscout::FileOffset,osmscout::NodeRef> nodeMap(10000);
  std::unordered_map<osmscout::FileOffset,osmscout::WayRef>  wayMap(10000);
  std::unordered_map<osmscout::FileOffset,osmscout::AreaRef> areaMap(10000);
  std::unordered_map<osmscout::FileOffset,osmscout::WayRef>  optimizedWayMap(10000);
  std::unordered_map<osmscout::FileOffset,osmscout::AreaRef> optimizedAreaMap(10000);

  osmscout::StopClock uniqueTime;

  for (auto tile : centerTiles) {
    tile->GetNodeData().CopyData([&nodeMap](const osmscout::NodeRef& node) {
      nodeMap[node->GetFileOffset()]=node;
    });

    //---

    tile->GetOptimizedWayData().CopyData([&optimizedWayMap](const osmscout::WayRef& way) {
      optimizedWayMap[way->GetFileOffset()]=way;
    });

    tile->GetWayData().CopyData([&wayMap](const osmscout::WayRef& way) {
      wayMap[way->GetFileOffset()]=way;
    });

    //---

    tile->GetOptimizedAreaData().CopyData([&optimizedAreaMap](const osmscout::AreaRef& area) {
      optimizedAreaMap[area->GetFileOffset()]=area;
    });

    tile->GetAreaData().CopyData([&areaMap](const osmscout::AreaRef& area) {
      areaMap[area->GetFileOffset()]=area;
    });
  }

  for (auto tile : ringTiles) {
    tile->GetNodeData().CopyData([&ringTypeDefinition,&nodeMap](const osmscout::NodeRef& node) {
      if (ringTypeDefinition.nodeTypes.IsSet(node->GetType())) {
        nodeMap[node->GetFileOffset()]=node;
      }
    });

    //---

    tile->GetOptimizedWayData().CopyData([&ringTypeDefinition,&optimizedWayMap](const osmscout::WayRef& way) {
      if (ringTypeDefinition.optimizedWayTypes.IsSet(way->GetType())) {
        optimizedWayMap[way->GetFileOffset()]=way;
      }
    });

    tile->GetWayData().CopyData([&ringTypeDefinition,&wayMap](const osmscout::WayRef& way) {
      if (ringTypeDefinition.wayTypes.IsSet(way->GetType())) {
        wayMap[way->GetFileOffset()]=way;
      }
    });

    //---

    tile->GetOptimizedAreaData().CopyData([&ringTypeDefinition,&optimizedAreaMap](const osmscout::AreaRef& area) {
      if (ringTypeDefinition.optimizedAreaTypes.IsSet(area->GetType())) {
        optimizedAreaMap[area->GetFileOffset()]=area;
      }
    });

    tile->GetAreaData().CopyData([&ringTypeDefinition,&areaMap](const osmscout::AreaRef& area) {
      if (ringTypeDefinition.areaTypes.IsSet(area->GetType())) {
        areaMap[area->GetFileOffset()]=area;
      }
    });
  }

  uniqueTime.Stop();

  //std::cout << "Make data unique time: " << uniqueTime.ResultString() << std::endl;

  osmscout::StopClock copyTime;

  data.nodes.reserve(nodeMap.size());
  data.ways.reserve(wayMap.size()+optimizedWayMap.size());
  data.areas.reserve(areaMap.size()+optimizedAreaMap.size());

  for (const auto& nodeEntry : nodeMap) {
    data.nodes.push_back(nodeEntry.second);
  }

  for (const auto& wayEntry : wayMap) {
    data.ways.push_back(wayEntry.second);
  }

  for (const auto& wayEntry : optimizedWayMap) {
    data.ways.push_back(wayEntry.second);
  }

  for (const auto& areaEntry : areaMap) {
    data.areas.push_back(areaEntry.second);
  }

  for (const auto& areaEntry : optimizedAreaMap) {
    data.areas.push_back(areaEntry.second);
  }

  copyTime.Stop();

  if (copyTime.GetMilliseconds()>20) {
    osmscout::log.Warn() << "Copying data from tile to MapData took " << copyTime.ResultString();
  }
}

int main(int argc, char* argv[])
{
  std::string  map;
  std::string  style;
  double       latTop,latBottom,lonLeft,lonRight;
  unsigned int startLevel;
  unsigned int endLevel;

  if (argc!=9) {
    std::cerr << "DrawMap ";
    std::cerr << "<map directory> <style-file> ";
    std::cerr << "<lat_top> <lon_left> <lat_bottom> <lon_right> ";
    std::cerr << "<start_zoom>" << std::endl;
    std::cerr << "<end_zoom>" << std::endl;
    return 1;
  }

  map=argv[1];
  style=argv[2];

  if (sscanf(argv[3],"%lf",&latTop)!=1) {
    std::cerr << "lon is not numeric!" << std::endl;
    return 1;
  }

  if (sscanf(argv[4],"%lf",&lonLeft)!=1) {
    std::cerr << "lat is not numeric!" << std::endl;
    return 1;
  }

  if (sscanf(argv[5],"%lf",&latBottom)!=1) {
    std::cerr << "lon is not numeric!" << std::endl;
    return 1;
  }

  if (sscanf(argv[6],"%lf",&lonRight)!=1) {
    std::cerr << "lat is not numeric!" << std::endl;
    return 1;
  }

  if (sscanf(argv[7],"%u",&startLevel)!=1) {
    std::cerr << "start zoom is not numeric!" << std::endl;
    return 1;
  }

  if (sscanf(argv[8],"%u",&endLevel)!=1) {
    std::cerr << "end zoom is not numeric!" << std::endl;
    return 1;
  }

  osmscout::DatabaseParameter databaseParameter;
  osmscout::DatabaseRef       database=std::make_shared<osmscout::Database>(databaseParameter);
  osmscout::MapServiceRef     mapService=std::make_shared<osmscout::MapService>(database);

  if (!database->Open(map.c_str())) {
    std::cerr << "Cannot open database" << std::endl;

    return 1;
  }

  osmscout::StyleConfigRef styleConfig=std::make_shared<osmscout::StyleConfig>(database->GetTypeConfig());

  if (!styleConfig->Load(style)) {
    std::cerr << "Cannot open style" << std::endl;
  }

  osmscout::TileProjection      projection;
  osmscout::MapParameter        drawParameter;
  osmscout::AreaSearchParameter searchParameter;

  // Change this, to match your system
  drawParameter.SetFontName("/usr/share/fonts/truetype/msttcorefonts/Verdana.ttf");
  drawParameter.SetFontName("/usr/share/fonts/TTF/DejaVuSans.ttf");
  drawParameter.SetFontSize(2.0);
  // Fadings make problems with tile approach, we disable it
  drawParameter.SetDrawFadings(false);
  // To get accurate label drawing at tile borders, we take into account labels
  // of other than the current tile, too.
  drawParameter.SetDropNotVisiblePointLabels(false);

  searchParameter.SetUseLowZoomOptimization(true);
  searchParameter.SetMaximumAreaLevel(3);

  osmscout::MapPainterAgg painter(styleConfig);

  for (size_t level=std::min(startLevel,endLevel);
       level<=std::max(startLevel,endLevel);
       level++) {
    osmscout::Magnification magnification;
    int                     xTileStart,xTileEnd,xTileCount,yTileStart,yTileEnd,yTileCount;

    magnification.SetLevel(level);

    xTileStart=osmscout::LonToTileX(std::min(lonLeft,lonRight),
                                    magnification);
    xTileEnd=osmscout::LonToTileX(std::max(lonLeft,lonRight),
                                  magnification);
    xTileCount=xTileEnd-xTileStart+1;

    yTileStart=osmscout::LatToTileY(std::max(latTop,latBottom),
                                    magnification);
    yTileEnd=osmscout::LatToTileY(std::min(latTop,latBottom),
                                  magnification);

    yTileCount=yTileEnd-yTileStart+1;

    std::cout << "Drawing zoom " << level << ", " << (xTileCount)*(yTileCount) << " tiles [" << xTileStart << "," << yTileStart << " - " <<  xTileEnd << "," << yTileEnd << "]" << std::endl;

    unsigned long bitmapSize=tileWidth*tileHeight*3*xTileCount*yTileCount;
    unsigned char *buffer=new unsigned char[bitmapSize];


    memset(buffer,0,bitmapSize);

    agg::rendering_buffer rbuf(buffer,
                               tileWidth*xTileCount,
                               tileHeight*yTileCount,
                               tileWidth*xTileCount*3);

    double minTime=std::numeric_limits<double>::max();
    double maxTime=0.0;
    double totalTime=0.0;

    osmscout::MapService::TypeDefinition typeDefinition;

    for (auto type : database->GetTypeConfig()->GetTypes()) {
      bool hasLabel=false;

      if (type->CanBeNode()) {
        if (styleConfig->HasNodeTextStyles(type,
                                           magnification)) {
          typeDefinition.nodeTypes.Set(type);
          hasLabel=true;
        }
      }

      if (type->CanBeArea()) {
        if (styleConfig->HasAreaTextStyles(type,
                                           magnification)) {
          if (type->GetOptimizeLowZoom() && searchParameter.GetUseLowZoomOptimization()) {
            typeDefinition.optimizedAreaTypes.Set(type);
          }
          else {
            typeDefinition.areaTypes.Set(type);
          }

          hasLabel=true;
        }
      }

      if (hasLabel) {
        std::cout << "TYPE " << type->GetName() << " might have labels" << std::endl;
      }
    }

    for (int y=yTileStart; y<=yTileEnd; y++) {
      for (int x=xTileStart; x<=xTileEnd; x++) {
        agg::pixfmt_rgb24   pf(rbuf);
        osmscout::StopClock timer;
        osmscout::GeoBox    boundingBox;
        osmscout::MapData   data;

        projection.Set(x,y,
                       magnification,
                       DPI,
                       tileWidth,
                       tileHeight);

        projection.GetDimensions(boundingBox);

        std::cout << "Drawing tile " << level << "." << y << "." << x << " " << boundingBox.GetDisplayText() << std::endl;


        std::list<osmscout::TileRef> centerTiles;

        mapService->LookupTiles(magnification,
                                boundingBox,
                                centerTiles);

        mapService->LoadMissingTileData(searchParameter,
                                        *styleConfig,
                                        centerTiles);

        std::map<osmscout::TileId, osmscout::TileRef> ringTileMap;

        for (int ringY=y-tileRingSize; ringY<=y+tileRingSize; ringY++) {
          for (int ringX=x-tileRingSize; ringX<=x+tileRingSize; ringX++) {
            if (ringX==x && ringY==y) {
              continue;
            }

            osmscout::GeoBox boundingBox(osmscout::GeoCoord(osmscout::TileYToLat(ringY,magnification),osmscout::TileXToLon(ringX,magnification)),
                                         osmscout::GeoCoord(osmscout::TileYToLat(ringY+1,magnification),osmscout::TileXToLon(ringX+1,magnification)));


            std::list<osmscout::TileRef> tiles;

            mapService->LookupTiles(magnification,
                                    boundingBox,
                                    tiles);

            for (const auto& tile : tiles) {
              ringTileMap[tile->GetId()]=tile;
            }
          }
        }

        std::list<osmscout::TileRef> ringTiles;

        for (const auto& tileEntry : ringTileMap) {
          ringTiles.push_back(tileEntry.second);
        }

        mapService->LoadMissingTileData(searchParameter,
                                        magnification,
                                        typeDefinition,
                                        ringTiles);

        MergeTilesToMapData(centerTiles,
                            typeDefinition,
                            ringTiles,
                            data);

        size_t bufferOffset=xTileCount*tileWidth*3*(y-yTileStart)*tileHeight+
                            (x-xTileStart)*tileWidth*3;

        rbuf.attach(buffer+bufferOffset,
                    tileWidth,tileHeight,
                    tileWidth*xTileCount*3);

        painter.DrawMap(projection,
                        drawParameter,
                        data,
                        &pf);

        timer.Stop();

        double time=timer.GetMilliseconds();

        minTime=std::min(minTime,time);
        maxTime=std::max(maxTime,time);
        totalTime+=time;

        std::string output=osmscout::NumberToString(level)+"_"+osmscout::NumberToString(x)+"_"+osmscout::NumberToString(y)+".ppm";

        write_ppm(rbuf,output.c_str());
      }
    }

    rbuf.attach(buffer,
                tileWidth*xTileCount,
                tileHeight*yTileCount,
                tileWidth*xTileCount*3);

    std::string output=osmscout::NumberToString(level)+"_full_map.ppm";

    write_ppm(rbuf,output.c_str());

    delete[] buffer;

    std::cout << "=> Time: ";
    std::cout << "total: " << totalTime << " msec ";
    std::cout << "min: " << minTime << " msec ";
    std::cout << "avg: " << totalTime/(xTileCount*yTileCount) << " msec ";
    std::cout << "max: " << maxTime << " msec" << std::endl;
  }

  database->Close();

  return 0;
}
