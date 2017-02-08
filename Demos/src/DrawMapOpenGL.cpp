/*
  DrawMap - a demo program for libosmscout
  Copyright (C) 2013  Tim Teulings

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

/*
  Example for the nordrhein-westfalen.osm (to be executed in the Demos top
  level directory):

  src/DrawMapOpenGL ../TravelJinni/ ../TravelJinni/standard.oss
*/

#include <iostream>

#include "config.h"

#if defined(HAVE_WINDOWS_H) && defined(_WIN32)
#  include <windows.h>
#endif

#if defined(HAVE_GL_GLUT_H)
#  include <GL/glut.h>
#elif defined(HAVE_GLUT_GLUT_H)
#  include <GLUT/glut.h>
#else
#  error "no glut.h"
#endif

#include <osmscout/Database.h>
#include <osmscout/MapService.h>

#include <osmscout/MapPainterOpenGL.h>

static int window=-1;

static int width=640;
static int height=480;

static const double DPI=96.0;

//static double lat=51.577;
//static double lon=7.46;
static const double lat=50.6811;
static const double lon=7.158;
static const double zoom=80000;

static osmscout::MercatorProjection projection;
static osmscout::MapParameter       drawParameter;
static osmscout::MapPainterOpenGL   *painter=NULL;

class Database
{
private:
  osmscout::DatabaseRef         database;
  osmscout::MapServiceRef       mapService;
  osmscout::StyleConfigRef      styleConfig;

  osmscout::AreaSearchParameter searchParameter;

public:
  Database()
  {
    // no code
  }

  bool Initialize(const std::string& map,
                  const std::string& style)
  {
    osmscout::DatabaseParameter databaseParameter;

    database=std::make_shared<osmscout::Database>(databaseParameter);
    mapService=std::make_shared<osmscout::MapService>(database);

    if (!database->Open(map.c_str())) {
      std::cerr << "Cannot open database" << std::endl;

      return false;
    }

    styleConfig=std::make_shared<osmscout::StyleConfig>(database->GetTypeConfig());

    if (!styleConfig->Load(style)) {
      std::cerr << "Cannot open style" << std::endl;

      return false;
    }

    searchParameter.SetUseLowZoomOptimization(true);

    return true;
  }

  ~Database()
  {
    delete painter;
    painter=NULL;
  }

  const osmscout::StyleConfigRef GetStyleConfig() const
  {
    return styleConfig;
  }

  bool LoadData(const osmscout::Projection& projection,
                osmscout::MapData& data)
  {
    painter = new osmscout::MapPainterOpenGL(styleConfig);
    bool result=true;

    std::list<osmscout::TileRef> tiles;

    mapService->LookupTiles(projection,tiles);
    result=mapService->LoadMissingTileData(searchParameter,*styleConfig,tiles);
    mapService->AddTileDataToMapData(tiles,data);

    return result;
  }
};

static Database database;

void OnInit()          // We call this right after our OpenGL window is created.
{
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);   // This Will Clear The Background Color To Black
  //glClearDepth(1.0);        // Enables Clearing Of The Depth Buffer
  //glDepthFunc(GL_LESS);       // The Type Of Depth Test To Do
  //glEnable(GL_DEPTH_TEST);      // Enables Depth Testing
  //glEnable(GL_LINE_SMOOTH);
  glHint(GL_LINE_SMOOTH_HINT,GL_NICEST);
  glShadeModel(GL_SMOOTH);      // Enables Smooth Color Shading

  glLoadIdentity();       // Reset The Projection Matrix
  glMatrixMode(GL_MODELVIEW);
}

void OnDisplay()
{
  osmscout::MapData data;

  projection.Set(osmscout::GeoCoord(lat,lon),
                 osmscout::Magnification(zoom),
                 DPI,
                 width,
                 height);

  if (!database.LoadData(projection,
                         data)) {
    std::cerr << "Cannot load data" << std::endl;
    return;
  }

  glClear(GL_COLOR_BUFFER_BIT);
  glLoadIdentity();

  //glTranslated(-width/2.0,-height/2.0,-9.0);
  //glRotatef(-15.0f,1.0f,0.0f,0.0f);

  if (!painter->DrawMap(projection,
                        drawParameter,
                        data)) {
    std::cerr << "Cannot render" << std::endl;
    return;
  }

  // swap buffers to display, since we're double buffered.
  glutSwapBuffers();
}

/* The function called when our window is resized (which shouldn't happen, because we're fullscreen) */
void OnResize(int width, int height)
{
  if (height==0)        // Prevent A Divide By Zero If The Window Is Too Small
    height=1;

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  glViewport(0, 0, width, height);    // Reset The Current Viewport And Perspective Transformation
  glOrtho(0.0,width,0.0,height,0.0f,10.0f);

  //gluPerspective(150.0f,(GLfloat)width/(GLfloat)height,0.0f,10.0f);
  glMatrixMode(GL_MODELVIEW);

  ::width=width;
  ::height=height;
}

void OnKeyPressed(unsigned char key, int /*x*/, int /*y*/)
{
  /* avoid thrashing this procedure */
  //usleep(100);

  std::cout << "Key pressed with code " << (int) key << std::endl;

  /* If escape is pressed, kill everything. */
  if (key == 27 || key == 113) { // Escape or "q"
    /* shut down our window */
    glutDestroyWindow(window);

    /* exit the program...normal termination. */
    exit(0);
  }
}

int main(int argc, char* argv[])
{
  std::string  map;
  std::string  style;

  glutInit(&argc, argv);                 // Initialize GLUT

  if (argc!=3) {
    std::cerr << "DrawMap <map directory> <style-file>" << std::endl;
    return 1;
  }

  map=argv[1];
  style=argv[2];

  if (!database.Initialize(map,
                           style)) {
    std::cerr << "Cannot open database" << std::endl;
  }

  drawParameter.SetDrawWaysWithFixedWidth(false);
  drawParameter.SetRenderSeaLand(true);
  drawParameter.SetDebugPerformance(true);

  glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_ALPHA);

  glutInitWindowSize(width, height);      // Set the window's initial width & height
  glutInitWindowPosition(50, 50);    // Position the window's initial top-left corner
  window=glutCreateWindow("DrawMapOpenGL"); // Create a window with the given title

  glutDisplayFunc(OnDisplay);          // Register display callback handler for window re-paint
  glutKeyboardFunc(OnKeyPressed);      // Keyboard handling
  glutReshapeFunc(OnResize);

  OnInit();

  glutMainLoop();                    // Enter the event-processing loop

  return 0;
}
