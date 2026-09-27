/*
  This source is part of the libosmscout library
  Copyright (C) 2026  Tim Teulings

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

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <list>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include <osmscout/TypeConfig.h>
#include <osmscout/db/Database.h>
#include <osmscout/util/StopClock.h>
#include <osmscoutmap/DataTileCache.h>
#include <osmscoutmap/MapService.h>
#include <osmscoutmap/StyleConfig.h>

namespace {

  /**
   * Number of conversions measured per side. Each side keeps its fastest run, so that a single slow
   * run of the machine does not decide the comparison.
   */
  constexpr size_t kRepetitions=20;

  /**
   * Share of the conversion the conversion under test may need of the conversion it replaces, for one
   * tile set. The measured share is well below it on every tile set of this test; the margin leaves
   * room for the load of the machine, so that the assertion reports a regression rather than noise.
   */
  constexpr double kMaxSharePerTileSet=0.9;

  /**
   * Share of the conversion the conversion under test may need of the conversion it replaces over all
   * tile sets of this test together. This is the speed gain the change is made for.
   */
  constexpr double kMaxShareOverall=0.8;

  /**
   * The tile sets the comparison converts: a zoom level and the share of the extent of the test
   * region it views. The tile sets differ in the number of tiles and in how often the tiles repeat
   * the same object - a larger view has more tiles and repeats its objects more often - because the
   * two mechanisms the comparison covers behave differently over that range.
   */
  struct TileSet
  {
    unsigned int level;
    double       fraction;
  };

  const std::vector<TileSet> kTileSets={
    {15,0.8},
    {16,0.4},
    {16,0.2},
    {17,0.26},
    {17,0.13},
    {18,0.13},
    {18,0.07},
    {18,0.035}
  };

  std::string GetEnv(const char* name,
                     const std::string& fallback)
  {
    const char* value=std::getenv(name);

    return value!=nullptr ? std::string(value) : fallback;
  }

  std::filesystem::path TestsTopDir()
  {
    return std::filesystem::path(GetEnv("TESTS_TOP_DIR",".."));
  }

  std::filesystem::path TestRegionDir()
  {
    return TestsTopDir()/"data"/"testregion";
  }

  std::filesystem::path TestRegionPolyFile()
  {
    return TestsTopDir()/"data"/"testregion.poly";
  }

  std::filesystem::path StyleSheetFile()
  {
    return TestsTopDir()/".."/"stylesheets"/"standard.oss";
  }

  /**
   * Read the extent of the test region from the region definition the database was generated from.
   * The bounding box the database stores is the file the data was extracted from, not the extract.
   */
  bool ReadTestRegionExtent(osmscout::GeoBox& extent)
  {
    std::ifstream file(TestRegionPolyFile());

    if (!file) {
      return false;
    }

    std::string line;
    double      minLat=0;
    double      maxLat=0;
    double      minLon=0;
    double      maxLon=0;
    bool        hasPoint=false;

    while (std::getline(file,line)) {
      std::istringstream stream(line);

      double lon;
      double lat;

      if (!(stream >> lon >> lat)) {
        continue;
      }

      if (!hasPoint) {
        minLat=maxLat=lat;
        minLon=maxLon=lon;
        hasPoint=true;
      }
      else {
        minLat=std::min(minLat,lat);
        maxLat=std::max(maxLat,lat);
        minLon=std::min(minLon,lon);
        maxLon=std::max(maxLon,lon);
      }
    }

    if (!hasPoint) {
      return false;
    }

    extent=osmscout::GeoBox(osmscout::GeoCoord(minLat,minLon),
                            osmscout::GeoCoord(maxLat,maxLon));

    return true;
  }

  /**
   * The conversion as it was before this change: one hash map per kind and source, an insertion per
   * object of every tile, and a second pass that copies the maps into the result vectors.
   *
   * The baseline lives in the test rather than in the library, so that the comparison survives the
   * removal of the old code.
   */
  osmscout::MapData ConvertWithHashMaps(const std::list<osmscout::TileRef>& tiles)
  {
    osmscout::MapData data;

    std::unordered_map<osmscout::FileOffset,osmscout::NodeRef>  nodeMap(10000);
    std::unordered_map<osmscout::FileOffset,osmscout::WayRef>   wayMap(10000);
    std::unordered_map<osmscout::FileOffset,osmscout::AreaRef>  areaMap(10000);
    std::unordered_map<osmscout::FileOffset,osmscout::RouteRef> routeMap(1000);
    std::unordered_map<osmscout::FileOffset,osmscout::WayRef>   optimizedWayMap(10000);
    std::unordered_map<osmscout::FileOffset,osmscout::AreaRef>  optimizedAreaMap(10000);

    for (const auto& tile : tiles) {
      tile->GetNodeData().CopyData([&nodeMap](const osmscout::NodeRef& node) {
        nodeMap[node->GetFileOffset()]=node;
      });

      tile->GetOptimizedWayData().CopyData([&optimizedWayMap](const osmscout::WayRef& way) {
        optimizedWayMap[way->GetFileOffset()]=way;
      });

      tile->GetWayData().CopyData([&wayMap](const osmscout::WayRef& way) {
        wayMap[way->GetFileOffset()]=way;
      });

      tile->GetOptimizedAreaData().CopyData([&optimizedAreaMap](const osmscout::AreaRef& area) {
        optimizedAreaMap[area->GetFileOffset()]=area;
      });

      tile->GetAreaData().CopyData([&areaMap](const osmscout::AreaRef& area) {
        areaMap[area->GetFileOffset()]=area;
      });

      tile->GetRouteData().CopyData([&routeMap](const osmscout::RouteRef& route) {
        routeMap[route->GetFileOffset()]=route;
      });
    }

    data.nodes.reserve(nodeMap.size());
    data.ways.reserve(wayMap.size()+optimizedWayMap.size());
    data.areas.reserve(areaMap.size()+optimizedAreaMap.size());
    data.routes.reserve(routeMap.size());

    for (const auto& entry : nodeMap) {
      data.nodes.push_back(entry.second);
    }

    for (const auto& entry : wayMap) {
      data.ways.push_back(entry.second);
    }

    for (const auto& entry : optimizedWayMap) {
      data.ways.push_back(entry.second);
    }

    for (const auto& entry : areaMap) {
      data.areas.push_back(entry.second);
    }

    for (const auto& entry : optimizedAreaMap) {
      data.areas.push_back(entry.second);
    }

    for (const auto& entry : routeMap) {
      data.routes.push_back(entry.second);
    }

    return data;
  }
}

TEST_CASE("The conversion is faster than the conversion it replaces","[TileDataConversionPerformance]")
{
  osmscout::DatabaseParameter databaseParameter;
  osmscout::DatabaseRef       database=std::make_shared<osmscout::Database>(databaseParameter);

  REQUIRE(database->Open(TestRegionDir().string()));

  osmscout::GeoBox region;

  REQUIRE(ReadTestRegionExtent(region));

  osmscout::StyleConfigRef styleConfig=std::make_shared<osmscout::StyleConfig>(database->GetTypeConfig());

  REQUIRE(styleConfig->Load(StyleSheetFile().string()));

  osmscout::AreaSearchParameter searchParameter;

  searchParameter.SetUseMultithreading(false);

  double baselineTotal=0.0;
  double conversionTotal=0.0;

  for (const auto& tileSet : kTileSets) {
    // A fresh service per tile set: the cache of a service hands a tile the data it holds from a
    // previous view, so a shared service would compare tile sets whose contents depend on the tile
    // sets that were converted before them.
    osmscout::MapServiceRef tileSetService=std::make_shared<osmscout::MapService>(database);

    osmscout::GeoCoord center=region.GetCenter();

    double width=(region.GetMaxLon()-region.GetMinLon())*tileSet.fraction;
    double height=(region.GetMaxLat()-region.GetMinLat())*tileSet.fraction;

    osmscout::GeoBox view(osmscout::GeoCoord(center.GetLat()-height/2,center.GetLon()-width/2),
                          osmscout::GeoCoord(center.GetLat()+height/2,center.GetLon()+width/2));

    std::list<osmscout::TileRef> tiles;

    tileSetService->LookupTiles(osmscout::Magnification{osmscout::MagnificationLevel(tileSet.level)},view,tiles);

    REQUIRE(tileSetService->LoadMissingTileData(searchParameter,*styleConfig,tiles));
    REQUIRE(!tiles.empty());

    double baselineBest=0.0;
    double conversionBest=0.0;

    osmscout::MapData data;

    for (size_t repetition=0; repetition<kRepetitions; repetition++) {
      {
        osmscout::StopClock timer;

        osmscout::MapData baseline=ConvertWithHashMaps(tiles);

        timer.Stop();

        baselineBest=repetition==0 ? timer.GetMilliseconds() : std::min(baselineBest,timer.GetMilliseconds());

        REQUIRE(!baseline.nodes.empty());
      }

      {
        data.nodes.clear();
        data.ways.clear();
        data.areas.clear();
        data.routes.clear();

        osmscout::StopClock timer;

        tileSetService->AddTileDataToMapData(tiles,data);

        timer.Stop();

        conversionBest=repetition==0 ? timer.GetMilliseconds() : std::min(conversionBest,timer.GetMilliseconds());
      }
    }

    REQUIRE(!data.nodes.empty());

    baselineTotal+=baselineBest;
    conversionTotal+=conversionBest;

    INFO("level " << tileSet.level << ", fraction " << tileSet.fraction
                  << ", tiles " << tiles.size()
                  << ", objects " << data.nodes.size()+data.ways.size()+data.areas.size()+data.routes.size());
    INFO("baseline " << baselineBest << " ms, conversion " << conversionBest
                     << " ms, share " << conversionBest/baselineBest);

    CHECK(conversionBest<=baselineBest*kMaxSharePerTileSet);
  }

  INFO("baseline over all tile sets " << baselineTotal << " ms, conversion " << conversionTotal
                                      << " ms, share " << conversionTotal/baselineTotal);

  CHECK(conversionTotal<=baselineTotal*kMaxShareOverall);
}
