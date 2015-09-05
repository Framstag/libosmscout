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
#if defined(HAVE_LIB_OSMSCOUTMAPQT)
#include <QApplication>
#include <QDesktopWidget>
#include <QPixmap>
#include <QScreen>
#include <osmscout/MapPainterQt.h>
#endif

#include <osmscout/MapPainterNoOp.h>

#include <osmscout/system/Math.h>

#include <osmscout/util/StopClock.h>
#include <osmscout/util/Tiling.h>

/*
  Example for the nordrhein-westfalen.osm (to be executed in the Demos top
  level directory), drawing the "Ruhrgebiet":

  src/PerformanceTest ../maps/nordrhein-westfalen ../stylesheets/standard.oss 51.2 6.5 51.7 8 10 13 480 800 cairo
*/

// See http://wiki.openstreetmap.org/wiki/Slippy_map_tilenames for details about
// coordinate transformation

static const double DPI=96.0;


int main(int argc, char* argv[])
{
  std::string   map;
  std::string   style;
  double        latTop,latBottom,lonLeft,lonRight;
  unsigned int  startZoom;
  unsigned int  endZoom;
  unsigned int  tileWidth;
  unsigned int  tileHeight;
  std::string   driver;

  if (argc!=12) {
    std::cerr << "DrawMap " << std::endl;
    std::cerr << "  <map directory> <style-file> " << std::endl;
    std::cerr << "  <lat_top> <lon_left> <lat_bottom> <lon_right> " << std::endl;
    std::cerr << "  <start zoom> <end zoom>" << std::endl;
    std::cerr << "  <tile width> <tile height>" << std::endl;
    std::cerr << "  <cairo|Qt|noop>" << std::endl;
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

  if (sscanf(argv[7],"%u",&startZoom)!=1) {
    std::cerr << "start zoom is not numeric!" << std::endl;
    return 1;
  }

  if (sscanf(argv[8],"%u",&endZoom)!=1) {
    std::cerr << "end zoom is not numeric!" << std::endl;
    return 1;
  }

  if (sscanf(argv[9],"%u",&tileWidth)!=1) {
    std::cerr << "tile width is not numeric!" << std::endl;
    return 1;
  }

  if (sscanf(argv[10],"%u",&tileHeight)!=1) {
    std::cerr << "tile height is not numeric!" << std::endl;
    return 1;
  }

  driver=argv[11];

#if defined(HAVE_LIB_OSMSCOUTMAPCAIRO)
  cairo_surface_t * cairoSurface=NULL;
  cairo_t         *cairo=NULL;
#endif

#if defined(HAVE_LIB_OSMSCOUTMAPQT)
  QPixmap         *qtPixmap=NULL;
  QPainter        *qtPainter=NULL;
  QApplication    application(argc,argv,true);
#endif

  if (driver=="cairo") {
    std::cout << "Using driver 'cairo'..." << std::endl;
#if defined(HAVE_LIB_OSMSCOUTMAPCAIRO)
    cairoSurface=cairo_image_surface_create(CAIRO_FORMAT_RGB24,tileWidth,tileHeight);

    if (cairoSurface==NULL) {
      std::cerr << "Cannot create cairo image cairoSurface" << std::endl;
      return 1;
    }

    cairo=cairo_create(cairoSurface);

    if (cairo==NULL) {
      std::cerr << "Cannot create cairo_t for image cairoSurface" << std::endl;
      return 1;
    }
#else
    std::cerr << "Driver 'cairo' is not enabled" << std::endl;
    return 1;
#endif
  }
  else if (driver=="Qt") {
    std::cout << "Using driver 'Qt'..." << std::endl;
#if defined(HAVE_LIB_OSMSCOUTMAPQT)
    qtPixmap=new QPixmap(tileWidth,tileHeight);

    if (qtPixmap==NULL) {
      std::cerr << "Cannot create QPixmap" << std::endl;
      return 1;
    }

    qtPainter=new QPainter(qtPixmap);

    if (qtPainter==NULL) {
      std::cerr << "Cannot create QPainter image cairoSurface" << std::endl;
      return 1;
    }
#else
    std::cerr << "Driver 'Qt' is not enabled" << std::endl;
  return 1;
#endif
  }
  else if (driver=="noop") {
    std::cout << "Using driver 'noop'..." << std::endl;
  }
  else {
    std::cerr << "Unsupported driver '" << driver << "'" << std::endl;
    return 1;
  }

  osmscout::DatabaseParameter databaseParameter;

  //databaseParameter.SetDebugPerformance(true);

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

  double dbMinTime=std::numeric_limits<double>::max();
  double dbMaxTime=0.0;
  double dbTotalTime=0.0;

  double drawMinTime=std::numeric_limits<double>::max();
  double drawMaxTime=0.0;
  double drawTotalTime=0.0;

  size_t nodeCountTotal=0;
  size_t wayCountTotal=0;
  size_t areaCountTotal=0;

  size_t tilesDrawn=0;

  for (uint32_t level=std::min(startZoom,endZoom);
       level<=std::max(startZoom,endZoom);
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

    std::cout << "----------" << std::endl;
    std::cout << "Drawing level " << level << ", " << (xTileCount)*(yTileCount) << " tiles [" << xTileStart << "," << yTileStart << " - " <<  xTileEnd << "," << yTileEnd << "]" << std::endl;

#if defined(HAVE_LIB_OSMSCOUTMAPCAIRO)
    osmscout::MapPainterCairo cairoMapPainter(styleConfig);
#endif
#if defined(HAVE_LIB_OSMSCOUTMAPQT)
    osmscout::MapPainterQt    qtMapPainter(styleConfig);
#endif
    osmscout::MapPainterNoOp  noOpMapPainter(styleConfig);

    for (int y=yTileStart; y<=yTileEnd; y++) {
      for (int x=xTileStart; x<=xTileEnd; x++) {
        osmscout::MapData  data;
        osmscout::GeoBox   boundingBox;
        osmscout::GeoCoord center;

        projection.Set(x-1,y-1,
                       x+1,y+1,
                       magnification,
                       DPI,
                       tileWidth,
                       tileHeight);

        projection.GetDimensions(boundingBox);

        center=boundingBox.GetCenter();

        std::cout << "Drawing tile with bounding box " << boundingBox.GetDisplayText() << " and center " << center.GetDisplayText() << std::endl;


        osmscout::StopClock dbTimer;

        mapService->GetObjects(searchParameter,
                               *styleConfig,
                               projection,
                               data);

        nodeCountTotal+=data.nodes.size();
        wayCountTotal+=data.ways.size();
        areaCountTotal+=data.areas.size();

        dbTimer.Stop();

        double dbTime=dbTimer.GetMilliseconds();

        dbMinTime=std::min(dbMinTime,dbTime);
        dbMaxTime=std::max(dbMaxTime,dbTime);
        dbTotalTime+=dbTime;

        osmscout::StopClock drawTimer;

#if defined(HAVE_LIB_OSMSCOUTMAPCAIRO)
        if (driver=="cairo") {
          //std::cout << data.nodes.size() << " " << data.ways.size() << " " << data.areas.size() << std::endl;
          cairoMapPainter.DrawMap(projection,
                                  drawParameter,
                                  data,
                                  cairo);
        }
#endif
#if defined(HAVE_LIB_OSMSCOUTMAPQT)
        if (driver=="Qt") {
          //std::cout << data.nodes.size() << " " << data.ways.size() << " " << data.areas.size() << std::endl;
          qtMapPainter.DrawMap(projection,
                               drawParameter,
                               data,
                               qtPainter);
        }
#endif
        if (driver=="noop") {
          noOpMapPainter.DrawMap(projection,
                                 drawParameter,
                                 data);
        }

        drawTimer.Stop();

        tilesDrawn++;

        double drawTime=drawTimer.GetMilliseconds();

        drawMinTime=std::min(drawMinTime,drawTime);
        drawMaxTime=std::max(drawMaxTime,drawTime);
        drawTotalTime+=drawTime;
      }
    }
  }

  std::cout << "==========" << std::endl;

  std::cout << "Data: ";
  std::cout << "nodes: " << nodeCountTotal << " ";
  std::cout << "way: " << wayCountTotal << " ";
  std::cout << "areas: " << areaCountTotal << std::endl;

  std::cout << "GetObjects: ";
  std::cout << "total: " << dbTotalTime << " msec ";
  std::cout << "min: " << dbMinTime << " msec ";
  std::cout << "avg: " << dbTotalTime/tilesDrawn << " msec ";
  std::cout << "max: " << dbMaxTime << " msec" << std::endl;

  std::cout << "DrawMap: ";
  std::cout << "total: " << drawTotalTime << " msec ";
  std::cout << "min: " << drawMinTime << " msec ";
  std::cout << "avg: " << drawTotalTime/tilesDrawn << " msec ";
  std::cout << "max: " << drawMaxTime << " msec" << std::endl;

  database->Close();

#if defined(HAVE_LIB_OSMSCOUTMAPCAIRO)
  if (driver=="cairo") {
    cairo_destroy(cairo);
    cairo_surface_destroy(cairoSurface);
  }
#endif

  return 0;
}
