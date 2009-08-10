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

#include <cstring>
#include <iostream>

#include <osmscout/TypeConfig.h>
#include <osmscout/TypeConfigLoader.h>

#include <osmscout/Preprocess.h>
#include <osmscout/GenAreaNodeIndex.h>
#include <osmscout/GenAreaWayIndex.h>
#include <osmscout/GenNodeIndex.h>
#include <osmscout/GenNodeUseIndex.h>
#include <osmscout/GenNodeDat.h>
#include <osmscout/GenCityStreetIndex.h>
#include <osmscout/GenWayDat.h>
#include <osmscout/GenWayIndex.h>

static size_t nodeIndexIntervalSize=500; // Must not be > max(uint16_t)!
static size_t wayIndexIntervalSize=500;  // Must not be > max(uint16_t)!

int main(int argc, char* argv[])
{
  bool        parameterError=false;
  size_t      startStep=1;
  const char* mapfile=NULL;

  // Simple way to analyse command line parameters, but enough for now...
  int i=1;
  while (i<argc) {
    if (strcmp(argv[i],"-s")==0) {
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
        else if (std::string(argv[i])=="4") {
          startStep=4;
        }
        else if (std::string(argv[i])=="5") {
          startStep=5;
        }
        else if (std::string(argv[i])=="6") {
          startStep=6;
        }
        else if (std::string(argv[i])=="7") {
          startStep=7;
        }
        else if (std::string(argv[i])=="8") {
          startStep=8;
        }
        else if (std::string(argv[i])=="9") {
          startStep=9;
        }
        else {
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

  TypeConfig typeConfig;

  if (!LoadTypeConfig("map.ost.xml",typeConfig)) {
    std::cerr << "Cannot load type configuration!" << std::endl;
  }

  if (startStep==1) {
    std::cout << "Preprocess..." << std::endl;
    if (!Preprocess(mapfile,typeConfig)) {
      std::cerr << "Cannot parse input file!" << std::endl;
      return 1;
    }

    startStep++;
  }

  if (startStep==2) {
    std::cout << "Generate 'nodes.dat'..." << std::endl;
    if (!GenerateNodeDat()) {
      std::cerr << "Cannot generate node data file!" << std::endl;
      return 1;
    }

    startStep++;
  }

  if (startStep==3) {
    std::cout << "Generate 'ways.dat'..." << std::endl;
    if (!GenerateWayDat(typeConfig)) {
      std::cerr << "Cannot generate way data file!" << std::endl;
      return 1;
    }

    startStep++;
  }

  if (startStep==4) {
    std::cout << "Generating 'node.idx'..." << std::endl;

    if (!GenerateNodeIndex(nodeIndexIntervalSize)) {
      std::cerr << "Cannot generate node index!" << std::endl;
      return 1;
    }

    startStep++;
  }

  if (startStep==5) {
    std::cout << "Generating 'areanode.idx'..." << std::endl;

    if (!GenerateAreaNodeIndex(nodeIndexIntervalSize)) {
      std::cerr << "Cannot generate area node index!" << std::endl;
      return 1;
    }

    startStep++;
  }

  if (startStep==6) {
    std::cout << "Generating 'way.idx'..." << std::endl;

    if (!GenerateWayIndex(wayIndexIntervalSize)) {
      std::cerr << "Cannot generate way index!" << std::endl;
      return 1;
    }

    startStep++;
  }

  if (startStep==7) {
    std::cout << "Generating 'areaway.idx'..." << std::endl;

    if (!GenerateAreaWayIndex(wayIndexIntervalSize)) {
      std::cerr << "Cannot generate area way index!" << std::endl;
      return 1;
    }

    startStep++;
  }

  if (startStep==8) {
    std::cout << "Generating 'citystreet.idx'..." << std::endl;

    if (!GenerateCityStreetIndex(typeConfig)) {
      std::cerr << "Cannot generate city street index!" << std::endl;
      return 1;
    }

    startStep++;
  }

  if (startStep==9) {
    std::cout << "Generating 'nodeuse.idx'..." << std::endl;

    if (!GenerateNodeUseIndex(typeConfig,nodeIndexIntervalSize)) {
      std::cerr << "Cannot generate node usage index!" << std::endl;
      return 1;
    }

    startStep++;
  }

  std::cout << "done." << std::endl;
}
