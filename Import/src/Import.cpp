/*
  TravelJinni - Openstreetmap offline viewer
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

#include <cerrno>
#include <cstring>
#include <cstdio>
#include <iostream>

#include <osmscout/Import.h>
#include <osmscout/Util.h>

void DumpHelp()
{
  std::cout << "Import -h -d -s <start step> -e <end step> <openstreetmapdata.osm>" << std::endl;
  std::cout << " -h              show this help" << std::endl;
  std::cout << " -d              show debug output" << std::endl;
  std::cout << " -s <start step> set starting step" << std::endl;
  std::cout << " -s <end step>   set finial step" << std::endl;
}

int main(int argc, char* argv[])
{
  osmscout::ImportParameter parameter;
  osmscout::ConsoleProgress progress;
  bool                      parameterError=false;
  size_t                    startStep=parameter.GetStartStep();
  size_t                    endStep=parameter.GetEndStep();
  const char*               mapfile=NULL;

  // Simple way to analyse command line parameters, but enough for now...
  int i=1;
  while (i<argc) {
    if (strcmp(argv[i],"-s")==0) {
      i++;

      if (i<argc) {
        if (!osmscout::StringToNumber(argv[i],startStep)) {
          std::cerr << "Cannot parse start step '" << argv[i] << "'" << std::endl;
          parameterError=true;
        }
      }
      else {
        std::cerr << "Missing parameter after -s option" << std::endl;
        parameterError=true;
      }
    }
    else if (strcmp(argv[i],"-e")==0) {
      i++;

      if (i<argc) {
        if (!osmscout::StringToNumber(argv[i],endStep)) {
          std::cerr << "Cannot parse end step '" << argv[i] << "'" << std::endl;
          parameterError=true;
        }
      }
      else {
        std::cerr << "Missing parameter after -e option" << std::endl;
        parameterError=true;
      }
    }
    else if (strcmp(argv[i],"-d")==0) {
      progress.SetOutputDebug(true);
    }
    else if (strcmp(argv[i],"-h")==0) {
      DumpHelp();
      return 0;
    }
    else if (mapfile==NULL) {
      mapfile=argv[i];
    }
    else {
      std::cerr << "Unknown option: " << argv[i] << std::endl;
      parameterError=true;
    }

    i++;
  }

  if (mapfile==NULL || parameterError) {
    DumpHelp();
    return 1;
  }

  parameter.SetMapfile(mapfile);
  parameter.SetSteps(startStep,endStep);

  if (osmscout::Import(parameter,progress)) {
    std::cout << "Import OK!" << std::endl;
  }
  else {
    std::cerr << "Import failed!" << std::endl;
  }
}
