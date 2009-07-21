#include <iostream>

#include <string.h>

#include <cairo/cairo.h>

#include "StyleConfig.h"
#include "StyleConfigLoader.h"
#include "TypeConfig.h"
#include "TypeConfigLoader.h"

#include "MapPainter.h"

int main(int argc, char* argv[])
{
  bool        parameterError=false;

  // Simple way to analyse command line parameters, but enough for now...
  int i=1;
  while (i<argc) {
/*    if (strcmp(argv[i],"-s")==0) {
      i++;

      if (i<argc) {
        if (std::string(argv[i])=="1") {
          startStep=1;
        }
        else if (std::string(argv[i])=="2") {
          startStep=2;
        }
        else if (std::string(argv[i])=="3") {
          startStep=3;
        }
        else {
          parameterError=true;
        }
      }
      else {
        parameterError=true;
      }
    }
    else if (strcmp(argv[i],"-m")==0) {
      map=true;
    }
    else if (mapfile==NULL) {
      mapfile=argv[i];
    }
    else {*/
      std::cerr << "Unknown option: " << argv[i] << std::endl;
      parameterError=true;
    /*}*/

    i++;
  }
  if (parameterError) {
    std::cerr << "Import -x <lon> -y <lat> [-m <mag>] [-w <width> -h <height>]" << std::endl;
    return 1;
  }

  std::string     path=".";
  TypeConfig      typeConfig;
  StyleConfig     styleConfig;
  Database        database;

  if (!LoadTypeConfig("types.xml",typeConfig)) {
    std::cerr << "Cannot load type configuration!" << std::endl;
  }

  if (!LoadStyleConfig("style.xml",typeConfig,styleConfig)) {
    std::cerr << "Cannot load style configuration!" << std::endl;
  }

  if (!database.Initialize(path)) {
    std::cerr << "Cannot initialize cache!" << std::endl;
    return 1;
  }

  MapPainter mapPainter(database);

  std::cout << "Drawing map..." << std::endl;

  // Wuppertal/Adlerbrücke
  //mapPainter.DrawMap(7.18668,52.26216,64);
  // Center of NRW
  //mapPainter.DrawMap(7.670765,51.427425,128,800,480);
  // Germany
  //mapPainter.DrawMap(9,51.427425,16,800,480);
  // Beethoven statue, Bonn
  //mapPainter.DrawMap(7.09912,50.73431,2*2*1024,800,480);
  // Promenadenweg 146
  mapPainter.PrintMap(styleConfig,7.13601,50.68924,2*2*2*2*1024,800,480);

  std::cout << "done." << std::endl;
}
