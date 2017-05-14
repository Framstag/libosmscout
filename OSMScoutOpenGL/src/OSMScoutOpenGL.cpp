#include <GLFW/glfw3.h>
#include <iostream>

#include <osmscout/Database.h>
#include <osmscout/MapService.h>

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    glfwSetWindowShouldClose(window, GLFW_TRUE);
}

bool button_down = false;

static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
  if(button == GLFW_MOUSE_BUTTON_LEFT)
  {
	if(action == GLFW_PRESS)
	{
      button_down = true;
	}
    else if(action == GLFW_RELEASE)
    {
      button_down = false;
    }
  }

  if(button_down){
    std::cout << "click" << std::endl;
  }

}

static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
  std::cout << "scroll" << std::endl;
}

static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
  if(button_down){
    std::cout << "drag" << std::endl;
  }
}

int main(int argc, char* argv[])
{
  std::string map;
  std::string style;
  size_t width,height;

  if(argc != 5){
    std::cerr << argc << " usage: ./OSMScoutOpenGL <path/to/map> <style-file> <width> <height>" << std::endl;
    return 1;
  }

  map = argv[1];
  style = argv[2];

  if (!osmscout::StringToNumber(argv[3],width)) {
    std::cerr << "width is not numeric!" << std::endl;
    return 1;
  }

  if (!osmscout::StringToNumber(argv[4],height)) {
    std::cerr << "height is not numeric!" << std::endl;
    return 1;
  }

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

  osmscout::GeoBox BoundingBox;
  bool b = database.get()->GetBoundingBox(BoundingBox);
  std::cout << " " << BoundingBox.GetCenter().GetLat() << " " << BoundingBox.GetCenter().GetLon() << std::endl;

  osmscout::MercatorProjection  projection;
  osmscout::MapParameter        drawParameter;
  osmscout::AreaSearchParameter searchParameter;
  osmscout::MapData             data;

  projection.Set(BoundingBox.GetCenter(),
                 osmscout::Magnification(1),
                 100,
                 width,
                 height);

  std::list<osmscout::TileRef> tiles;

  mapService->LookupTiles(projection,tiles);
  mapService->LoadMissingTileData(searchParameter,*styleConfig,tiles);
  mapService->AddTileDataToMapData(tiles,data);

  GLFWwindow* window;
  if (!glfwInit())
    return -1;
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  window = glfwCreateWindow(width, height, "OSMScoutOpenGL", NULL, NULL);
  if (!window)
  {
    glfwTerminate();
    return -1;
  }
  glfwSetKeyCallback(window, key_callback);
  glfwSetMouseButtonCallback(window, mouse_button_callback);
  glfwSetScrollCallback(window, scroll_callback);
  glfwSetCursorPosCallback(window, cursor_position_callback);
  glfwMakeContextCurrent(window);

  while (!glfwWindowShouldClose(window))
  {
    glClear(GL_COLOR_BUFFER_BIT);
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;

}
