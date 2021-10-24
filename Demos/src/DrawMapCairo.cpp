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

#include <DrawMap.h>

#include <iostream>

#include <osmscoutmapcairo/MapPainterCairo.h>

/*
  Example for the nordrhein-westfalen.osm (to be executed in meson build directory):

  Demos/DrawMapCairo ../maps/nordrhein-westfalen ../stylesheets/standard.oss 800 480 51.51241 7.46525 70000 test.png
 */

int main(int argc, char* argv[])
{
  DrawMapDemo drawDemo("DrawMapCairo", argc, argv);

  if (!drawDemo.OpenDatabase()){
    return 2;
  }

  drawDemo.LoadData();

  Arguments args = drawDemo.GetArguments();

  cairo_surface_t *surface;

  surface=cairo_image_surface_create(CAIRO_FORMAT_RGB24,
                                     (int)args.width,
                                     (int)args.height);

  if (surface!=nullptr) {
    cairo_t *cairo=cairo_create(surface);

    if (cairo!=nullptr) {
      osmscout::MapPainterCairo     painter(drawDemo.styleConfig);

      if (painter.DrawMap(drawDemo.projection,
                          drawDemo.drawParameter,
                          drawDemo.data,
                          cairo)) {
        if (cairo_surface_write_to_png(surface,args.output.c_str())!=CAIRO_STATUS_SUCCESS) {
          std::cerr << "Cannot write PNG" << std::endl;
        }
      }

      cairo_destroy(cairo);
    }
    else {
      std::cerr << "Cannot create cairo cairo" << std::endl;
    }

    cairo_surface_destroy(surface);
  }
  else {
    std::cerr << "Cannot create cairo surface" << std::endl;
  }

  return 0;
}
