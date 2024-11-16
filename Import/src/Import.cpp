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
#include <cstdio>

#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include <optional>

#include <osmscout/cli/CmdLineParsing.h>
#include <osmscout/util/String.h>

#include <osmscout/io/File.h>

#include <osmscoutimport/Import.h>

static std::string VehicleMaskToString(osmscout::VehicleMask vehicleMask)
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

std::string TextIndexVariantStr(osmscout::ImportParameter::TextIndexVariant variant)
{
  switch (variant) {
    case osmscout::ImportParameter::TextIndexVariant::original:
      return "original";
    case osmscout::ImportParameter::TextIndexVariant::transliterate:
      return "transliterate";
    case osmscout::ImportParameter::TextIndexVariant::both:
      return "both";
  }
  assert(false);
  return "unknown";
}

void DumpHelp(osmscout::ImportParameter& parameter)
{
  std::cout << "Import -h -d -s <start step> -e <end step> [*.osm|*.pbf]..." << std::endl;
  std::cout << " -h|--help                            show this help and exit" << std::endl;
  std::cout << " --data-version                       print output data version and exit" << std::endl;
  std::cout << " -d                                   show debug output during import" << std::endl;
  std::cout << " -s <number>                          set starting processing step" << std::endl;
  std::cout << " -e <number>                          set final processing step" << std::endl;
  std::cout << " --typefile <*.ost>                   path and name of the map.ost file (default: " << parameter.GetTypefile() << ")" << std::endl;
  std::cout << " --destinationDirectory <path>        destination for generated map files (default: " << parameter.GetDestinationDirectory() << ")" << std::endl;
  std::cout << std::endl;
  std::cout << " --bounding-polygon <*.poly>          optional polygon file containing the bounding polygon of the import area" << std::endl;
  std::cout << std::endl;

  std::cout << " --router <router description>        definition of a router (default: car,bicycle,foot:router)" << std::endl;
  std::cout << std::endl;

  std::cout << " --strictAreas true|false             assure that areas are simple (default: " << osmscout::BoolToString(parameter.GetStrictAreas()) << ")" << std::endl;

  std::cout << " --processingQueueSize <number>       size of of the processing worker queues (default: " << parameter.GetProcessingQueueSize() << ")" << std::endl;
  std::cout << std::endl;

  std::cout << " --numericIndexPageSize <number>      size of an numeric index page in bytes (default: " << parameter.GetNumericIndexPageSize() << ")" << std::endl;

  std::cout << " --rawCoordBlockSize <number>         number of raw coords resolved in block (default: " << parameter.GetRawCoordBlockSize() << ")" << std::endl;

  std::cout << " --rawNodeDataMemoryMaped true|false  memory maped raw node data file access (default: " << osmscout::BoolToString(parameter.GetRawNodeDataMemoryMaped()) << ")" << std::endl;

  std::cout << " --rawWayIndexMemoryMaped true|false  memory maped raw way index file access (default: " << osmscout::BoolToString(parameter.GetRawWayIndexMemoryMaped()) << ")" << std::endl;
  std::cout << " --rawWayDataMemoryMaped true|false   memory maped raw way data file access (default: " << osmscout::BoolToString(parameter.GetRawWayDataMemoryMaped()) << ")" << std::endl;
  std::cout << " --rawWayIndexCacheSize <number>      raw way index cache size (default: " << parameter.GetRawWayIndexCacheSize() << ")" << std::endl;
  std::cout << " --rawWayBlockSize <number>           number of raw ways resolved in block (default: " << parameter.GetRawWayBlockSize() << ")" << std::endl;

  std::cout << " --noSort                             do not sort objects" << std::endl;
  std::cout << " --sortBlockSize <number>             size of one data block during sorting (default: " << parameter.GetSortBlockSize() << ")" << std::endl;

  std::cout << " --coordDataMemoryMaped true|false    memory mapped coord data file access (default: " << osmscout::BoolToString(parameter.GetCoordDataMemoryMaped()) << ")" << std::endl;
  std::cout << " --coordIndexCacheSize <number>       coord index cache size (default: " << parameter.GetCoordIndexCacheSize() << ")" << std::endl;
  std::cout << " --coordBlockSize <number>            number of coords resolved in block (default: " << parameter.GetCoordBlockSize() << ")" << std::endl;

  std::cout << " --relMaxWays <number>                maximum of ways for a relation to get resolved (default: " << parameter.GetRelMaxWays() << ")" << std::endl;
  std::cout << " --relMaxCoords <number>              maximum of coords for a relation to get resolved (default: " << parameter.GetRelMaxCoords() << ")" << std::endl;

  std::cout << " --areaDataMemoryMaped true|false     memory maped area data file access (default: " << osmscout::BoolToString(parameter.GetAreaDataMemoryMaped()) << ")" << std::endl;
  std::cout << " --areaDataCacheSize <number>         area data cache size (default: " << parameter.GetAreaDataCacheSize() << ")" << std::endl;

  std::cout << " --wayDataMemoryMaped true|false      memory maped way data file access (default: " << osmscout::BoolToString(parameter.GetWayDataMemoryMaped()) << ")" << std::endl;
  std::cout << " --wayDataCacheSize <number>          way data cache size (default: " << parameter.GetWayDataCacheSize() << ")" << std::endl;

  std::cout << " --areaWayIndexMinMag <number>        minimum index level for area way index analysis (default: " << parameter.GetAreaWayIndexMinMag() << ")" << std::endl;
  std::cout << " --areaWayIndexMaxMag <number>        maximum index level for area way index analysis (default: " << parameter.GetAreaWayIndexMaxMag() << ")" << std::endl;

  std::cout << " --routeNodeBlockSize <number>        number of route nodes resolved in block (default: " << parameter.GetRouteNodeBlockSize() << ")" << std::endl;
  std::cout << std::endl;
  std::cout << " --langOrder <#|lang1[,#|lang2]..>    language order when parsing lang[:language] and place_name[:language] tags" << std::endl
            << "                                      efault language (no :language) (default: #)" << std::endl;
  std::cout << " --altLangOrder <#|lang1[,#|lang2]..> same as --langOrder for a second alternate language (default: none)" << std::endl;
  std::cout << std::endl;
  std::cout << " --maxAdminLevel <number>             maximum admin level evaluated (default: " << parameter.GetMaxAdminLevel() << ")" << std::endl;
  std::cout << std::endl;
  std::cout << " --eco true|false                     do delete temporary fiels ASAP" << std::endl;
  std::cout << " --delete-temporary-files true|false  deletes all temporary files after execution of the importer" << std::endl;
  std::cout << " --delete-debugging-files true|false  deletes all debugging files after execution of the importer" << std::endl;
  std::cout << " --delete-analysis-files true|false   deletes all analysis files after execution of the importer" << std::endl;
  std::cout << " --delete-report-files true|false     deletes all report files after execution of the importer" << std::endl;
  std::cout << " --textIndexVariant transliterate|original|both" << std::endl;
  std::cout << "                                      store transliterated, original or both strings to string index (default: " + TextIndexVariantStr(parameter.GetTextIndexVariant()) + ")" << std::endl;
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
    return nullptr;
  }

  std::string argument=argv[argumentIndex];
  size_t      pos=argument.rfind(':');

  if (pos==std::string::npos) {
    std::cerr << "Cannot separate vehicles from filename base in router definition '" << argument << "'" << std::endl;
    return nullptr;
  }

  filenamebase=argument.substr(pos+1);

  if (filenamebase.empty()) {
    std::cerr << "Empty filename base in router definition '" << argument << "'" << std::endl;
    return nullptr;
  }

  std::string vehicles=argument.substr(0,pos);

  if (vehicles.empty()) {
    std::cerr << "Empty vehicle list in router definition '" << argument << "'" << std::endl;
    return nullptr;
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
      return nullptr;
    }

    start=devider+1;
    devider=vehicles.find(',',start);
  }

  return std::make_shared<osmscout::ImportParameter::Router>(vehicleMask,filenamebase);
}

std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, elems);
    return elems;
}

std::vector<std::string> ParseLangOrderArgument(int argc,
                                                char* argv[],
                                                int& currentIndex)
{
    std::vector<std::string> langVec;
    int                      parameterIndex=currentIndex;
    int                      argumentIndex=currentIndex+1;

    currentIndex+=2;

    if (argumentIndex>=argc) {
        std::cerr << "Missing parameter after option '" << argv[parameterIndex] << "'" << std::endl;
        return langVec;
    }

    std::string argument=argv[argumentIndex];
    langVec = split(argument, ',');

    return langVec;
}

std::optional<osmscout::ImportParameter::TextIndexVariant> ParseTextIndexVariant(int argc,
                                                                                 char* argv[],
                                                                                 int& currentIndex)
{
  int parameterIndex=currentIndex;
  int argumentIndex=currentIndex+1;

  currentIndex+=2;

  if (argumentIndex>=argc) {
    std::cerr << "Missing parameter after option '" << argv[parameterIndex] << "'" << std::endl;
    return std::nullopt;
  }

  std::string argument(argv[argumentIndex]);
  if (argument == "transliterate") {
    return std::make_optional(osmscout::ImportParameter::TextIndexVariant::transliterate);
  }
  if (argument == "original") {
    return std::make_optional(osmscout::ImportParameter::TextIndexVariant::original);
  }
  if (argument == "both") {
    return std::make_optional(osmscout::ImportParameter::TextIndexVariant::both);
  }

  std::cerr << "Uknown '" << argv[parameterIndex] << "' parameter '" << argument << "'" << std::endl;
  return std::nullopt;
}

static void InitializeLocale(osmscout::Progress& progress)
{
  try {
    std::locale::global(std::locale(""));
  }
  catch (const std::runtime_error& e) {
    progress.Error("Cannot set locale: \""+std::string(e.what())+"\"");
    progress.Error("Note that (near) future version of the Importer require a working locale environment!");
  }
}

static void DumpParameter(const osmscout::ImportParameter& parameter,
                          osmscout::Progress& progress)
{
  progress.SetStep("Dump parameter");
  for (const auto& filename : parameter.GetMapfiles()) {
    progress.Info("Mapfile: {}",filename);
  }

  progress.Info("Typefile: {}",parameter.GetTypefile());
  progress.Info("Destination directory: {}",parameter.GetDestinationDirectory());
  progress.Info("Steps: {} - {}",
                parameter.GetStartStep(),
                parameter.GetEndStep());



  for (const auto& router : parameter.GetRouter()) {
    progress.Info("Router: {} - '{}'",VehicleMaskToString(router.GetVehicleMask()),router.GetFilenamebase());
  }

  progress.Info("StrictAreas: {}",parameter.GetStrictAreas());
  progress.Info("ProcessingQueueSize: {}",parameter.GetProcessingQueueSize());
  progress.Info("NumericIndexPageSize: {}",parameter.GetNumericIndexPageSize());
  progress.Info("RawCoordBlockSize: {}",parameter.GetRawCoordBlockSize());

  progress.Info("RawNodeDataMemoryMaped: {}",parameter.GetRawNodeDataMemoryMaped());
  progress.Info("RawWayIndexMemoryMaped: {}",parameter.GetRawWayIndexMemoryMaped());
  progress.Info("RawWayDataMemoryMaped: {}",parameter.GetRawWayDataMemoryMaped());
  progress.Info("RawWayIndexCacheSize: {}",parameter.GetRawWayIndexCacheSize());
  progress.Info("RawWayBlockSize: {}",parameter.GetRawWayBlockSize());

  progress.Info("SortObjects: {}",parameter.GetSortObjects());
  progress.Info("SortBlockSize: {}",parameter.GetSortBlockSize());

  progress.Info("CoordDataMemoryMaped: {}",parameter.GetCoordDataMemoryMaped());
  progress.Info("CoordIndexCacheSize: {}",parameter.GetCoordIndexCacheSize());
  progress.Info("CoordBlockSize: {}",parameter.GetCoordBlockSize());

  progress.Info("AreaDataMemoryMaped: {}",parameter.GetAreaDataMemoryMaped());
  progress.Info("AreaDataCacheSize: {}",parameter.GetAreaDataCacheSize());
  progress.Info("AreaWayIndexMinMag: {}",parameter.GetAreaWayIndexMinMag().Get());
  progress.Info("AreaWayIndexMaxMag: {}",parameter.GetAreaWayIndexMaxMag().Get());

  progress.Info("WayDataMemoryMaped: {}",parameter.GetWayDataMemoryMaped());
  progress.Info("WayDataCacheSize: {}",parameter.GetWayDataCacheSize());

  progress.Info("AreaNodeGridMag: {}",parameter.GetAreaNodeGridMag().Get());
  progress.Info("AreaNodeSimpleListLimit: {}",parameter.GetAreaNodeSimpleListLimit());
  progress.Info("AreaNodeTileListLimit: {}",parameter.GetAreaNodeTileListLimit());
  progress.Info("AreaNodeTileListCoordLimit: {}",parameter.GetAreaNodeTileListCoordLimit());
  progress.Info("AreaNodeBitmapMaxMag: {}",parameter.GetAreaNodeBitmapMaxMag().Get());
  progress.Info("AreaNodeBitmapLimit: {}",parameter.GetAreaNodeBitmapLimit());

  progress.Info("RouteNodeBlockSize: {}",parameter.GetRouteNodeBlockSize());


  progress.Info("MaxAdminLevel: {}",parameter.GetMaxAdminLevel());

  progress.Info("Eco: {}",parameter.IsEco());

  progress.Info("TextIndexVariant: {}",TextIndexVariantStr(parameter.GetTextIndexVariant()));
}

bool DumpDataSize(const osmscout::ImportParameter& parameter,
                  const osmscout::Importer& importer,
                  osmscout::Progress& progress)
{
  osmscout::FileOffset dataSize=0;

  progress.Info("Mandatory files:");

  try {
    for (const auto& filename : importer.GetProvidedFiles()) {
      osmscout::FileOffset fileSize=0;
      std::string          filePath=osmscout::AppendFileToDir(parameter.GetDestinationDirectory(),
                                                              filename);

      fileSize=osmscout::GetFileSize(filePath);

      progress.Info("File '{}': {}",filename,osmscout::ByteSizeToString(fileSize));

      dataSize+=fileSize;
    }

    progress.Info("=> {}",osmscout::ByteSizeToString(dataSize));

    progress.Info("Optional files:");

    dataSize=0;
    for (const auto& filename : importer.GetProvidedOptionalFiles()) {
      osmscout::FileOffset fileSize=0;
      std::string          filePath=osmscout::AppendFileToDir(parameter.GetDestinationDirectory(),
                                                              filename);

      fileSize=osmscout::GetFileSize(filePath);

      progress.Info("File '{}': {}",filename,osmscout::ByteSizeToString(fileSize));

      dataSize+=fileSize;
    }

    progress.Info("=> {}",osmscout::ByteSizeToString(dataSize));
  }
  catch (osmscout::IOException& e) {
    progress.Error(e.GetDescription());
    return false;
  }

  return true;
}

static void DeleteFilesIgnoreError(const osmscout::ImportParameter& parameter,
                                   const std::list<std::string>& filenames,
                                   osmscout::Progress& progress)
{
  for (const auto& relativeFilename : filenames)
  {
    std::string absoluteFilename=osmscout::AppendFileToDir(parameter.GetDestinationDirectory(),relativeFilename);

    if (osmscout::ExistsInFilesystem(absoluteFilename))
    {
      progress.Info("Deleting '" + absoluteFilename +"'");

      osmscout::RemoveFile(absoluteFilename);
    }
  }
}

int main(int argc, char* argv[])
{
  osmscout::ImportParameter    parameter;
  osmscout::StatImportProgress progress;
  bool                         parameterError=false;
  bool                         firstRouterOption=true;

  std::list<std::string>       mapfiles;

  osmscout::VehicleMask        defaultVehicleMask=osmscout::vehicleBicycle|osmscout::vehicleFoot|osmscout::vehicleCar;
  bool                         deleteTemporaries=false;
  bool                         deleteDebugging=false;
  bool                         deleteAnalysis=false;
  bool                         deleteReport=false;

  InitializeLocale(progress);

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
    else if (strcmp(argv[i],"--data-version")==0) {
      std::cout << std::to_string(osmscout::FILE_FORMAT_VERSION) << std::endl;
      return 0;
    }
    else if (strcmp(argv[i],"-s")==0) {
      size_t startStep;

      if (osmscout::ParseSizeTArgument(argc,
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

      if (osmscout::ParseSizeTArgument(argc,
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

      if (osmscout::ParseBoolArgument(argc,
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

      if (osmscout::ParseStringArgument(argc,
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

      if (osmscout::ParseStringArgument(argc,
                                        argv,
                                        i,
                                        destinationDirectory)) {
        parameter.SetDestinationDirectory(destinationDirectory);
      }
      else {
        parameterError=true;
      }
    }
    else if (strcmp(argv[i],"--bounding-polygon")==0) {
      std::string boundingPolygonFile;

      if (osmscout::ParseStringArgument(argc,
                                        argv,
                                        i,
                                        boundingPolygonFile)) {
        parameter.SetBoundingPolygonFile(boundingPolygonFile);
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

      if (osmscout::ParseBoolArgument(argc,
                                      argv,
                                      i,
                                      strictAreas)) {
        parameter.SetStrictAreas(strictAreas);
      }
      else {
        parameterError=true;
      }
    }
    else if (strcmp(argv[i],"--processingQueueSize")==0) {
      size_t processingQueueSize;

      if (osmscout::ParseSizeTArgument(argc,
                                       argv,
                                       i,
                                       processingQueueSize)) {
        parameter.SetProcessingQueueSize(processingQueueSize);
      }
      else {
        parameterError=true;
      }
    }
    else if (strcmp(argv[i],"--numericIndexPageSize")==0) {
      size_t numericIndexPageSize;

      if (osmscout::ParseSizeTArgument(argc,
                                       argv,
                                       i,
                                       numericIndexPageSize)) {
        parameter.SetNumericIndexPageSize(numericIndexPageSize);
      }
      else {
        parameterError=true;
      }
    }
    else if (strcmp(argv[i],"--rawCoordBlockSize")==0) {
      size_t rawCoordBlockSize;

      if (osmscout::ParseSizeTArgument(argc,
                                       argv,
                                       i,
                                       rawCoordBlockSize)) {
        parameter.SetRawCoordBlockSize(rawCoordBlockSize);
      }
      else {
        parameterError=true;
      }
    }
    else if (strcmp(argv[i],"--rawNodeDataMemoryMaped")==0) {
      bool rawNodeDataMemoryMaped;

      if (osmscout::ParseBoolArgument(argc,
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

      if (osmscout::ParseBoolArgument(argc,
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

      if (osmscout::ParseBoolArgument(argc,
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

      if (osmscout::ParseSizeTArgument(argc,
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

      if (osmscout::ParseSizeTArgument(argc,
                                       argv,
                                       i,
                                       rawWayBlockSize)) {
        parameter.SetRawWayBlockSize(rawWayBlockSize);
      }
      else {
        parameterError=true;
      }
    }
    else if (strcmp(argv[i],"--noSort")==0) {
      parameter.SetSortObjects(false);

      i++;
    }
    else if (strcmp(argv[i],"--sortBlockSize")==0) {
      size_t sortBlockSize;

      if (osmscout::ParseSizeTArgument(argc,
                                       argv,
                                       i,
                                       sortBlockSize)) {
        parameter.SetSortBlockSize(sortBlockSize);
      }
      else {
        parameterError=true;
      }
    }
    else if (strcmp(argv[i],"--coordDataMemoryMaped")==0) {
      bool coordDataMemoryMaped;

      if (osmscout::ParseBoolArgument(argc,
                                      argv,
                                      i,
                                      coordDataMemoryMaped)) {
        parameter.SetCoordDataMemoryMaped(coordDataMemoryMaped);
      }
      else {
        parameterError=true;
      }
    }
    else if (strcmp(argv[i],"--coordIndexCacheSize")==0) {
      size_t coordIndexCacheSize;

      if (osmscout::ParseSizeTArgument(argc,
                                       argv,
                                       i,
                                       coordIndexCacheSize)) {
        parameter.SetCoordIndexCacheSize(coordIndexCacheSize);
      }
      else {
        parameterError=true;
      }
    }
    else if (strcmp(argv[i],"--coordBlockSize")==0) {
      size_t coordBlockSize;

      if (osmscout::ParseSizeTArgument(argc,
                                       argv,
                                       i,
                                       coordBlockSize)) {
        parameter.SetCoordBlockSize(coordBlockSize);
      }
      else {
        parameterError=true;
      }
    }
    else if (strcmp(argv[i],"--relMaxWays")==0) {
      size_t relMaxWays;

      if (osmscout::ParseSizeTArgument(argc,
                                       argv,
                                       i,
                                       relMaxWays)) {
        parameter.SetRelMaxWays(relMaxWays);
      }
      else {
        parameterError=true;
      }
    }
    else if (strcmp(argv[i],"--relMaxCoords")==0) {
      size_t relMaxCoords;

      if (osmscout::ParseSizeTArgument(argc,
                                       argv,
                                       i,
                                       relMaxCoords)) {
        parameter.SetRelMaxCoords(relMaxCoords);
      }
      else {
        parameterError=true;
      }
    }
    else if (strcmp(argv[i],"--areaDataMemoryMaped")==0) {
      bool areaDataMemoryMaped;

      if (osmscout::ParseBoolArgument(argc,
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

      if (osmscout::ParseSizeTArgument(argc,
                                       argv,
                                       i,
                                       areaDataCacheSize)) {
        parameter.SetAreaDataCacheSize(areaDataCacheSize);
      }
      else {
        parameterError=true;
      }
    }
    else if (strcmp(argv[i],"--areaWayIndexMinMag")==0) {
      size_t areaWayIndexMinMag;

      if (osmscout::ParseSizeTArgument(argc,
                                       argv,
                                       i,
                                       areaWayIndexMinMag)) {
        parameter.SetAreaWayIndexMinMag(osmscout::MagnificationLevel(areaWayIndexMinMag));
      }
      else {
        parameterError=true;
      }
    }
    else if (strcmp(argv[i],"--areaWayIndexMaxMag")==0) {
      size_t areaWayIndexMaxMag;

      if (osmscout::ParseSizeTArgument(argc,
                                       argv,
                                       i,
                                       areaWayIndexMaxMag)) {
        parameter.SetAreaWayIndexMaxMag(osmscout::MagnificationLevel(areaWayIndexMaxMag));
      }
      else {
        parameterError=true;
      }
    }
    else if (strcmp(argv[i],"--wayDataMemoryMaped")==0) {
      bool wayDataMemoryMaped;

      if (osmscout::ParseBoolArgument(argc,
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

      if (osmscout::ParseSizeTArgument(argc,
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

      if (osmscout::ParseSizeTArgument(argc,
                                       argv,
                                       i,
                                       routeNodeBlockSize)) {
        parameter.SetRouteNodeBlockSize(routeNodeBlockSize);
      }
      else {
        parameterError=true;
      }
    }
    else if (strcmp(argv[i],"--langOrder")==0) {
        std::vector<std::string> langOrder;

        langOrder = ParseLangOrderArgument(argc,
                                           argv,
                                           i);
        if (!langOrder.empty()) {
            parameter.SetLangOrder(langOrder);
        }
        else {
            parameterError=true;
        }
    }
    else if (strcmp(argv[i],"--altLangOrder")==0) {
        std::vector<std::string> langOrder;

        langOrder = ParseLangOrderArgument(argc,
                                           argv,
                                           i);
        if (!langOrder.empty()) {
            parameter.SetAltLangOrder(langOrder);
        }
        else {
            parameterError=true;
        }
    }
    else if (strcmp(argv[i],"--maxAdminLevel")==0) {
      size_t maxAdminLevel;

      if (osmscout::ParseSizeTArgument(argc,
                                       argv,
                                       i,
                                       maxAdminLevel)) {
        parameter.SetMaxAdminLevel(maxAdminLevel);
      }
      else {
        parameterError=true;
      }
    }
    else if (strcmp(argv[i],"--delete-temporary-files")==0) {
      if (!osmscout::ParseBoolArgument(argc,
                                       argv,
                                       i,
                                       deleteTemporaries)) {
        parameterError=true;
      }
    }
    else if (strcmp(argv[i],"--delete-debugging-files")==0) {
      if (!osmscout::ParseBoolArgument(argc,
                                       argv,
                                       i,
                                       deleteDebugging)) {
        parameterError=true;
      }
    }
    else if (strcmp(argv[i],"--delete-analysis-files")==0) {
      if (!osmscout::ParseBoolArgument(argc,
                                       argv,
                                       i,
                                       deleteAnalysis)) {
        parameterError=true;
      }
    }
    else if (strcmp(argv[i],"--delete-report-files")==0) {
      if (!osmscout::ParseBoolArgument(argc,
                                       argv,
                                       i,
                                       deleteReport)) {
        parameterError=true;
      }
    }
    else if (strcmp(argv[i],"--textIndexVariant")==0) {
      std::optional<osmscout::ImportParameter::TextIndexVariant> textIndexVariant;

      textIndexVariant = ParseTextIndexVariant(argc,
                                               argv,
                                               i);
      if (textIndexVariant) {
        parameter.SetTextIndexVariant(*textIndexVariant);
      }
      else {
        parameterError=true;
      }
    }
    else if (strncmp(argv[i],"--",2)==0) {
      progress.Error("Unknown option: "+std::string(argv[i]));

      parameterError=true;
      i++;
    }
    else {
      mapfiles.emplace_back(argv[i]);

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

  try {
    if (!osmscout::ExistsInFilesystem(parameter.GetDestinationDirectory())) {
      progress.Error("Destination directory does not exist!");
      return 1;
    }

    if (!osmscout::IsDirectory(parameter.GetDestinationDirectory())) {
      progress.Error("Destination ist not a directory!");
      return 1;
    }

    for (const auto& mapfile: mapfiles) {
      if (!osmscout::ExistsInFilesystem(mapfile)) {
        progress.Error("Input '"+mapfile+"' does not exist!");
        return 1;
      }

      if (osmscout::IsDirectory(mapfile)) {
        progress.Error("Input '"+mapfile+"' is a directory!");
        return 1;
      }
    }

    if (!parameter.GetBoundingPolygonFile().empty()) {
      std::string boundingPolygonFile=parameter.GetBoundingPolygonFile();

      if (!osmscout::ExistsInFilesystem(boundingPolygonFile)) {
        progress.Error("Bounding polygon file '"+boundingPolygonFile+"' does not exist!");
      }

      if (osmscout::IsDirectory(boundingPolygonFile)) {
        progress.Error("Bounding polygon file '"+boundingPolygonFile+"' is a directory!");
        return 1;
      }
    }
  }
  catch (osmscout::IOException& /*e*/) {
    // we ignore this exception, since it is likely a "not implemented" exception
  }

  parameter.SetMapfiles(mapfiles);
  parameter.SetOptimizationWayMethod(osmscout::TransPolygon::quality);

  DumpParameter(parameter,
                progress);

  int exitCode=0;
  try {
    osmscout::Importer importer(parameter);

    bool result=importer.Import(progress);

    progress.SetStep("Summary");

    if (result) {

      if (!progress.DumpDotStats(osmscout::AppendFileToDir(
          parameter.GetDestinationDirectory(), "stats.dot"))){
        progress.Error("Error while writing stats");
      }

      if (!DumpDataSize(parameter,
                        importer,
                        progress)) {
        progress.Error("Error while retrieving data size");
      }
      progress.Info("Import OK!");
    }
    else {
      progress.Error("Import failed!");
      exitCode=1;
    }

    if (deleteTemporaries) {
      progress.SetAction(("Deleting temporary files"));

      std::list<std::string> temporaries=importer.GetProvidedTemporaryFiles();

      DeleteFilesIgnoreError(parameter,
                             temporaries,
                             progress);
    }

    if (deleteDebugging) {
      progress.SetAction(("Deleting debugging files"));

      std::list<std::string> temporaries=importer.GetProvidedDebuggingFiles();

      DeleteFilesIgnoreError(parameter,
                             temporaries,
                             progress);
    }

    if (deleteAnalysis) {
      progress.SetAction(("Deleting analysis files"));

      std::list<std::string> temporaries=importer.GetProvidedAnalysisFiles();

      DeleteFilesIgnoreError(parameter,
                             temporaries,
                             progress);
    }

    if (deleteReport) {
      progress.SetAction(("Deleting report files"));

      std::list<std::string> temporaries=importer.GetProvidedReportFiles();

      DeleteFilesIgnoreError(parameter,
                             temporaries,
                             progress);
    }
  }
  catch (osmscout::IOException& e) {
    progress.Error("Import failed: "+e.GetDescription());
    exitCode=1;
  }


  return exitCode;
}
