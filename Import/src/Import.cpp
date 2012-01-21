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


#include <osmscout/Util.h>

#include <osmscout/util/String.h>

#include <osmscout/import/Import.h>

static bool StringToBool(const char* string, bool& value)
{
  if (strcmp(string,"true")) {
    value=true;

    return true;
  }
  else if (strcmp(string,"false")) {
    value=false;

    return true;
  }

  return false;
}

static const char* BoolToString(bool value)
{
  if (value) {
    return "true";
  }
  else {
    return "false";
  }
}

void DumpHelp(osmscout::ImportParameter& parameter)
{
  std::cout << "Import -h -d -s <start step> -e <end step> [openstreetmapdata.osm|openstreetmapdata.osm.pbf]" << std::endl;
  std::cout << " -h                                   show this help" << std::endl;
  std::cout << " -d                                   show debug output" << std::endl;
  std::cout << " -s <start step>                      set starting step" << std::endl;
  std::cout << " -s <end step>                        set final step" << std::endl;
  std::cout << " --typefile <path>                    path and name of the map.ost file (default: " << parameter.GetTypefile() << ")" << std::endl;
  std::cout << " --destinationDirectory <path>        destination for generated map files (default: " << parameter.GetDestinationDirectory() << ")" << std::endl;
  std::cout << " --numericIndexPageSize <number>      size of an numeric index page in bytes (default: " << parameter.GetNumericIndexPageSize() << ")" << std::endl;
  std::cout << " --rawNodeIndexMemoryMaped true|false memory map raw node index file access (default: " << BoolToString(parameter.GetRawNodeIndexMemoryMaped()) << ")" << std::endl;
  std::cout << " --rawNodeDataMemoryMaped true|false  memory map raw node data file access (default: " << BoolToString(parameter.GetRawNodeDataMemoryMaped()) << ")" << std::endl;
  std::cout << " --rawWayIndexMemoryMaped true|false  memory map raw way index file access (default: " << BoolToString(parameter.GetRawWayIndexMemoryMaped()) << ")" << std::endl;
  std::cout << " --rawWayDataMemoryMaped true|false   memory map raw way data file access (default: " << BoolToString(parameter.GetRawWayDataMemoryMaped()) << ")" << std::endl;
  std::cout << " --nodesLoadSize <number>             number of nodes to load in one step (default: " << parameter.GetNodesLoadSize() << ")" << std::endl;
  std::cout << " --nodeDataCacheSize <number>         node data cache size (default: " << parameter.GetNodeDataCacheSize() << ")" << std::endl;
  std::cout << " --nodeIndexCacheSize <number>        node index cache size (default: " << parameter.GetNodeIndexCacheSize() << ")" << std::endl;
  std::cout << " --wayDataCacheSize <number>          way data cache size (default: " << parameter.GetWayDataCacheSize() << ")" << std::endl;
  std::cout << " --wayIndexCacheSize <number>         way index cache size (default: " << parameter.GetWayIndexCacheSize() << ")" << std::endl;
}

int main(int argc, char* argv[])
{
  osmscout::ImportParameter parameter;
  osmscout::ConsoleProgress progress;
  bool                      parameterError=false;

  std::string               mapfile=parameter.GetMapfile();
  std::string               typefile=parameter.GetTypefile();
  std::string               destinationDirectory=parameter.GetDestinationDirectory();

  size_t                    startStep=parameter.GetStartStep();
  size_t                    endStep=parameter.GetEndStep();

  size_t                    numericIndexPageSize=parameter.GetNumericIndexPageSize();

  bool                      rawNodeIndexMemoryMaped=parameter.GetRawNodeIndexMemoryMaped();
  bool                      rawNodeDataMemoryMaped=parameter.GetRawNodeDataMemoryMaped();

  bool                      rawWayIndexMemoryMaped=parameter.GetRawWayIndexMemoryMaped();
  bool                      rawWayDataMemoryMaped=parameter.GetRawWayDataMemoryMaped();
  size_t                    rawWayBlockSize=parameter.GetRawWayBlockSize();

  size_t                    nodesLoadSize=parameter.GetNodesLoadSize();
  size_t                    nodeDataCacheSize=parameter.GetNodeDataCacheSize();
  size_t                    nodeIndexCacheSize=parameter.GetNodeIndexCacheSize();
  size_t                    wayDataCacheSize=parameter.GetWayDataCacheSize();
  size_t                    wayIndexCacheSize=parameter.GetWayIndexCacheSize();

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
      DumpHelp(parameter);
      return 0;
    }
    else if (strcmp(argv[i],"--typefile")==0) {
      i++;

      if (i<argc) {
        typefile=argv[i];
      }
      else {
        std::cerr << "Missing parameter after --typefile option" << std::endl;
        parameterError=true;
      }
    }
    else if (strcmp(argv[i],"--destinationDirectory")==0) {
      i++;

      if (i<argc) {
        destinationDirectory=argv[i];
      }
      else {
        std::cerr << "Missing parameter after --destinationDirectory option" << std::endl;
        parameterError=true;
      }
    }
    else if (strcmp(argv[i],"--numericIndexPageSize")==0) {
      i++;

      if (i<argc) {
        if (!osmscout::StringToNumber(argv[i],numericIndexPageSize)) {
          std::cerr << "Cannot parse numericIndexPageSize '" << argv[i] << "'" << std::endl;
          parameterError=true;
        }
      }
      else {
        std::cerr << "Missing parameter after --numericIndexPageSize option" << std::endl;
        parameterError=true;
      }
    }
    else if (strcmp(argv[i],"--rawNodeIndexMemoryMaped")==0) {
      i++;

      if (i<argc) {
        if (!StringToBool(argv[i],rawNodeIndexMemoryMaped)) {
          std::cerr << "Cannot parse rawNodeIndexMemoryMaped '" << argv[i] << "'" << std::endl;
          parameterError=true;
        }
      }
      else {
        std::cerr << "Missing parameter after --rawNodeIndexMemoryMaped option" << std::endl;
        parameterError=true;
      }
    }
    else if (strcmp(argv[i],"--rawNodeDataMemoryMaped")==0) {
      i++;

      if (i<argc) {
        if (!StringToBool(argv[i],rawNodeDataMemoryMaped)) {
          std::cerr << "Cannot parse rawNodeDataMemoryMaped '" << argv[i] << "'" << std::endl;
          parameterError=true;
        }
      }
      else {
        std::cerr << "Missing parameter after --rawNodeDataMemoryMaped option" << std::endl;
        parameterError=true;
      }
    }
    else if (strcmp(argv[i],"--rawWayIndexMemoryMaped")==0) {
      i++;

      if (i<argc) {
        if (!StringToBool(argv[i],rawWayIndexMemoryMaped)) {
          std::cerr << "Cannot parse rawWayIndexMemoryMaped '" << argv[i] << "'" << std::endl;
          parameterError=true;
        }
      }
      else {
        std::cerr << "Missing parameter after --rawWayIndexMemoryMaped option" << std::endl;
        parameterError=true;
      }
    }
    else if (strcmp(argv[i],"--rawWayDataMemoryMaped")==0) {
      i++;

      if (i<argc) {
        if (!StringToBool(argv[i],rawWayDataMemoryMaped)) {
          std::cerr << "Cannot parse rawWayDataMemoryMaped '" << argv[i] << "'" << std::endl;
          parameterError=true;
        }
      }
      else {
        std::cerr << "Missing parameter after --rawWayDataMemoryMaped option" << std::endl;
        parameterError=true;
      }
    }
    else if (strcmp(argv[i],"--rawWayBlockSize")==0) {
      i++;

      if (i<argc) {
        if (!osmscout::StringToNumber(argv[i],rawWayBlockSize)) {
          std::cerr << "Cannot parse rawWayBlockSize '" << argv[i] << "'" << std::endl;
          parameterError=true;
        }
      }
      else {
        std::cerr << "Missing parameter after --rawWayBlockSize option" << std::endl;
        parameterError=true;
      }
    }
    else if (strcmp(argv[i],"--nodesLoadSize")==0) {
      i++;

      if (i<argc) {
        if (!osmscout::StringToNumber(argv[i],nodesLoadSize)) {
          std::cerr << "Cannot parse nodesLoadSize '" << argv[i] << "'" << std::endl;
          parameterError=true;
        }
      }
      else {
        std::cerr << "Missing parameter after --nodesLoadSize option" << std::endl;
        parameterError=true;
      }
    }
    else if (strcmp(argv[i],"--nodeDataCacheSize")==0) {
      i++;

      if (i<argc) {
        if (!osmscout::StringToNumber(argv[i],nodeDataCacheSize)) {
          std::cerr << "Cannot parse nodeDataCacheSize '" << argv[i] << "'" << std::endl;
          parameterError=true;
        }
      }
      else {
        std::cerr << "Missing parameter after --nodeDataCacheSize option" << std::endl;
        parameterError=true;
      }
    }
    else if (strcmp(argv[i],"--nodeIndexCacheSize")==0) {
      i++;

      if (i<argc) {
        if (!osmscout::StringToNumber(argv[i],nodeIndexCacheSize)) {
          std::cerr << "Cannot parse nodeIndexCacheSize '" << argv[i] << "'" << std::endl;
          parameterError=true;
        }
      }
      else {
        std::cerr << "Missing parameter after --nodeIndexCacheSize option" << std::endl;
        parameterError=true;
      }
    }
    else if (strcmp(argv[i],"--wayDataCacheSize")==0) {
      i++;

      if (i<argc) {
        if (!osmscout::StringToNumber(argv[i],wayDataCacheSize)) {
          std::cerr << "Cannot parse wayDataCacheSize '" << argv[i] << "'" << std::endl;
          parameterError=true;
        }
      }
      else {
        std::cerr << "Missing parameter after --wayDataCacheSize option" << std::endl;
        parameterError=true;
      }
    }
    else if (strcmp(argv[i],"--wayIndexCacheSize")==0) {
      i++;

      if (i<argc) {
        if (!osmscout::StringToNumber(argv[i],wayIndexCacheSize)) {
          std::cerr << "Cannot parse wayIndexCacheSize '" << argv[i] << "'" << std::endl;
          parameterError=true;
        }
      }
      else {
        std::cerr << "Missing parameter after --wayIndexCacheSize option" << std::endl;
        parameterError=true;
      }
    }
    else if (mapfile.empty()) {
      mapfile=argv[i];
    }
    else {
      std::cerr << "Unknown option: " << argv[i] << std::endl;
      parameterError=true;
    }

    i++;
  }

  if (startStep==1 &&
      mapfile.empty()) {
    parameterError=true;
  }

  if (parameterError) {
    DumpHelp(parameter);
    return 1;
  }

  parameter.SetMapfile(mapfile);
  parameter.SetTypefile(typefile);
  parameter.SetDestinationDirectory(destinationDirectory);
  parameter.SetSteps(startStep,endStep);

  parameter.SetNumericIndexPageSize(numericIndexPageSize);

  parameter.SetRawNodeIndexMemoryMaped(rawNodeIndexMemoryMaped);
  parameter.SetRawNodeDataMemoryMaped(rawNodeDataMemoryMaped);

  parameter.SetRawWayIndexMemoryMaped(rawWayIndexMemoryMaped);
  parameter.SetRawWayDataMemoryMaped(rawWayDataMemoryMaped);
  parameter.SetRawWayBlockSize(rawWayBlockSize);

  parameter.SetNodesLoadSize(nodesLoadSize);
  parameter.SetNodeDataCacheSize(nodeDataCacheSize);
  parameter.SetNodeIndexCacheSize(nodeIndexCacheSize);

  parameter.SetWayDataCacheSize(wayDataCacheSize);
  parameter.SetWayIndexCacheSize(wayIndexCacheSize);

  progress.SetStep("Dump parameter");
  progress.Info(std::string("Mapfile: ")+parameter.GetMapfile());
  progress.Info(std::string("typefile: ")+parameter.GetTypefile());
  progress.Info(std::string("Destination directory: ")+parameter.GetDestinationDirectory());
  progress.Info(std::string("Steps: ")+
                osmscout::NumberToString(parameter.GetStartStep())+
                " - "+
                osmscout::NumberToString(parameter.GetEndStep()));
  progress.Info(std::string("NumericIndexPageSize: ")+
                osmscout::NumberToString(parameter.GetNumericIndexPageSize()));
  progress.Info(std::string("RawNodeIndexMemoryMaped: ")+
                (parameter.GetRawNodeIndexMemoryMaped() ? "true" : "false"));
  progress.Info(std::string("RawNodeDataMemoryMaped: ")+
                (parameter.GetRawNodeDataMemoryMaped() ? "true" : "false"));
  progress.Info(std::string("RawWayIndexMemoryMaped: ")+
                (parameter.GetRawWayIndexMemoryMaped() ? "true" : "false"));
  progress.Info(std::string("RawWayDataMemoryMaped: ")+
                (parameter.GetRawWayDataMemoryMaped() ? "true" : "false"));
  progress.Info(std::string("RawWayBlockSize: ")+
                osmscout::NumberToString(parameter.GetRawWayBlockSize()));
  progress.Info(std::string("NodesLoadSize: ")+
                osmscout::NumberToString(parameter.GetNodesLoadSize()));
  progress.Info(std::string("NodeDataCacheSize: ")+
                osmscout::NumberToString(parameter.GetNodeDataCacheSize()));
  progress.Info(std::string("NodeIndexCacheSize: ")+
                osmscout::NumberToString(parameter.GetNodeIndexCacheSize()));
  progress.Info(std::string("WayDataCacheSize: ")+
                osmscout::NumberToString(parameter.GetWayDataCacheSize()));
  progress.Info(std::string("WayIndexCacheSize: ")+
                osmscout::NumberToString(parameter.GetWayIndexCacheSize()));

  if (osmscout::Import(parameter,progress)) {
    std::cout << "Import OK!" << std::endl;
  }
  else {
    std::cerr << "Import failed!" << std::endl;
  }

  return 0;
}
