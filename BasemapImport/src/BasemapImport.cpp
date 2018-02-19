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

#include <cstring>
#include <cstdio>

#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include <osmscout/util/CmdLineParsing.h>
#include <osmscout/util/File.h>

#include <osmscout/import/Import.h>
#include <osmscout/import/ShapeFileScanner.h>

#include <osmscout/import/WaterIndexProcessor.h>

class CoastlineShapeFileVisitor : public osmscout::ShapeFileVisitor
{
private:
  osmscout::Progress                                 &progress;
  uint32_t                                           coastlineCount;
  bool                                               continuation;
  std::vector<osmscout::GeoCoord>                    coordBuffer;
  osmscout::Id                                       currentId;
public:
  std::list<osmscout::WaterIndexProcessor::CoastRef> coasts;

public:
  CoastlineShapeFileVisitor(const std::string& coastlineShapeFile,
                            osmscout::Progress& progress)
    : progress(progress)
  {
    progress.SetAction("Scanning world coastline file '"+coastlineShapeFile+"'");

    coastlineCount=0;
    continuation=false;
    currentId=std::numeric_limits<osmscout::Id>::max();
  }

  ~CoastlineShapeFileVisitor() override
  {
    if (continuation) {
      progress.Error("Last element is not properly closed");
    }

    progress.Info("Found "+std::to_string(coastlineCount)+ " coastline(s)");
  }

  void AddCoast(const std::vector<osmscout::GeoCoord>& coords)
  {
    if (coords.size()<2){
      return;
    }
    osmscout::WaterIndexProcessor::CoastRef coastline=std::make_shared<osmscout::WaterIndexProcessor::Coast>();

    coastline->id=currentId;
    coastline->isArea=coords.front()==coords.back();
    // note that shapefile coastlines has reverse direction than OSM!
    coastline->left=osmscout::WaterIndexProcessor::CoastState::water;
    coastline->right=osmscout::WaterIndexProcessor::CoastState::land;
    coastline->frontNodeId=coords.front().GetHash();
    coastline->backNodeId=coords.back().GetHash();

    coastline->coast.reserve(coords.size());

    for (auto& coord : coords) {
      coastline->coast.emplace_back(0,coord);
    }

    coasts.push_back(coastline);

    currentId--;
  }

  void OnProgress(double current,
                  double total) override
  {
    progress.SetProgress(current,total);
  }

  void OnPolyline(int32_t /*recordNumber*/,
                  const osmscout::GeoBox& /*boundingBox*/,
                  const std::vector<osmscout::GeoCoord>& coords) override
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

/*
static const char* BoolToString(bool value)
{
  if (value) {
    return "true";
  }
  else {
    return "false";
  }
}*/

void DumpHelp()
{
  std::cout << "BasemapImport " << std::endl;
  std::cout << " -h|--help                     show this help" << std::endl;
  std::cout << " -d                            show debug output" << std::endl;
  std::cout << " --destinationDirectory <path> destination for generated map files" << std::endl;
  std::cout << " --minIndexLevel <number>      minimum water index zoom level (default 4)" << std::endl;
  std::cout << " --maxIndexLevel <number>      maximum water index zoom level (default 10)" << std::endl;
  std::cout << std::endl;
  std::cout << " --coastlines <*.shape>        optional shape file containing world-wide coastlines" << std::endl;
  std::cout << std::endl;
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
                             osmscout::Progress& progress,
                             size_t indexMinMag,
                             size_t indexMaxMag)
{
  progress.SetAction("Reading coastline shape file");

  osmscout::FileWriter                              writer;
  osmscout::WaterIndexProcessor                     processor;
  std::vector<osmscout::WaterIndexProcessor::Level> levels;

  levels.reserve(indexMaxMag-indexMinMag+1);

  double cellWidth=360.0;
  double cellHeight=180.0;

  osmscout::GeoBox boundingBox(osmscout::GeoCoord(-90.0,-180.0),
                               osmscout::GeoCoord(90.0,180.0));

  for (uint32_t zoomLevel=0; zoomLevel<=indexMaxMag; zoomLevel++) {
    if (zoomLevel>=indexMinMag &&
        zoomLevel<=indexMaxMag) {
      osmscout::WaterIndexProcessor::Level level;

      level.level=zoomLevel;
      level.SetBox(boundingBox,
                   cellWidth,
                   cellHeight);

      levels.push_back(level);
    }

    cellWidth=cellWidth/2.0;
    cellHeight=cellHeight/2.0;
  }

  try {
    osmscout::ShapeFileScanner shapefileScanner(coastlineShapeFile);
    CoastlineShapeFileVisitor  visitor(coastlineShapeFile,
                                       progress);

    shapefileScanner.Open();
    shapefileScanner.Visit(visitor);
    shapefileScanner.Close();

    processor.MergeCoastlines(progress,visitor.coasts);

    // - close coastlines crossing antimeridian
    //  -- splitted to two ways in OSM data
    // - close Antarctica coastline
    // - remove rest of unclosed ways
    size_t removedCoastlines=0;
    auto it=visitor.coasts.begin();
    while (it!=visitor.coasts.end()){
      osmscout::WaterIndexProcessor::CoastRef coastline=*it;
      if (!coastline->isArea){
        // close ways touching antimeridian
        if (std::abs(coastline->coast.front().GetLon())>179.999 &&
            std::abs(coastline->coast.front().GetLon()-coastline->coast.back().GetLon())<0.001){
          coastline->isArea=true;
          coastline->coast.push_back(coastline->coast.front());
        }else if (coastline->coast.front().GetLon()<-179.999 && // reverse direction than OSM data!
                  coastline->coast.back().GetLon()>+179.999 &&
                  coastline->coast.front().GetLat()<-56 &&
                  coastline->coast.back().GetLat()<-56){
          // hack for Antarctica
          coastline->isArea=true;
          coastline->coast.emplace_back(0,osmscout::GeoCoord(-90,+180));
          coastline->coast.emplace_back(0,osmscout::GeoCoord(-90,-180));
        }else{
          it=visitor.coasts.erase(it);
          removedCoastlines++;
          continue;
        }
      }
      it++;
    }
    if (removedCoastlines>0){
      progress.Warning("Removed "+std::to_string(removedCoastlines)+" unclosed coastlines");
    }

    writer.Open(osmscout::AppendFileToDir(destinationDirectory,
                                          "water.idx"));

    processor.DumpIndexHeader(writer,
                              levels);
    progress.Info("Generating index for level "+std::to_string(indexMinMag)+" to "+std::to_string(indexMaxMag));

    for (auto& level : levels) {
      osmscout::Magnification                                    magnification;
      osmscout::MercatorProjection                               projection;
      std::list<osmscout::WaterIndexProcessor::CoastRef>         boundingPolygons;
      std::map<osmscout::Pixel,std::list<osmscout::GroundTile> > cellGroundTileMap;

      magnification.SetLevel(level.level);

      projection.Set(osmscout::GeoCoord(0.0,0.0),magnification,72,640,480);

      progress.SetAction("Building tiles for level "+std::to_string(level.level));

      if (!visitor.coasts.empty()) {
        osmscout::WaterIndexProcessor::Data data;

        // Collects, calculates and generates a number of data about a coastline
        processor.CalculateCoastlineData(progress,
                                         osmscout::TransPolygon::fast,
                                         /*tolerance*/ 10.0,
                                         /*minObjectDimension*/ 1.0,
                                         projection,
                                         level.stateMap,
                                         visitor.coasts,
                                         data);

        // Mark cells that intersect a coastline as coast
        processor.MarkCoastlineCells(progress,
                                     level.stateMap,
                                     data);

        // Fills coords information for cells that intersect a coastline
        processor.HandleCoastlinesPartiallyInACell(progress,
                                                   level.stateMap,
                                                   cellGroundTileMap,
                                                   data);

        // Fills coords information for cells that completely contain a coastline
        processor.HandleAreaCoastlinesCompletelyInACell(progress,
                                                        level.stateMap,
                                                        data,
                                                        cellGroundTileMap);
      }

      // Calculate the cell type for cells directly around coast cells
      processor.CalculateCoastEnvironment(progress,
                                          level.stateMap,
                                          cellGroundTileMap);

      if (!visitor.coasts.empty()) {
        // Marks all still 'unknown' cells neighbouring 'water' cells as 'water', too
        processor.FillWater(progress,
                            level,
                            20,
                            boundingPolygons);

        processor.FillWaterAroundIsland(progress,
                                        level.stateMap,
                                        cellGroundTileMap,
                                        boundingPolygons);
      }

      // Marks all still 'unknown' cells between 'coast' or 'land' and 'land' cells as 'land', too
      processor.FillLand(progress,
                         level.stateMap);

      processor.CalculateHasCellData(level,
                                     cellGroundTileMap);

      processor.WriteTiles(progress,
                           cellGroundTileMap,
                           level,
                           writer);
    }

    writer.Close();
  }
  catch (osmscout::IOException& e) {
    progress.Error(e.GetDescription());

    writer.CloseFailsafe();

    return false;
  }

  return true;
}

int main(int argc, char* argv[])
{
  std::string               destinationDirectory;
  std::string               coastlineShapeFile;
  size_t                    minIndexLevel=4;
  size_t                    maxIndexLevel=10;
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
      if (osmscout::ParseStringArgument(argc,
                                        argv,
                                        i,
                                        destinationDirectory)) {
      }
      else {
        parameterError=true;
      }
    }
    else if (strcmp(argv[i],"--coastlines")==0) {
      if (osmscout::ParseStringArgument(argc,
                                        argv,
                                        i,
                                        coastlineShapeFile)) {
      }
      else {
        parameterError=true;
      }
    }
    else if (strcmp(argv[i],"--minIndexLevel")==0) {
      if (osmscout::ParseSizeTArgument(argc,
                                       argv,
                                       i,
                                       minIndexLevel)) {
      }
      else {
        parameterError=true;
      }
    }
    else if (strcmp(argv[i],"--maxIndexLevel")==0) {
      if (osmscout::ParseSizeTArgument(argc,
                                       argv,
                                       i,
                                       maxIndexLevel)) {
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
  if (minIndexLevel>maxIndexLevel || maxIndexLevel>20) {
    progress.Error("Invalid min/max index level");
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
  catch (osmscout::IOException& /*e*/) {
    // we ignore this exception, since it is likely a "not implemented" exception
  }

  int exitCode=0;
  try {
    if (!coastlineShapeFile.empty()) {
      ImportCoastlines(destinationDirectory,
                       coastlineShapeFile,
                       progress,
                       minIndexLevel,
                       maxIndexLevel);
    }
  }
  catch (osmscout::IOException& e) {
    progress.Error("Import failed: "+e.GetDescription());
    exitCode=1;
  }


  return exitCode;
}
