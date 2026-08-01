/*
  DrawMapSkia - a demo program for libosmscout
  Copyright (C) 2025  Tim Teulings

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

#include <osmscout/db/Database.h>

#include <osmscoutmap/MapService.h>

#include <osmscoutmapskia/MapPainterSkia.h>

#include <core/SkBitmap.h>
#include <core/SkCanvas.h>
#include <core/SkImageInfo.h>
#include <core/SkSurface.h>

static bool WritePPM(const SkBitmap& bitmap,
                     const std::string& file_name)
{
  FILE* fd = fopen(file_name.c_str(), "wb");
  if (!fd) {
    return false;
  }

  SkImageInfo info = bitmap.info();
  fprintf(fd, "P6\n%zu %zu\n255\n",
          static_cast<size_t>(info.width()),
          static_cast<size_t>(info.height()));

  bitmap.readPixels(bitmap.info(), bitmap.getPixels(), bitmap.rowBytes(), 0, 0);

  // Write RGB data (skip alpha channel)
  for (int y = 0; y < info.height(); ++y) {
    for (int x = 0; x < info.width(); ++x) {
      SkColor pixel = bitmap.getColor(x, y);
      unsigned char rgb[3] = {
          static_cast<unsigned char>(SkColorGetR(pixel)),
          static_cast<unsigned char>(SkColorGetG(pixel)),
          static_cast<unsigned char>(SkColorGetB(pixel))
      };
      fwrite(rgb, 1, 3, fd);
    }
  }

  fclose(fd);
  return true;
}

int main(int argc, char* argv[])
{
  DrawMapDemo drawDemo("DrawMapSkia", argc, argv);

  if (!drawDemo.OpenDatabase()){
    return 2;
  }

  drawDemo.LoadData();
  Arguments args = drawDemo.GetArguments();

  if (!ValidateFontArguments(args)) {
    return 1;
  }

  SkImageInfo imageInfo = SkImageInfo::MakeN32Premul(
      static_cast<int>(args.width),
      static_cast<int>(args.height));

  sk_sp<SkSurface> surface = SkSurfaces::Raster(imageInfo);

  if (!surface) {
    std::cerr << "ERROR: Cannot create Skia surface" << std::endl;
    return 1;
  }

  SkCanvas* canvas = surface->getCanvas();
  canvas->clear(SK_ColorWHITE);

  osmscout::MapPainterSkia painter;
  if (painter.DrawMap(drawDemo.projection,
                      drawDemo.drawParameter,
                      drawDemo.data,
                      canvas)) {
    SkBitmap bitmap;
    bitmap.allocPixels(imageInfo);
    if (!surface->readPixels(bitmap, 0, 0)) {
      std::cerr << "ERROR: Cannot read pixels from surface" << std::endl;
      return 1;
    }

    if (WritePPM(bitmap, args.output)) {
      std::cout << "Wrote " << args.output << " ("
                << args.width << "x" << args.height << ")"
                << std::endl;
    } else {
      std::cerr << "ERROR: Cannot write " << args.output << std::endl;
      return 1;
    }
  }

  return 0;
}
