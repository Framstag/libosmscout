/*
  This source is part of the libosmscout-map library
  Copyright (C) 2017  Fanny Monori

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
*/

#include <cstdlib>
#include <iostream>
#include <future>
#include <chrono>
#include <osmscout/Database.h>
#include <osmscout/MapService.h>
#include <osmscout/MapPainterOpenGL.h>
#include <osmscout/util/CmdLineParsing.h>
#include <osmscout/util/Logger.h>
#include <GLFW/glfw3.h>


struct Arguments
{
  bool               help;
  std::string        databaseDirectory;
  std::string        styleFileDirectory;
  size_t             width;
  size_t             height;

  Arguments()
          : help(false)
  {
    // no code
  }
};

osmscout::DatabaseParameter databaseParameter;
osmscout::DatabaseRef database;
osmscout::MapServiceRef mapService;
osmscout::StyleConfigRef styleConfig;
osmscout::GeoBox BoundingBox;
osmscout::MercatorProjection projection;
osmscout::MapParameter drawParameter;
osmscout::AreaSearchParameter searchParameter;
osmscout::MapData data;
std::list<osmscout::TileRef> tiles;
osmscout::MapPainterOpenGL *renderer;
osmscout::Magnification m;
std::string map;
std::string style;
std::future<bool> result;

int zoomLevel;
int level;
int zoom = 0;
double prevX = 0;
double prevY = 0;
unsigned long long lastZoom;
bool loadData = 0;
bool loadingInProgress = 0;

size_t width;
size_t height;

bool LoadData() {
  data.ClearDBData();
  tiles.clear();
  //m.SetMagnification(zoomLevel);
  level++;
  m.SetLevel(level);
  osmscout::MagnificationConverter mm;
  //float l = pow(2.0,zoomLevel);
  //std::cout << "l: " << l << std::endl;
  std::string s;
  mm.Convert(m.GetLevel(),s);
  std::cout << "mag: " << s << std::endl;
  projection.Set(osmscout::GeoCoord(BoundingBox.GetCenter()),
                 m,
                 96,
                 width,
                 height);

  searchParameter.SetUseLowZoomOptimization(false);
  mapService->LookupTiles(zoomLevel, BoundingBox, tiles);
  mapService->LoadMissingTileData(searchParameter, *styleConfig, tiles);
  mapService->AddTileDataToMapData(tiles, data);
  mapService->GetGroundTiles(BoundingBox, zoomLevel, data.groundTiles);
  renderer->loadData(data, drawParameter, projection, styleConfig, BoundingBox);

  return true;
}

void ErrorCallback(int, const char *err_str) {
  std::cerr << "GLFW Error: " << err_str << std::endl;
}

static void key_callback(GLFWwindow *window, int key, int /*scancode*/, int action, int /*mods*/) {
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, true);
  }
  if(key == GLFW_KEY_LEFT){
    renderer->onTranslation(prevX, prevY, prevX + 10, prevY);
    prevX = prevX + 10;
  }
  if(key == GLFW_KEY_RIGHT){
    renderer->onTranslation(prevX, prevY, prevX - 10, prevY);
    prevX = prevX - 10;
  }
  if(key == GLFW_KEY_UP){
    renderer->onTranslation(prevX, prevY, prevX, prevY + 10);
    prevY = prevY + 10;
  }
  if(key == GLFW_KEY_DOWN){
    renderer->onTranslation(prevX, prevY, prevX, prevY - 10);
    prevY = prevY - 10;
  }
  if(key == GLFW_KEY_KP_ADD || key == GLFW_KEY_I){
    zoomLevel += 100;
    renderer->onZoom(1, 0.05);
    lastZoom = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
    loadData = 1;
  }
  if(key == GLFW_KEY_KP_SUBTRACT || key == GLFW_KEY_O){
    zoomLevel -= 100;
    renderer->onZoom(-1, 0.05);
    lastZoom = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
    loadData = 1;
  }

}

bool button_down = false;

static void mouse_button_callback(GLFWwindow *window, int button, int action, int /*mods*/) {
  if (button == GLFW_MOUSE_BUTTON_LEFT) {
    if (action == GLFW_PRESS) {
      button_down = true;
      double x, y;
      glfwGetCursorPos(window, &x, &y);
      prevX = x;
      prevY = y;
    } else if (action == GLFW_RELEASE) {
      button_down = false;
    }
  }

  if (button_down) {
    std::cout << "click" << std::endl;
  }

}

static void scroll_callback(GLFWwindow *window, double /*xoffset*/, double yoffset) {
  zoomLevel += yoffset * 100;
  double x, y;
  glfwGetCursorPos(window, &x, &y);
  //renderer->onZoom(yoffset, 0.05);
  renderer->onZoom(yoffset, 0.05 + std::pow(2,level)/(float)100000);


std::cout << m.GetLevel() << " " << m.GetMagnification() << " " << std::pow(2,level)/(float)100000 << std::endl;


  //m.SetLevel(zoomLevel);
  //std::cout << m.GetLevel() << " " << m.GetMagnification() << std::endl;
  lastZoom = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now().time_since_epoch()).count();
  loadData = 1;
  osmscout::log.Info() << "Magnification: " << zoomLevel;
  osmscout::log.Info() << "BoundingBox: [" << BoundingBox.GetMinLon() << " "  << BoundingBox.GetMinLat() << " "
                                << BoundingBox.GetMaxLon() << " " << BoundingBox.GetMaxLat() << "]";
}

static void cursor_position_callback(GLFWwindow */*window*/, double xpos, double ypos) {
  if (button_down) {
    renderer->onTranslation(prevX, prevY, xpos, ypos);
    prevX = xpos;
    prevY = ypos;
  }
}

int main(int argc, char *argv[]) {

  osmscout::CmdLineParser   argParser("OSMScoutOpenGL",
                                      argc,argv);
  std::vector<std::string>  helpArgs{"h","help"};
  Arguments                 args;

  argParser.AddOption(osmscout::CmdLineFlag([&args](const bool& value) {
                        args.help=value;
                      }),
                      helpArgs,
                      "Return argument help",
                      true);

  argParser.AddPositional(osmscout::CmdLineStringOption([&args](const std::string& value) {
                            args.databaseDirectory=value;
                          }),
                          "DATABASE",
                          "Directory of the database to use");

  argParser.AddPositional(osmscout::CmdLineStringOption([&args](const std::string& value) {
                            args.styleFileDirectory=value;
                          }),
                          "STYLEFILE",
                          "Directory of the stylefile to use");

  argParser.AddPositional(osmscout::CmdLineSizeTOption([&args](const size_t& value) {
                            args.width=value;
                          }),
                          "WIDTH",
                          "Width of the window");

  argParser.AddPositional(osmscout::CmdLineSizeTOption([&args](const size_t& value) {
                            args.height=value;
                          }),
                          "HEIGHT",
                          "Height of the window");

  osmscout::CmdLineParseResult cmdResult=argParser.Parse();

  if (cmdResult.HasError()) {
    std::cerr << "ERROR: " << cmdResult.GetErrorDescription() << std::endl;
    std::cout << argParser.GetHelp() << std::endl;
    return 1;
  }
  else if (args.help) {
    std::cout << argParser.GetHelp() << std::endl;
    return 0;
  }

  database = osmscout::DatabaseRef(new osmscout::Database(databaseParameter));
  mapService = osmscout::MapServiceRef(new osmscout::MapService(database));

  if (!database->Open(args.databaseDirectory)) {
    std::cerr << "Cannot open database" << std::endl;
    return 1;
  }

  styleConfig = osmscout::StyleConfigRef(new osmscout::StyleConfig(database->GetTypeConfig()));

  if (!styleConfig->Load(args.styleFileDirectory)) {
    std::cerr << "Cannot open style" << std::endl;
  }

  width = args.width;
  height = args.height;

  database.get()->GetBoundingBox(BoundingBox);


  drawParameter.SetFontSize(3.0);

  zoomLevel = 100;
  level = 10;
  m.SetLevel(level);
  projection.Set(osmscout::GeoCoord(BoundingBox.GetCenter()),
                 m,
                 96,
                 width,
                 height);

  searchParameter.SetUseLowZoomOptimization(false);
  mapService->LookupTiles(zoomLevel, BoundingBox, tiles);
  mapService->LoadMissingTileData(searchParameter, *styleConfig, tiles);
  mapService->AddTileDataToMapData(tiles, data);
  mapService->GetGroundTiles(BoundingBox, zoomLevel, data.groundTiles);

  glfwWindowHint(GLFW_SAMPLES, 4);
  GLFWwindow *window;
  glfwSetErrorCallback(ErrorCallback);
  if (!glfwInit())
    return -1;
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  const GLFWvidmode *mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
  int screenWidth = mode->width;
  int screenHeight = mode->height;

  window = glfwCreateWindow(width, height, "OSMScoutOpenGL", NULL, NULL);
  if (!window) {
    glfwTerminate();
    return -1;
  }
  glfwSetKeyCallback(window, key_callback);
  glfwSetMouseButtonCallback(window, mouse_button_callback);
  glfwSetScrollCallback(window, scroll_callback);
  glfwSetCursorPosCallback(window, cursor_position_callback);
  glfwMakeContextCurrent(window);

  renderer = new osmscout::MapPainterOpenGL(width, height, screenWidth, screenHeight);
  renderer->loadData(data, drawParameter, projection, styleConfig, BoundingBox);
  renderer->SwapData();

  unsigned long long currentTime;
  while (!glfwWindowShouldClose(window)) {
    glfwSwapBuffers(window);
    glfwPollEvents();

    renderer->DrawMap();

    if (loadData) {
      currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::steady_clock::now().time_since_epoch()).count();
      if (((currentTime - lastZoom) > 1000) && (!loadingInProgress)) {
        osmscout::log.Info() << "Loading data...";
        result = std::future<bool>(std::async(std::launch::async, LoadData));
        loadingInProgress = 1;
      }

      if (loadingInProgress) {
        auto status = result.wait_for(std::chrono::milliseconds(0));

        if (status == std::future_status::ready) {
          loadData = 0;
          loadingInProgress = 0;
          auto success = result.get();
          if (success)
            renderer->SwapData();
          osmscout::log.Info() << "Data loading ended.";
        }
      }
    }
  }

  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;

}
