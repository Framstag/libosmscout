/*
  DrawMap - a demo program for libosmscout
  Copyright (C) 2009  Tim Teulings

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

#include <QApplication>
#include <QDesktopWidget>
#include <QPixmap>
#include <QScreen>

#include <osmscout/Database.h>
#include <osmscout/MapService.h>

#include <osmscout/MapPainterQt.h>

/*
  Example for the nordrhein-westfalen.osm (to be executed in the Demos top
  level directory):

  src/DrawMapQt ../TravelJinni/ ../TravelJinni/standard.oss 640 480 7.13 50.69 10000 test.png
*/

int main(int argc, char* argv[])
{
  std::string   map;
  std::string   style;
  std::string   output;
  size_t        width,height;
  double        lon,lat,zoom;

  if (argc!=9) {
    std::cerr << "DrawMapQt <map directory> <style-file> <width> <height> <lon> <lat> <zoom> <output>" << std::endl;
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

  QApplication application(argc,argv,true);

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

  QPixmap *pixmap=new QPixmap(width,height);

  if (pixmap!=NULL) {
    QPainter* painter=new QPainter(pixmap);

    if (painter!=NULL) {
      osmscout::MercatorProjection  projection;
      osmscout::MapParameter        drawParameter;
      osmscout::AreaSearchParameter searchParameter;
      osmscout::MapData             data;
      osmscout::MapPainterQt        mapPainter(styleConfig);
      double                        dpi=application.screens().at(application.desktop()->primaryScreen())->physicalDotsPerInch();

      drawParameter.SetFontSize(3.0);

      projection.Set(osmscout::GeoCoord(lat,lon),
                     osmscout::Magnification(zoom),
                     dpi,
                     width,
                     height);

      std::list<osmscout::TileRef> tiles;

      mapService->LookupTiles(projection,tiles);
      mapService->LoadMissingTileData(searchParameter,*styleConfig,tiles);
      mapService->AddTileDataToMapData(tiles,data);

      if (mapPainter.DrawMap(projection,
                             drawParameter,
                             data,
                             painter)) {
        if (!pixmap->save(output.c_str(),"PNG",-1)) {
          std::cerr << "Cannot write PNG" << std::endl;
        }

      }

      delete painter;
    }
    else {
      std::cout << "Cannot create QPainter" << std::endl;
    }

    delete pixmap;
  }
  else {
    std::cerr << "Cannot create QPixmap" << std::endl;
  }

  return 0;
}
