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

#include <osmscout/db/Database.h>

#include <osmscout/cli/CmdLineParsing.h>
#include <osmscout/log/Logger.h>

#include <osmscoutmap/MapService.h>

#include <osmscoutmapopengl/MapPainterOpenGL.h>
#include <GLFW/glfw3.h>

struct Arguments {
  bool help=false;
  std::string databaseDirectory;
  std::string styleFile;
  std::string iconDirectory;
  size_t width=0;
  size_t height=0;
  std::string fontPath=DEFAULT_FONT_FILE;
  long defaultTextSize=18;
  double dpi=0;
  std::string shaderPath=SHADER_INSTALL_DIR;
  bool debug=false;
};

osmscout::DatabaseParameter databaseParameter;
osmscout::AreaSearchParameter searchParameter;
osmscout::DatabaseRef database;
osmscout::MapServiceRef mapService;
osmscout::StyleConfigRef styleConfig;
std::list <osmscout::TileRef> tiles;
osmscout::MapData data;

osmscout::MercatorProjection projection; // projection used for data loading

osmscout::MapPainterOpenGL *renderer;

std::string map;
std::string style;

// framebuffer dimensions
int width;
int height;

std::future<bool> result;

double prevX;
double prevY;
std::chrono::steady_clock::time_point lastEvent=std::chrono::steady_clock::time_point::min();
bool loadData = 0;
bool loadingInProgress = 0;
int offset;
double dpi;

bool LoadData() {
  data.ClearDBData();
  tiles.clear();

  {
    int level = projection.GetMagnification().GetLevel();
    osmscout::MagnificationConverter mm;
    std::string s;
    mm.Convert(osmscout::MagnificationLevel(level), s);
    osmscout::log.Info() << "Load zoom level: " << s << " " << level;
  }

  searchParameter.SetUseLowZoomOptimization(true);
  mapService->LookupTiles(projection, tiles);
  mapService->LoadMissingTileData(searchParameter, *styleConfig, tiles);
  mapService->AddTileDataToMapData(tiles, data);
  mapService->GetGroundTiles(projection, data.groundTiles);
  osmscout::log.Info() << "Start processing data...";
  renderer->ProcessData(data, projection, styleConfig);
  osmscout::log.Info() << "Ended processing data.";

  return true;
}

void ErrorCallback(int, const char *err_str) {
  std::cerr << "GLFW Error: " << err_str << std::endl;
}

static void key_callback(GLFWwindow *window, int key, int /*scancode*/, int action, int /*mods*/) {
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, true);
  }
  if (key == GLFW_KEY_LEFT) {
    renderer->OnTranslation(prevX, prevY, prevX + 10, prevY);
    prevX = prevX + 10;
    loadData = 1;
  }
  if (key == GLFW_KEY_RIGHT) {
    renderer->OnTranslation(prevX, prevY, prevX - 10, prevY);
    prevX = prevX - 10;
    loadData = 1;
  }
  if (key == GLFW_KEY_UP) {
    renderer->OnTranslation(prevX, prevY, prevX, prevY + 10);
    prevY = prevY + 10;
    loadData = 1;
  }
  if (key == GLFW_KEY_DOWN) {
    renderer->OnTranslation(prevX, prevY, prevX, prevY - 10);
    prevY = prevY - 10;
    loadData = 1;
  }
  if (key == GLFW_KEY_KP_ADD || key == GLFW_KEY_I) {
    renderer->OnZoom(1);
    loadData = 1;
  }
  if (key == GLFW_KEY_KP_SUBTRACT || key == GLFW_KEY_O) {
    renderer->OnZoom(-1);
    loadData = 1;
  }

  lastEvent = std::chrono::steady_clock::now();
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
  }

}

static void scroll_callback(GLFWwindow */*window*/, double /*xoffset*/, double yoffset) {
  renderer->OnZoom(yoffset);
  lastEvent = std::chrono::steady_clock::now();
  loadData = 1;
  osmscout::log.Info() << "Zoom level: " << renderer->GetMagnification().GetLevel();
  osmscout::log.Info() << "Magnification: " << renderer->GetMagnification().GetMagnification();
}

static void cursor_position_callback(GLFWwindow */*window*/, double xpos, double ypos) {
  if (button_down) {
    renderer->OnTranslation(prevX, prevY, xpos, ypos);
    loadData = 1;
    lastEvent = std::chrono::steady_clock::now();

    osmscout::GeoBox boundingBox(renderer->GetProjection().GetDimensions());
    osmscout::log.Info() << "BoundingBox: [" << boundingBox.GetMinLon() << " " << boundingBox.GetMinLat() << " "
                         << boundingBox.GetMaxLon() << " " << boundingBox.GetMaxLat() << "]";
    osmscout::log.Info() << "Center: [" << renderer->GetCenter().GetLon() << " " << renderer->GetCenter().GetLat() << "]";
  }
  prevX = xpos;
  prevY = ypos;
}

static void framebuffer_size_callback(GLFWwindow */*window*/, int w, int h) {
  width=w;
  height=h;
  osmscout::log.Info() << "Window resize " << width << " x " << height;
  glViewport(0, 0, width, height);
  renderer->SetSize(width, height);
  loadData = 1;
  lastEvent = std::chrono::steady_clock::now();
}

int main(int argc, char *argv[]) {

  osmscout::CmdLineParser argParser("OSMScoutOpenGL",
                                    argc, argv);
  std::vector <std::string> helpArgs{"h", "help"};
  Arguments args;

  argParser.AddOption(osmscout::CmdLineFlag([&args](const bool &value) {
                        args.help = value;
                      }),
                      helpArgs,
                      "Return argument help",
                      true);

  argParser.AddOption(osmscout::CmdLineFlag([&args](const bool& value) {
                        args.debug=value;
                      }),
                      "debug",
                      "Enable debug output",
                      false);

  argParser.AddOption(osmscout::CmdLineStringOption([&args](const std::string& value) {
                        args.fontPath=value;
                      }),
                      "font",
                      "Font file (*.ttf, default: " + args.fontPath + ")",
                      false);

  argParser.AddOption(osmscout::CmdLineUIntOption([&args](const unsigned int& value) {
                        args.defaultTextSize=value;
                      }),
                      "fontsize",
                      "Default font size (default: " + std::to_string(args.defaultTextSize) + ")",
                      false);

  argParser.AddOption(osmscout::CmdLineStringOption([&args](const std::string& value) {
                        args.shaderPath=value;
                      }),
                      "shaders",
                      "Path to shaders (default: " + args.shaderPath + ")",
                      false);

  argParser.AddOption(osmscout::CmdLineDoubleOption([&args](const double& value) {
                        args.dpi=value;
                      }),
                      "dpi",
                      "Rendering dpi (default is screen dpi)",
                      false);

  argParser.AddPositional(osmscout::CmdLineStringOption([&args](const std::string &value) {
                            args.databaseDirectory = value;
                          }),
                          "DATABASE",
                          "Directory of the db to use");

  argParser.AddPositional(osmscout::CmdLineStringOption([&args](const std::string &value) {
                            args.styleFile = value;
                          }),
                          "STYLEFILE",
                          "Map stylesheet file to use");

  argParser.AddPositional(osmscout::CmdLineStringOption([&args](const std::string &value) {
                            args.iconDirectory = value;
                          }),
                          "ICONS",
                          "Directory of the icons");

  argParser.AddPositional(osmscout::CmdLineSizeTOption([&args](const size_t &value) {
                            args.width = value;
                          }),
                          "WIDTH",
                          "Width of the window");

  argParser.AddPositional(osmscout::CmdLineSizeTOption([&args](const size_t &value) {
                            args.height = value;
                          }),
                          "HEIGHT",
                          "Height of the window");


  osmscout::CmdLineParseResult cmdResult = argParser.Parse();

  if (cmdResult.HasError()) {
    std::cerr << "ERROR: " << cmdResult.GetErrorDescription() << std::endl;
    std::cout << argParser.GetHelp() << std::endl;
    return 1;
  } else if (args.help) {
    std::cout << argParser.GetHelp() << std::endl;
    return 0;
  }

  osmscout::log.Debug(args.debug);

  database = osmscout::DatabaseRef(new osmscout::Database(databaseParameter));
  mapService = osmscout::MapServiceRef(new osmscout::MapService(database));

  if (!database->Open(args.databaseDirectory)) {
    std::cerr << "Cannot open db" << std::endl;
    return 1;
  }

  styleConfig = osmscout::StyleConfigRef(new osmscout::StyleConfig(database->GetTypeConfig()));

  if (!styleConfig->Load(args.styleFile)) {
    std::cerr << "Cannot open style" << std::endl;
    return 1;
  }

  osmscout::MapParameter drawParameter;
  drawParameter.SetFontSize(args.defaultTextSize);
  std::list<std::string>       paths;
  paths.push_back(args.iconDirectory);
  drawParameter.SetIconPaths(paths);

  glfwWindowHint(GLFW_SAMPLES, 4);
  GLFWwindow *window;
  glfwSetErrorCallback(ErrorCallback);
  if (!glfwInit()) {
    return -1;
  }
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  if (args.dpi > 0) {
    dpi = args.dpi;
  } else {
    int widthMM, heightMM;
    const GLFWvidmode *mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    GLFWmonitor *monitor = glfwGetPrimaryMonitor();
    glfwGetMonitorPhysicalSize(monitor, &widthMM, &heightMM);
    dpi = mode->width / (widthMM / 25.4);
    osmscout::log.Debug() << "Using screen dpi: " << dpi;
  }

  window = glfwCreateWindow(args.width, args.height, "OSMScoutOpenGL", nullptr, nullptr);
  if (!window) {
    glfwTerminate();
    return -1;
  }

  glfwGetFramebufferSize(window, &width, &height);
  glViewport(0, 0, width, height);

  glfwSetKeyCallback(window, key_callback);
  glfwSetMouseButtonCallback(window, mouse_button_callback);
  glfwSetScrollCallback(window, scroll_callback);
  glfwSetCursorPosCallback(window, cursor_position_callback);
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
  glfwMakeContextCurrent(window);

  renderer = new osmscout::MapPainterOpenGL(width, height, dpi,
                                            args.fontPath,
                                            args.shaderPath,
                                            drawParameter);
  if (!renderer->IsInitialized()) {
    delete renderer;
    glfwDestroyWindow(window);
    glfwTerminate();
    return 1;
  }

  // set initial view
  {
    osmscout::GeoBox boundingBox;
    database->GetBoundingBox(boundingBox);
    renderer->SetCenter(boundingBox.GetCenter());
  }
  {
    int level = 6;
    osmscout::Magnification magnification;
    magnification.SetLevel(osmscout::MagnificationLevel(level));
    renderer->SetMagnification(magnification);
  }

  // initial synchronous rendering
  projection = renderer->GetProjection();
  LoadData();
  renderer->SwapData();

  std::chrono::steady_clock::time_point currentTime;
  while (!glfwWindowShouldClose(window)) {

    renderer->DrawMap();
    glfwSwapBuffers(window);
    glfwPollEvents();

    if (loadData) {
      currentTime = std::chrono::steady_clock::now();
      if (( std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastEvent).count() > 100)
            && (!loadingInProgress)) {
        // double x, y;
        // glfwGetCursorPos(window, &x, &y);
        osmscout::log.Info() << "Loading data...";
        projection = renderer->GetProjection();
        result = std::future<bool>(std::async(std::launch::async, LoadData));
        loadingInProgress = 1;
      }

      if (loadingInProgress) {
        auto status = result.wait_for(std::chrono::milliseconds(0));

        if (status == std::future_status::ready) {
          loadData = 0;
          loadingInProgress = 0;
          auto success = result.get();
          if (success) {
            renderer->SwapData();
          }

          osmscout::log.Info() << "Data loading ended.";

          auto viewProjection=renderer->GetProjection();
          if (projection.GetCenter() != viewProjection.GetCenter() ||
              projection.GetMagnification() != viewProjection.GetMagnification() ||
              projection.GetWidth() != viewProjection.GetWidth() ||
              projection.GetHeight() != viewProjection.GetHeight()){
            projection = renderer->GetProjection();
            loadData = 1;
          }
        }
      }
    }
  }

  if (loadingInProgress) {
    osmscout::log.Debug() << "Waiting for loading thread";
    result.wait();
    osmscout::log.Info() << "Data loading ended.";
  }

  delete renderer;
  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;

}
