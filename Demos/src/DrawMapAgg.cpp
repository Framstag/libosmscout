/*
  DrawMapAgg - a demo program for libosmscout
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

#include <iostream>
#include <iomanip>

#include <osmscout/Database.h>
#include <osmscout/MapService.h>

#include <osmscout/MapPainterAgg.h>

/*
  Example for the nordrhein-westfalen.osm (to be executed in the Demos top
  level directory):

  src/DrawMapAgg ../TravelJinni/ ../TravelJinni/standard.oss 640 480 7.13 50.69 10000 test.ppm
  src/DrawMapAgg ../TravelJinni/ ../TravelJinni/standard.oss 640 480 7.45274 51.49256 50000 test.ppm
*/

bool write_ppm(const unsigned char* buf,
               unsigned width,
               unsigned height,
               const char* file_name)
{
  FILE* fd=fopen(file_name, "wb");

  if (fd) {
    fprintf(fd,"P6 %ud %ud 255\n", width,height);
    fwrite(buf,1,width*height*3,fd);
    fclose(fd);
    return true;
  }

  return false;
}

int main(int argc, char* argv[])
{
  DrawMapDemo drawDemo("DrawMapAgg", argc, argv);

  if (!drawDemo.OpenDatabase()){
    return 2;
  }

  drawDemo.LoadData();
  Arguments args = drawDemo.GetArguments();

  unsigned char *buffer=new unsigned char[args.width*args.height*3];

  memset(buffer,255,args.width*args.height*3);

  agg::rendering_buffer rbuf(buffer,
                             args.width,
                             args.height,
                             args.width*3);

  agg::pixfmt_rgb24 pf(rbuf);

  osmscout::MapPainterAgg       painter(drawDemo.styleConfig);
  if (painter.DrawMap(drawDemo.projection,
                      drawDemo.drawParameter,
                      drawDemo.data,
                      &pf)) {
    write_ppm(buffer,args.width,args.height,args.output.c_str());
  }

  return 0;
}
