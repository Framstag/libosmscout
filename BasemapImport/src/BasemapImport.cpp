/*
  TravelJinni - Openstreetmap offline viewer
  Copyright (C) 2017  Tim Teulings

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
#include <sstream>
#include <string>
#include <vector>

#include <osmscout/util/File.h>
#include <osmscout/util/String.h>

#include <osmscout/import/Import.h>
#include <osmscout/import/ShapeFileScanner.h>

#include <osmscout/import/GenWaterIndex.h>

class CoastlineShapeFileVisitor : public osmscout::ShapeFileVisitor
{
private:
  osmscout::Progress                                   &progress;
  uint32_t                                             coastlineCount;
  bool                                                 continuation;
  std::vector<osmscout::GeoCoord>                      coordBuffer;
  std::vector<osmscout::WaterIndexGenerator::CoastRef> coasts;
  osmscout::Id                                         currentId;

public:
  CoastlineShapeFileVisitor(const std::string& destinationDirectory,
                            const std::string& coastlineShapeFile,
                            osmscout::Progress& progress)
    : progress(progress)
  {
    progress.SetAction("Scanning world coastline file '"+coastlineShapeFile+"'");

    coastlineCount=0;
    continuation=false;
    currentId=std::numeric_limits<osmscout::Id>::max();
  }

  ~CoastlineShapeFileVisitor()
  {
    if (continuation) {
      progress.Error("Last element is not properly closed");
    }

    progress.Info("Found "+osmscout::NumberToString(coastlineCount)+ " coastline(s)");
  }

  void AddCoast(const std::vector<osmscout::GeoCoord>& coords)
  {
    osmscout::WaterIndexGenerator::CoastRef coastline=std::make_shared<osmscout::WaterIndexGenerator::Coast>();

    coastline->id=currentId;
    coastline->isArea=true;
    coastline->left=osmscout::WaterIndexGenerator::CoastState::water;
    coastline->right=osmscout::WaterIndexGenerator::CoastState::land;

    coastline->coast.reserve(coords.size());

    for (auto& coord : coords) {
      coastline->coast.push_back(osmscout::Point(0,coord));
    }

    currentId--;
  }

  void OnProgress(double current,
                  double total)
  {
    progress.SetProgress(current,total);
  }

  void OnPolyline(int32_t /*recordNumber*/,
                  const osmscout::GeoBox& /*boundingBox*/,
                  const std::vector<osmscout::GeoCoord>& coords)
  {
    if (continuation) {
      if (coords.size()<1000 || coordBuffer.front()==coords.back()) {
        coordBuffer.insert(coordBuffer.end(),coords.begin(),coords.end());
        AddCoast(coordBuffer);
        coastlineCount++;
        continuation=false;
      }
      else {
        coordBuffer.insert(coordBuffer.end(),coords.begin(),coords.end());
      }
    }
    else if (coords.size()<1000 || coords.front()==coords.back()) {
      AddCoast(coords);
      coastlineCount++;
    }
    else {
      coordBuffer=coords;
      continuation=true;
    }
  }
};

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

void DumpHelp()
{
  std::cout << "BasemapImport " << std::endl;
  std::cout << " -h|--help                     show this help" << std::endl;
  std::cout << " -d                            show debug output" << std::endl;
  std::cout << " --destinationDirectory <path> destination for generated map files" << std::endl;
  std::cout << std::endl;
  std::cout << " --coastlines <*.shape>        optional shape file containing world-wide coastlines" << std::endl;
  std::cout << std::endl;
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

static bool ImportCoastlines(const std::string& destinationDirectory,
                             const std::string& coastlineShapeFile,
                             osmscout::Progress& progress)
{
  progress.SetAction("Reading coastline shape file");

  try {
    osmscout::ShapeFileScanner shapefileScanner(coastlineShapeFile);
    CoastlineShapeFileVisitor  visitor(destinationDirectory,
                                       coastlineShapeFile,
                                       progress);

    shapefileScanner.Open();
    shapefileScanner.Visit(visitor);
    shapefileScanner.Close();
  }
  catch (osmscout::IOException& e) {
    progress.Error(e.GetDescription());

    return false;
  }

  return true;
}

int main(int argc, char* argv[])
{
  std::string               destinationDirectory;
  std::string               coastlineShapeFile;
  osmscout::ConsoleProgress progress;
  bool                      parameterError=false;

  InitializeLocale(progress);

  // Simple way to analyze command line parameters, but enough for now...
  int i=1;
  while (i<argc) {
    if (strcmp(argv[i],"-h")==0 ||
        strcmp(argv[i],"-?")==0 ||
        strcmp(argv[i],"--help")==0) {
      DumpHelp();

      return 0;
    }
    else if (strcmp(argv[i],"-d")==0) {
      progress.SetOutputDebug(true);

      i++;
    }
    else if (strcmp(argv[i],"--destinationDirectory")==0) {
      if (ParseStringArgument(argc,
                              argv,
                              i,
                              destinationDirectory)) {
      }
      else {
        parameterError=true;
      }
    }
    else if (strcmp(argv[i],"--coastlines")==0) {
      if (ParseStringArgument(argc,
                              argv,
                              i,
                              coastlineShapeFile)) {
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
      progress.Error("Unknown parameter/option: "+std::string(argv[i]));

      parameterError=true;
      i++;
    }
  }

  if (destinationDirectory.empty()) {
    progress.Error("Mandatory destination directory not set");
    parameterError=true;
  }

  if (parameterError) {
    DumpHelp();
    return 1;
  }

  try {
    if (!osmscout::ExistsInFilesystem(destinationDirectory)) {
      progress.Error("Destination directory does not exist!");
      return 1;
    }

    if (!osmscout::IsDirectory(destinationDirectory)) {
      progress.Error("Destination ist not a directory!");
      return 1;
    }

    if (!coastlineShapeFile.empty()) {
      if (!osmscout::ExistsInFilesystem(coastlineShapeFile)) {
        progress.Error("Coastline shapefile does not exist!");
        return 1;
      }
    }
  }
  catch (osmscout::IOException& e) {
    // we ignore this exception, since it is likely a "not implemented" exception
  }

  int exitCode=0;
  try {
    if (!coastlineShapeFile.empty()) {
      ImportCoastlines(destinationDirectory,
                       coastlineShapeFile,
                       progress);
    }
  }
  catch (osmscout::IOException& e) {
    progress.Error("Import failed: "+e.GetDescription());
    exitCode=1;
  }


  return exitCode;
}
