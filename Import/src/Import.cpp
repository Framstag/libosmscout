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

#include <string.h>
#include <stdio.h>

#include <iostream>

#include <osmscout/util/File.h>
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
  std::cout << " --typefile <path>                    path and name of the map.ost file (default: " << parameter.GetTypefile() << ")" << std::endl;
  std::cout << " --destinationDirectory <path>        destination for generated map files (default: " << parameter.GetDestinationDirectory() << ")" << std::endl;

  std::cout << " --strictAreas true|false             assure that areas are simple (default: " << BoolToString(parameter.GetStrictAreas()) << ")" << std::endl;

  std::cout << " --numericIndexPageSize <number>      size of an numeric index page in bytes (default: " << parameter.GetNumericIndexPageSize() << ")" << std::endl;

  std::cout << " --coordDataMemoryMaped true|false    memory maped coord data file access (default: " << BoolToString(parameter.GetCoordDataMemoryMaped()) << ")" << std::endl;

  std::cout << " --rawNodeDataMemoryMaped true|false  memory maped raw node data file access (default: " << BoolToString(parameter.GetRawNodeDataMemoryMaped()) << ")" << std::endl;
  std::cout << " --rawNodeDataCacheSize <number>      raw node data cache size (default: " << parameter.GetRawNodeDataCacheSize() << ")" << std::endl;

  std::cout << " --rawWayIndexMemoryMaped true|false  memory maped raw way index file access (default: " << BoolToString(parameter.GetRawWayIndexMemoryMaped()) << ")" << std::endl;
  std::cout << " --rawWayDataMemoryMaped true|false   memory maped raw way data file access (default: " << BoolToString(parameter.GetRawWayDataMemoryMaped()) << ")" << std::endl;
  std::cout << " --rawWayDataCacheSize <number>       raw way data cache size (default: " << parameter.GetRawWayDataCacheSize() << ")" << std::endl;
  std::cout << " --rawWayIndexCacheSize <number>      raw way index cache size (default: " << parameter.GetRawWayIndexCacheSize() << ")" << std::endl;
  std::cout << " --rawWayBlockSize <number>           number of raw ways resolved in block (default: " << parameter.GetRawWayBlockSize() << ")" << std::endl;

  std::cout << " --noSort                             do not sort objects" << std::endl;
  std::cout << " --sortBlockSize <number>             size of one data block during sorting (default: " << parameter.GetSortBlockSize() << ")" << std::endl;

  std::cout << " --areaDataMemoryMaped true|false     memory maped area data file access (default: " << BoolToString(parameter.GetAreaDataMemoryMaped()) << ")" << std::endl;
  std::cout << " --areaDataCacheSize <number>         area data cache size (default: " << parameter.GetAreaDataCacheSize() << ")" << std::endl;

  std::cout << " --wayDataMemoryMaped true|false      memory maped way data file access (default: " << BoolToString(parameter.GetWayDataMemoryMaped()) << ")" << std::endl;
  std::cout << " --wayDataCacheSize <number>          way data cache size (default: " << parameter.GetWayDataCacheSize() << ")" << std::endl;

  std::cout << " --routeNodeBlockSize <number>        number of route nodes resolved in block (default: " << BoolToString(parameter.GetRouteNodeBlockSize()) << ")" << std::endl;
}

bool ParseBoolArgument(int argc,
                       char* argv[],
                       int& currentIndex,
                       bool& value)
{
  int parameterIndex=currentIndex;
  int argumentIndex=currentIndex+1;

  currentIndex+=2;

  if (argumentIndex<argc) {
    if (!StringToBool(argv[argumentIndex],
                      value)) {
      std::cerr << "Cannot parse argument for parameter '" << argv[parameterIndex] << "'" << std::endl;
      return false;
    }
  }
  else {
    std::cerr << "Missing parameter after option '" << argv[parameterIndex] << "'" << std::endl;
    return false;
  }

  return true;
}

bool ParseStringArgument(int argc,
                         char* argv[],
                         int& currentIndex,
                         std::string& value)
{
  int parameterIndex=currentIndex;
  int argumentIndex=currentIndex+1;

  currentIndex+=2;

  if (argumentIndex<argc) {
    value=argv[argumentIndex];
  }
  else {
    std::cerr << "Missing parameter after option '" << argv[parameterIndex] << "'" << std::endl;
    return false;
  }

  return true;
}

bool ParseSizeTArgument(int argc,
                        char* argv[],
                        int& currentIndex,
                        size_t& value)
{
  int parameterIndex=currentIndex;
  int argumentIndex=currentIndex+1;

  currentIndex+=2;

  if (argumentIndex<argc) {
    if (!osmscout::StringToNumber(argv[argumentIndex],
                                  value)) {
      std::cerr << "Cannot parse argument for parameter '" << argv[parameterIndex] << "'" << std::endl;
      return false;
    }
  }
  else {
    std::cerr << "Missing parameter after option '" << argv[parameterIndex] << "'" << std::endl;
    return false;
  }

  return true;
}

bool GetFileSize(const std::string& filename,
                 osmscout::FileOffset& fileSize)
{
  if (!osmscout::GetFileSize(filename,
                             fileSize)) {
    std::cerr << "Cannot read file size of file '" << filename << "'" << std::endl;
    return false;
  }

  return true;
}

bool CountDataSize(osmscout::Progress& progress,
                   const std::string& mapPath,
                   double& dataSize)
{
  std::string              fileName;
  osmscout::FileOffset     fileSize=0;
  std::vector<std::string> files;

  files.push_back("types.dat");
  files.push_back("bounding.dat");

  files.push_back("nodes.dat");
  files.push_back("areas.dat");
  files.push_back("ways.dat");

  files.push_back("areasopt.dat");
  files.push_back("waysopt.dat");

  files.push_back("areanode.idx");
  files.push_back("areaarea.idx");
  files.push_back("areaway.idx");

  files.push_back("location.idx");

  files.push_back("water.idx");

  files.push_back("intersections.dat");
  files.push_back("intersections.idx");
  files.push_back("routefoot.dat");
  files.push_back("routefoot.idx");
  files.push_back("routebicycle.dat");
  files.push_back("routebicycle.idx");
  files.push_back("routecar.dat");
  files.push_back("routecar.idx");

  dataSize=0;

  for (std::vector<std::string>:: const_iterator filename=files.begin();
      filename!=files.end();
      ++filename) {
    std::string filePath=osmscout::AppendFileToDir(mapPath,
                                                   *filename);

    if (!GetFileSize(filePath,
                     fileSize)) {
      return false;
    }

    progress.Info(std::string("File ")+*filename+": "+osmscout::ByteSizeToString(fileSize));

    dataSize+=fileSize;
  }

  return true;
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

  bool                      strictAreas=parameter.GetStrictAreas();

  size_t                    numericIndexPageSize=parameter.GetNumericIndexPageSize();

  size_t                    sortBlockSize=parameter.GetSortBlockSize();

  bool                      coordDataMemoryMaped=parameter.GetCoordDataMemoryMaped();

  bool                      rawNodeDataMemoryMaped=parameter.GetRawNodeDataMemoryMaped();
  size_t                    rawNodeDataCacheSize=parameter.GetRawNodeDataCacheSize();

  bool                      rawWayIndexMemoryMaped=parameter.GetRawWayIndexMemoryMaped();
  bool                      rawWayDataMemoryMaped=parameter.GetRawWayDataMemoryMaped();
  size_t                    rawWayDataCacheSize=parameter.GetRawWayDataCacheSize();
  size_t                    rawWayIndexCacheSize=parameter.GetRawWayIndexCacheSize();
  size_t                    rawWayBlockSize=parameter.GetRawWayBlockSize();

  bool                      areaDataMemoryMaped=parameter.GetAreaDataMemoryMaped();
  size_t                    areaDataCacheSize=parameter.GetAreaDataCacheSize();

  bool                      wayDataMemoryMaped=parameter.GetWayDataMemoryMaped();
  size_t                    wayDataCacheSize=parameter.GetWayDataCacheSize();

  size_t                    routeNodeBlockSize=parameter.GetRouteNodeBlockSize();

  // Simple way to analyse command line parameters, but enough for now...
  int i=1;
  while (i<argc) {
    if (strcmp(argv[i],"-s")==0) {
      parameterError=!ParseSizeTArgument(argc,
                                         argv,
                                         i,
                                         startStep);
    }
    else if (strcmp(argv[i],"-e")==0) {
      parameterError=!ParseSizeTArgument(argc,
                                         argv,
                                         i,
                                         endStep);
    }
    else if (strcmp(argv[i],"-d")==0) {
      progress.SetOutputDebug(true);

      i++;
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
      parameterError=!ParseStringArgument(argc,
                                          argv,
                                          i,
                                          typefile);
    }
    else if (strcmp(argv[i],"--destinationDirectory")==0) {
      parameterError=!ParseStringArgument(argc,
                                          argv,
                                          i,
                                          destinationDirectory);
    }
    else if (strcmp(argv[i],"--strictAreas")==0) {
      parameterError=!ParseBoolArgument(argc,
                                        argv,
                                        i,
                                        strictAreas);
    }
    else if (strcmp(argv[i],"--numericIndexPageSize")==0) {
      parameterError=!ParseSizeTArgument(argc,
                                         argv,
                                         i,
                                         numericIndexPageSize);
    }
    else if (strcmp(argv[i],"--coordDataMemoryMaped")==0) {
      parameterError=!ParseBoolArgument(argc,
                                        argv,
                                        i,
                                        coordDataMemoryMaped);
    }
    else if (strcmp(argv[i],"--rawNodeDataMemoryMaped")==0) {
      parameterError=!ParseBoolArgument(argc,
                                        argv,
                                        i,
                                        rawNodeDataMemoryMaped);
    }
    else if (strcmp(argv[i],"--rawNodeDataCacheSize")==0) {
      parameterError=!ParseSizeTArgument(argc,
                                         argv,
                                         i,
                                         rawNodeDataCacheSize);
    }
    else if (strcmp(argv[i],"--rawWayIndexMemoryMaped")==0) {
      parameterError=!ParseBoolArgument(argc,
                                        argv,
                                        i,
                                        rawWayIndexMemoryMaped);
    }
    else if (strcmp(argv[i],"--rawWayDataMemoryMaped")==0) {
      parameterError=!ParseBoolArgument(argc,
                                        argv,
                                        i,
                                        rawWayDataMemoryMaped);
    }
    else if (strcmp(argv[i],"--rawWayDataCacheSize")==0) {
      parameterError=!ParseSizeTArgument(argc,
                                         argv,
                                         i,
                                         rawWayDataCacheSize);
    }
    else if (strcmp(argv[i],"--rawWayIndexCacheSize")==0) {
      parameterError=!ParseSizeTArgument(argc,
                                         argv,
                                         i,
                                         rawWayIndexCacheSize);
    }
    else if (strcmp(argv[i],"--rawWayBlockSize")==0) {
      parameterError=!ParseSizeTArgument(argc,
                                         argv,
                                         i,
                                         rawWayBlockSize);
    }
    else if (strcmp(argv[i],"-noSort")==0) {
      parameter.SetSortObjects(false);

      i++;
    }
    else if (strcmp(argv[i],"--sortBlockSize")==0) {
      parameterError=!ParseSizeTArgument(argc,
                                         argv,
                                         i,
                                         sortBlockSize);
    }
    else if (strcmp(argv[i],"--areaDataMemoryMaped")==0) {
      parameterError=!ParseBoolArgument(argc,
                                        argv,
                                        i,
                                        areaDataMemoryMaped);
    }
    else if (strcmp(argv[i],"--areaDataCacheSize")==0) {
      parameterError=!ParseSizeTArgument(argc,
                                         argv,
                                         i,
                                         areaDataCacheSize);
    }
    else if (strcmp(argv[i],"--wayDataMemoryMaped")==0) {
      parameterError=!ParseBoolArgument(argc,
                                        argv,
                                        i,
                                        wayDataMemoryMaped);
    }
    else if (strcmp(argv[i],"--wayDataCacheSize")==0) {
      parameterError=!ParseSizeTArgument(argc,
                                         argv,
                                         i,
                                         wayDataCacheSize);
    }
    else if (strcmp(argv[i],"--routeNodeBlockSize")==0) {
      parameterError=!ParseSizeTArgument(argc,
                                         argv,
                                         i,
                                         routeNodeBlockSize);
    }
    else if (mapfile.empty()) {
      mapfile=argv[i];

      i++;
    }
    else {
      std::cerr << "Unknown option: " << argv[i] << std::endl;

      parameterError=true;
      i++;
    }
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

  parameter.SetStrictAreas(strictAreas);

  parameter.SetNumericIndexPageSize(numericIndexPageSize);

  parameter.SetSortBlockSize(sortBlockSize);

  parameter.SetCoordDataMemoryMaped(coordDataMemoryMaped);

  parameter.SetRawNodeDataMemoryMaped(rawNodeDataMemoryMaped);
  parameter.SetRawNodeDataCacheSize(rawNodeDataCacheSize);

  parameter.SetRawWayIndexMemoryMaped(rawWayIndexMemoryMaped);
  parameter.SetRawWayDataMemoryMaped(rawWayDataMemoryMaped);
  parameter.SetRawWayDataCacheSize(rawWayDataCacheSize);
  parameter.SetRawWayIndexCacheSize(rawWayIndexCacheSize);
  parameter.SetRawWayBlockSize(rawWayBlockSize);

  parameter.SetAreaDataMemoryMaped(areaDataMemoryMaped);
  parameter.SetAreaDataCacheSize(areaDataCacheSize);

  parameter.SetWayDataMemoryMaped(wayDataMemoryMaped);
  parameter.SetWayDataCacheSize(wayDataCacheSize);

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

  progress.Info(std::string("StrictAreas: ")+
                (parameter.GetStrictAreas() ? "true" : "false"));

  progress.Info(std::string("NumericIndexPageSize: ")+
                osmscout::NumberToString(parameter.GetNumericIndexPageSize()));

  progress.Info(std::string("CoordDataMemoryMaped: ")+
                (parameter.GetCoordDataMemoryMaped() ? "true" : "false"));

  progress.Info(std::string("RawNodeDataMemoryMaped: ")+
                (parameter.GetRawNodeDataMemoryMaped() ? "true" : "false"));
  progress.Info(std::string("RawNodeDataCacheSize: ")+
                osmscout::NumberToString(parameter.GetRawNodeDataCacheSize()));

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


  progress.Info(std::string("SortObjects: ")+
                (parameter.GetSortObjects() ? "true" : "false"));
  progress.Info(std::string("SortBlockSize: ")+
                osmscout::NumberToString(parameter.GetSortBlockSize()));

  progress.Info(std::string("AreaDataMemoryMaped: ")+
                (parameter.GetAreaDataMemoryMaped() ? "true" : "false"));
  progress.Info(std::string("AreaDataCacheSize: ")+
                osmscout::NumberToString(parameter.GetAreaDataCacheSize()));

  progress.Info(std::string("WayDataMemoryMaped: ")+
                (parameter.GetWayDataMemoryMaped() ? "true" : "false"));
  progress.Info(std::string("WayDataCacheSize: ")+
                osmscout::NumberToString(parameter.GetWayDataCacheSize()));

  progress.Info(std::string("RouteNodeBlockSize: ")+
                osmscout::NumberToString(parameter.GetRouteNodeBlockSize()));

  bool result=osmscout::Import(parameter,
                               progress);

  progress.SetStep("Summary");

  if (result) {

    double dataSize=0;

    if (!CountDataSize(progress,
                       destinationDirectory,
                       dataSize)) {
      progress.Error("Error while retrieving data size");
    }
    else {
      progress.Info(std::string("Resulting data size: ")+osmscout::ByteSizeToString(dataSize));
    }

    progress.Info("Import OK!");
  }
  else {
    progress.Error("Import failed!");
  }

  return 0;
}
