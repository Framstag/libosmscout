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
#include <osmscout/MapPainterSVG.h>
#include <osmscout/StyleConfigLoader.h>

/*
  Example for the nordrhein-westfalen.osm (to be executed in the Demos top
  level directory):

  src/DrawMapSVG ../TravelJinni/ ../TravelJinni/standard.oss 51.2 6.5 51.7 8 1000 1000 test.svg
  src/DrawMapSVG ../TravelJinni/ ../TravelJinni/standard.oss 51.565 7.45 51.58 7.47 40000 1000 test.svg
  src/DrawMapSVG ../TravelJinni/ ../TravelJinni/standard.oss 51.48 7.45 51.50 7.47 40000 1000 test.svg
*/

int main(int argc, char* argv[])
{
  std::string   map;
  std::string   style;
  double        latTop,latBottom,lonLeft,lonRight;
  double        zoom;
  size_t        width;
  std::string   output;

  if (argc!=10) {
    std::cerr << "DrawMap <map directory> <style-file> ";
    std::cerr << "<lat_top> <lon_left> <lat_bottom> <lon_right> ";
    std::cerr << "<zoom> <width> ";
    std::cerr << "<output>" << std::endl;
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

  if (sscanf(argv[7],"%lf",&zoom)!=1) {
    std::cerr << "zoom is not numeric!" << std::endl;
    return 1;
  }

  if (!osmscout::StringToNumber(argv[8],width)) {
    std::cerr << "width is not numeric!" << std::endl;
    return 1;
  }

  output=argv[9];

  osmscout::DatabaseParameter databaseParameter;
  osmscout::Database          database(databaseParameter);

  if (!database.Open(map.c_str())) {
    std::cerr << "Cannot open database" << std::endl;

    return 1;
  }

  osmscout::StyleConfig styleConfig(database.GetTypeConfig());

  if (!osmscout::LoadStyleConfig(style.c_str(),styleConfig)) {
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
  osmscout::MapPainterSVG       painter;

  projection.Set(std::min(lonLeft,lonRight),
                 std::min(latTop,latBottom),
                 std::max(lonLeft,lonRight),
                 std::max(latTop,latBottom),
                 zoom,
                 width);

  osmscout::TypeSet              nodeTypes;
  std::vector<osmscout::TypeSet> wayTypes;
  osmscout::TypeSet              areaTypes;

  styleConfig.GetNodeTypesWithMaxMag(projection.GetMagnification(),
                                     nodeTypes);

  styleConfig.GetWayTypesByPrioWithMaxMag(projection.GetMagnification(),
                                          wayTypes);

  styleConfig.GetAreaTypesWithMaxMag(projection.GetMagnification(),
                                     areaTypes);

  database.GetObjects(nodeTypes,
                      wayTypes,
                      areaTypes,
                      projection.GetLonMin(),
                      projection.GetLatMin(),
                      projection.GetLonMax(),
                      projection.GetLatMax(),
                      projection.GetMagnification(),
                      searchParameter,
                      data.nodes,
                      data.ways,
                      data.areas);

  searchParameter.SetMaximumNodes(std::numeric_limits<size_t>::max());
  searchParameter.SetMaximumWays(std::numeric_limits<size_t>::max());
  searchParameter.SetMaximumAreas(std::numeric_limits<size_t>::max());
  searchParameter.SetMaximumAreaLevel(6);

  painter.DrawMap(styleConfig,
                      projection,
                      drawParameter,
                      data,
                      stream);

  stream.close();

  return 0;
}
