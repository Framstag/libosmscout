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
#if defined(HAVE_LIB_OSMSCOUTMAPAGG)
#include <osmscout/MapPainterAgg.h>
#endif
#if defined(HAVE_LIB_OSMSCOUTMAPOPENGL)
#include <osmscout/MapPainterOpenGL.h>
#include <GLFW/glfw3.h>
#endif

#if defined(HAVE_LIB_GPERFTOOLS)
#include <gperftools/tcmalloc.h>
#include <gperftools/heap-profiler.h>
#include <malloc.h> // mallinfo
#else
#if defined(HAVE_MALLINFO)
#include <malloc.h> // mallinfo
#endif
#endif

#include <osmscout/MapPainterNoOp.h>

#include <osmscout/system/Math.h>

#include <osmscout/util/StopClock.h>
#include <osmscout/util/Tiling.h>

/*
  Example for the nordrhein-westfalen.osm (to be executed in the Demos top
  level directory), drawing the "Ruhrgebiet":

  src/PerformanceTest ../maps/nordrhein-westfalen ../stylesheets/standard.oss 51.4 7.3 51.6 7.7 10 15 256 256 cairo
*/

// See http://wiki.openstreetmap.org/wiki/Slippy_map_tilenames for details about
// coordinate transformation

static const double DPI=96.0;

struct LevelStats
{
  size_t level;

  double dbMinTime;
  double dbMaxTime;
  double dbTotalTime;

  double drawMinTime;
  double drawMaxTime;
  double drawTotalTime;

  double allocMax;
  double allocSum;

  size_t nodeCount;
  size_t wayCount;
  size_t areaCount;

  size_t tileCount;

  LevelStats(size_t level)
  : level(level),
    dbMinTime(std::numeric_limits<double>::max()),
    dbMaxTime(0.0),
    dbTotalTime(0.0),
    drawMinTime(std::numeric_limits<double>::max()),
    drawMaxTime(0.0),
    drawTotalTime(0.0),
    allocMax(0.0),
    allocSum(0.0),
    nodeCount(0),
    wayCount(0),
    areaCount(0),
    tileCount(0)
  {
    // no code
  }
};

std::string formatAlloc(double size)
{
    std::string units = " bytes";
    if (size > 1024){
        units = " KiB";
        size = size / 1024;
    }
    if (size > 1024){
        units = " MiB";
        size = size / 1024;
    }
    std::ostringstream buff;
    buff << std::setprecision(6) << size << units;
    return buff.str();
}

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

#if defined(HAVE_LIB_GPERFTOOLS)
  bool          heapProfile;
  std::string   heapProfilePrefix;
#endif

  if (argc<12) {
    std::cerr << "DrawMap " << std::endl;
    std::cerr << "  <map directory> <style-file> " << std::endl;
    std::cerr << "  <lat_top> <lon_left> <lat_bottom> <lon_right> " << std::endl;
    std::cerr << "  <start zoom> <end zoom>" << std::endl;
    std::cerr << "  <tile width> <tile height>" << std::endl;
    std::cerr << "  <cairo|Qt|ag|opengl|noop|none>" << std::endl;
#if defined(HAVE_LIB_GPERFTOOLS)
    std::cerr << "  [heap profile prefix]" << std::endl;
#endif
    return 1;
  }

#if defined(HAVE_LIB_GPERFTOOLS)
  heapProfile = false;

  if (argc>12) {
      heapProfile = true;
      heapProfilePrefix = argv[12];
  }
#endif

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

#if defined(HAVE_LIB_OSMSCOUTMAPAGG)
  unsigned char* buffer;
  agg::rendering_buffer* rbuf;
  osmscout::MapPainterAgg::AggPixelFormat* pf;
#endif

#if defined(HAVE_LIB_OSMSCOUTMAPOPENGL)

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
  } else if (driver == "agg") {
    std::cout << "Using driver 'Agg'..." << std::endl;
#if defined(HAVE_LIB_OSMSCOUTMAPAGG)
    buffer = new unsigned char[tileWidth * tileHeight * 3];
    rbuf = new agg::rendering_buffer(buffer, tileWidth, tileHeight, tileWidth * 3);
    pf = new osmscout::MapPainterAgg::AggPixelFormat(*rbuf);
#else
    std::cerr << "Driver 'Agg' is not enabled" << std::endl;
    return 1;
#endif
  } else if (driver == "opengl") {
    std::cout << "Using driver 'OpenGL'..." << std::endl;
#if defined(HAVE_LIB_OSMSCOUTMAPOPENGL)
    // Create the offscreen renderer
    glfwSetErrorCallback([](int, const char *err_str) {
      std::cerr << "GLFW Error: " << err_str << std::endl;
    });
    if (!glfwInit())
      return 1;
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_VISIBLE, false);
    GLFWwindow* offscreen_context = glfwCreateWindow(tileWidth, tileHeight, "", NULL, NULL);
    if (!offscreen_context) {
      std::cerr << "Failed to create offscreen context." << std::endl;
      return 1;
    }
    glfwMakeContextCurrent(offscreen_context);
#else
    std::cerr << "Driver 'OpenGL' is not enabled" << std::endl;
    return 1;
#endif
  }
  else if (driver=="noop") {
    std::cout << "Using driver 'noop'..." << std::endl;
  }
  else if (driver=="none") {
    std::cout << "Using driver 'none'..." << std::endl;
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
    return 1;
  }

#if defined(HAVE_LIB_GPERFTOOLS)
  if (heapProfile){
    HeapProfilerStart(heapProfilePrefix.c_str());
  }
#endif

  osmscout::TileProjection      projection;
  osmscout::MapParameter        drawParameter;
  osmscout::AreaSearchParameter searchParameter;
  std::list<LevelStats>         statistics;

  // TODO: Use some way to find a valid font on the system (Agg display a ton of messages otherwise)
  drawParameter.SetFontName("/usr/share/fonts/TTF/DejaVuSans.ttf");
  searchParameter.SetUseMultithreading(true);

  for (uint32_t level=std::min(startZoom,endZoom);
       level<=std::max(startZoom,endZoom);
       level++) {
    LevelStats              stats(level);
    osmscout::Magnification magnification;

    magnification.SetLevel(level);

    osmscout::OSMTileId     tileA(osmscout::OSMTileId::GetOSMTile(osmscout::GeoCoord(latBottom,lonLeft),
                                                                  magnification));
    osmscout::OSMTileId     tileB(osmscout::OSMTileId::GetOSMTile(osmscout::GeoCoord(latTop,lonRight),
                                                                  magnification));
    osmscout::OSMTileIdBox  tileArea(tileA,tileB);

    std::cout << "----------" << std::endl;
    std::cout << "Drawing level " << level << ", " << tileArea.GetCount() << " tiles " << tileArea.GetDisplayText() << std::endl;

#if defined(HAVE_LIB_OSMSCOUTMAPCAIRO)
    osmscout::MapPainterCairo cairoMapPainter(styleConfig);
#endif
#if defined(HAVE_LIB_OSMSCOUTMAPQT)
    osmscout::MapPainterQt    qtMapPainter(styleConfig);
#endif
#if defined(HAVE_LIB_OSMSCOUTMAPAGG)
    osmscout::MapPainterAgg aggMapPainter(styleConfig);
#endif
#if defined(HAVE_LIB_OSMSCOUTMAPOPENGL)
    osmscout::MapPainterOpenGL* openglMapPainter;
    if (driver == "opengl") // This driver need a valid existing context
      openglMapPainter =
            new osmscout::MapPainterOpenGL(tileWidth, tileHeight, DPI, tileWidth, tileHeight,
            "/usr/share/fonts/TTF/DejaVuSans.ttf");
#endif
    osmscout::MapPainterNoOp noOpMapPainter(styleConfig);

    size_t current=1;
    size_t tileCount=tileArea.GetCount();
    size_t delta=tileCount/20;

    if (delta==0) {
      delta=1;
    }

    for (const auto& tile : tileArea) {
      osmscout::MapData       data;
      osmscout::OSMTileIdBox  tileBox(osmscout::OSMTileId(tile.GetX()-1,tile.GetY()-1),
                                      osmscout::OSMTileId(tile.GetX()+1,tile.GetY()+1));
      osmscout::GeoBox        boundingBox;

      if ((current % delta)==0) {
        std::cout << current*100/tileCount << "% " << current;

        if (stats.tileCount>0) {
          std::cout << " " << stats.dbTotalTime/stats.tileCount;
          std::cout << " " << stats.drawTotalTime/stats.tileCount;
        }

        std::cout << std::endl;
      }

      projection.Set(tile,
                     magnification,
                     DPI,
                     tileWidth,
                     tileHeight);

      projection.GetDimensions(boundingBox);
      projection.SetLinearInterpolationUsage(level >= 10);

      osmscout::StopClock dbTimer;

      osmscout::GeoBox dataBoundingBox(tileBox.GetBoundingBox(magnification));

      std::list<osmscout::TileRef> tiles;

      // set cache size almost unlimited,
      // for better estimate of peak memory usage by tile loading
      mapService->SetCacheSize(10000000);

      mapService->LookupTiles(magnification,dataBoundingBox,tiles);
      mapService->LoadMissingTileData(searchParameter,*styleConfig,tiles);
      mapService->AddTileDataToMapData(tiles,data);

      stats.nodeCount+=data.nodes.size();
      stats.wayCount+=data.ways.size();
      stats.areaCount+=data.areas.size();


#if defined(HAVE_LIB_GPERFTOOLS)
      if (heapProfile){
          std::ostringstream buff;
          buff << "load-" << level << "-" << tile.GetX() << "-" << tile.GetY();
          HeapProfilerDump(buff.str().c_str());
      }
      struct mallinfo alloc_info = tc_mallinfo();
#else
#if defined(HAVE_MALLINFO)
      struct mallinfo alloc_info = mallinfo();
#endif
#endif
#if defined(HAVE_MALLINFO) || defined(HAVE_LIB_GPERFTOOLS)
      std::cout << "memory usage: " << formatAlloc(alloc_info.uordblks) << std::endl;
      stats.allocMax = std::max(stats.allocMax, (double)alloc_info.uordblks);
      stats.allocSum = stats.allocSum + (double)alloc_info.uordblks;
#endif

      // set cache size back to default
      mapService->SetCacheSize(25);
      dbTimer.Stop();

      double dbTime=dbTimer.GetMilliseconds();

      stats.dbMinTime=std::min(stats.dbMinTime,dbTime);
      stats.dbMaxTime=std::max(stats.dbMaxTime,dbTime);
      stats.dbTotalTime+=dbTime;

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
#if defined(HAVE_LIB_OSMSCOUTMAPAGG)
      if (driver == "agg") {
        //std::cout << data.nodes.size() << " " << data.ways.size() << " " << data.areas.size() << std::endl;
        aggMapPainter.DrawMap(projection,
                drawParameter,
                data,
                pf);
      }
#endif
#if defined(HAVE_LIB_OSMSCOUTMAPOPENGL)
      if (driver == "opengl") {
        //std::cout << data.nodes.size() << " " << data.ways.size() << " " << data.areas.size() << std::endl;
        openglMapPainter->ProcessData(data, drawParameter, projection, styleConfig);
        openglMapPainter->SwapData();
        openglMapPainter->DrawMap();
      }
#endif
      if (driver == "noop") {
        noOpMapPainter.DrawMap(projection,
                               drawParameter,
                               data);
      }
      if (driver=="none") {
        // Do nothing
      }

      drawTimer.Stop();

      stats.tileCount++;

      double drawTime=drawTimer.GetMilliseconds();

      stats.drawMinTime=std::min(stats.drawMinTime,drawTime);
      stats.drawMaxTime=std::max(stats.drawMaxTime,drawTime);
      stats.drawTotalTime+=drawTime;

      current++;
    }

    statistics.push_back(stats);
  }

#if defined(HAVE_LIB_GPERFTOOLS)
  if (heapProfile){
    HeapProfilerStop();
  }
#endif

  std::cout << "==========" << std::endl;

  for (const auto& stats : statistics) {
    std::cout << "Level: " << stats.level << std::endl;
    std::cout << "Tiles: " << stats.tileCount << std::endl;

#if defined(HAVE_MALLINFO) || defined(HAVE_LIB_GPERFTOOLS)
    std::cout << " Used memory: ";
    std::cout << "max: " << formatAlloc(stats.allocMax) << " ";
    std::cout << "avg: " << formatAlloc(stats.allocSum / stats.tileCount) << std::endl;
#endif

    std::cout << " Tot. data  : ";
    std::cout << "nodes: " << stats.nodeCount << " ";
    std::cout << "way: " << stats.wayCount << " ";
    std::cout << "areas: " << stats.areaCount << std::endl;

    if (stats.tileCount>0) {
      std::cout << " Avg. data  : ";
      std::cout << "nodes: " << stats.nodeCount/stats.tileCount << " ";
      std::cout << "way: " << stats.wayCount/stats.tileCount << " ";
      std::cout << "areas: " << stats.areaCount/stats.tileCount << std::endl;
    }

    std::cout << " DB         : ";
    std::cout << "total: " << stats.dbTotalTime << " ";
    std::cout << "min: " << stats.dbMinTime << " ";
    if (stats.tileCount>0) {
      std::cout << "avg: " << stats.dbTotalTime/stats.tileCount << " ";
    }
    std::cout << "max: " << stats.dbMaxTime << " " << std::endl;

    std::cout << " Map        : ";
    std::cout << "total: " << stats.drawTotalTime << " ";
    std::cout << "min: " << stats.drawMinTime << " ";
    if (stats.tileCount>0) {
      std::cout << "avg: " << stats.drawTotalTime/stats.tileCount << " ";
    }
    std::cout << "max: " << stats.drawMaxTime << std::endl;
  }

  database->Close();

#if defined(HAVE_LIB_OSMSCOUTMAPCAIRO)
  if (driver=="cairo") {
    cairo_destroy(cairo);
    cairo_surface_destroy(cairoSurface);
  }
#endif

  return 0;
}
