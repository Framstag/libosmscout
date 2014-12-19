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

#include "config.h"

#include <osmscout/Database.h>
#include <osmscout/MapService.h>

#if defined(HAVE_LIB_OSMSCOUTMAPCAIRO)
#include <osmscout/MapPainterCairo.h>
#endif

#include <osmscout/system/Math.h>

#include <osmscout/util/StopClock.h>
#include <osmscout/util/Tiling.h>

/*
  Example for the nordrhein-westfalen.osm (to be executed in the Demos top
  level directory), drawing the "Ruhrgebiet":

  src/PerformanceTest ../TravelJinni/ ../TravelJinni/standard.oss 51.2 6.5 51.7 8 10 13
*/

// See http://wiki.openstreetmap.org/wiki/Slippy_map_tilenames for details about
// coordinate transformation

static const double DPI=96.0;


int main(int argc, char* argv[])
{
  std::string   map;
  std::string   style;
  double        latTop,latBottom,lonLeft,lonRight;
  unsigned long xTileStart,xTileEnd,xTileCount,yTileStart,yTileEnd,yTileCount;
  unsigned long startZoom;
  unsigned long endZoom;
  unsigned long tileWidth;
  unsigned long tileHeight;
  std::string   driver;

  if (argc!=12) {
    std::cerr << "DrawMap ";
    std::cerr << "<map directory> <style-file> ";
    std::cerr << "<lat_top> <lon_left> <lat_bottom> <lon_right> ";
    std::cerr << "<start zoom>" << std::endl;
    std::cerr << "<end zoom>" << std::endl;
    std::cerr << "<tile width>" << std::endl;
    std::cerr << "<tile height>" << std::endl;
    std::cerr << "<driver>" << std::endl;
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

  if (sscanf(argv[7],"%lu",&startZoom)!=1) {
    std::cerr << "start zoom is not numeric!" << std::endl;
    return 1;
  }

  if (sscanf(argv[8],"%lu",&endZoom)!=1) {
    std::cerr << "end zoom is not numeric!" << std::endl;
    return 1;
  }

  if (sscanf(argv[9],"%lu",&tileWidth)!=1) {
    std::cerr << "tile width is not numeric!" << std::endl;
    return 1;
  }

  if (sscanf(argv[10],"%lu",&tileHeight)!=1) {
    std::cerr << "tile height is not numeric!" << std::endl;
    return 1;
  }

  driver=argv[11];

#if defined(HAVE_LIB_OSMSCOUTMAPCAIRO)
  cairo_surface_t *surface=NULL;
  cairo_t         *cairo=NULL;
#endif

  if (driver=="cairo") {
    std::cout << "Using driver 'cairo'..." << std::endl;
#if defined(HAVE_LIB_OSMSCOUTMAPCAIRO)
    surface=cairo_image_surface_create(CAIRO_FORMAT_RGB24,tileWidth,tileHeight);

    if (surface==NULL) {
      std::cerr << "Cannot create cairo image surface" << std::endl;
      return 1;
    }

    cairo=cairo_create(surface);

    if (cairo==NULL) {
      std::cerr << "Cannot create cairo_t for image surface" << std::endl;
      return 1;
    }
#else
    std::cerr << "Driver 'cairo' is not enabled" << std::endl;
    return 1;
#endif
  }
  else {
    std::cerr << "Unsupported driver '" << driver << "'" << std::endl;
    return 1;
  }

  osmscout::DatabaseParameter databaseParameter;

  //databaseParameter.SetDebugPerformance(true);

  osmscout::DatabaseRef       database(new osmscout::Database(databaseParameter));
  osmscout::MapServiceRef     mapService(new osmscout::MapService(database));

  if (!database->Open(map.c_str())) {
    std::cerr << "Cannot open database" << std::endl;
    return 1;
  }

  osmscout::StyleConfigRef styleConfig(new osmscout::StyleConfig(database->GetTypeConfig()));

  if (!styleConfig->Load(style)) {
    std::cerr << "Cannot open style" << std::endl;
  }

  osmscout::TileProjection      projection;
  osmscout::MapParameter        drawParameter;
  osmscout::AreaSearchParameter searchParameter;

  for (size_t zoom=std::min(startZoom,endZoom);
       zoom<=std::max(startZoom,endZoom);
       zoom++) {
    xTileStart=osmscout::LonToTileX(std::min(lonLeft,lonRight),zoom);
    xTileEnd=osmscout::LonToTileX(std::max(lonLeft,lonRight),zoom);
    xTileCount=xTileEnd-xTileStart+1;

    yTileStart=osmscout::LatToTileY(std::max(latTop,latBottom),zoom);
    yTileEnd=osmscout::LatToTileY(std::min(latTop,latBottom),zoom);
    yTileCount=yTileEnd-yTileStart+1;

    std::cout << "Drawing zoom " << zoom;
    //<< ", " << (xTileCount)*(yTileCount) << " tiles [" << xTileStart << "," << yTileStart << " - " <<  xTileEnd << "," << yTileEnd << "]";
    std::cout << std::endl;

#if defined(HAVE_LIB_OSMSCOUTMAPCAIRO)
    osmscout::MapPainterCairo cairoPainter(styleConfig);
#endif

    osmscout::Magnification   magnification;

    magnification.SetLevel(zoom);

    double dbMinTime=std::numeric_limits<double>::max();
    double dbMaxTime=0.0;
    double dbTotalTime=0.0;

    double drawMinTime=std::numeric_limits<double>::max();
    double drawMaxTime=0.0;
    double drawTotalTime=0.0;

    for (size_t y=yTileStart; y<=yTileEnd; y++) {
      for (size_t x=xTileStart; x<=xTileEnd; x++) {
        double                         lat,lon;
        osmscout::TypeSet              nodeTypes;
        std::vector<osmscout::TypeSet> wayTypes;
        osmscout::TypeSet              areaTypes;
        osmscout::MapData              data;
        double                         lonMin,lonMax,latMin,latMax;

        lat=(osmscout::TileYToLat(y,zoom)+osmscout::TileYToLat(y+1,zoom))/2;
        lon=(osmscout::TileXToLon(x,zoom)+osmscout::TileXToLon(x+1,zoom))/2;

        //std::cout << "Drawing tile at " << lat << "," << lon << "/";
        //std::cout << x << "," << y << "/";
        //std::cout << x-xTileStart << "," << y-yTileStart << std::endl;

        projection.Set(lon,
                       lat,
                       magnification,
                       DPI,
                       tileWidth,
                       tileHeight);


        projection.GetDimensions(lonMin,latMin,lonMax,latMax);

        styleConfig->GetNodeTypesWithMaxMag(projection.GetMagnification(),
                                            nodeTypes);

        styleConfig->GetWayTypesByPrioWithMaxMag(projection.GetMagnification(),
                                                 wayTypes);

        styleConfig->GetAreaTypesWithMaxMag(projection.GetMagnification(),
                                            areaTypes);

        osmscout::StopClock dbTimer;

        mapService->GetObjects(nodeTypes,
                               wayTypes,
                               areaTypes,
                               lonMin,
                               latMin,
                               lonMax,
                               latMax,
                               projection.GetMagnification(),
                               searchParameter,
                               data.nodes,
                               data.ways,
                               data.areas);

        dbTimer.Stop();

        double dbTime=dbTimer.GetMilliseconds();

        dbMinTime=std::min(dbMinTime,dbTime);
        dbMaxTime=std::max(dbMaxTime,dbTime);
        dbTotalTime+=dbTime;

        osmscout::StopClock drawTimer;

#if defined(HAVE_LIB_OSMSCOUTMAPCAIRO)
        if (driver=="cairo") {
          //std::cout << data.nodes.size() << " " << data.ways.size() << " " << data.areas.size() << std::endl;
          cairoPainter.DrawMap(projection,
                               drawParameter,
                               data,
                               cairo);
        }
#endif

        drawTimer.Stop();

        double drawTime=drawTimer.GetMilliseconds();

        drawMinTime=std::min(drawMinTime,drawTime);
        drawMaxTime=std::max(drawMaxTime,drawTime);
        drawTotalTime+=drawTime;
      }
    }

    std::cout << "GetObjects: ";
    std::cout << "total: " << dbTotalTime << " msec ";
    std::cout << "min: " << dbMinTime << " msec ";
    std::cout << "avg: " << dbTotalTime/(xTileCount*yTileCount) << " msec ";
    std::cout << "max: " << dbMaxTime << " msec" << std::endl;

    std::cout << "DrawMap: ";
    std::cout << "total: " << drawTotalTime << " msec ";
    std::cout << "min: " << drawMinTime << " msec ";
    std::cout << "avg: " << drawTotalTime/(xTileCount*yTileCount) << " msec ";
    std::cout << "max: " << drawMaxTime << " msec" << std::endl;
  }

  database->Close();

#if defined(HAVE_LIB_OSMSCOUTMAPCAIRO)
  if (driver=="cairo") {
    cairo_destroy(cairo);
    cairo_surface_destroy(surface);
  }
#endif
  return 0;
}
