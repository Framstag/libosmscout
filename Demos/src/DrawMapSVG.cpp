/*
  DrawMap - a demo program for libosmscout
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

#include <fstream>
#include <iostream>
#include <iomanip>

#include <osmscout/Database.h>
#include <osmscout/MapService.h>

#include <osmscout/MapPainterSVG.h>

/*
  Example for the nordrhein-westfalen.osm (to be executed in the Demos top
  level directory):

  src/DrawMapSVG <path-to-map> ../stylesheets/standard.oss 1000 1000 51.2   6.5    1000 test.svg
  src/DrawMapSVG <path-to-map> ../stylesheets/standard.oss 1000 1000 51.565 7.45 160000 test.svg
  src/DrawMapSVG <path-to-map> ../stylesheets/standard.oss 1000 1000 51.48  7.45 160000 test.svg
*/

int main(int argc, char* argv[])
{
  std::string   map;
  std::string   style;
  double        lat,lon;
  double        zoom;
  size_t        width;
  size_t        height;
  std::string   output;

  if (argc!=9) {
    std::cerr << "DrawMap <map directory> <style-file> ";
    std::cerr << "<width> <height> ";
    std::cerr << "<lon> <lat> ";
    std::cerr << "<zoom> ";
    std::cerr << "<output>" << std::endl;
    return 1;
  }

  map=argv[1];
  style=argv[2];

  if (!osmscout::StringToNumber(argv[3],width)) {
    std::cerr << "width is not numeric!" << std::endl;
    return 1;
  }

  if (!osmscout::StringToNumber(argv[4],height)) {
    std::cerr << "height is not numeric!" << std::endl;
    return 1;
  }

  if (sscanf(argv[5],"%lf",&lon)!=1) {
    std::cerr << "lon is not numeric!" << std::endl;
    return 1;
  }

  if (sscanf(argv[6],"%lf",&lat)!=1) {
    std::cerr << "lat is not numeric!" << std::endl;
    return 1;
  }

  if (sscanf(argv[7],"%lf",&zoom)!=1) {
    std::cerr << "zoom is not numeric!" << std::endl;
    return 1;
  }

  output=argv[8];

  osmscout::DatabaseParameter databaseParameter;

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

  std::ofstream stream(output.c_str(), std::ios_base::binary|std::ios_base::trunc|std::ios_base::out);

  if (!stream) {
    std::cerr << "Cannot open '" << output << "' for writing!" << std::endl;
  }

  osmscout::MercatorProjection  projection;
  osmscout::MapParameter        drawParameter;
  osmscout::AreaSearchParameter searchParameter;
  osmscout::MapData             data;
  osmscout::MapPainterSVG       painter(styleConfig);

  drawParameter.SetFontName("sans-serif");
  drawParameter.SetFontSize(2.0);
  drawParameter.SetDebugPerformance(true);

  projection.Set(osmscout::GeoCoord(lat,lon),
                 osmscout::Magnification(zoom),
                 /*dpi*/ 150, // TODO: configurable
                 width,
                 height);

  searchParameter.SetMaximumAreaLevel(6);

  std::list<osmscout::TileRef> tiles;

  mapService->LookupTiles(projection,tiles);
  mapService->LoadMissingTileData(searchParameter,*styleConfig,tiles);
  mapService->AddTileDataToMapData(tiles,data);

  painter.DrawMap(projection,
                  drawParameter,
                  data,
                  stream);

  stream.close();

  return 0;
}
