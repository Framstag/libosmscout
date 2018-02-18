/*
  This source is part of the libosmscout library
  Copyright (C) 2010  Tim Teulings

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
*/

#include <osmscout/import/GenWaterIndex.h>

#include <osmscout/Way.h>

#include <osmscout/DataFile.h>
#include <osmscout/BoundingBoxDataFile.h>
#include <osmscout/CoordDataFile.h>
#include <osmscout/TypeFeatures.h>
#include <osmscout/WaterIndex.h>

#include <osmscout/system/Math.h>

#include <osmscout/util/File.h>
#include <osmscout/util/FileScanner.h>
#include <osmscout/util/Geometry.h>

#include <osmscout/import/Preprocess.h>
#include <osmscout/import/RawCoastline.h>
#include <osmscout/import/RawNode.h>
#include <osmscout/WayDataFile.h>

#if !defined(DEBUG_COASTLINE)
//#define DEBUG_COASTLINE
#endif

#if !defined(DEBUG_TILING)
//#define DEBUG_TILING
#endif

namespace osmscout {

  bool WaterIndexGenerator::LoadCoastlines(const ImportParameter& parameter,
                                           Progress& progress,
                                           std::list<WaterIndexProcessor::CoastRef>& coastlines)
  {
    progress.SetAction("Scanning for coastlines");
    return LoadRawBoundaries(parameter,
                             progress,
                             coastlines,
                             Preprocess::RAWCOASTLINE_DAT,
                             WaterIndexProcessor::CoastState::land,
                             WaterIndexProcessor::CoastState::water);
  }

  bool WaterIndexGenerator::LoadBoundingPolygons(const ImportParameter& parameter,
                                                 Progress& progress,
                                                 std::list<WaterIndexProcessor::CoastRef>& boundingPolygons)
  {
    progress.SetAction("Scanning data polygon");
    return LoadRawBoundaries(parameter,
                             progress,
                             boundingPolygons,
                             Preprocess::RAWDATAPOLYGON_DAT,
                             WaterIndexProcessor::CoastState::undefined,
                             WaterIndexProcessor::CoastState::unknown);
  }

  bool WaterIndexGenerator::LoadRawBoundaries(const ImportParameter& parameter,
                                              Progress& progress,
                                              std::list<WaterIndexProcessor::CoastRef>& coastlines,
                                              const char* rawFile,
                                              WaterIndexProcessor::CoastState leftState,
                                              WaterIndexProcessor::CoastState rightState)
  {
    // We must have coastline type defined
    FileScanner                scanner;
    std::list<RawCoastlineRef> rawCoastlines;

    coastlines.clear();

    try {
      uint32_t coastlineCount=0;
      size_t   wayCoastCount=0;
      size_t   areaCoastCount=0;

      scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                   rawFile),
                   FileScanner::Sequential,
                   true);

      scanner.Read(coastlineCount);

      progress.Info(std::to_string(coastlineCount)+" coastlines");

      for (uint32_t c=1; c<=coastlineCount; c++) {
        progress.SetProgress(c,coastlineCount);

        RawCoastlineRef coastline=std::make_shared<RawCoastline>();

        coastline->Read(scanner);

        rawCoastlines.push_back(coastline);
      }

      progress.SetAction("Resolving nodes of coastline");

      CoordDataFile coordDataFile;

      if (!coordDataFile.Open(parameter.GetDestinationDirectory(),
                              parameter.GetCoordDataMemoryMaped())) {
        progress.Error("Cannot open coord file!");
        return false;
      }

      std::set<OSMId> nodeIds;

      for (const auto& coastline : rawCoastlines) {
        for (size_t n=0; n<coastline->GetNodeCount(); n++) {
          nodeIds.insert(coastline->GetNodeId(n));
        }
      }

      CoordDataFile::ResultMap coordsMap;

      if (!coordDataFile.Get(nodeIds,
                             coordsMap)) {
        progress.Error("Cannot read nodes!");
        return false;
      }

      nodeIds.clear();

      progress.SetAction("Enriching coastline with node data");

      while (!rawCoastlines.empty()) {
        RawCoastlineRef coastline=rawCoastlines.front();
        bool            processingError=false;

        rawCoastlines.pop_front();

        WaterIndexProcessor::CoastRef coast=std::make_shared<WaterIndexProcessor::Coast>();

        coast->id=coastline->GetId();
        coast->isArea=coastline->IsArea();

        coast->coast.resize(coastline->GetNodeCount());
        coast->left=leftState;
        coast->right=rightState;

        for (size_t n=0; n<coastline->GetNodeCount(); n++) {
          CoordDataFile::ResultMap::const_iterator coord=coordsMap.find(coastline->GetNodeId(n));

          if (coord==coordsMap.end()) {
            processingError=true;

            progress.Error("Cannot resolve node with id "+
                           std::to_string(coastline->GetNodeId(n))+
                           " for coastline "+
                           std::to_string(coastline->GetId()));

            break;
          }

          if (n==0) {
            coast->frontNodeId=coord->second.GetOSMScoutId();
          }

          if (n==coastline->GetNodeCount()-1) {
            coast->backNodeId=coord->second.GetOSMScoutId();
          }

          coast->coast[n]=Point(coord->second.GetSerial(),
                                coord->second.GetCoord());
        }

        if (!processingError) {
          if (coast->isArea) {
            areaCoastCount++;
          }
          else {
            wayCoastCount++;
          }

          coastlines.push_back(coast);
        }
      }

      progress.Info(std::to_string(wayCoastCount)+" way coastline(s), "+std::to_string(areaCoastCount)+" area coastline(s)");

      scanner.Close();
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());
      return false;
    }

    return true;
  }

  /**
  * Assume cell type 'land' for cells that intersect with 'land' object types
  *
  * Every cell that is unknown but contains a way (that is marked
  * as "to be ignored"), must be land.
  */
  bool WaterIndexGenerator::AssumeLand(const ImportParameter& parameter,
                                       Progress& progress,
                                       const TypeConfig& typeConfig,
                                       const WaterIndexProcessor& processor,
                                       WaterIndexProcessor::StateMap& stateMap)
  {
    progress.Info("Assume land");

    BridgeFeatureReader bridgeFeatureRader(typeConfig);
    TunnelFeatureReader tunnelFeatureRader(typeConfig);
    EmbankmentFeatureReader embankmentFeatureRader(typeConfig);
    FileScanner         scanner;

    // We do not yet know if we handle borders as ways or areas

    try {
      uint32_t wayCount=0;

      scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                   WayDataFile::WAYS_DAT),
                   FileScanner::Sequential,
                   parameter.GetWayDataMemoryMaped());

      scanner.Read(wayCount);

      for (uint32_t w=1; w<=wayCount; w++) {
        progress.SetProgress(w,wayCount);

        Way way;

        way.Read(typeConfig,
                 scanner);

        if (way.GetType()!=typeConfig.typeInfoIgnore &&
            !way.GetType()->GetIgnoreSeaLand() &&
            !tunnelFeatureRader.IsSet(way.GetFeatureValueBuffer()) &&
            !bridgeFeatureRader.IsSet(way.GetFeatureValueBuffer()) &&
            !embankmentFeatureRader.IsSet(way.GetFeatureValueBuffer()) &&
            way.nodes.size()>=2) {
          std::set<Pixel> coords;

          processor.GetCells(stateMap,
                             way.nodes,
                             coords);

          for (const auto& coord : coords) {
            if (stateMap.IsInAbsolute(coord.x,coord.y)) {
              if (stateMap.GetStateAbsolute(coord.x,coord.y)==WaterIndexProcessor::unknown) {
#if defined(DEBUG_TILING)
                std::cout << "Assume land: " << coord.x-stateMap.GetXStart() << "," << coord.y-stateMap.GetYStart() << " Way " << way.GetFileOffset() << " " << way.GetType()->GetName() << " is defining area as land" << std::endl;
#endif
                stateMap.SetStateAbsolute(coord.x,coord.y,WaterIndexProcessor::land);
              }
            }
          }
        }
      }

      scanner.Close();
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());
      return false;
    }

    return true;
  }

  void WaterIndexGenerator::GetDescription(const ImportParameter& /*parameter*/,
                                              ImportModuleDescription& description) const
  {
    description.SetName("WaterIndexGenerator");
    description.SetDescription("Create index for lookup of ground/see tiles");

    description.AddRequiredFile(BoundingBoxDataFile::BOUNDINGBOX_DAT);

    description.AddRequiredFile(Preprocess::RAWCOASTLINE_DAT);

    description.AddRequiredFile(Preprocess::RAWDATAPOLYGON_DAT);

    description.AddRequiredFile(CoordDataFile::COORD_DAT);

    description.AddRequiredFile(WayDataFile::WAYS_DAT);

    description.AddProvidedFile(WaterIndex::WATER_IDX);
  }

  bool WaterIndexGenerator::Import(const TypeConfigRef& typeConfig,
                                   const ImportParameter& parameter,
                                   Progress& progress)
  {
    std::list<WaterIndexProcessor::CoastRef> coastlines;
    std::list<WaterIndexProcessor::CoastRef> boundingPolygons;

    GeoBox                                   boundingBox;

    std::vector<WaterIndexProcessor::Level>  levels;

    WaterIndexProcessor                      processor;

    //
    // Read bounding box
    //

    BoundingBoxDataFile boundingBoxDataFile;

    if (!boundingBoxDataFile.Load(parameter.GetDestinationDirectory())) {
      progress.Error("Error loading file '"+boundingBoxDataFile.GetFilename()+"'");

      return false;
    }

    boundingBox=boundingBoxDataFile.GetBoundingBox();

    //
    // Initialize levels
    //

    levels.reserve(parameter.GetWaterIndexMaxMag()-parameter.GetWaterIndexMinMag()+1);

    double cellWidth=360.0;
    double cellHeight=180.0;

    for (uint32_t zoomLevel=0; zoomLevel<=parameter.GetWaterIndexMaxMag(); zoomLevel++) {
      if (zoomLevel>=parameter.GetWaterIndexMinMag() &&
          zoomLevel<=parameter.GetWaterIndexMaxMag()) {
        WaterIndexProcessor::Level level;

        level.level=zoomLevel;
        level.SetBox(boundingBox,
                     cellWidth,
                     cellHeight);

        levels.push_back(level);
      }

      cellWidth=cellWidth/2.0;
      cellHeight=cellHeight/2.0;
    }

    //
    // Load data polygon
    //

    if (!LoadBoundingPolygons(parameter,
                              progress,
                              boundingPolygons)) {
      return false;
    }

    //
    // Load and merge coastlines
    //

    if (!LoadCoastlines(parameter,
                        progress,
                        coastlines)) {
      return false;
    }

    processor.MergeCoastlines(progress,coastlines);

    if (!boundingPolygons.empty()) {
      processor.SynthesizeCoastlines(progress,
                                     coastlines,
                                     boundingPolygons);
    }

    progress.SetAction("Writing 'water.idx'");

    FileWriter writer;

    try {
      writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                  WaterIndex::WATER_IDX));

      processor.DumpIndexHeader(writer,
                                levels);
      progress.Info("Generating index for level "+std::to_string(parameter.GetWaterIndexMinMag())+" to "+std::to_string(parameter.GetWaterIndexMaxMag()));

      for (auto& level : levels) {
        Magnification                          magnification;
        MercatorProjection                     projection;
        std::map<Pixel,std::list<GroundTile> > cellGroundTileMap;

        magnification.SetLevel(level.level);

        projection.Set(GeoCoord(0.0,0.0),magnification,72,640,480);

        progress.SetAction("Building tiles for level "+std::to_string(level.level));

        if (!coastlines.empty()) {
          WaterIndexProcessor::Data data;

          // Collects, calculates and generates a number of data about a coastline
          processor.CalculateCoastlineData(progress,
                                           parameter.GetOptimizationWayMethod(),
                                           1.0,
                                           4.0,
                                           projection,
                                           level.stateMap,
                                           coastlines,
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

        if (parameter.GetAssumeLand()==ImportParameter::AssumeLandStrategy::enable ||
            (parameter.GetAssumeLand()==ImportParameter::AssumeLandStrategy::automatic && boundingPolygons.empty())) {
          // Assume cell type 'land' for cells that intersect with 'land' object types
          AssumeLand(parameter,
                     progress,
                     *typeConfig,
                     processor,
                     level.stateMap);
        }

        if (!coastlines.empty()) {
          // Marks all still 'unknown' cells neighbouring 'water' cells as 'water', too
          processor.FillWater(progress,
                              level,
                              parameter.GetFillWaterArea(),
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

      coastlines.clear();

      writer.Close();
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());

      writer.CloseFailsafe();

      return false;
    }

    return true;
  }
}
