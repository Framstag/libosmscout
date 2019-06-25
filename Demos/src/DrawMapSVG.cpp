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

#include <DrawMap.h>

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
  DrawMapDemo drawDemo("DrawMapSVG", argc, argv);

  if (!drawDemo.OpenDatabase()){
    return 2;
  }

  drawDemo.LoadData();
  Arguments args = drawDemo.GetArguments();

  std::ofstream stream(args.output.c_str(), std::ios_base::binary|std::ios_base::trunc|std::ios_base::out);

  if (!stream) {
    std::cerr << "Cannot open '" << args.output << "' for writing!" << std::endl;
  }

  osmscout::MapPainterSVG painter(drawDemo.styleConfig);

  painter.DrawMap(drawDemo.projection,
                  drawDemo.drawParameter,
                  drawDemo.data,
                  stream);

  stream.close();

  return 0;
}
