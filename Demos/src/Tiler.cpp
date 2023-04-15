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

#include <osmscout/projection/TileProjection.h>

#include <osmscout/util/StopClock.h>
#include <osmscout/util/Tiling.h>
#include <osmscout/util/CmdLineParsing.h>

#include <osmscoutmap/MapService.h>

#include <osmscoutmapagg/MapPainterAgg.h>

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

  for (const auto& tile : centerTiles) {
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

  for (const auto& tile : ringTiles) {
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

struct Arguments {
  bool help{false};
  bool debug{false};
  std::string databaseDirectory{"."};
  std::string style{"stylesheets/standard.oss"};
  std::string font{"/usr/share/fonts/TTF/DejaVuSans.ttf"};
  osmscout::MagnificationLevel startZoom{0};
  osmscout::MagnificationLevel endZoom{20};
  osmscout::GeoCoord coordTopLeft;
  osmscout::GeoCoord coordBottomRight;
};

int main(int argc, char* argv[])
{
  osmscout::CmdLineParser     argParser("Tiler",
                                        argc,argv);
  Arguments                   args;

  argParser.AddOption(osmscout::CmdLineFlag([&args](const bool& value) {
                        args.help=value;
                      }),
                      std::vector<std::string>{"h","help"},
                      "Display help",
                      true);
  argParser.AddOption(osmscout::CmdLineFlag([&args](const bool& value) {
                        args.debug=value;
                      }),
                      "debug",
                      "Enable debug output",
                      false);
  argParser.AddOption(osmscout::CmdLineStringOption([&args](const std::string& value) {
                        args.font=value;
                      }),
                      "font",
                      "Used font, default: " + args.font,
                      false);

  argParser.AddPositional(osmscout::CmdLineStringOption([&args](const std::string& value) {
                            args.databaseDirectory=value;
                          }),
                          "databaseDir",
                          "Database directory");
  argParser.AddPositional(osmscout::CmdLineStringOption([&args](const std::string& value) {
                            args.style=value;
                          }),
                          "stylesheet",
                          "Map stylesheet");
  argParser.AddPositional(osmscout::CmdLineGeoCoordOption([&args](const osmscout::GeoCoord& coord) {
                            args.coordTopLeft = coord;
                          }),
                          "lat_top lon_left",
                          "Bounding box top-left coordinate");
  argParser.AddPositional(osmscout::CmdLineGeoCoordOption([&args](const osmscout::GeoCoord& coord) {
                            args.coordBottomRight = coord;
                          }),
                          "lat_bottom lon_right",
                          "Bounding box bottom-right coordinate");
  argParser.AddPositional(osmscout::CmdLineUIntOption([&args](const unsigned int& value) {
                            args.startZoom=osmscout::MagnificationLevel(value);
                          }),
                          "start-zoom",
                          "Start zoom");
  argParser.AddPositional(osmscout::CmdLineUIntOption([&args](const unsigned int& value) {
                            args.endZoom=osmscout::MagnificationLevel(value);
                          }),
                          "end-zoom",
                          "End zoom");

  osmscout::CmdLineParseResult argResult=argParser.Parse();
  if (argResult.HasError()) {
    std::cerr << "ERROR: " << argResult.GetErrorDescription() << std::endl;
    std::cout << argParser.GetHelp() << std::endl;
    return 1;
  }

  if (args.help) {
    std::cout << argParser.GetHelp() << std::endl;
    return 0;
  }

  osmscout::log.Debug(args.debug);

  osmscout::DatabaseParameter databaseParameter;
  osmscout::DatabaseRef       database=std::make_shared<osmscout::Database>(databaseParameter);
  osmscout::MapServiceRef     mapService=std::make_shared<osmscout::MapService>(database);

  if (!database->Open(args.databaseDirectory)) {
    std::cerr << "Cannot open database" << std::endl;

    return 1;
  }

  osmscout::StyleConfigRef styleConfig=std::make_shared<osmscout::StyleConfig>(database->GetTypeConfig());

  if (!styleConfig->Load(args.style)) {
    std::cerr << "Cannot open style" << std::endl;
  }

  osmscout::TileProjection      projection;
  osmscout::MapParameter        drawParameter;
  osmscout::AreaSearchParameter searchParameter;

  // Change this, to match your system
  drawParameter.SetFontName(args.font);
  drawParameter.SetFontSize(2.0);
  // Fadings make problems with tile approach, we disable it
  drawParameter.SetDrawFadings(false);

  searchParameter.SetUseLowZoomOptimization(true);
  searchParameter.SetMaximumAreaLevel(3);

  osmscout::MapPainterAgg painter(styleConfig);

  for (osmscout::MagnificationLevel level=osmscout::MagnificationLevel(std::min(args.startZoom,args.endZoom));
       level<=osmscout::MagnificationLevel(std::max(args.startZoom,args.endZoom));
       level++) {
    osmscout::Magnification magnification(level);

    osmscout::OSMTileId     tileA(osmscout::OSMTileId::GetOSMTile(magnification,
                                                                  args.coordBottomRight));
    osmscout::OSMTileId     tileB(osmscout::OSMTileId::GetOSMTile(magnification,
                                                                  args.coordTopLeft));
    uint32_t                xTileStart=std::min(tileA.GetX(),tileB.GetX());
    uint32_t                xTileEnd=std::max(tileA.GetX(),tileB.GetX());
    uint32_t                xTileCount=xTileEnd-xTileStart+1;
    uint32_t                yTileStart=std::min(tileA.GetY(),tileB.GetY());
    uint32_t                yTileEnd=std::max(tileA.GetY(),tileB.GetY());
    uint32_t                yTileCount=yTileEnd-yTileStart+1;

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

    for (const auto& type : database->GetTypeConfig()->GetTypes()) {
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

    for (uint32_t y=yTileStart; y<=yTileEnd; y++) {
      for (uint32_t x=xTileStart; x<=xTileEnd; x++) {
        agg::pixfmt_rgb24   pf(rbuf);
        osmscout::StopClock timer;
        osmscout::MapData   data;

        projection.Set(osmscout::OSMTileId(x,y),
                       magnification,
                       DPI,
                       tileWidth,
                       tileHeight);

        osmscout::GeoBox boundingBox(projection.GetDimensions());

        std::cout << "Drawing tile " << level << "." << y << "." << x << " " << boundingBox.GetDisplayText() << std::endl;


        std::list<osmscout::TileRef> centerTiles;

        mapService->LookupTiles(magnification,
                                boundingBox,
                                centerTiles);

        mapService->LoadMissingTileData(searchParameter,
                                        *styleConfig,
                                        centerTiles);

        std::map<osmscout::TileKey,osmscout::TileRef> ringTileMap;

        for (uint32_t ringY=y-tileRingSize; ringY<=y+tileRingSize; ringY++) {
          for (uint32_t ringX=x-tileRingSize; ringX<=x+tileRingSize; ringX++) {
            if (ringX==x && ringY==y) {
              continue;
            }

            osmscout::GeoBox boundingBox(osmscout::OSMTileId(ringX,ringY).GetBoundingBox(magnification));


            std::list<osmscout::TileRef> tiles;

            mapService->LookupTiles(magnification,
                                    boundingBox,
                                    tiles);

            for (const auto& tile : tiles) {
              ringTileMap[tile->GetKey()]=tile;
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

        std::string output=std::to_string(level.Get())+"_"+std::to_string(x)+"_"+std::to_string(y)+".ppm";

        write_ppm(rbuf,output.c_str());
      }
    }

    rbuf.attach(buffer,
                tileWidth*xTileCount,
                tileHeight*yTileCount,
                tileWidth*xTileCount*3);

    std::string output=std::to_string(level.Get())+"_full_map.ppm";

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
