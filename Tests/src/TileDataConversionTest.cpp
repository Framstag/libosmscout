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

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <list>
#include <memory>
#include <mutex>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <osmscout/TypeConfig.h>
#include <osmscout/db/Database.h>
#include <osmscout/log/Logger.h>
#include <osmscoutmap/DataTileCache.h>
#include <osmscoutmap/MapService.h>
#include <osmscoutmap/StyleConfig.h>

#include <TestAllocationCounter.h>

// Since we refer to the ERROR enumeration member below and the Windows headers define ERROR as a
// macro, which one of the headers included above establishes after Logger.h has already cleared it:
// the macro is cleared here, after every include of this file.
#if defined(ERROR)
  #undef ERROR
#endif

namespace {

  /**
   * Share of the extent of the test region the test views, so that the tile set of a test stays small
   * and comfortably inside the region.
   */
  constexpr double kViewFraction=0.8;

  /**
   * Heap blocks one conversion may allocate on top of its result and its seen-offset sets: the
   * per-source state of a conversion and the bucket arrays of the sets as they grow. A conversion
   * does not allocate per object it examines, so this budget does not grow with the objects of the
   * tiles or with the tiles that repeat them - the case below checks that.
   */
  constexpr size_t kConversionBlockBudget=256;

  /**
   * The default threshold a conversion phase is reported at, the value the copy phase of the previous
   * implementation warned at.
   */
  constexpr double kDefaultPhaseWarningThreshold=20.0;

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
   *
   * The bounding box the database stores is not the extent of the region: it describes the file the
   * data was extracted from, which is far larger than the extract, so a view derived from it would
   * miss the data. The region definition is the extent the data was clipped to.
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

      // The name, the ring counter and the 'END' lines do not carry a coordinate pair.
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

  struct Context
  {
    osmscout::DatabaseRef   database;
    osmscout::MapServiceRef mapService;
    osmscout::TypeConfigRef typeConfig;
  };

  bool OpenContext(Context& context)
  {
    osmscout::DatabaseParameter parameter;

    context.database=std::make_shared<osmscout::Database>(parameter);

    if (!context.database->Open(TestRegionDir().string())) {
      return false;
    }

    context.mapService=std::make_shared<osmscout::MapService>(context.database);
    context.typeConfig=context.database->GetTypeConfig();

    return true;
  }

  /**
   * Load a view of the test region into a tile set.
   */
  bool LoadTiles(Context& context,
                 unsigned int level,
                 std::list<osmscout::TileRef>& tiles)
  {
    osmscout::GeoBox region;

    if (!ReadTestRegionExtent(region)) {
      return false;
    }

    osmscout::GeoCoord center=region.GetCenter();

    double width=(region.GetMaxLon()-region.GetMinLon())*kViewFraction;
    double height=(region.GetMaxLat()-region.GetMinLat())*kViewFraction;

    osmscout::GeoBox view(osmscout::GeoCoord(center.GetLat()-height/2,center.GetLon()-width/2),
                          osmscout::GeoCoord(center.GetLat()+height/2,center.GetLon()+width/2));

    osmscout::StyleConfigRef styleConfig=std::make_shared<osmscout::StyleConfig>(context.typeConfig);

    if (!styleConfig->Load(StyleSheetFile().string())) {
      return false;
    }

    osmscout::AreaSearchParameter searchParameter;

    searchParameter.SetUseMultithreading(false);

    osmscout::Magnification magnification{osmscout::MagnificationLevel(level)};

    context.mapService->LookupTiles(magnification,view,tiles);

    return context.mapService->LoadMissingTileData(searchParameter,*styleConfig,tiles);
  }

  /**
   * The offsets of the objects one source data file of the tiles holds, each of them once.
   */
  template<typename O>
  std::set<osmscout::FileOffset> TileOffsets(const std::list<osmscout::TileRef>& tiles,
                                             const std::function<const osmscout::TileData<O>&(const osmscout::TileRef&)>& getData)
  {
    std::set<osmscout::FileOffset> offsets;

    for (const auto& tile : tiles) {
      getData(tile).CopyData([&offsets](const O& object) {
        offsets.insert(object->GetFileOffset());
      });
    }

    return offsets;
  }

  template<typename O>
  std::vector<osmscout::FileOffset> OffsetsOf(const std::vector<O>& objects)
  {
    std::vector<osmscout::FileOffset> offsets;

    offsets.reserve(objects.size());

    for (const auto& object : objects) {
      offsets.push_back(object->GetFileOffset());
    }

    return offsets;
  }

  /**
   * The offsets of the result of one source data file: the sequence the conversion returned, sorted,
   * so that it can be compared with the set of the offsets the tiles hold.
   */
  template<typename O>
  std::vector<osmscout::FileOffset> SortedOffsetsOf(const std::vector<O>& objects)
  {
    std::vector<osmscout::FileOffset> offsets=OffsetsOf(objects);

    std::sort(offsets.begin(),offsets.end());

    return offsets;
  }

  /**
   * Return true if the sequence holds no offset twice.
   */
  template<typename O>
  bool HoldsEachObjectOnce(const std::vector<O>& objects)
  {
    std::vector<osmscout::FileOffset> offsets=OffsetsOf(objects);

    std::sort(offsets.begin(),offsets.end());

    return std::adjacent_find(offsets.begin(),offsets.end())==offsets.end();
  }

  size_t Insertions(const std::list<osmscout::TileRef>& tiles)
  {
    size_t insertions=0;

    for (const auto& tile : tiles) {
      insertions+=tile->GetNodeData().GetDataSize();
      insertions+=tile->GetWayData().GetDataSize();
      insertions+=tile->GetOptimizedWayData().GetDataSize();
      insertions+=tile->GetAreaData().GetDataSize();
      insertions+=tile->GetOptimizedAreaData().GetDataSize();
      insertions+=tile->GetRouteData().GetDataSize();
    }

    return insertions;
  }

  template<typename O>
  size_t CountOf(const std::vector<O>& objects)
  {
    return objects.size();
  }

  /**
   * Number of objects a conversion placed in the given map data.
   */
  size_t Distinct(const osmscout::MapData& data)
  {
    return CountOf(data.nodes)+CountOf(data.ways)+CountOf(data.areas)+CountOf(data.routes);
  }

  /**
   * A log destination that captures the warnings of the library into a string.
   *
   * The MapService of the tests loads tiles on worker threads, so the destination is written to from
   * more than one thread and guards its output.
   */
  class CapturingDestination : public osmscout::Logger::Destination
  {
  private:
    std::shared_ptr<std::string> output;
    std::mutex                   mutex;
    bool                         capturing{false};

  public:
    explicit CapturingDestination(const std::shared_ptr<std::string>& output)
    : output(output)
    {
      // no code
    }

    void Capture(bool state)
    {
      std::scoped_lock<std::mutex> guard(mutex);

      capturing=state;
    }

    void Print(const std::string& value) override
    {
      std::scoped_lock<std::mutex> guard(mutex);

      if (capturing) {
        *output+=value;
      }
    }

    void Print(const std::string_view& value) override
    {
      std::scoped_lock<std::mutex> guard(mutex);

      if (capturing) {
        output->append(value);
      }
    }

    void Print(const char* value) override
    {
      std::scoped_lock<std::mutex> guard(mutex);

      if (capturing) {
        *output+=value;
      }
    }

    void Print(bool value) override
    {
      std::scoped_lock<std::mutex> guard(mutex);

      if (capturing) {
        *output+=value ? "true" : "false";
      }
    }

    void Print(short value) override
    {
      std::scoped_lock<std::mutex> guard(mutex);

      if (capturing) {
        *output+=std::to_string(value);
      }
    }

    void Print(unsigned short value) override
    {
      std::scoped_lock<std::mutex> guard(mutex);

      if (capturing) {
        *output+=std::to_string(value);
      }
    }

    void Print(int value) override
    {
      std::scoped_lock<std::mutex> guard(mutex);

      if (capturing) {
        *output+=std::to_string(value);
      }
    }

    void Print(unsigned int value) override
    {
      std::scoped_lock<std::mutex> guard(mutex);

      if (capturing) {
        *output+=std::to_string(value);
      }
    }

    void Print(long value) override
    {
      std::scoped_lock<std::mutex> guard(mutex);

      if (capturing) {
        *output+=std::to_string(value);
      }
    }

    void Print(unsigned long value) override
    {
      std::scoped_lock<std::mutex> guard(mutex);

      if (capturing) {
        *output+=std::to_string(value);
      }
    }

    void Print(long long value) override
    {
      std::scoped_lock<std::mutex> guard(mutex);

      if (capturing) {
        *output+=std::to_string(value);
      }
    }

    void Print(unsigned long long value) override
    {
      std::scoped_lock<std::mutex> guard(mutex);

      if (capturing) {
        *output+=std::to_string(value);
      }
    }

    void PrintLn() override
    {
      std::scoped_lock<std::mutex> guard(mutex);

      if (capturing) {
        *output+='\n';
      }
    }
  };

  /**
   * A logger that captures the warnings and the errors of the library.
   */
  class CapturingLogger : public osmscout::Logger
  {
  private:
    CapturingDestination destination;

  public:
    explicit CapturingLogger(const std::shared_ptr<std::string>& output)
    : destination(output)
    {
      // no code
    }

    CapturingDestination& Captured()
    {
      return destination;
    }

    osmscout::Logger::Line Log(osmscout::Logger::Level level) override
    {
      destination.Capture(level==osmscout::Logger::WARN || level==osmscout::Logger::ERROR);

      return osmscout::Logger::Line(destination);
    }
  };

  size_t Occurrences(const std::string& text,
                     const std::string& needle)
  {
    size_t count=0;
    size_t position=text.find(needle);

    while (position!=std::string::npos) {
      count++;
      position=text.find(needle,position+needle.size());
    }

    return count;
  }

  /**
   * Install a logger that captures the warnings of the library.
   *
   * The logger stays installed for the rest of this test binary: the default logger of the library is
   * created inside the library and cannot be re-created from outside, so a test cannot restore it.
   */
  std::shared_ptr<CapturingLogger> InstallCapturingLogger(const std::shared_ptr<std::string>& output)
  {
    auto logger=std::make_shared<CapturingLogger>(output);

    osmscout::log.SetLogger(logger);

    return logger;
  }
}

TEST_CASE("A tile set converts to each object exactly once", "[TileDataConversion]")
{
  Context context;

  REQUIRE(OpenContext(context));

  std::list<osmscout::TileRef> tiles;

  REQUIRE(LoadTiles(context,15,tiles));
  REQUIRE(!tiles.empty());

  osmscout::MapData data;

  context.mapService->AddTileDataToMapData(tiles,data);

  REQUIRE(Distinct(data)>0);

  std::set<osmscout::FileOffset> tileNodes=TileOffsets<osmscout::NodeRef>(tiles,[](const osmscout::TileRef& tile) -> const osmscout::TileNodeData& {
    return tile->GetNodeData();
  });

  std::set<osmscout::FileOffset> tileWays=TileOffsets<osmscout::WayRef>(tiles,[](const osmscout::TileRef& tile) -> const osmscout::TileWayData& {
    return tile->GetWayData();
  });

  std::set<osmscout::FileOffset> tileAreas=TileOffsets<osmscout::AreaRef>(tiles,[](const osmscout::TileRef& tile) -> const osmscout::TileAreaData& {
    return tile->GetAreaData();
  });

  // Every object the tiles hold is in the result, and no object is in it twice.
  CHECK(SortedOffsetsOf(data.nodes)==std::vector<osmscout::FileOffset>(tileNodes.begin(),tileNodes.end()));
  CHECK(SortedOffsetsOf(data.ways)==std::vector<osmscout::FileOffset>(tileWays.begin(),tileWays.end()));
  CHECK(SortedOffsetsOf(data.areas)==std::vector<osmscout::FileOffset>(tileAreas.begin(),tileAreas.end()));

  CHECK(HoldsEachObjectOnce(data.nodes));
  CHECK(HoldsEachObjectOnce(data.ways));
  CHECK(HoldsEachObjectOnce(data.areas));
  CHECK(HoldsEachObjectOnce(data.routes));

  // The tiles of the view share objects - an object at a tile border is data of both tiles - so the
  // assertions above genuinely exercise the deduplication.
  REQUIRE(Insertions(tiles)>Distinct(data));
}

TEST_CASE("The result of a kind holds its regular source before its optimized source", "[TileDataConversion]")
{
  Context context;

  REQUIRE(OpenContext(context));

  std::list<osmscout::TileRef> tiles;

  REQUIRE(LoadTiles(context,15,tiles));
  REQUIRE(!tiles.empty());

  osmscout::MapData data;

  context.mapService->AddTileDataToMapData(tiles,data);

  std::set<osmscout::FileOffset> optimizedAreaOffsets=TileOffsets<osmscout::AreaRef>(tiles,[](const osmscout::TileRef& tile) -> const osmscout::TileAreaData& {
    return tile->GetOptimizedAreaData();
  });

  // Every area of the optimized data file comes after the areas of the regular data file.
  bool seenOptimized=false;

  for (const auto& area : data.areas) {
    bool isOptimized=optimizedAreaOffsets.find(area->GetFileOffset())!=optimizedAreaOffsets.end();

    if (isOptimized) {
      seenOptimized=true;
    }
    else {
      CHECK(!seenOptimized);
    }
  }
}

TEST_CASE("The same tile list converts to the same sequence", "[TileDataConversion]")
{
  Context context;

  REQUIRE(OpenContext(context));

  std::list<osmscout::TileRef> tiles;

  REQUIRE(LoadTiles(context,15,tiles));
  REQUIRE(!tiles.empty());

  osmscout::MapData first;
  osmscout::MapData second;

  context.mapService->AddTileDataToMapData(tiles,first);
  context.mapService->AddTileDataToMapData(tiles,second);

  REQUIRE(Distinct(first)>0);

  CHECK(OffsetsOf(first.nodes)==OffsetsOf(second.nodes));
  CHECK(OffsetsOf(first.ways)==OffsetsOf(second.ways));
  CHECK(OffsetsOf(first.areas)==OffsetsOf(second.areas));
  CHECK(OffsetsOf(first.routes)==OffsetsOf(second.routes));
}

TEST_CASE("An object carried by several tiles appears once", "[TileDataConversion]")
{
  Context context;

  REQUIRE(OpenContext(context));

  std::list<osmscout::TileRef> tiles;

  REQUIRE(LoadTiles(context,15,tiles));
  REQUIRE(!tiles.empty());

  osmscout::MapData once;

  context.mapService->AddTileDataToMapData(tiles,once);

  REQUIRE(Distinct(once)>0);

  // The same tiles a second time: every object of the tile set is now carried twice, and the result
  // may not change.
  std::list<osmscout::TileRef> doubled=tiles;

  doubled.insert(doubled.end(),tiles.begin(),tiles.end());

  osmscout::MapData twice;

  context.mapService->AddTileDataToMapData(doubled,twice);

  CHECK(OffsetsOf(twice.nodes)==OffsetsOf(once.nodes));
  CHECK(OffsetsOf(twice.ways)==OffsetsOf(once.ways));
  CHECK(OffsetsOf(twice.areas)==OffsetsOf(once.areas));
}

TEST_CASE("Objects of two data files that share an offset both appear", "[TileDataConversion]")
{
  Context context;

  REQUIRE(OpenContext(context));

  std::list<osmscout::TileRef> tiles;

  REQUIRE(LoadTiles(context,15,tiles));
  REQUIRE(!tiles.empty());

  osmscout::TileRef tile=tiles.front();

  // Put the regular ways of a tile into its optimized way data as well. The two data files are
  // independent offset spaces, so an implementation that deduplicated across them would treat the
  // pair as one object and drop one of the two.
  std::vector<osmscout::WayRef> ways;

  tile->GetWayData().CopyData([&ways](const osmscout::WayRef& way) {
    ways.push_back(way);
  });

  REQUIRE(!ways.empty());

  osmscout::TypeInfoSet types;

  tile->GetOptimizedWayData().SetData(types,ways);

  // The same for the areas of the tile.
  std::vector<osmscout::AreaRef> areas;

  tile->GetAreaData().CopyData([&areas](const osmscout::AreaRef& area) {
    areas.push_back(area);
  });

  REQUIRE(!areas.empty());

  tile->GetOptimizedAreaData().SetData(types,areas);

  std::list<osmscout::TileRef> oneTile{tile};

  osmscout::MapData data;

  context.mapService->AddTileDataToMapData(oneTile,data);

  CHECK(data.ways.size()==2*ways.size());
  CHECK(data.areas.size()==2*areas.size());
}

TEST_CASE("A restricted conversion holds the same contract", "[TileDataConversion]")
{
  Context context;

  REQUIRE(OpenContext(context));

  std::list<osmscout::TileRef> tiles;

  REQUIRE(LoadTiles(context,15,tiles));
  REQUIRE(!tiles.empty());

  osmscout::MapData all;

  context.mapService->AddTileDataToMapData(tiles,all);

  REQUIRE(Distinct(all)>0);
  REQUIRE(!all.ways.empty());

  // Request the ways of one type only.
  osmscout::MapService::TypeDefinition definition;

  definition.wayTypes.Set(all.ways.front()->GetType());

  osmscout::MapData filtered;

  context.mapService->AddTileDataToMapData(tiles,definition,filtered);

  CHECK(filtered.nodes.empty());
  CHECK(filtered.areas.empty());
  CHECK(filtered.routes.empty());

  // The restricted conversion returns every way of the requested type, each of them once, and only
  // ways the unrestricted conversion returned.
  std::vector<osmscout::FileOffset> allWayOffsets=OffsetsOf(all.ways);
  std::set<osmscout::FileOffset>    unrestricted(allWayOffsets.begin(),allWayOffsets.end());

  CHECK(!filtered.ways.empty());
  CHECK(HoldsEachObjectOnce(filtered.ways));

  for (const auto& way : filtered.ways) {
    CHECK(definition.wayTypes.IsSet(way->GetType()));
    CHECK(unrestricted.find(way->GetFileOffset())!=unrestricted.end());
  }

  // The restricted conversion returns exactly the matching ways of the tiles: the offsets of the ways
  // in the result are the offsets of the tile ways the filter accepts.
  std::set<osmscout::FileOffset> matching;

  for (const auto& tile : tiles) {
    tile->GetWayData().CopyData([&matching,&definition](const osmscout::WayRef& way) {
      if (definition.wayTypes.IsSet(way->GetType())) {
        matching.insert(way->GetFileOffset());
      }
    });
  }

  REQUIRE(!matching.empty());

  std::vector<osmscout::FileOffset> filteredOffsets=OffsetsOf(filtered.ways);
  std::set<osmscout::FileOffset>    filteredSet(filteredOffsets.begin(),filteredOffsets.end());

  CHECK(filteredSet==matching);
}

TEST_CASE("Every conversion phase over its threshold is reported by name", "[TileDataConversion]")
{
  Context context;

  REQUIRE(OpenContext(context));

  std::list<osmscout::TileRef> tiles;

  REQUIRE(LoadTiles(context,15,tiles));
  REQUIRE(!tiles.empty());

  auto output=std::make_shared<std::string>();
  auto logger=InstallCapturingLogger(output);

  // Every phase is over its threshold, so every phase that converts an object is reported.
  context.mapService->SetConversionPhaseWarningThreshold(0.0);

  osmscout::MapData data;

  context.mapService->AddTileDataToMapData(tiles,data);

  REQUIRE(Distinct(data)>0);

  logger->Captured().Capture(false);

  // One warning per slow phase, naming that phase.
  CHECK(Occurrences(*output,"phase nodes")==1);
  CHECK(Occurrences(*output,"phase ways")==1);
  CHECK(Occurrences(*output,"phase optimized ways")==1);
  CHECK(Occurrences(*output,"phase areas")==1);
  CHECK(Occurrences(*output,"phase optimized areas")==1);
  CHECK(Occurrences(*output,"phase routes")==1);
}

TEST_CASE("A conversion below the threshold reports nothing", "[TileDataConversion]")
{
  Context context;

  REQUIRE(OpenContext(context));

  std::list<osmscout::TileRef> tiles;

  REQUIRE(LoadTiles(context,15,tiles));
  REQUIRE(!tiles.empty());

  auto output=std::make_shared<std::string>();
  auto logger=InstallCapturingLogger(output);

  CHECK(context.mapService->GetConversionPhaseWarningThreshold()==kDefaultPhaseWarningThreshold);

  // A threshold no phase can exceed, so that the case does not depend on the speed of the machine.
  context.mapService->SetConversionPhaseWarningThreshold(1.0e9);

  osmscout::MapData data;

  context.mapService->AddTileDataToMapData(tiles,data);

  REQUIRE(Distinct(data)>0);

  logger->Captured().Capture(false);

  CHECK(output->empty());
}

TEST_CASE("The conversion does not allocate per object it examines", "[TileDataConversion]")
{
  Context context;

  REQUIRE(OpenContext(context));

  std::list<osmscout::TileRef> tiles;

  REQUIRE(LoadTiles(context,15,tiles));
  REQUIRE(!tiles.empty());

  if (!osmscout::test::AllocationCounterEnabled()) {
    WARN("Counting allocator is not compiled in, the allocation bound is not checked");
    return;
  }

  osmscout::MapData data;

  size_t blocksBefore=osmscout::test::GetAllocationCount();

  context.mapService->AddTileDataToMapData(tiles,data);

  size_t blocks=osmscout::test::GetAllocationCount()-blocksBefore;

  REQUIRE(Distinct(data)>0);

  // The result vectors are reserved once and every source data file holds the objects it has seen in
  // a set, which allocates once per distinct object: the number of blocks is bounded by the distinct
  // object count plus the budget of the per-source state.
  CHECK(blocks<=Distinct(data)+kConversionBlockBudget);

  // The same tiles a second time carry every object twice. The conversion examines the repeat and
  // finds the object already seen, so the allocation may not grow with the repeats.
  std::list<osmscout::TileRef> doubled=tiles;

  doubled.insert(doubled.end(),tiles.begin(),tiles.end());

  osmscout::MapData doubledData;

  size_t doubledBefore=osmscout::test::GetAllocationCount();

  context.mapService->AddTileDataToMapData(doubled,doubledData);

  size_t doubledBlocks=osmscout::test::GetAllocationCount()-doubledBefore;

  CHECK(Distinct(doubledData)==Distinct(data));
  CHECK(doubledBlocks<=blocks);
}
