/*
  ResourceConsumption - a demo program for libosmscout
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


#include <osmscout/Database.h>
#include <osmscout/MapService.h>

#include <osmscout/MapPainter.h>

#include <osmscout/util/Projection.h>
#include <osmscout/util/StopClock.h>

/*
  Example for the germany.osm, show germany overview, then zooms into Bonn city
  and then zoom into Dortmund city and then zomm into München overview.

  ./ResourceConsumption ../../TravelJinni/ ../../TravelJinni/standard.oss 640 480 51.1924, 10.4567 32 50.7345, 7.09993 32768  51.5114, 7.46517 32768 48.1061 11.6186  1024
*/

struct Action
{
  double lon;
  double lat;
  double magnification;
};

void DumpHelp()
{
  std::cerr << "ResourceConsumption <map directory> <style-file> <width> <height> [<lat> <lon> <zoom>]..." << std::endl;
}

int main(int argc, char* argv[])
{
  std::string         map;
  std::string         style;
  size_t              width,height;
  std::vector<Action> actions;

  if (argc<5) {
    DumpHelp();
    return 1;
  }

  if ((argc-5)%3!=0) {
    DumpHelp();
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

  int arg=5;

  while (arg<argc) {
    Action action;

    if (sscanf(argv[arg],"%lf",&action.lat)!=1) {
      std::cerr << "lat is not numeric!" << std::endl;
      return 1;
    }

    arg++;

    if (sscanf(argv[arg],"%lf",&action.lon)!=1) {
      std::cerr << "lon is not numeric!" << std::endl;
      return 1;
    }

    arg++;

    if (sscanf(argv[arg],"%lf",&action.magnification)!=1) {
      std::cerr << "zoom is not numeric!" << std::endl;
      return 1;
    }

    arg++;

    actions.push_back(action);
  }

  std::cout << "# General program resources initialized, press return to start rendering emulation!" << std::endl;

  std::cin.get();

  {
    osmscout::DatabaseParameter databaseParameter;

    databaseParameter.SetAreaAreaIndexCacheSize(0);

    osmscout::DatabaseRef database(new osmscout::Database(databaseParameter));
    osmscout::MapServiceRef mapService(new osmscout::MapService(database));

    if (!database->Open(map.c_str())) {
      std::cerr << "Cannot open database" << std::endl;

      return 1;
    }

    database->DumpStatistics();

    osmscout::StyleConfigRef styleConfig(new osmscout::StyleConfig(database->GetTypeConfig()));

    if (!styleConfig->Load(style)) {
      std::cerr << "Cannot open style" << std::endl;
    }

    for (const auto& action : actions) {
      std::cout << "-------------------" << std::endl;
      std::cout << "# Rendering " << action.lat << "," << action.lon << " with zoom " << action.magnification << " and size " << width << "x" << height << std::endl;

      osmscout::MercatorProjection  projection;
      osmscout::AreaSearchParameter searchParameter;
      osmscout::MapData             data;

      projection.Set(osmscout::GeoCoord(action.lat,
                                        action.lon),
                     osmscout::Magnification(action.magnification),
                     96.0,
                     width,
                     height);

      osmscout::StopClock dbTimer;

      std::list<osmscout::TileRef> tiles;

      mapService->LookupTiles(projection,tiles);
      mapService->LoadMissingTileData(searchParameter,*styleConfig,tiles);
      mapService->AddTileDataToMapData(tiles,data);

      dbTimer.Stop();

      std::cout << "# DB access time " << dbTimer << std::endl;
      database->DumpStatistics();
    }

    std::cout << "# Press return to close database" << std::endl;

    std::cin.get();

    database->Close();
  }

  std::cout << "# Press return to end application" << std::endl;

  std::cin.get();

  return 0;
}
