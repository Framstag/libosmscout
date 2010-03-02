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
        int res=sscanf(argv[i],"%zu",&startStep);

        if (res!=1) {
          parameterError=true;
        }
      }
      else {
        parameterError=true;
      }
    }
    else if (strcmp(argv[i],"-e")==0) {
      i++;

      if (i<argc) {
        int res=sscanf(argv[i],"%zu",&endStep);

        if (res!=1) {
          parameterError=true;
        }
      }
      else {
        parameterError=true;
      }
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
    std::cerr << "Import [-s <startstep>] <openstreetmapdata.osm>" << std::endl;
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
