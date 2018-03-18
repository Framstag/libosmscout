/*
  DrawMapOpenGL - a demo program for libosmscout
  Copyright (C) 2018  Alexis Metge

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
#include <cstdlib>
#include <osmscout/Database.h>
#include <osmscout/MapService.h>
#include <osmscout/MapPainterOpenGL.h>
#include <osmscout/util/CmdLineParsing.h>
#include <osmscout/util/Logger.h>
#include <GLFW/glfw3.h>

/*
  Example for the nordrhein-westfalen.osm (to be executed in the Demos top
  level directory):

  DrawMapOpenGL ../maps/nordrhein-westfalen ../stylesheets/standard.oss 1024 800 7.46525 51.51241 70000 test.ppm
 */

static const double DPI = 96.0;

int main(int argc, char* argv[]) {
  std::string map;
  std::string style;
  std::string output;
  size_t width, height;
  double lon, lat, zoom;

  if (argc != 9) {
    std::cerr << "DrawMap <map directory> <style-file> <width> <height> <lon> <lat> <zoom> <output>" << std::endl;
    return 1;
  }

  map = argv[1];
  style = argv[2];

  if (!osmscout::StringToNumber(argv[3], width)) {
    std::cerr << "width is not numeric!" << std::endl;
    return 1;
  }

  if (!osmscout::StringToNumber(argv[4], height)) {
    std::cerr << "height is not numeric!" << std::endl;
    return 1;
  }

  if (sscanf(argv[5], "%lf", &lon) != 1) {
    std::cerr << "lon is not numeric!" << std::endl;
    return 1;
  }

  if (sscanf(argv[6], "%lf", &lat) != 1) {
    std::cerr << "lat is not numeric!" << std::endl;
    return 1;
  }

  if (sscanf(argv[7], "%lf", &zoom) != 1) {
    std::cerr << "zoom is not numeric!" << std::endl;
    return 1;
  }

  output = argv[8];

  osmscout::DatabaseParameter databaseParameter;
  osmscout::DatabaseRef database(new osmscout::Database(databaseParameter));
  osmscout::MapServiceRef mapService(new osmscout::MapService(database));

  if (!database->Open(map.c_str())) {
    std::cerr << "Cannot open database" << std::endl;
    return 1;
  }

  osmscout::StyleConfigRef styleConfig(new osmscout::StyleConfig(database->GetTypeConfig()));

  if (!styleConfig->Load(style)) {
    std::cerr << "Cannot open style" << std::endl;
  }

  osmscout::MercatorProjection projection;
  osmscout::MapParameter drawParameter;
  osmscout::AreaSearchParameter searchParameter;
  osmscout::MapData data;

  // Create the offscreen renderer
  glfwSetErrorCallback([](int, const char *err_str) {
    std::cerr << "GLFW Error: " << err_str << std::endl;
  });
  if (!glfwInit())
    return 1;
  glfwWindowHint(GLFW_SAMPLES, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_VISIBLE, false);
  GLFWwindow* offscreen_context = glfwCreateWindow(width, height, "", NULL, NULL);
  if (!offscreen_context) {
    std::cerr << "Failed to create offscreen context." << std::endl;
    return 1;
  }
  glfwMakeContextCurrent(offscreen_context);

  osmscout::MapPainterOpenGL* painter = new osmscout::MapPainterOpenGL(width, height, DPI, width, height, "/usr/share/fonts/TTF/LiberationSans-Regular.ttf");

  drawParameter.SetFontSize(12);

  projection.Set(osmscout::GeoCoord(lat, lon),
          osmscout::Magnification(zoom),
          DPI,
          width,
          height);

  std::list<osmscout::TileRef> tiles;

  mapService->LookupTiles(projection, tiles);
  mapService->LoadMissingTileData(searchParameter, *styleConfig, tiles);
  mapService->AddTileDataToMapData(tiles, data);

  painter->ProcessData(data, drawParameter, projection, styleConfig);
  painter->SwapData();

  painter->DrawMap();

  // Save to file
  unsigned char* image = new unsigned char[3 * width * height];
  glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, image);

  std::ofstream file(output, std::ofstream::binary);
  file << "P6 " << width << " " << height << " 255\n";
  for (int i = height - 1; i >= 0; --i) {
    for (unsigned int j = 0; j < width * 3; ++j) {
      file << image[i * width * 3 + j];
    }
  }
  delete[] image;

  return 0;
}

