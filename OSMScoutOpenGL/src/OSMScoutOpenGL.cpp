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

#include <iostream>
#include <osmscout/Database.h>
#include <osmscout/MapService.h>
#include <osmscout/MapPainterOpenGL.h>
#include <future>
#include <chrono>
#include <GLFW/glfw3.h>

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
std::string map;
std::string style;
std::future<bool> result;

int zoomLevel;
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
  projection.Set(osmscout::GeoCoord(BoundingBox.GetCenter()),
                 osmscout::Magnification(zoomLevel),
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

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);
}

bool button_down = false;

static void mouse_button_callback(GLFWwindow *window, int button, int action, int mods) {
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


static void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
  zoomLevel += yoffset * 10;
  double x, y;
  glfwGetCursorPos(window, &x, &y);
  renderer->onZoom(yoffset, 0.01);
  lastZoom = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now().time_since_epoch()).count();
  loadData = 1;
}

static void cursor_position_callback(GLFWwindow *window, double xpos, double ypos) {
  if (button_down) {
    renderer->onTranslation(prevX, prevY, xpos, ypos);
    prevX = xpos;
    prevY = ypos;
  }
}

int main(int argc, char *argv[]) {

  if (argc != 5) {
    std::cerr << argc << " usage: ./OSMScoutOpenGL <path/to/map> <style-file> <width> <height>" << std::endl;
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


  database = osmscout::DatabaseRef(new osmscout::Database(databaseParameter));
  mapService = osmscout::MapServiceRef(new osmscout::MapService(database));

  if (!database->Open(map.c_str())) {
    std::cerr << "Cannot open database" << std::endl;
    return 1;
  }

  styleConfig = osmscout::StyleConfigRef(new osmscout::StyleConfig(database->GetTypeConfig()));

  if (!styleConfig->Load(style)) {
    std::cerr << "Cannot open style" << std::endl;
  }

  database.get()->GetBoundingBox(BoundingBox);


  drawParameter.SetFontSize(3.0);

  zoomLevel = 1000;
  projection.Set(osmscout::GeoCoord(BoundingBox.GetCenter()),
                 osmscout::Magnification(zoomLevel),
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
      if ((abs(currentTime - lastZoom) > 1000) && (!loadingInProgress)) {
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
        }
      }
    }
  }

  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;

}
