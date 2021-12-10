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
#include <tuple>

#include "config.h"

#include <osmscout/Database.h>
#include <osmscoutmap/MapService.h>

#if defined(HAVE_LIB_OSMSCOUTMAPCAIRO)
#include <osmscoutmapcairo/MapPainterCairo.h>
#endif
#if defined(HAVE_LIB_OSMSCOUTMAPQT)
#include <QApplication>
#include <QDesktopWidget>
#include <QPixmap>
#include <QScreen>
#include <osmscoutmapqt/MapPainterQt.h>
#endif
#if defined(HAVE_LIB_OSMSCOUTMAPAGG)
#include <osmscoutmapagg/MapPainterAgg.h>
#endif
#if defined(HAVE_LIB_OSMSCOUTMAPOPENGL)
#include <osmscoutmapopengl/MapPainterOpenGL.h>
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

#include <osmscoutmap/MapPainterNoOp.h>

#include <osmscout/system/Math.h>

#include <osmscout/util/CmdLineParsing.h>
#include <osmscout/util/StopClock.h>
#include <osmscout/util/Tiling.h>

/*
  Example for the nordrhein-westfalen.osm (to be executed in the Demos top
  level directory), drawing the "Ruhrgebiet":

  src/PerformanceTest ../maps/nordrhein-westfalen ../stylesheets/standard.oss 51.4 7.3 51.6 7.7 10 15 256 256 cairo
*/

// See http://wiki.openstreetmap.org/wiki/Slippy_map_tilenames for details about
// coordinate transformation

struct Arguments {
  bool help{false};
  bool debug{false};
  std::string databaseDirectory{"."};
  std::string style{"stylesheets/standard.oss"};
  osmscout::GeoCoord coordTopLeft;
  osmscout::GeoCoord coordBottomRight;
  osmscout::MagnificationLevel startZoom{0};
  osmscout::MagnificationLevel endZoom{20};
  std::tuple<size_t, size_t> tileDimension{std::make_tuple(256, 256)};
  std::string driver{"none"};
  // TODO: Use some way to find a valid font on the system (Agg display a ton of messages otherwise)
  std::string font{"/usr/share/fonts/TTF/DejaVuSans.ttf"};
  std::list<std::string> icons;
  double dpi{96};
  size_t drawRepeat{1};
  size_t loadRepeat{1};
  bool flushCache{false};
  bool flushDiskCache{false};

#if defined(HAVE_LIB_GPERFTOOLS)
  bool heapProfile{false};
  std::string heapProfilePrefix;
#endif

  size_t TileWidth() const
  {
    return std::get<0>(tileDimension);
  }

  size_t TileHeight() const
  {
    return std::get<1>(tileDimension);
  }

  double LatBottom() const
  {
    return coordBottomRight.GetLat();
  }

  double LonLeft() const
  {
    return coordTopLeft.GetLon();
  }

  double LatTop() const
  {
    return coordTopLeft.GetLat();
  }

  double LonRight() const
  {
    return coordBottomRight.GetLon();
  }
};

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

  explicit LevelStats(size_t level)
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

class PerformanceTestBackend {
public:
  virtual ~PerformanceTestBackend() = default;

  virtual void DrawMap(const osmscout::TileProjection &/*projection*/,
                       const osmscout::MapParameter &/*drawParameter*/,
                       const osmscout::MapData &/*data*/)
  {
    // none
  }
};

#if defined(HAVE_LIB_OSMSCOUTMAPCAIRO)
class PerformanceTestBackendCairo: public PerformanceTestBackend {
private:
  cairo_surface_t *cairoSurface = nullptr;
  cairo_t *cairo = nullptr;
  osmscout::MapPainterCairo *cairoMapPainter{nullptr};

public:
  PerformanceTestBackendCairo(size_t tileWidth, size_t tileHeight,
                              const osmscout::StyleConfigRef& styleConfig)
  {
    cairoSurface=cairo_image_surface_create(CAIRO_FORMAT_RGB24,tileWidth,tileHeight);
    if (cairoSurface==nullptr) {
      throw std::runtime_error("Cannot create cairo image cairoSurface");
    }

    cairo=cairo_create(cairoSurface);
    if (cairo==nullptr) {
      cairo_surface_destroy(cairoSurface);
      throw std::runtime_error("Cannot create cairo_t for image cairoSurface");
    }
    cairoMapPainter = new osmscout::MapPainterCairo(styleConfig);
  }

  ~PerformanceTestBackendCairo()
  {
    cairo_destroy(cairo);
    cairo_surface_destroy(cairoSurface);
  }

  void DrawMap(const osmscout::TileProjection &projection,
               const osmscout::MapParameter &drawParameter,
               const osmscout::MapData &data) override
  {
    cairoMapPainter->DrawMap(projection,
                            drawParameter,
                            data,
                            cairo);
  }
};
#endif

#if defined(HAVE_LIB_OSMSCOUTMAPQT)
class PerformanceTestBackendQt: public PerformanceTestBackend {
private:
  QApplication application;
  QPixmap qtPixmap;
  QPainter qtPainter;
  osmscout::MapPainterQt qtMapPainter;
public:
  PerformanceTestBackendQt(int argc, char* argv[], int tileWidth, int tileHeight,
                           const osmscout::StyleConfigRef& styleConfig):
    application(argc, argv, true),
    qtPixmap{tileWidth,tileHeight},
    qtPainter{&qtPixmap},
    qtMapPainter{styleConfig}
  {
  }

  void DrawMap(const osmscout::TileProjection &projection,
                       const osmscout::MapParameter &drawParameter,
                       const osmscout::MapData &data) override
  {
    qtMapPainter.DrawMap(projection,
                         drawParameter,
                         data,
                         &qtPainter);
  }
};
#endif

#if defined(HAVE_LIB_OSMSCOUTMAPAGG)
class PerformanceTestBackendAGG: public PerformanceTestBackend {
private:
  unsigned char*                           buffer=nullptr;
  agg::rendering_buffer*                   rbuf=nullptr;
  osmscout::MapPainterAgg::AggPixelFormat* pf=nullptr;
  osmscout::MapPainterAgg aggMapPainter;
public:
  PerformanceTestBackendAGG(size_t tileWidth, size_t tileHeight, const osmscout::StyleConfigRef& styleConfig):
    buffer{new unsigned char[tileWidth * tileHeight * 3]},
    rbuf{new agg::rendering_buffer(buffer, tileWidth, tileHeight, tileWidth * 3)},
    pf{new osmscout::MapPainterAgg::AggPixelFormat(*rbuf)},
    aggMapPainter{styleConfig}
  {
  }

  ~PerformanceTestBackendAGG()
  {
    delete pf;
    delete rbuf;
  }

  void DrawMap(const osmscout::TileProjection &projection,
               const osmscout::MapParameter &drawParameter,
               const osmscout::MapData &data) override
  {
    aggMapPainter.DrawMap(projection,
                          drawParameter,
                          data,
                          pf);
  }
};
#endif

#if defined(HAVE_LIB_OSMSCOUTMAPOPENGL)
class PerformanceTestBackendOGL: public PerformanceTestBackend {
private:
  osmscout::MapPainterOpenGL* openglMapPainter{nullptr};
  osmscout::StyleConfigRef styleConfig;
public:
  PerformanceTestBackendOGL(size_t tileWidth, size_t tileHeight, size_t dpi, osmscout::StyleConfigRef styleConfig):
    styleConfig{styleConfig}
  {
    // Create the offscreen renderer
    glfwSetErrorCallback([](int, const char *err_str) {
      std::cerr << "GLFW Error: " << err_str << std::endl;
    });
    if (!glfwInit())
      throw std::runtime_error("Can't initilise GLFW library");
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_VISIBLE, false);
    GLFWwindow* offscreen_context;
    offscreen_context=glfwCreateWindow(tileWidth,
                                       tileHeight,
                                       "",
                                       nullptr,
                                       nullptr);
    if (!offscreen_context) {
      throw std::runtime_error("Failed to create offscreen context.");
    }
    glfwMakeContextCurrent(offscreen_context);

    // This driver need a valid existing context
    openglMapPainter=new osmscout::MapPainterOpenGL(tileWidth,
                                                    tileHeight,
                                                    dpi,
                                                    tileWidth,
                                                    tileHeight,
                                                    "/usr/share/fonts/TTF/DejaVuSans.ttf",
                                                    SHADER_INSTALL_DIR);
  }

  ~PerformanceTestBackendOGL()
  {
    delete openglMapPainter;
  }

  void DrawMap(const osmscout::TileProjection &projection,
               const osmscout::MapParameter &drawParameter,
               const osmscout::MapData &data) override
  {
    openglMapPainter->ProcessData(data, drawParameter, projection, styleConfig);
    openglMapPainter->SwapData();
    openglMapPainter->DrawMap();
  }
};
#endif

class PerformanceTestBackendNoOp: public PerformanceTestBackend {
private:
  osmscout::MapPainterNoOp noOpMapPainter;
public:
  explicit PerformanceTestBackendNoOp(const osmscout::StyleConfigRef& styleConfig)
  : noOpMapPainter(styleConfig)
  {}

  void DrawMap(const osmscout::TileProjection &projection,
               const osmscout::MapParameter &drawParameter,
               const osmscout::MapData &data) override
  {
    noOpMapPainter.DrawMap(projection,
                           drawParameter,
                           data);
  }
};

using PerformanceTestBackendPtr = std::shared_ptr<PerformanceTestBackend>;

PerformanceTestBackendPtr PrepareBackend(int argc, char* argv[], const Arguments &args, osmscout::StyleConfigRef styleConfig)
{
  if (args.driver=="cairo") {
    std::cout << "Using driver 'cairo'..." << std::endl;
#if defined(HAVE_LIB_OSMSCOUTMAPCAIRO)
    try{
      return std::make_shared<PerformanceTestBackendCairo>(args.TileWidth(),args.TileHeight(),styleConfig);
    } catch (std::runtime_error &e){
      std::cerr << e.what() << std::endl;
      return nullptr;
    }
#else
    std::cerr << "Driver 'cairo' is not enabled" << std::endl;
    return nullptr;
#endif
  }
  else if (args.driver=="Qt") {
    std::cout << "Using driver 'Qt'..." << std::endl;
#if defined(HAVE_LIB_OSMSCOUTMAPQT)
    return std::make_shared<PerformanceTestBackendQt>(argc, argv, args.TileWidth(), args.TileHeight(), styleConfig);
#else
    std::cerr << "Driver 'Qt' is not enabled" << std::endl;
    return nullptr;
#endif
  } else if (args.driver == "agg") {
    std::cout << "Using driver 'Agg'..." << std::endl;
#if defined(HAVE_LIB_OSMSCOUTMAPAGG)
    return std::make_shared<PerformanceTestBackendAGG>(args.TileWidth(), args.TileHeight(), styleConfig);
#else
    std::cerr << "Driver 'Agg' is not enabled" << std::endl;
    return nullptr;
#endif
  } else if (args.driver == "opengl") {
    std::cout << "Using driver 'OpenGL'..." << std::endl;
#if defined(HAVE_LIB_OSMSCOUTMAPOPENGL)
    try{
      return std::make_shared<PerformanceTestBackendOGL>(args.TileWidth(), args.TileHeight(), args.dpi, styleConfig);
    } catch (std::runtime_error &e){
      std::cerr << e.what() << std::endl;
      return nullptr;
    }
#else
    std::cerr << "Driver 'OpenGL' is not enabled" << std::endl;
    return nullptr;
#endif
  }
  else if (args.driver=="noop") {
    std::cout << "Using driver 'noop'..." << std::endl;
    return std::make_shared<PerformanceTestBackendNoOp>(styleConfig);
  }
  else if (args.driver=="none") {
    std::cout << "Using driver 'none'..." << std::endl;
    return std::make_shared<PerformanceTestBackend>();
  }
  else {
    std::cerr << "Unsupported driver '" << args.driver << "'" << std::endl;
    return nullptr;
  }
}

int main(int argc, char* argv[])
{
  osmscout::CmdLineParser     argParser("PerformanceTest",
                                        argc,argv);
  Arguments                   args;
  osmscout::DatabaseParameter databaseParameter;

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
  argParser.AddOption(osmscout::CmdLineUIntOption([&args](const unsigned int& value) {
                        args.startZoom=osmscout::MagnificationLevel(value);
                      }),
                      "start-zoom",
                      "Start zoom, default: " + std::to_string(args.startZoom.Get()),
                      false);
  argParser.AddOption(osmscout::CmdLineUIntOption([&args](const unsigned int& value) {
                        args.endZoom=osmscout::MagnificationLevel(value);
                      }),
                      "end-zoom",
                      "End zoom, default: " + std::to_string(args.endZoom.Get()),
                      false);
  argParser.AddOption(osmscout::CmdLineUIntOption([&args](const unsigned int& value) {
                        args.tileDimension=std::make_tuple(value, std::get<1>(args.tileDimension));
                      }),
                      "tile-width",
                      "Tile width, default: " + std::to_string(std::get<0>(args.tileDimension)),
                      false);
  argParser.AddOption(osmscout::CmdLineUIntOption([&args](const unsigned int& value) {
                        args.tileDimension=std::make_tuple(std::get<0>(args.tileDimension), value);
                      }),
                      "tile-height",
                      "Tile height, default: " + std::to_string(std::get<1>(args.tileDimension)),
                      false);
  argParser.AddOption(osmscout::CmdLineStringOption([&args](const std::string& value) {
                        args.driver = value;
                      }),
                      "driver",
                      "Rendering driver (cairo|Qt|agg|opengl|noop|none), default: " + args.driver,
                      false);
  argParser.AddOption(osmscout::CmdLineDoubleOption([&args](const double& value) {
                        if (value > 0) {
                          args.dpi = value;
                        } else {
                          std::cerr << "DPI can't be negative or zero" << std::endl;
                        }
                      }),
                      "dpi",
                      "Rendering DPI, default: " + std::to_string(args.dpi),
                      false);
  argParser.AddOption(osmscout::CmdLineUIntOption([&args](const unsigned int& value) {
                        args.drawRepeat = value;
                      }),
                      "draw-repeat",
                      "Repeat every draw call, default: " + std::to_string(args.drawRepeat),
                      false);
  argParser.AddOption(osmscout::CmdLineUIntOption([&args](const unsigned int& value) {
                        args.loadRepeat = value;
                      }),
                      "load-repeat",
                      "Repeat every load call, default: " + std::to_string(args.loadRepeat),
                      false);
  argParser.AddOption(osmscout::CmdLineFlag([&args](const bool& value) {
                        args.flushCache=value;
                      }),
                      "flush-cache",
                      "Flush data caches after each data load, default: " + std::to_string(args.flushCache),
                      false);
  argParser.AddOption(osmscout::CmdLineFlag([&args](const bool& value) {
                        args.flushDiskCache=value;
                      }),
                      "flush-disk",
                      "Flush system disk caches after each data load, default: " + std::to_string(args.flushDiskCache) +
                      " (It work just on Linux with admin rights.)",
                      false);

  argParser.AddOption(osmscout::CmdLineUIntOption([&databaseParameter](const unsigned int& value) {
                        databaseParameter.SetNodeDataCacheSize(value);
                      }),
                      "cache-nodes",
                      "Cache size for nodes, default: " + std::to_string(databaseParameter.GetNodeDataCacheSize()),
                      false);
  argParser.AddOption(osmscout::CmdLineUIntOption([&databaseParameter](const unsigned int& value) {
                        databaseParameter.SetWayDataCacheSize(value);
                      }),
                      "cache-ways",
                      "Cache size for ways, default: " + std::to_string(databaseParameter.GetWayDataCacheSize()),
                      false);
  argParser.AddOption(osmscout::CmdLineUIntOption([&databaseParameter](const unsigned int& value) {
                        databaseParameter.SetAreaDataCacheSize(value);
                      }),
                      "cache-areas",
                      "Cache size for areas, default: " + std::to_string(databaseParameter.GetAreaDataCacheSize()),
                      false);
  argParser.AddOption(osmscout::CmdLineStringOption([&args](const std::string& value) {
                        args.icons.push_back(value);
                      }),
                      "icons",
                      "Directory with (SVG) icons",
                      false);
  argParser.AddOption(osmscout::CmdLineStringOption([&args](const std::string& value) {
                        args.font=value;
                      }),
                      "font",
                      "Used font, default: " + args.font,
                      false);

#if defined(HAVE_LIB_GPERFTOOLS)
  argParser.AddOption(osmscout::CmdLineStringOption([&args](const std::string& value) {
                        args.heapProfilePrefix = value;
                        args.heapProfile = !args.heapProfilePrefix.empty();
                      }),
                      "heap-profile",
                      "GPerf heap profile prefix, profiler is disabled by default",
                      false);
#endif

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

  osmscout::CmdLineParseResult argResult=argParser.Parse();
  if (argResult.HasError()) {
    std::cerr << "ERROR: " << argResult.GetErrorDescription() << std::endl;
    std::cout << argParser.GetHelp() << std::endl;
    return 1;
  }
  else if (args.help) {
    std::cout << argParser.GetHelp() << std::endl;
    return 0;
  }

  osmscout::log.Debug(args.debug);
  //databaseParameter.SetDebugPerformance(true);

  osmscout::DatabaseRef       database=std::make_shared<osmscout::Database>(databaseParameter);
  osmscout::MapServiceRef     mapService=std::make_shared<osmscout::MapService>(database);

  if (!database->Open(args.databaseDirectory)) {
    std::cerr << "Cannot open database" << std::endl;
    return 1;
  }

  osmscout::StyleConfigRef styleConfig=std::make_shared<osmscout::StyleConfig>(database->GetTypeConfig());

  if (!styleConfig->Load(args.style)) {
    std::cerr << "Cannot open style" << std::endl;
    return 1;
  }

  PerformanceTestBackendPtr backendPtr = PrepareBackend(argc, argv, args, styleConfig);
  if (!backendPtr){
    return 1;
  }

#if defined(HAVE_LIB_GPERFTOOLS)
  if (args.heapProfile){
    HeapProfilerStart(args.heapProfilePrefix.c_str());
  }
#endif

  osmscout::TileProjection      projection;
  osmscout::MapParameter        drawParameter;
  osmscout::AreaSearchParameter searchParameter;
  std::list<LevelStats>         statistics;

  drawParameter.SetFontName(args.font);
  drawParameter.SetIconPaths(args.icons);
  drawParameter.SetPatternPaths(args.icons); // for simplicity use same directories for lookup
  drawParameter.SetIconMode(osmscout::MapParameter::IconMode::Scalable);
  drawParameter.SetPatternMode(osmscout::MapParameter::PatternMode::Scalable);
  searchParameter.SetUseMultithreading(true);

  for (osmscout::MagnificationLevel level=osmscout::MagnificationLevel(std::min(args.startZoom,args.endZoom));
       level<=osmscout::MagnificationLevel(std::max(args.startZoom,args.endZoom));
       level++) {
    LevelStats              stats(level.Get());
    osmscout::Magnification magnification(level);

    osmscout::OSMTileId     tileA(osmscout::OSMTileId::GetOSMTile(magnification,
                                                                  osmscout::GeoCoord(args.LatBottom(),args.LonLeft())));
    osmscout::OSMTileId     tileB(osmscout::OSMTileId::GetOSMTile(magnification,
                                                                  osmscout::GeoCoord(args.LatTop(),args.LonRight())));
    osmscout::OSMTileIdBox  tileArea(tileA,tileB);

    std::cout << "----------" << std::endl;
    std::cout << "Drawing level " << level << ", " << tileArea.GetCount() << " tiles " << tileArea.GetDisplayText() << std::endl;

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
                     args.dpi,
                     args.TileWidth(),
                     args.TileHeight());

      projection.GetDimensions(boundingBox);
      projection.SetLinearInterpolationUsage(level.Get() >= 10);

      for (size_t i=0; i<args.loadRepeat; i++) {
        data.nodes.clear();
        data.ways.clear();
        data.areas.clear();

        osmscout::StopClock dbTimer;

        osmscout::GeoBox dataBoundingBox(tileBox.GetBoundingBox(magnification));

        std::list<osmscout::TileRef> tiles;

        // set cache size almost unlimited,
        // for better estimate of peak memory usage by tile loading
        mapService->SetCacheSize(10000000);

        mapService->LookupTiles(magnification, dataBoundingBox, tiles);
        mapService->LoadMissingTileData(searchParameter, *styleConfig, tiles);
        mapService->AddTileDataToMapData(tiles, data);

#if defined(HAVE_LIB_GPERFTOOLS)
        if (args.heapProfile) {
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
        stats.allocMax = std::max(stats.allocMax, (double) alloc_info.uordblks);
        stats.allocSum = stats.allocSum + (double) alloc_info.uordblks;
#endif

        // set cache size back to default
        mapService->SetCacheSize(25);
        dbTimer.Stop();

        double dbTime = dbTimer.GetMilliseconds();

        stats.dbMinTime = std::min(stats.dbMinTime, dbTime);
        stats.dbMaxTime = std::max(stats.dbMaxTime, dbTime);
        stats.dbTotalTime += dbTime;

        if (args.flushCache) {
          tiles.clear(); // following flush method removes only tiles with use_count() == 1
          mapService->FlushTileCache();

          // simplest way howto flush database caches is close it and open again
          database->Close();
          if (!database->Open(args.databaseDirectory)) {
            std::cerr << "Cannot open database" << std::endl;
            return 1;
          }
        }
        if (args.flushDiskCache) {
          // Linux specific
          if (osmscout::ExistsInFilesystem("/proc/sys/vm/drop_caches")){
            osmscout::FileWriter f;
            try {
              f.Open("/proc/sys/vm/drop_caches");
              f.Write(std::string("3"));
              f.Close();
            }catch(const osmscout::IOException &e){
              std::cerr << "Can't flush disk cache: " << e.what() << std::endl;
            }
          }else{
            std::cerr << "Can't flush disk cache, \"/proc/sys/vm/drop_caches\" file don't exists" << std::endl;
          }
        }
      }

      stats.nodeCount+=data.nodes.size();
      stats.wayCount+=data.ways.size();
      stats.areaCount+=data.areas.size();

      stats.tileCount++;
      for (size_t i=0; i<args.drawRepeat; i++) {
        osmscout::StopClock drawTimer;
        backendPtr->DrawMap(projection, drawParameter, data);
        drawTimer.Stop();

        double drawTime = drawTimer.GetMilliseconds();

        stats.drawMinTime = std::min(stats.drawMinTime, drawTime);
        stats.drawMaxTime = std::max(stats.drawMaxTime, drawTime);
        stats.drawTotalTime += drawTime;
      }

      current++;
    }

    statistics.push_back(stats);
  }

#if defined(HAVE_LIB_GPERFTOOLS)
  if (args.heapProfile){
    HeapProfilerStop();
  }
#endif

  std::cout << "==========" << std::endl;

  for (const auto& stats : statistics) {
    std::cout << "Level: " << stats.level << std::endl;
    std::cout << "Tiles: " << stats.tileCount << " (load " << args.loadRepeat << "x, drawn " << args.drawRepeat << "x)" << std::endl;

#if defined(HAVE_MALLINFO) || defined(HAVE_LIB_GPERFTOOLS)
    std::cout << " Used memory: ";
    std::cout << "max: " << formatAlloc(stats.allocMax) << " ";
    std::cout << "avg: " << formatAlloc(stats.allocSum / (stats.tileCount * args.loadRepeat)) << std::endl;
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
      std::cout << "avg: " << stats.dbTotalTime/(stats.tileCount * args.loadRepeat) << " ";
    }
    std::cout << "max: " << stats.dbMaxTime << " " << std::endl;

    std::cout << " Map        : ";
    std::cout << "total: " << stats.drawTotalTime << " ";
    std::cout << "min: " << stats.drawMinTime << " ";
    if (stats.tileCount>0) {
      std::cout << "avg: " << stats.drawTotalTime/(stats.tileCount * args.drawRepeat) << " ";
    }
    std::cout << "max: " << stats.drawMaxTime << std::endl;
  }

  database->Close();

  return 0;
}
