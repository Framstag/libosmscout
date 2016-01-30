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
#include <memory>

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

static std::string VehcileMaskToString(osmscout::VehicleMask vehicleMask)
{
  std::string result;

  if (vehicleMask & osmscout::vehicleFoot) {
    if (!result.empty()) {
      result+="+";
    }
    result+="foot";
  }

  if (vehicleMask & osmscout::vehicleBicycle) {
    if (!result.empty()) {
      result+="+";
    }
    result+="bicycle";
  }

  if (vehicleMask & osmscout::vehicleCar) {
    if (!result.empty()) {
      result+="+";
    }
    result+="car";
  }

  return result;
}

void DumpHelp(osmscout::ImportParameter& parameter)
{
  std::cout << "Import -h -d -s <start step> -e <end step> [openstreetmapdata.osm|openstreetmapdata.osm.pbf]..." << std::endl;
  std::cout << " -h|--help                            show this help" << std::endl;
  std::cout << " -d                                   show debug output" << std::endl;
  std::cout << " -s <start step>                      set starting step" << std::endl;
  std::cout << " -s <end step>                        set final step" << std::endl;
  std::cout << " --eco                                do delete temporary fiels ASAP" << std::endl;
  std::cout << " --typefile <path>                    path and name of the map.ost file (default: " << parameter.GetTypefile() << ")" << std::endl;
  std::cout << " --destinationDirectory <path>        destination for generated map files (default: " << parameter.GetDestinationDirectory() << ")" << std::endl;

  std::cout << " --router <router description>        definition of a router (default: car,bicycle,foot:router)" << std::endl;

  std::cout << " --strictAreas true|false             assure that areas are simple (default: " << BoolToString(parameter.GetStrictAreas()) << ")" << std::endl;

  std::cout << " --numericIndexPageSize <number>      size of an numeric index page in bytes (default: " << parameter.GetNumericIndexPageSize() << ")" << std::endl;

  std::cout << " --coordDataMemoryMaped true|false    memory maped coord data file access (default: " << BoolToString(parameter.GetCoordDataMemoryMaped()) << ")" << std::endl;

  std::cout << " --rawNodeDataMemoryMaped true|false  memory maped raw node data file access (default: " << BoolToString(parameter.GetRawNodeDataMemoryMaped()) << ")" << std::endl;

  std::cout << " --rawWayIndexMemoryMaped true|false  memory maped raw way index file access (default: " << BoolToString(parameter.GetRawWayIndexMemoryMaped()) << ")" << std::endl;
  std::cout << " --rawWayDataMemoryMaped true|false   memory maped raw way data file access (default: " << BoolToString(parameter.GetRawWayDataMemoryMaped()) << ")" << std::endl;
  std::cout << " --rawWayIndexCacheSize <number>      raw way index cache size (default: " << parameter.GetRawWayIndexCacheSize() << ")" << std::endl;
  std::cout << " --rawWayBlockSize <number>           number of raw ways resolved in block (default: " << parameter.GetRawWayBlockSize() << ")" << std::endl;

  std::cout << " --noSort                             do not sort objects" << std::endl;
  std::cout << " --sortBlockSize <number>             size of one data block during sorting (default: " << parameter.GetSortBlockSize() << ")" << std::endl;

  std::cout << " --areaDataMemoryMaped true|false     memory maped area data file access (default: " << BoolToString(parameter.GetAreaDataMemoryMaped()) << ")" << std::endl;
  std::cout << " --areaDataCacheSize <number>         area data cache size (default: " << parameter.GetAreaDataCacheSize() << ")" << std::endl;

  std::cout << " --wayDataMemoryMaped true|false      memory maped way data file access (default: " << BoolToString(parameter.GetWayDataMemoryMaped()) << ")" << std::endl;
  std::cout << " --wayDataCacheSize <number>          way data cache size (default: " << parameter.GetWayDataCacheSize() << ")" << std::endl;

  std::cout << " --routeNodeBlockSize <number>        number of route nodes resolved in block (default: " << parameter.GetRouteNodeBlockSize() << ")" << std::endl;
}

bool ParseBoolArgument(int argc,
                       char* argv[],
                       int& currentIndex,
                       bool& value)
{
  int parameterIndex=currentIndex;
  int argumentIndex=currentIndex+1;

  currentIndex+=2;

  if (argumentIndex>=argc) {
    std::cerr << "Missing parameter after option '" << argv[parameterIndex] << "'" << std::endl;
    return false;
  }

  if (!StringToBool(argv[argumentIndex],
                    value)) {
    std::cerr << "Cannot parse argument for parameter '" << argv[parameterIndex] << "'" << std::endl;
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

  if (argumentIndex>=argc) {
    std::cerr << "Missing parameter after option '" << argv[parameterIndex] << "'" << std::endl;
    return false;
  }

  value=argv[argumentIndex];

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

  if (argumentIndex>=argc) {
    std::cerr << "Missing parameter after option '" << argv[parameterIndex] << "'" << std::endl;
    return false;
  }

  if (!osmscout::StringToNumber(argv[argumentIndex],
                                value)) {
    std::cerr << "Cannot parse argument for parameter '" << argv[parameterIndex] << "'" << std::endl;
    return false;
  }

  return true;
}

osmscout::ImportParameter::RouterRef ParseRouterArgument(int argc,
                                                         char* argv[],
                                                         int& currentIndex)
{
  int                   parameterIndex=currentIndex;
  int                   argumentIndex=currentIndex+1;
  osmscout::VehicleMask vehicleMask=0;
  std::string           filenamebase;

  currentIndex+=2;

  if (argumentIndex>=argc) {
    std::cerr << "Missing parameter after option '" << argv[parameterIndex] << "'" << std::endl;
    return NULL;
  }

  std::string argument=argv[argumentIndex];
  size_t      pos=argument.rfind(':');

  if (pos==std::string::npos) {
    std::cerr << "Cannot separate vehicles from filename base in router definition '" << argument << "'" << std::endl;
    return NULL;
  }

  filenamebase=argument.substr(pos+1);

  if (filenamebase.empty()) {
    std::cerr << "Empty filename base in router definition '" << argument << "'" << std::endl;
    return NULL;
  }

  std::string vehicles=argument.substr(0,pos);

  if (vehicles.empty()) {
    std::cerr << "Empty vehicle list in router definition '" << argument << "'" << std::endl;
    return NULL;
  }

  size_t start=0;
  size_t devider=vehicles.find(',',start);

  while (start<vehicles.length()) {
    if (devider==std::string::npos) {
      devider=vehicles.length();
    }

    std::string vehicle=vehicles.substr(start,devider-start);

    if (vehicle=="car") {
      vehicleMask = vehicleMask | osmscout::vehicleCar;
    }
    else if (vehicle=="bicycle") {
      vehicleMask = vehicleMask | osmscout::vehicleBicycle;
    }
    else if (vehicle=="foot") {
      vehicleMask = vehicleMask | osmscout::vehicleFoot;
    }
    else {
      std::cerr << "Empty vehicle '" << vehicle << "' in router definition '" << argument << "'" << std::endl;
      return NULL;
    }

    start=devider+1;
    devider=vehicles.find(',',start);
  }

  return std::make_shared<osmscout::ImportParameter::Router>(vehicleMask,filenamebase);
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

bool DumpDataSize(const osmscout::ImportParameter& parameter,
                  const osmscout::Importer& importer,
                  osmscout::Progress& progress)
{
  osmscout::FileOffset dataSize=0;

  progress.Info("Mandatory files:");

  for (const auto& filename : importer.GetProvidedFiles()) {
    osmscout::FileOffset fileSize=0;
    std::string          filePath=osmscout::AppendFileToDir(parameter.GetDestinationDirectory(),
                                                   filename);

    if (!GetFileSize(filePath,
                     fileSize)) {
      return false;
    }

    progress.Info(std::string("File ")+filename+": "+osmscout::ByteSizeToString(fileSize));

    dataSize+=fileSize;
  }

  progress.Info(std::string("=> ")+osmscout::ByteSizeToString(dataSize));

  progress.Info("Optional files:");

  dataSize=0;
  for (const auto& filename : importer.GetProvidedOptionalFiles()) {
    osmscout::FileOffset fileSize=0;
    std::string          filePath=osmscout::AppendFileToDir(parameter.GetDestinationDirectory(),
                                                            filename);

    if (!GetFileSize(filePath,
                     fileSize)) {
      return false;
    }

    progress.Info(std::string("File ")+filename+": "+osmscout::ByteSizeToString(fileSize));

    dataSize+=fileSize;
  }

  progress.Info(std::string("=> ")+osmscout::ByteSizeToString(dataSize));

  return true;
}

int main(int argc, char* argv[])
{
  osmscout::ImportParameter parameter;
  osmscout::ConsoleProgress progress;
  bool                      parameterError=false;
  bool                      firstRouterOption=true;

  std::list<std::string>    mapfiles;

  osmscout::VehicleMask     defaultVehicleMask=osmscout::vehicleBicycle|osmscout::vehicleFoot|osmscout::vehicleCar;

  parameter.AddRouter(osmscout::ImportParameter::Router(defaultVehicleMask,
                                                        "router"));

  // Simple way to analyze command line parameters, but enough for now...
  int i=1;
  while (i<argc) {
    if (strcmp(argv[i],"-h")==0 ||
        strcmp(argv[i],"-?")==0 ||
        strcmp(argv[i],"--help")==0) {
      DumpHelp(parameter);

      return 0;
    }
    else if (strcmp(argv[i],"-s")==0) {
      size_t startStep;

      if (ParseSizeTArgument(argc,
                             argv,
                             i,
                             startStep)) {
        parameter.SetSteps(startStep,
                           parameter.GetEndStep());
      }
      else {
        parameterError=true;
      }
    }
    else if (strcmp(argv[i],"-e")==0) {
      size_t endStep;

      if (ParseSizeTArgument(argc,
                             argv,
                             i,
                             endStep)) {
        parameter.SetSteps(parameter.GetStartStep(),
                           endStep);

      }
      else {
        parameterError=true;
      }
    }
    else if (strcmp(argv[i],"--eco")==0) {
      bool eco;

      if (ParseBoolArgument(argc,
                            argv,
                            i,
                            eco)) {
        parameter.SetEco(eco);
      }
      else {
        parameterError=true;
      }
    }
    else if (strcmp(argv[i],"-d")==0) {
      progress.SetOutputDebug(true);

      i++;
    }
    else if (strcmp(argv[i],"--typefile")==0) {
      std::string typefile;

      if (ParseStringArgument(argc,
                              argv,
                              i,
                              typefile)) {
        parameter.SetTypefile(typefile);
      }
      else {
        parameterError=true;
      }
    }
    else if (strcmp(argv[i],"--destinationDirectory")==0) {
      std::string destinationDirectory;

      if (ParseStringArgument(argc,
                              argv,
                              i,
                              destinationDirectory)) {
        parameter.SetDestinationDirectory(destinationDirectory);
      }
      else {
        parameterError=true;
      }
    }
    else if (strcmp(argv[i],"--router")==0) {
      if (firstRouterOption) {
        parameter.ClearRouter();
        firstRouterOption=false;
      }

      osmscout::ImportParameter::RouterRef router=ParseRouterArgument(argc,
                                                                      argv,
                                                                      i);
      if (router) {
        parameter.AddRouter(*router);
      }
      else {
        parameterError=true;
      }

    }
    else if (strcmp(argv[i],"--strictAreas")==0) {
      bool strictAreas;

      if (ParseBoolArgument(argc,
                            argv,
                            i,
                            strictAreas)) {
        parameter.SetStrictAreas(strictAreas);
      }
      else {
        parameterError=true;
      }
    }
    else if (strcmp(argv[i],"--numericIndexPageSize")==0) {
      size_t numericIndexPageSize;

      if (ParseSizeTArgument(argc,
                             argv,
                             i,
                             numericIndexPageSize)) {
        parameter.SetNumericIndexPageSize(numericIndexPageSize);
      }
      else {
        parameterError=true;
      }
    }
    else if (strcmp(argv[i],"--coordDataMemoryMaped")==0) {
      bool coordDataMemoryMaped;

      if (ParseBoolArgument(argc,
                            argv,
                            i,
                            coordDataMemoryMaped)) {
        parameter.SetCoordDataMemoryMaped(coordDataMemoryMaped);
      }
      else {
        parameterError=true;
      }
    }
    else if (strcmp(argv[i],"--rawNodeDataMemoryMaped")==0) {
      bool rawNodeDataMemoryMaped;

      if (ParseBoolArgument(argc,
                            argv,
                            i,
                            rawNodeDataMemoryMaped)) {
        parameter.SetRawNodeDataMemoryMaped(rawNodeDataMemoryMaped);
      }
      else {
        parameterError=true;
      }
    }
    else if (strcmp(argv[i],"--rawWayIndexMemoryMaped")==0) {
      bool rawWayIndexMemoryMaped;

      if (ParseBoolArgument(argc,
                            argv,
                            i,
                            rawWayIndexMemoryMaped)) {
        parameter.SetRawWayIndexMemoryMaped(rawWayIndexMemoryMaped);
      }
      else {
        parameterError=true;
      }
    }
    else if (strcmp(argv[i],"--rawWayDataMemoryMaped")==0) {
      bool rawWayDataMemoryMaped;

      if (ParseBoolArgument(argc,
                            argv,
                            i,
                            rawWayDataMemoryMaped)) {
        parameter.SetRawWayDataMemoryMaped(rawWayDataMemoryMaped);
      }
      else {
        parameterError=true;
      }
    }
    else if (strcmp(argv[i],"--rawWayIndexCacheSize")==0) {
      size_t rawWayIndexCacheSize;

      if (ParseSizeTArgument(argc,
                             argv,
                             i,
                             rawWayIndexCacheSize)) {
        parameter.SetRawWayIndexCacheSize(rawWayIndexCacheSize);
      }
      else {
        parameterError=true;
      }
    }
    else if (strcmp(argv[i],"--rawWayBlockSize")==0) {
      size_t rawWayBlockSize;

      if (ParseSizeTArgument(argc,
                             argv,
                             i,
                             rawWayBlockSize)) {
        parameter.SetRawWayBlockSize(rawWayBlockSize);
      }
      else {
        parameterError=true;
      }
    }
    else if (strcmp(argv[i],"-noSort")==0) {
      parameter.SetSortObjects(false);

      i++;
    }
    else if (strcmp(argv[i],"--sortBlockSize")==0) {
      size_t sortBlockSize;

      if (ParseSizeTArgument(argc,
                             argv,
                             i,
                             sortBlockSize)) {
        parameter.SetSortBlockSize(sortBlockSize);
      }
      else {
        parameterError=true;
      }
    }
    else if (strcmp(argv[i],"--areaDataMemoryMaped")==0) {
      bool areaDataMemoryMaped;

      if (ParseBoolArgument(argc,
                            argv,
                            i,
                            areaDataMemoryMaped)) {
        parameter.SetAreaDataMemoryMaped(areaDataMemoryMaped);
      }
      else {
        parameterError=true;
      }
    }
    else if (strcmp(argv[i],"--areaDataCacheSize")==0) {
      size_t areaDataCacheSize;

      if (ParseSizeTArgument(argc,
                             argv,
                             i,
                             areaDataCacheSize)) {
        parameter.SetAreaDataCacheSize(areaDataCacheSize);
      }
      else {
        parameterError=true;
      }
    }
    else if (strcmp(argv[i],"--wayDataMemoryMaped")==0) {
      bool wayDataMemoryMaped;

      if (ParseBoolArgument(argc,
                            argv,
                            i,
                            wayDataMemoryMaped)) {
        parameter.SetWayDataMemoryMaped(wayDataMemoryMaped);
      }
      else {
        parameterError=true;
      }
    }
    else if (strcmp(argv[i],"--wayDataCacheSize")==0) {
      size_t wayDataCacheSize;

      if (ParseSizeTArgument(argc,
                             argv,
                             i,
                             wayDataCacheSize)) {
        parameter.SetWayDataCacheSize(wayDataCacheSize);
      }
      else {
        parameterError=true;
      }
    }
    else if (strcmp(argv[i],"--routeNodeBlockSize")==0) {
      size_t routeNodeBlockSize;

      if (ParseSizeTArgument(argc,
                             argv,
                             i,
                             routeNodeBlockSize)) {
        parameter.SetRouteNodeBlockSize(routeNodeBlockSize);
      }
      else {
        parameterError=true;
      }
    }
    else if (strncmp(argv[i],"--",2)==0) {
      std::cerr << "Unknown option: " << argv[i] << std::endl;

      parameterError=true;
      i++;
    }
    else {
      mapfiles.push_back(argv[i]);

      i++;
    }
  }

  if (parameter.GetStartStep()==1 &&
      mapfiles.empty()) {
    parameterError=true;
  }

  if (parameterError) {
    DumpHelp(parameter);
    return 1;
  }

  parameter.SetMapfiles(mapfiles);

  parameter.SetOptimizationWayMethod(osmscout::TransPolygon::quality);

  progress.SetStep("Dump parameter");
  for (const auto& filename : parameter.GetMapfiles()) {
    progress.Info(std::string("Mapfile: ")+filename);
  }

  progress.Info(std::string("typefile: ")+parameter.GetTypefile());
  progress.Info(std::string("Destination directory: ")+parameter.GetDestinationDirectory());
  progress.Info(std::string("Steps: ")+
                osmscout::NumberToString(parameter.GetStartStep())+
                " - "+
                osmscout::NumberToString(parameter.GetEndStep()));
  progress.Info(std::string("Eco: ")+
                (parameter.IsEco() ? "true" : "false"));

  for (const auto& router : parameter.GetRouter()) {
    progress.Info(std::string("Router: ")+VehcileMaskToString(router.GetVehicleMask())+ " - '"+router.GetFilenamebase()+"'");
  }

  progress.Info(std::string("StrictAreas: ")+
                (parameter.GetStrictAreas() ? "true" : "false"));

  progress.Info(std::string("NumericIndexPageSize: ")+
                osmscout::NumberToString(parameter.GetNumericIndexPageSize()));

  progress.Info(std::string("CoordDataMemoryMaped: ")+
                (parameter.GetCoordDataMemoryMaped() ? "true" : "false"));

  progress.Info(std::string("RawNodeDataMemoryMaped: ")+
                (parameter.GetRawNodeDataMemoryMaped() ? "true" : "false"));

  progress.Info(std::string("RawWayIndexMemoryMaped: ")+
                (parameter.GetRawWayIndexMemoryMaped() ? "true" : "false"));
  progress.Info(std::string("RawWayDataMemoryMaped: ")+
                (parameter.GetRawWayDataMemoryMaped() ? "true" : "false"));
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

  osmscout::Importer importer(parameter);

  bool result=importer.Import(progress);

  progress.SetStep("Summary");

  if (result) {

    if (!DumpDataSize(parameter,
                      importer,
                      progress)) {
      progress.Error("Error while retrieving data size");
    }
    progress.Info("Import OK!");
  }
  else {
    progress.Error("Import failed!");
  }

  return 0;
}
