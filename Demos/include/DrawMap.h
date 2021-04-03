#ifndef DEMO_LIBOSMSCOUT_DRAWMAP_H
#define DEMO_LIBOSMSCOUT_DRAWMAP_H

/*
  DrawMap - a part of demo programs for libosmscout
  Copyright (C) 2009-2019  Tim Teulings
  Copyright (C) 2019  Lukas Karas

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

#include <osmscout/util/CmdLineParsing.h>

#include <osmscout/Database.h>
#include <osmscout/MapService.h>
#include <osmscout/BasemapDatabase.h>

#include <iostream>

struct Arguments {
  bool       help=false;
#ifdef NDEBUG
  bool       debug=false;
#else
  bool       debug=true;
#endif
  double     dpi=96.0;

  bool renderContourLines=false;
  bool renderHillShading=false;

  std::string map;
  std::string style;
  std::string output;
  size_t      width=1920;
  size_t      height=1080;

  std::string basemap;
  std::string srtmDirectory;

  osmscout::GeoCoord       center;
  osmscout::Magnification  zoom{osmscout::Magnification::magClose};

  osmscout::MapParameter::IconMode iconMode{osmscout::MapParameter::IconMode::FixedSizePixmap};
  std::list<std::string> iconPaths;

  double fontSize{3.0};
#ifdef _WIN32
  std::string fontName{ "C:\\Windows\\Fonts\\arial.ttf" };
#else
  std::string fontName{"/usr/share/fonts/TTF/LiberationSans-Regular.ttf"};
#endif
};

enum DrawMapArgParserWindowStyle
{
  ARG_WS_CONSOLE,
  ARG_WS_WINDOW
};

class DrawMapArgParser: public osmscout::CmdLineParser
{
private:
  Arguments args;

public:
  DrawMapArgParser(const std::string& appName,
                   int argc, char* argv[],
                   double dpi,
                   DrawMapArgParserWindowStyle windowStyle=ARG_WS_CONSOLE)
                   : osmscout::CmdLineParser(appName, argc, argv)
  {
    args.dpi = dpi;

    AddOption(osmscout::CmdLineFlag([this](const bool& value) {
                args.help=value;
              }),
              std::vector<std::string>{"h","help"},
              "Display help",
              true);
    AddOption(osmscout::CmdLineFlag([this](const bool& value) {
                args.debug=value;
              }),
              "debug",
              "Enable debug output",
              false);
    AddOption(osmscout::CmdLineDoubleOption([this](const double& value) {
                args.dpi=value;
              }),
              "dpi",
              "Rendering DPI (" + std::to_string(args.dpi) + ")",
              false);
    AddOption(osmscout::CmdLineStringOption([this](const std::string& value) {
                args.fontName = value;
              }),
              "fontName",
              "Rendering font (" + args.fontName + ")",
              false);
    AddOption(osmscout::CmdLineDoubleOption([this](const double& value) {
                args.fontSize=value;
              }),
              "fontSize",
              "Rendering font size (" + std::to_string(args.fontSize) + ")",
              false);
    AddOption(osmscout::CmdLineStringOption([this](const std::string& value) {
                args.iconPaths.push_back(value);
              }),
              "iconPath",
              "Icon lookup directory",
              false);
    AddOption(osmscout::CmdLineStringOption([this](const std::string& value) {

                if (value == "FixedSizePixmap"){
                  args.iconMode = osmscout::MapParameter::IconMode::FixedSizePixmap;
                } else if (value == "ScaledPixmap"){
                  args.iconMode = osmscout::MapParameter::IconMode::ScaledPixmap;
                } else if (value == "OriginalPixmap"){
                  args.iconMode = osmscout::MapParameter::IconMode::OriginalPixmap;
                } else if (value == "Scalable"){
                  args.iconMode = osmscout::MapParameter::IconMode::Scalable;
                } else {
                  std::cerr << "Unknown icon mode " << value << std::endl;
                }
              }),
              "iconMode",
              "FixedSizePixmap | ScaledPixmap | OriginalPixmap | Scalable",
              false);
    AddOption(osmscout::CmdLineStringOption([this](const std::string& value) {
                args.basemap=value;
              }),
              "baseMap",
              "Directory with world base map",
              false);
    AddOption(osmscout::CmdLineBoolOption([this](const bool& value) {
                args.renderHillShading=value;
              }),
              "hillshading",
              "Enable hillshading",
              false);
    AddOption(osmscout::CmdLineBoolOption([this](const bool& value) {
                args.renderContourLines=value;
              }),
              "contourlines",
              "Enable contour lines",
              false);
    if (windowStyle == ARG_WS_WINDOW) {
        AddOption(osmscout::CmdLineSizeTOption([this](const size_t& value) {
                    args.width = value;
                  }),
                  "width",
                  "Image width");
        AddOption(osmscout::CmdLineSizeTOption([this](const size_t& value) {
                    args.height = value;
                  }),
                  "height",
                  "Image height");
    }
    AddOption(osmscout::CmdLineStringOption([this](const std::string& value) {
                args.srtmDirectory=value;
              }),
              "srtmDirectory",
              "SRTM data lookup directory",
              false);

    AddPositional(osmscout::CmdLineStringOption([this](const std::string& value) {
                    args.map=value;
                  }),
                  "databaseDir",
                  "Database directory");
    AddPositional(osmscout::CmdLineStringOption([this](const std::string& value) {
                    args.style=value;
                  }),
                  "stylesheet",
                  "Map stylesheet");
    if (windowStyle == ARG_WS_CONSOLE) {
        AddPositional(osmscout::CmdLineSizeTOption([this](const size_t& value) {
                        args.width=value;
                      }),
                      "width",
                      "Image width");
        AddPositional(osmscout::CmdLineSizeTOption([this](const size_t& value) {
                        args.height=value;
                      }),
                      "height",
                      "Image height");
    }
    AddPositional(osmscout::CmdLineGeoCoordOption([this](const osmscout::GeoCoord& coord) {
                    args.center = coord;
                  }),
                  "lat lon",
                  "Rendering center");
    AddPositional(osmscout::CmdLineDoubleOption([this](const double& value) {
                    args.zoom.SetMagnification(value);
                  }),
                  "zoom",
                  "Rendering zoom");
    if (windowStyle == ARG_WS_CONSOLE) {
        AddPositional(osmscout::CmdLineStringOption([this](const std::string& value) {
                        args.output=value;
                      }),
                      "output",
                      "Image output");
    }
  }

  Arguments GetArguments() const
  {
    return args;
  }
};

class DrawMapDemo
{
public:
  DrawMapArgParser argParser;

  osmscout::DatabaseParameter databaseParameter;
  osmscout::DatabaseRef       database;
  osmscout::MapServiceRef     mapService;
  osmscout::StyleConfigRef    styleConfig;

  osmscout::BasemapDatabaseRef basemapDatabase;

  osmscout::MercatorProjection  projection;
  osmscout::MapParameter        drawParameter;
  osmscout::AreaSearchParameter searchParameter;
  osmscout::MapData             data;

public:
  DrawMapDemo(const std::string& appName,
              int argc, char* argv[],
              double dpi=96.0,
              DrawMapArgParserWindowStyle windowStyle=ARG_WS_CONSOLE):
      argParser(appName, argc, argv, dpi, windowStyle)
  {

  }

  bool OpenDatabase()
  {
    osmscout::CmdLineParseResult argResult=argParser.Parse();
    if (argResult.HasError()) {
      std::cerr << "ERROR: " << argResult.GetErrorDescription() << std::endl;
      std::cout << argParser.GetHelp() << std::endl;
      return false;
    }

    Arguments args=argParser.GetArguments();
    if (args.help) {
      std::cout << argParser.GetHelp() << std::endl;
      return false;
    }

    osmscout::log.Debug(args.debug);

    if (!args.srtmDirectory.empty()) {
      databaseParameter.SetSRTMDirectory(args.srtmDirectory);
    }

    database=std::make_shared<osmscout::Database>(databaseParameter);

    if (!database->Open(args.map)) {
      std::cerr << "Cannot open database" << std::endl;
      return false;
    }

    mapService=std::make_shared<osmscout::MapService>(database);

    styleConfig = std::make_shared<osmscout::StyleConfig>(database->GetTypeConfig());
    if (!styleConfig->Load(args.style)) {
      std::cerr << "Cannot open style" << std::endl;
      return false;
    }

    if (!args.fontName.empty()) {
      drawParameter.SetFontName(args.fontName);
    }

    drawParameter.SetFontSize(args.fontSize);
    drawParameter.SetRenderSeaLand(true);
    drawParameter.SetRenderUnknowns(false);
    drawParameter.SetRenderBackground(false);
    drawParameter.SetRenderContourLines(args.renderContourLines);
    drawParameter.SetRenderHillShading(args.renderHillShading);

    drawParameter.SetIconMode(args.iconMode);
    drawParameter.SetIconPaths(args.iconPaths);

    drawParameter.SetDebugData(args.debug);
    drawParameter.SetDebugPerformance(args.debug);

    // TODO: arguments
    drawParameter.SetLabelLineMinCharCount(15);
    drawParameter.SetLabelLineMaxCharCount(30);
    drawParameter.SetLabelLineFitToArea(true);

    projection.Set(args.center,
                   args.zoom,
                   args.dpi,
                   args.width,
                   args.height);

    if (!args.basemap.empty()) {
      basemapDatabase=std::make_shared<osmscout::BasemapDatabase>(osmscout::BasemapDatabaseParameter{});
      if (!basemapDatabase->Open(args.basemap)){
        std::cerr << "Cannot open base map" << std::endl;
        return false;
      }
    }

    return true;
  }

  void LoadData()
  {
    std::list<osmscout::TileRef> tiles;

    assert(database);
    assert(database->IsOpen());
    assert(mapService);
    assert(styleConfig);

    mapService->LookupTiles(projection,tiles);
    mapService->LoadMissingTileData(searchParameter,*styleConfig,tiles);
    mapService->AddTileDataToMapData(tiles,data);
    mapService->GetGroundTiles(projection,data.groundTiles);

    if (GetArguments().renderHillShading) {
      data.srtmTile=mapService->GetSRTMData(projection);
    }

    LoadBaseMapTiles(data.baseMapTiles);
  }

  bool LoadBaseMapTiles(std::list<osmscout::GroundTile> &tiles)
  {
    if (!basemapDatabase) {
      return true;
    }

    osmscout::WaterIndexRef waterIndex = basemapDatabase->GetWaterIndex();
    if (!waterIndex) {
      return true;
    }

    osmscout::GeoBox boundingBox;
    projection.GetDimensions(boundingBox);
    if (!waterIndex->GetRegions(boundingBox,
                                projection.GetMagnification(),
                                tiles)) {
      std::cerr << "Failed to read base map tiles" << std::endl;
      return false;
    }

    return true;
  }

  Arguments GetArguments() const
  {
    return argParser.GetArguments();
  }
};


#endif //DEMO_LIBOSMSCOUT_DRAWMAP_H
