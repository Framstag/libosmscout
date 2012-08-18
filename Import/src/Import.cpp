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

#include <osmscout/util/String.h>

#include <osmscout/import/Import.h>

static bool StringToBool(const char* string, bool& value)
{
  if (strcmp(string,"true")==0) {
    value=true;

    return true;
  }
  else if (strcmp(string,"false")==0) {
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
  std::cout << " -h|--help                            show this help" << std::endl;
  std::cout << " -d                                   show debug output" << std::endl;
  std::cout << " -s <start step>                      set starting step" << std::endl;
  std::cout << " -s <end step>                        set final step" << std::endl;
  std::cout << " -r                                   renumber ids" << std::endl;
  std::cout << " --typefile <path>                    path and name of the map.ost file (default: " << parameter.GetTypefile() << ")" << std::endl;
  std::cout << " --destinationDirectory <path>        destination for generated map files (default: " << parameter.GetDestinationDirectory() << ")" << std::endl;

  std::cout << " --numericIndexPageSize <number>      size of an numeric index page in bytes (default: " << parameter.GetNumericIndexPageSize() << ")" << std::endl;

  std::cout << " --rawNodeIndexMemoryMaped true|false memory map raw node index file access (default: " << BoolToString(parameter.GetRawNodeIndexMemoryMaped()) << ")" << std::endl;
  std::cout << " --rawNodeDataMemoryMaped true|false  memory map raw node data file access (default: " << BoolToString(parameter.GetRawNodeDataMemoryMaped()) << ")" << std::endl;
  std::cout << " --rawNodeDataCacheSize <number>      raw node data cache size (default: " << parameter.GetRawNodeDataCacheSize() << ")" << std::endl;
  std::cout << " --rawNodeIndexCacheSize <number>     raw node index cache size (default: " << parameter.GetRawNodeIndexCacheSize() << ")" << std::endl;

  std::cout << " --rawWayIndexMemoryMaped true|false  memory map raw way index file access (default: " << BoolToString(parameter.GetRawWayIndexMemoryMaped()) << ")" << std::endl;
  std::cout << " --rawWayDataMemoryMaped true|false   memory map raw way data file access (default: " << BoolToString(parameter.GetRawWayDataMemoryMaped()) << ")" << std::endl;
  std::cout << " --rawWayDataCacheSize <number>       raw way data cache size (default: " << parameter.GetRawWayDataCacheSize() << ")" << std::endl;
  std::cout << " --rawWayIndexCacheSize <number>      raw way index cache size (default: " << parameter.GetRawWayIndexCacheSize() << ")" << std::endl;
  std::cout << " --rawWayBlockSize <number>           number of raw ways resolved in block (default: " << parameter.GetRawWayBlockSize() << ")" << std::endl;

  std::cout << " --renumberBlockSize <number>         size of one data block during renumbering (default: " << parameter.GetRenumberBlockSize() << ")" << std::endl;

  std::cout << " --wayIndexMemoryMaped true|false     memory map way index file access (default: " << BoolToString(parameter.GetWayIndexMemoryMaped()) << ")" << std::endl;
  std::cout << " --wayDataMemoryMaped true|false      memory map way data file access (default: " << BoolToString(parameter.GetWayDataMemoryMaped()) << ")" << std::endl;
  std::cout << " --wayDataCacheSize <number>          way data cache size (default: " << parameter.GetWayDataCacheSize() << ")" << std::endl;
  std::cout << " --wayIndexCacheSize <number>         way index cache size (default: " << parameter.GetWayIndexCacheSize() << ")" << std::endl;

  std::cout << " --routeNodeBlockSize <number>        number of route nodes resolved in block (default: " << BoolToString(parameter.GetRouteNodeBlockSize()) << ")" << std::endl;
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

  size_t                    renumberBlockSize=parameter.GetRenumberBlockSize();

  bool                      rawNodeIndexMemoryMaped=parameter.GetRawNodeIndexMemoryMaped();
  bool                      rawNodeDataMemoryMaped=parameter.GetRawNodeDataMemoryMaped();
  size_t                    rawNodeDataCacheSize=parameter.GetRawNodeDataCacheSize();
  size_t                    rawNodeIndexCacheSize=parameter.GetRawNodeIndexCacheSize();

  bool                      rawWayIndexMemoryMaped=parameter.GetRawWayIndexMemoryMaped();
  bool                      rawWayDataMemoryMaped=parameter.GetRawWayDataMemoryMaped();
  size_t                    rawWayDataCacheSize=parameter.GetRawWayDataCacheSize();
  size_t                    rawWayIndexCacheSize=parameter.GetRawWayIndexCacheSize();
  size_t                    rawWayBlockSize=parameter.GetRawWayBlockSize();


  bool                      wayIndexMemoryMaped=parameter.GetWayIndexMemoryMaped();
  bool                      wayDataMemoryMaped=parameter.GetWayDataMemoryMaped();
  size_t                    wayDataCacheSize=parameter.GetWayDataCacheSize();
  size_t                    wayIndexCacheSize=parameter.GetWayIndexCacheSize();

  size_t                    routeNodeBlockSize=parameter.GetRouteNodeBlockSize();

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
    else if (strcmp(argv[i],"-r")==0) {
      parameter.SetRenumberIds(true);
    }
    else if (strcmp(argv[i],"-d")==0) {
      progress.SetOutputDebug(true);
    }
    else if (strcmp(argv[i],"-h")==0) {
      DumpHelp(parameter);
      return 0;
    }
    else if (strcmp(argv[i],"--help")==0) {
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
    else if (strcmp(argv[i],"--rawNodeDataCacheSize")==0) {
      i++;

      if (i<argc) {
        if (!osmscout::StringToNumber(argv[i],rawNodeDataCacheSize)) {
          std::cerr << "Cannot parse rawNodeDataCacheSize '" << argv[i] << "'" << std::endl;
          parameterError=true;
        }
      }
      else {
        std::cerr << "Missing parameter after --rawNodeDataCacheSize option" << std::endl;
        parameterError=true;
      }
    }
    else if (strcmp(argv[i],"--rawNodeIndexCacheSize")==0) {
      i++;

      if (i<argc) {
        if (!osmscout::StringToNumber(argv[i],rawNodeIndexCacheSize)) {
          std::cerr << "Cannot parse rawNodeIndexCacheSize '" << argv[i] << "'" << std::endl;
          parameterError=true;
        }
      }
      else {
        std::cerr << "Missing parameter after --rawNodeIndexCacheSize option" << std::endl;
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
    else if (strcmp(argv[i],"--rawWayDataCacheSize")==0) {
      i++;

      if (i<argc) {
        if (!osmscout::StringToNumber(argv[i],rawWayDataCacheSize)) {
          std::cerr << "Cannot parse rawWayDataCacheSize '" << argv[i] << "'" << std::endl;
          parameterError=true;
        }
      }
      else {
        std::cerr << "Missing parameter after --rawWayDataCacheSize option" << std::endl;
        parameterError=true;
      }
    }
    else if (strcmp(argv[i],"--rawWayIndexCacheSize")==0) {
      i++;

      if (i<argc) {
        if (!osmscout::StringToNumber(argv[i],rawWayIndexCacheSize)) {
          std::cerr << "Cannot parse rawWayIndexCacheSize '" << argv[i] << "'" << std::endl;
          parameterError=true;
        }
      }
      else {
        std::cerr << "Missing parameter after --rawWayIndexCacheSize option" << std::endl;
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
    else if (strcmp(argv[i],"--renumberBlockSize")==0) {
      i++;

      if (i<argc) {
        if (!osmscout::StringToNumber(argv[i],renumberBlockSize)) {
          std::cerr << "Cannot parse renumberBlockSize '" << argv[i] << "'" << std::endl;
          parameterError=true;
        }
      }
      else {
        std::cerr << "Missing parameter after --renumberBlockSize option" << std::endl;
        parameterError=true;
      }
    }
    else if (strcmp(argv[i],"--wayIndexMemoryMaped")==0) {
      i++;

      if (i<argc) {
        if (!StringToBool(argv[i],wayIndexMemoryMaped)) {
          std::cerr << "Cannot parse wayIndexMemoryMaped '" << argv[i] << "'" << std::endl;
          parameterError=true;
        }
      }
      else {
        std::cerr << "Missing parameter after --wayIndexMemoryMaped option" << std::endl;
        parameterError=true;
      }
    }
    else if (strcmp(argv[i],"--wayDataMemoryMaped")==0) {
      i++;

      if (i<argc) {
        if (!StringToBool(argv[i],wayDataMemoryMaped)) {
          std::cerr << "Cannot parse wayDataMemoryMaped '" << argv[i] << "'" << std::endl;
          parameterError=true;
        }
      }
      else {
        std::cerr << "Missing parameter after --wayDataMemoryMaped option" << std::endl;
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
    else if (strcmp(argv[i],"--routeNodeBlockSize")==0) {
      i++;

      if (i<argc) {
        if (!osmscout::StringToNumber(argv[i],routeNodeBlockSize)) {
          std::cerr << "Cannot parse routeNodeBlockSize '" << argv[i] << "'" << std::endl;
          parameterError=true;
        }
      }
      else {
        std::cerr << "Missing parameter after --routeNodeBlockSize option" << std::endl;
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

  parameter.SetRenumberBlockSize(renumberBlockSize);

  parameter.SetRawNodeIndexMemoryMaped(rawNodeIndexMemoryMaped);
  parameter.SetRawNodeDataMemoryMaped(rawNodeDataMemoryMaped);
  parameter.SetRawNodeDataCacheSize(rawNodeDataCacheSize);
  parameter.SetRawNodeIndexCacheSize(rawNodeIndexCacheSize);

  parameter.SetRawWayIndexMemoryMaped(rawWayIndexMemoryMaped);
  parameter.SetRawWayDataMemoryMaped(rawWayDataMemoryMaped);
  parameter.SetRawWayDataCacheSize(rawWayDataCacheSize);
  parameter.SetRawWayIndexCacheSize(rawWayIndexCacheSize);
  parameter.SetRawWayBlockSize(rawWayBlockSize);


  parameter.SetWayIndexMemoryMaped(wayIndexMemoryMaped);
  parameter.SetWayDataMemoryMaped(wayDataMemoryMaped);
  parameter.SetWayDataCacheSize(wayDataCacheSize);
  parameter.SetWayIndexCacheSize(wayIndexCacheSize);

  parameter.SetRouteNodeBlockSize(routeNodeBlockSize);

  parameter.SetOptimizationWayMethod(osmscout::TransPolygon::quality);

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
  progress.Info(std::string("RawNodeDataCacheSize: ")+
                osmscout::NumberToString(parameter.GetRawNodeDataCacheSize()));
  progress.Info(std::string("RawNodeIndexCacheSize: ")+
                osmscout::NumberToString(parameter.GetRawNodeIndexCacheSize()));

  progress.Info(std::string("RawWayIndexMemoryMaped: ")+
                (parameter.GetRawWayIndexMemoryMaped() ? "true" : "false"));
  progress.Info(std::string("RawWayDataMemoryMaped: ")+
                (parameter.GetRawWayDataMemoryMaped() ? "true" : "false"));
  progress.Info(std::string("RawWayDataCacheSize: ")+
                osmscout::NumberToString(parameter.GetRawWayDataCacheSize()));
  progress.Info(std::string("RawWayIndexCacheSize: ")+
                osmscout::NumberToString(parameter.GetRawWayIndexCacheSize()));
  progress.Info(std::string("RawWayBlockSize: ")+
                osmscout::NumberToString(parameter.GetRawWayBlockSize()));


  progress.Info(std::string("RenumberIds: ")+
                (parameter.GetRenumberIds() ? "true" : "false"));
  progress.Info(std::string("RenumberBlockSize: ")+
                osmscout::NumberToString(parameter.GetRenumberBlockSize()));

  progress.Info(std::string("WayIndexMemoryMaped: ")+
                (parameter.GetWayIndexMemoryMaped() ? "true" : "false"));
  progress.Info(std::string("WayDataMemoryMaped: ")+
                (parameter.GetWayDataMemoryMaped() ? "true" : "false"));
  progress.Info(std::string("WayDataCacheSize: ")+
                osmscout::NumberToString(parameter.GetWayDataCacheSize()));
  progress.Info(std::string("WayIndexCacheSize: ")+
                osmscout::NumberToString(parameter.GetWayIndexCacheSize()));

  progress.Info(std::string("RouteNodeBlockSize: ")+
                osmscout::NumberToString(parameter.GetRouteNodeBlockSize()));

  if (osmscout::Import(parameter,progress)) {
    std::cout << "Import OK!" << std::endl;
  }
  else {
    std::cerr << "Import failed!" << std::endl;
  }

  return 0;
}
