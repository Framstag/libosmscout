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

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <list>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <osmscout/db/Database.h>
#include <osmscout/io/FileWriter.h>
#include <osmscout/Area.h>
#include <osmscout/Node.h>
#include <osmscout/OSMScoutTypes.h>
#include <osmscout/TypeConfig.h>
#include <osmscout/TypeInfoSet.h>
#include <osmscout/util/GeoBox.h>
#include <osmscout/util/Magnification.h>
#include <osmscout/util/TileId.h>

#include <osmscoutmap/DataTileCache.h>
#include <osmscoutmap/MapData.h>
#include <osmscoutmap/MapDataAccounting.h>
#include <osmscoutmap/MapDataBudget.h>
#include <osmscoutmap/MapService.h>

#include <catch2/catch_test_macros.hpp>

namespace {

  /**
   * Magnification the tests look up tiles and load data at
   */
  const osmscout::Magnification testMagnification{15};

  /**
   * Return a tile key for the given tile id at the magnification of the tests
   */
  osmscout::TileKey GetTileKey(size_t tileId)
  {
    return {testMagnification,
            osmscout::TileId(static_cast<uint32_t>(tileId),
                             static_cast<uint32_t>(tileId))};
  }

  /**
   * Fill the tile with the given number of nodes, which weigh the least of the cached kinds
   */
  void FillNodeTile(const osmscout::TileRef& tile,
                    size_t nodeCount)
  {
    osmscout::TypeInfoSet          types;
    std::vector<osmscout::NodeRef> nodes;

    nodes.reserve(nodeCount);

    for (size_t i=0; i<nodeCount; i++) {
      nodes.push_back(std::make_shared<osmscout::Node>());
    }

    tile->GetNodeData().SetData(types,
                                std::move(nodes));
  }

  /**
   * Fill the tile with the given number of areas
   */
  void FillAreaTile(const osmscout::TileRef& tile,
                    size_t areaCount)
  {
    osmscout::TypeInfoSet          types;
    std::vector<osmscout::AreaRef> areas;

    areas.reserve(areaCount);

    for (size_t i=0; i<areaCount; i++) {
      areas.push_back(std::make_shared<osmscout::Area>());
    }

    tile->GetAreaData().SetData(types,
                                std::move(areas));
  }

  /**
   * Return the file name of the stylesheet the test database is rendered with
   */
  std::string GetTestStyleSheet()
  {
    const char * topDir=getenv("TESTS_TOP_DIR");

    REQUIRE(topDir!=nullptr);

    return std::string(topDir)+"/../stylesheets/standard.oss";
  }

  /**
   * Return the directory of the test database, from the environment the test is started with
   */
  std::string GetTestDatabaseDirectory()
  {
    const char * topDir=getenv("TESTS_TOP_DIR");

    REQUIRE(topDir!=nullptr);

    return std::string(topDir)+"/data/testregion";
  }

  /**
   * Return a set of all node types of the given type configuration
   */
  osmscout::TypeInfoSet GetNodeTypes(const osmscout::TypeConfigRef& typeConfig)
  {
    osmscout::TypeInfoSet nodeTypes;

    for (size_t t=0; t<typeConfig->GetTypeCount(); t++) {
      osmscout::TypeInfoRef type=typeConfig->GetTypeInfo(t);

      if (type->CanBeNode()) {
        nodeTypes.Set(type);
      }
    }

    return nodeTypes;
  }

  /**
   * Return the file offsets of the nodes of the test database, in ascending order
   */
  std::vector<osmscout::FileOffset> GetNodeOffsets(const osmscout::DatabaseRef& database)
  {
    osmscout::GeoBox                  boundingBox;
    std::vector<osmscout::FileOffset> offsets;
    osmscout::TypeInfoSet             loadedTypes;

    REQUIRE(database->GetBoundingBox(boundingBox));

    auto areaNodeIndex=database->GetAreaNodeIndex();

    REQUIRE(areaNodeIndex!=nullptr);
    REQUIRE(areaNodeIndex->GetOffsets(boundingBox,
                                      GetNodeTypes(database->GetTypeConfig()),
                                      offsets,
                                      loadedTypes));

    std::ranges::sort(offsets);

    return offsets;
  }

  /**
   * Return the directory for files the test writes, from the environment the test is started with
   */
  std::string GetTempDirectory()
  {
    const char * tmpDir=getenv("TESTS_TMP_DIR");

    REQUIRE(tmpDir!=nullptr);

    return std::string(tmpDir)+"/map-data-budget";
  }

  /**
   * Create a database directory that holds the type configuration of the test database but a
   * bounding box of its own, so that a database exists whose extent does not cover the testregion
   */
  std::string CreateDatabaseWithBoundingBox(const std::string& sourceDirectory,
                                            const std::string& name,
                                            const osmscout::GeoBox& boundingBox)
  {
    std::string directory=GetTempDirectory()+"/"+name;

    std::filesystem::remove_all(directory);
    std::filesystem::create_directories(directory);
    std::filesystem::copy_file(sourceDirectory+"/types.dat",
                               directory+"/types.dat");

    osmscout::FileWriter writer;

    writer.Open(directory+"/bounding.dat");

    REQUIRE(writer.IsOpen());

    writer.WriteBox(boundingBox);
    writer.Close();

    return directory;
  }

  /**
   * Create a copy of the test database, so that two databases with the same content but separate
   * caches exist
   */
  std::string CreateDatabaseCopy(const std::string& sourceDirectory,
                                 const std::string& name)
  {
    std::string directory=GetTempDirectory()+"/"+name;

    std::filesystem::remove_all(directory);
    std::filesystem::copy(sourceDirectory,
                          directory,
                          std::filesystem::copy_options::recursive);

    return directory;
  }

  /**
   * Return the file offsets of the objects of every kind of the given map data, sorted
   */
  std::vector<osmscout::FileOffset> GetObjectOffsets(const osmscout::MapData& data)
  {
    std::vector<osmscout::FileOffset> offsets;

    for (const auto& node : data.nodes) {
      offsets.push_back(node->GetFileOffset());
    }

    for (const auto& way : data.ways) {
      offsets.push_back(way->GetFileOffset());
    }

    for (const auto& area : data.areas) {
      offsets.push_back(area->GetFileOffset());
    }

    for (const auto& route : data.routes) {
      offsets.push_back(route->GetFileOffset());
    }

    std::ranges::sort(offsets);

    return offsets;
  }
}

/**
 * The weights of the accounted kinds must relate the kinds to each other: a cached area carries a
 * list of points and a list of segment bounding boxes per ring, a cached way only a list of points
 * and a cached node a single coordinate, so one area entry has to weigh more than one way entry and
 * one way entry more than one node entry.
 */
TEST_CASE("Map data accounting relates the kinds to each other")
{
  using osmscout::MapDataAccounting;

  REQUIRE(MapDataAccounting::weightPerNode<MapDataAccounting::weightPerWay);
  REQUIRE(MapDataAccounting::weightPerWay<MapDataAccounting::weightPerArea);
  REQUIRE(MapDataAccounting::weightPerRoute<MapDataAccounting::weightPerArea);
  REQUIRE(MapDataAccounting::weightPerNode<MapDataAccounting::weightPerArea);
}

/**
 * Accounting a cache counts its content, and the weight of the content is the sum of the weights of
 * the entries of every kind.
 */
TEST_CASE("Map data accounting sums the accounted content")
{
  using osmscout::MapDataAccounting;

  MapDataAccounting::Cost empty;

  REQUIRE(empty.IsEmpty());
  REQUIRE(empty.GetEntryCount()==0);
  REQUIRE(MapDataAccounting::GetWeight(empty)==0);

  MapDataAccounting::Cost cost;

  cost.nodeCount=10;
  cost.wayCount=4;
  cost.areaCount=2;
  cost.routeCount=1;
  cost.indexEntryCount=7;

  REQUIRE(!cost.IsEmpty());
  REQUIRE(cost.GetEntryCount()==24);

  size_t expectedWeight=10*MapDataAccounting::weightPerNode+
                         4*MapDataAccounting::weightPerWay+
                         2*MapDataAccounting::weightPerArea+
                         1*MapDataAccounting::weightPerRoute+
                         7*MapDataAccounting::weightPerIndexEntry;

  REQUIRE(MapDataAccounting::GetWeight(cost)==expectedWeight);
  REQUIRE(expectedWeight==10+12+60+3+7);
}

/**
 * A weight stands for a number of bytes, and the conversion back rounds up: a content that is not
 * empty never accounts as no weight at all, otherwise a budget could not be satisfied by a single
 * small entry.
 */
TEST_CASE("Map data accounting converts between weight and bytes")
{
  using osmscout::MapDataAccounting;

  REQUIRE(MapDataAccounting::GetBytes(0)==0);
  REQUIRE(MapDataAccounting::GetBytes(1)==MapDataAccounting::bytesPerWeight);
  REQUIRE(MapDataAccounting::GetBytes(3)==3*MapDataAccounting::bytesPerWeight);

  REQUIRE(MapDataAccounting::GetWeightForBytes(0)==0);
  REQUIRE(MapDataAccounting::GetWeightForBytes(1)==1);
  REQUIRE(MapDataAccounting::GetWeightForBytes(MapDataAccounting::bytesPerWeight)==1);
  REQUIRE(MapDataAccounting::GetWeightForBytes(MapDataAccounting::bytesPerWeight+1)==2);

  REQUIRE(MapDataAccounting::GetWeightForBytes(MapDataAccounting::GetBytes(5))==5);
}

/**
 * A budget that was never set does not bound anything, a budget that was set can be read back and
 * can be dropped again without losing the reported usage.
 */
TEST_CASE("Map data budget is set, read and reset")
{
  using osmscout::MapDataAccounting;
  using osmscout::MapDataBudget;

  MapDataBudget budget;

  REQUIRE(!budget.IsBounded());
  REQUIRE(budget.GetTotalBudget()==0);
  REQUIRE(budget.GetTotalBudgetWeight()==0);
  REQUIRE(!budget.IsExceeded());

  size_t bytes=MapDataAccounting::bytesPerWeight*1024;

  REQUIRE(budget.SetTotalBudget(bytes));
  REQUIRE(budget.IsBounded());
  REQUIRE(budget.GetTotalBudget()==bytes);
  REQUIRE(budget.GetTotalBudgetWeight()==1024);

  budget.ResetBudget();

  REQUIRE(!budget.IsBounded());
  REQUIRE(budget.GetTotalBudget()==0);
}

/**
 * A budget that cannot hold even the floor of a single cache is not a bound and is rejected, and a
 * budget of zero bytes is rejected as well: a client that wants no bound resets the budget.
 */
TEST_CASE("Map data budget rejects a budget that cannot be satisfied")
{
  using osmscout::MapDataBudget;

  MapDataBudget budget;

  REQUIRE(!budget.SetTotalBudget(0));
  REQUIRE(!budget.IsBounded());

  REQUIRE(budget.SetTotalBudget(4096));
  REQUIRE(budget.SetFloor(1024));

  REQUIRE(!budget.SetTotalBudget(512));
  REQUIRE(budget.GetTotalBudget()==4096);

  REQUIRE(budget.SetTotalBudget(1024));
  REQUIRE(budget.GetTotalBudget()==1024);

  REQUIRE(!budget.SetFloor(2048));
  REQUIRE(budget.GetFloor()==1024);
}

/**
 * A cache reports its accounted content as a weight under the id it is registered with, and the
 * usage of the budget is the sum over all registered caches.
 */
TEST_CASE("Map data budget adds up the usage of its contributors")
{
  using osmscout::MapDataAccounting;
  using osmscout::MapDataBudget;

  MapDataBudget budget;

  REQUIRE(budget.AddContributor()!=budget.AddContributor());

  MapDataBudget::ContributorId first=budget.AddContributor();
  MapDataBudget::ContributorId second=budget.AddContributor();

  REQUIRE(budget.GetContributorCount()==4);

  budget.SetContributorWeight(first,10);
  budget.SetContributorWeight(second,7);

  REQUIRE(budget.GetContributorWeight(first)==10);
  REQUIRE(budget.GetContributorWeight(second)==7);
  REQUIRE(budget.GetUsageWeight()==17);
  REQUIRE(budget.GetUsage()==MapDataAccounting::GetBytes(17));

  budget.RemoveContributor(first);

  REQUIRE(budget.GetContributorWeight(first)==0);
  REQUIRE(budget.GetUsageWeight()==7);

  // An unregistered id is ignored instead of being added silently
  auto unknown=budget.AddContributor();

  budget.RemoveContributor(unknown);
  budget.SetContributorWeight(unknown,1000);

  REQUIRE(budget.GetUsageWeight()==7);
}

/**
 * The usage is compared to the budget in weight units, and a budget without a bound is never
 * exceeded.
 */
TEST_CASE("Map data budget reports an exceeded usage")
{
  using osmscout::MapDataAccounting;
  using osmscout::MapDataBudget;

  MapDataBudget budget;

  auto          contributor=budget.AddContributor();

  budget.SetContributorWeight(contributor,100);

  REQUIRE(!budget.IsExceeded());

  REQUIRE(budget.SetTotalBudget(MapDataAccounting::GetBytes(100)));
  REQUIRE(!budget.IsExceeded());

  budget.SetContributorWeight(contributor,101);
  REQUIRE(budget.IsExceeded());

  budget.ResetBudget();
  REQUIRE(!budget.IsExceeded());
}

/**
 * A tile cache reports the accounted weight of the content of its tiles, and the weight of a tile
 * follows the kinds of the objects it holds.
 */
TEST_CASE("Tile cache accounts the content of its tiles")
{
  using osmscout::DataTileCache;
  using osmscout::MapDataAccounting;

  DataTileCache cache(25);

  REQUIRE(cache.GetAccountedWeight()==0);
  REQUIRE(cache.GetWeightSize()==0);

  FillNodeTile(cache.GetTile(GetTileKey(1)),3);

  REQUIRE(cache.GetAccountedWeight()==3*MapDataAccounting::weightPerNode);

  FillAreaTile(cache.GetTile(GetTileKey(2)),2);

  REQUIRE(cache.GetAccountedWeight()==3*MapDataAccounting::weightPerNode+
          2*MapDataAccounting::weightPerArea);

  // A tile that was requested twice is accounted once
  FillNodeTile(cache.GetTile(GetTileKey(1)),3);

  REQUIRE(cache.GetAccountedWeight()==3*MapDataAccounting::weightPerNode+
          2*MapDataAccounting::weightPerArea);
}

/**
 * Once a weight size is configured it is the bound of the cache: the least recently used tiles are
 * dropped until the accounted weight of the remaining content fits, and the tile count no longer
 * bounds the cache.
 */
TEST_CASE("Tile cache drops the least recently used tiles to fit its weight size")
{
  using osmscout::DataTileCache;
  using osmscout::MapDataAccounting;

  DataTileCache cache(25);

  // The cache only drops tiles that nobody else holds, so the references of the caller have to be
  // released before the cache is asked to fit its weight size
  {
    auto tile=cache.GetTile(GetTileKey(1));

    FillNodeTile(tile,1);
  }

  {
    auto tile=cache.GetTile(GetTileKey(2));

    FillNodeTile(tile,1);
  }

  {
    auto tile=cache.GetTile(GetTileKey(3));

    FillAreaTile(tile,1);
  }

  REQUIRE(cache.GetAccountedWeight()==MapDataAccounting::weightPerNode+
          MapDataAccounting::weightPerNode+
          MapDataAccounting::weightPerArea);

  cache.SetWeightSize(MapDataAccounting::weightPerArea);

  REQUIRE(cache.GetAccountedWeight()==MapDataAccounting::weightPerArea);
  REQUIRE(cache.GetWeightSize()==MapDataAccounting::weightPerArea);
  REQUIRE(cache.GetCachedTile(GetTileKey(1))==nullptr);
  REQUIRE(cache.GetCachedTile(GetTileKey(2))==nullptr);
  REQUIRE(cache.GetCachedTile(GetTileKey(3))!=nullptr);
}

/**
 * The tile count is not the bound of a cache that is bounded by the accounted weight of its content:
 * returning to the tile count bound enforces it again.
 */
TEST_CASE("Tile cache bounds either its tile count or its weight size")
{
  using osmscout::DataTileCache;
  using osmscout::MapDataAccounting;

  DataTileCache cache(25);

  // Room for ten node entries, which is more tiles than the tile count bound of one below
  cache.SetWeightSize(10);

  for (size_t i=1; i<=3; i++) {
    auto tile=cache.GetTile(GetTileKey(i));

    FillNodeTile(tile,1);
  }

  cache.SetSize(1);

  REQUIRE(cache.GetAccountedWeight()==3*MapDataAccounting::weightPerNode);
  REQUIRE(cache.GetCachedTile(GetTileKey(1))!=nullptr);
  REQUIRE(cache.GetCachedTile(GetTileKey(2))!=nullptr);
  REQUIRE(cache.GetCachedTile(GetTileKey(3))!=nullptr);

  // A weight size of zero returns to the tile count bound, which is one tile
  cache.SetWeightSize(0);

  REQUIRE(cache.GetWeightSize()==0);
  REQUIRE(cache.GetAccountedWeight()==MapDataAccounting::weightPerNode);
  REQUIRE(cache.GetCachedTile(GetTileKey(1))==nullptr);
  REQUIRE(cache.GetCachedTile(GetTileKey(2))==nullptr);
  REQUIRE(cache.GetCachedTile(GetTileKey(3))!=nullptr);
}

/**
 * A tile that a caller still holds is not dropped from the cache, because dropping it would not free
 * its memory.
 */
TEST_CASE("Tile cache keeps tiles that are still in use")
{
  using osmscout::DataTileCache;
  using osmscout::MapDataAccounting;

  DataTileCache cache(25);

  // 'heldNode' stays alive for the rest of the test, so the cache must not drop it
  auto heldNode=cache.GetTile(GetTileKey(1));

  FillNodeTile(heldNode,1);

  {
    auto tile=cache.GetTile(GetTileKey(2));

    FillAreaTile(tile,1);
  }

  cache.SetWeightSize(MapDataAccounting::weightPerArea);

  // The tile the caller holds stays, the cache drops the tile it can drop
  REQUIRE(cache.GetAccountedWeight()==MapDataAccounting::weightPerNode);
  REQUIRE(cache.GetCachedTile(GetTileKey(1))!=nullptr);
  REQUIRE(cache.GetCachedTile(GetTileKey(2))==nullptr);
}

/**
 * The object cache of a data file can be resized at runtime: the oldest entries are stripped, and the
 * file still serves reads afterwards.
 */
TEST_CASE("Data file resizes its object cache")
{
  osmscout::DatabaseParameter parameter;

  parameter.SetNodeDataCacheSize(5);

  auto database=std::make_shared<osmscout::Database>(parameter);

  REQUIRE(database->Open(GetTestDatabaseDirectory()));

  auto nodeDataFile=database->GetNodeDataFile();

  REQUIRE(nodeDataFile!=nullptr);
  REQUIRE(nodeDataFile->GetCacheSize()==5);
  REQUIRE(nodeDataFile->GetCachedEntryCount()==0);

  std::vector<osmscout::FileOffset> offsets=GetNodeOffsets(database);

  REQUIRE(offsets.size()>=3);

  for (size_t i=0; i<3; i++) {
    osmscout::NodeRef node;

    REQUIRE(nodeDataFile->GetByOffset(offsets[i],node));
    REQUIRE(node!=nullptr);
  }

  REQUIRE(nodeDataFile->GetCachedEntryCount()==3);

  nodeDataFile->SetCacheSize(1);

  REQUIRE(nodeDataFile->GetCacheSize()==1);
  REQUIRE(nodeDataFile->GetCachedEntryCount()==1);

  // The resized cache still serves reads
  {
    osmscout::NodeRef node;

    REQUIRE(nodeDataFile->GetByOffset(offsets[0],node));
    REQUIRE(node!=nullptr);
  }

  database->Close();
}

/**
 * The database resizes the caches of its data files: a file that is created after the change uses the
 * new size, and a file that already exists is resized immediately.
 */
TEST_CASE("Database resizes the object caches of its data files")
{
  osmscout::DatabaseParameter parameter;

  parameter.SetNodeDataCacheSize(5);
  parameter.SetWayDataCacheSize(6);
  parameter.SetAreaDataCacheSize(7);
  parameter.SetRouteDataCacheSize(8);
  parameter.SetAreaAreaIndexCacheSize(9);

  auto database=std::make_shared<osmscout::Database>(parameter);

  REQUIRE(database->Open(GetTestDatabaseDirectory()));

  size_t nodeCacheSize=0;
  size_t wayCacheSize=0;
  size_t areaCacheSize=0;
  size_t routeCacheSize=0;
  size_t areaAreaIndexCacheSize=0;

  database->GetDataCacheSizes(nodeCacheSize,
                              wayCacheSize,
                              areaCacheSize,
                              routeCacheSize,
                              areaAreaIndexCacheSize);

  REQUIRE(nodeCacheSize==5);
  REQUIRE(wayCacheSize==6);
  REQUIRE(areaCacheSize==7);
  REQUIRE(routeCacheSize==8);
  REQUIRE(areaAreaIndexCacheSize==9);

  // No data file exists yet, so the sizes only have to be remembered
  database->SetDataCacheSizes(11,12,13,14,15);

  database->GetDataCacheSizes(nodeCacheSize,
                              wayCacheSize,
                              areaCacheSize,
                              routeCacheSize,
                              areaAreaIndexCacheSize);

  REQUIRE(nodeCacheSize==11);
  REQUIRE(wayCacheSize==12);
  REQUIRE(areaCacheSize==13);
  REQUIRE(routeCacheSize==14);
  REQUIRE(areaAreaIndexCacheSize==15);

  // A data file created now uses the remembered size
  auto nodeDataFile=database->GetNodeDataFile();

  REQUIRE(nodeDataFile!=nullptr);
  REQUIRE(nodeDataFile->GetCacheSize()==11);

  // A data file that exists is resized by the database
  database->SetDataCacheSizes(3,12,13,14,15);

  REQUIRE(nodeDataFile->GetCacheSize()==3);

  database->GetDataCacheSizes(nodeCacheSize,
                              wayCacheSize,
                              areaCacheSize,
                              routeCacheSize,
                              areaAreaIndexCacheSize);

  REQUIRE(nodeCacheSize==3);

  database->Close();
}

/**
 * The relevance of a database to a view follows the geographic extent of the database: a view over the
 * testregion database covers it and not a database whose extent lies elsewhere.
 */
TEST_CASE("Map service relates a database to the view")
{
  const std::string databaseDirectory=GetTestDatabaseDirectory();

  osmscout::GeoBox  databaseBox;

  {
    auto database=std::make_shared<osmscout::Database>(osmscout::DatabaseParameter());

    REQUIRE(database->Open(databaseDirectory));
    REQUIRE(database->GetBoundingBox(databaseBox));
    database->Close();
  }

  // Far away, in an area the testregion database does not cover
  osmscout::GeoBox farBox(osmscout::GeoCoord(-60.0,100.0),
                          osmscout::GeoCoord(-50.0,110.0));

  REQUIRE(!farBox.Intersects(databaseBox));

  auto nearDatabase=std::make_shared<osmscout::Database>(osmscout::DatabaseParameter());
  auto farDatabase=std::make_shared<osmscout::Database>(osmscout::DatabaseParameter());

  REQUIRE(nearDatabase->Open(databaseDirectory));
  REQUIRE(farDatabase->Open(CreateDatabaseWithBoundingBox(databaseDirectory,
                                                          "far-database",
                                                          farBox)));

  osmscout::MapDataBudgetRef budget=std::make_shared<osmscout::MapDataBudget>();

  auto                       nearService=std::make_shared<osmscout::MapService>(nearDatabase,budget);
  auto                       farService=std::make_shared<osmscout::MapService>(farDatabase,budget);

  // A view over the testregion database covers exactly one of the two databases
  REQUIRE(nearService->IsRelevantToView(databaseBox));
  REQUIRE(!farService->IsRelevantToView(databaseBox));

  // A view over the other database covers exactly the other one
  REQUIRE(!nearService->IsRelevantToView(farBox));
  REQUIRE(farService->IsRelevantToView(farBox));

  // Both services report the same view, and exactly the covering database becomes relevant
  osmscout::Magnification      magnification=testMagnification;
  std::list<osmscout::TileRef> tiles;

  nearService->LookupTiles(magnification,databaseBox,tiles);
  farService->LookupTiles(magnification,databaseBox,tiles);

  REQUIRE(budget->GetRelevantContributorCount()==1);

  nearDatabase->Close();
  farDatabase->Close();
}

/**
 * The relevant databases share the budget, the others are reduced to the floor, and the shares of all
 * databases together stay within the budget.
 */
TEST_CASE("Map data budget distributes the budget over the relevant databases")
{
  using osmscout::MapDataAccounting;

  const std::string databaseDirectory=GetTestDatabaseDirectory();

  osmscout::GeoBox  databaseBox;

  auto              nearDatabase=std::make_shared<osmscout::Database>(osmscout::DatabaseParameter());

  REQUIRE(nearDatabase->Open(databaseDirectory));
  REQUIRE(nearDatabase->GetBoundingBox(databaseBox));

  osmscout::GeoBox farBox(osmscout::GeoCoord(-60.0,100.0),
                          osmscout::GeoCoord(-50.0,110.0));

  auto farDatabase=std::make_shared<osmscout::Database>(osmscout::DatabaseParameter());

  REQUIRE(farDatabase->Open(CreateDatabaseWithBoundingBox(databaseDirectory,
                                                          "far-database",
                                                          farBox)));

  osmscout::MapDataBudgetRef budget=std::make_shared<osmscout::MapDataBudget>();

  REQUIRE(budget->SetTotalBudget(16*1024*1024));
  REQUIRE(budget->SetFloor(1*1024*1024));
  budget->SetSettlingTime(std::chrono::milliseconds(0));

  auto                         nearService=std::make_shared<osmscout::MapService>(nearDatabase,budget);
  auto                         farService=std::make_shared<osmscout::MapService>(farDatabase,budget);

  osmscout::Magnification      magnification=testMagnification;
  std::list<osmscout::TileRef> tiles;

  nearService->LookupTiles(magnification,databaseBox,tiles);
  farService->LookupTiles(magnification,databaseBox,tiles);

  REQUIRE(budget->GetDistributedWeight()<=budget->GetTotalBudgetWeight());

  REQUIRE(nearService->GetShareWeight()>farService->GetShareWeight());
  REQUIRE(farService->GetShareWeight()==budget->GetFloorWeight());

  // A share is larger than the floor, but never larger than the whole budget
  REQUIRE(nearService->GetShareWeight()<budget->GetTotalBudgetWeight());

  // The databases report their usage, and the usage of all caches stays within the budget
  REQUIRE(budget->GetUsageWeight()<=budget->GetTotalBudgetWeight());

  // A view that covers only the far database makes it the relevant one and drops the other to its
  // floor
  farService->LookupTiles(magnification,farBox,tiles);

  REQUIRE(budget->GetRelevantContributorCount()==1);
  REQUIRE(nearService->GetShareWeight()==budget->GetFloorWeight());
  REQUIRE(farService->GetShareWeight()>budget->GetFloorWeight());
  REQUIRE(budget->GetDistributedWeight()<=budget->GetTotalBudgetWeight());

  // Without a budget nothing is distributed and the caches keep their own sizes
  osmscout::MapDataBudgetRef noneBudget=std::make_shared<osmscout::MapDataBudget>();
  auto                       plainService=std::make_shared<osmscout::MapService>(nearDatabase,noneBudget);

  plainService->LookupTiles(magnification,databaseBox,tiles);

  REQUIRE(!noneBudget->IsBounded());
  REQUIRE(plainService->GetShareWeight()==0);

  nearDatabase->Close();
  farDatabase->Close();
}

/**
 * A distribution is only applied once the set of relevant databases has been stable for the settling
 * period, and a database that becomes relevant again inside that period keeps its share.
 */
TEST_CASE("Map data budget distributes the budget only for a stable relevant set")
{
  using osmscout::MapDataBudget;

  MapDataBudget budget;

  REQUIRE(budget.SetTotalBudget(16*1024*1024));
  REQUIRE(budget.SetFloor(1*1024*1024));
  budget.SetSettlingTime(std::chrono::seconds(5));

  // Two databases whose extents do not overlap
  osmscout::GeoBox firstBox(osmscout::GeoCoord(50.0,7.0),
                            osmscout::GeoCoord(51.0,8.0));
  osmscout::GeoBox secondBox(osmscout::GeoCoord(50.0,9.0),
                             osmscout::GeoCoord(51.0,10.0));

  auto first=budget.AddContributor();
  auto second=budget.AddContributor();

  budget.SetContributorExtent(first,firstBox);
  budget.SetContributorExtent(second,secondBox);

  auto start=std::chrono::steady_clock::now();

  budget.ReportView(firstBox,start);

  // A change of the relevant set restarts the settling period
  REQUIRE(!budget.DistributeIfStable(start+std::chrono::milliseconds(4999)));
  REQUIRE(budget.DistributeIfStable(start+std::chrono::milliseconds(5000)));

  size_t firstShare=budget.GetShareWeight(first);
  size_t secondShare=budget.GetShareWeight(second);

  REQUIRE(firstShare>secondShare);
  REQUIRE(secondShare==budget.GetFloorWeight());

  // The first database leaves the view and returns within the settling period
  budget.ReportView(secondBox,start+std::chrono::milliseconds(6000));

  REQUIRE(!budget.DistributeIfStable(start+std::chrono::milliseconds(7000)));
  REQUIRE(budget.GetShareWeight(first)==firstShare);

  budget.ReportView(firstBox,start+std::chrono::milliseconds(8000));

  REQUIRE(!budget.DistributeIfStable(start+std::chrono::milliseconds(10000)));
  REQUIRE(budget.GetShareWeight(first)==firstShare);
  REQUIRE(budget.GetShareWeight(second)==secondShare);

  // It stays out of the view this time, so the distribution is applied
  budget.ReportView(secondBox,start+std::chrono::milliseconds(20000));

  REQUIRE(budget.DistributeIfStable(start+std::chrono::milliseconds(25000)));
  REQUIRE(budget.GetShareWeight(first)==budget.GetFloorWeight());
  REQUIRE(budget.GetShareWeight(second)>budget.GetFloorWeight());

  // The other database becomes the view
  budget.ReportView(firstBox,start+std::chrono::milliseconds(25000));

  REQUIRE(!budget.DistributeIfStable(start+std::chrono::milliseconds(29000)));
  REQUIRE(budget.DistributeIfStable(start+std::chrono::milliseconds(30000)));
  REQUIRE(budget.GetShareWeight(first)>budget.GetFloorWeight());
  REQUIRE(budget.GetShareWeight(second)==budget.GetFloorWeight());

  // The caches never may hold more than the budget together
  REQUIRE(budget.GetDistributedWeight()<=budget.GetTotalBudgetWeight());
}

/**
 * Bounding the budget changes how much is kept, never which objects a view contains.
 */
TEST_CASE("Bounding the budget does not change the objects of a view")
{
  const std::string databaseDirectory=GetTestDatabaseDirectory();

  auto              database=std::make_shared<osmscout::Database>(osmscout::DatabaseParameter());

  REQUIRE(database->Open(databaseDirectory));

  osmscout::StyleConfigRef styleConfig=std::make_shared<osmscout::StyleConfig>(database->GetTypeConfig());

  REQUIRE(styleConfig->Load(GetTestStyleSheet()));

  osmscout::GeoBox databaseBox;

  REQUIRE(database->GetBoundingBox(databaseBox));

  osmscout::AreaSearchParameter searchParameter;
  osmscout::Magnification       magnification=testMagnification;

  // A budget that is far below the data of the view
  osmscout::MapDataBudgetRef tinyBudget=std::make_shared<osmscout::MapDataBudget>();

  REQUIRE(tinyBudget->SetTotalBudget(64*1024));
  tinyBudget->SetSettlingTime(std::chrono::milliseconds(0));

  auto                         tinyService=std::make_shared<osmscout::MapService>(database,tinyBudget);

  std::list<osmscout::TileRef> tinyTiles;

  tinyService->LookupTiles(magnification,databaseBox,tinyTiles);

  REQUIRE(tinyService->LoadMissingTileData(searchParameter,*styleConfig,tinyTiles));

  osmscout::MapData tinyData;

  tinyService->AddTileDataToMapData(tinyTiles,tinyData);

  // A budget that is far above the data of the view
  osmscout::MapDataBudgetRef hugeBudget=std::make_shared<osmscout::MapDataBudget>();

  REQUIRE(hugeBudget->SetTotalBudget(512*1024*1024));
  hugeBudget->SetSettlingTime(std::chrono::milliseconds(0));

  auto                         hugeService=std::make_shared<osmscout::MapService>(database,hugeBudget);

  std::list<osmscout::TileRef> hugeTiles;

  hugeService->LookupTiles(magnification,databaseBox,hugeTiles);

  REQUIRE(hugeService->LoadMissingTileData(searchParameter,*styleConfig,hugeTiles));

  osmscout::MapData hugeData;

  hugeService->AddTileDataToMapData(hugeTiles,hugeData);

  // The same objects of every kind, whatever the budget allows the caches to keep
  REQUIRE(tinyData.nodes.size()==hugeData.nodes.size());
  REQUIRE(tinyData.ways.size()==hugeData.ways.size());
  REQUIRE(tinyData.areas.size()==hugeData.areas.size());
  REQUIRE(tinyData.routes.size()==hugeData.routes.size());
  REQUIRE(GetObjectOffsets(tinyData)==GetObjectOffsets(hugeData));

  // The bounded service keeps less than the unbounded one and stays within its budget
  REQUIRE(tinyBudget->GetUsageWeight()<=tinyBudget->GetTotalBudgetWeight());
  REQUIRE(tinyService->GetAccountedWeight()<=hugeService->GetAccountedWeight());

  database->Close();
}

/**
 * A database that stays out of view for longer than the idle time gives its content back, and it
 * loads its data again once it is part of the view again.
 */
TEST_CASE("A database that stays out of view releases its caches")
{
  const std::string databaseDirectory=GetTestDatabaseDirectory();

  auto              database=std::make_shared<osmscout::Database>(osmscout::DatabaseParameter());

  REQUIRE(database->Open(databaseDirectory));

  osmscout::StyleConfigRef styleConfig=std::make_shared<osmscout::StyleConfig>(database->GetTypeConfig());

  REQUIRE(styleConfig->Load(GetTestStyleSheet()));

  osmscout::GeoBox databaseBox;

  REQUIRE(database->GetBoundingBox(databaseBox));

  osmscout::GeoBox otherBox(osmscout::GeoCoord(-60.0,100.0),
                            osmscout::GeoCoord(-50.0,110.0));

  REQUIRE(!otherBox.Intersects(databaseBox));

  osmscout::MapDataBudgetRef budget=std::make_shared<osmscout::MapDataBudget>();

  REQUIRE(budget->SetTotalBudget(32*1024*1024));
  REQUIRE(budget->SetFloor(2*1024*1024));
  budget->SetSettlingTime(std::chrono::milliseconds(0));
  budget->SetIdleTime(std::chrono::seconds(0));

  auto                          service=std::make_shared<osmscout::MapService>(database,budget);

  osmscout::AreaSearchParameter searchParameter;
  osmscout::Magnification       magnification=testMagnification;

  // Load the data of the database while the view covers it
  std::list<osmscout::TileRef> tiles;

  service->LookupTiles(magnification,databaseBox,tiles);

  REQUIRE(service->LoadMissingTileData(searchParameter,*styleConfig,tiles));

  size_t loadedWeight=service->GetAccountedWeight();

  REQUIRE(loadedWeight>0);
  REQUIRE(budget->GetUsageWeight()==loadedWeight);

  // The user looks elsewhere, and the idle time of the cache has passed
  service->LookupTiles(magnification,otherBox,tiles);

  REQUIRE(service->GetAccountedWeight()==0);
  REQUIRE(budget->GetUsageWeight()==0);

  // The database is part of the view again: it loads its data again
  service->LookupTiles(magnification,databaseBox,tiles);

  REQUIRE(service->LoadMissingTileData(searchParameter,*styleConfig,tiles));

  osmscout::MapData data;

  service->AddTileDataToMapData(tiles,data);

  REQUIRE(service->GetAccountedWeight()>0);
  REQUIRE(budget->GetUsageWeight()>0);

  database->Close();
}

/**
 * The caches of two databases share one budget: the budget holds the usage of both of them, and it
 * reports a service that is gone no longer.
 */
TEST_CASE("Map services share the usage of one budget")
{
  const std::string databaseDirectory=GetTestDatabaseDirectory();

  auto              firstDatabase=std::make_shared<osmscout::Database>(osmscout::DatabaseParameter());
  auto              secondDatabase=std::make_shared<osmscout::Database>(osmscout::DatabaseParameter());

  REQUIRE(firstDatabase->Open(databaseDirectory));
  REQUIRE(secondDatabase->Open(CreateDatabaseCopy(databaseDirectory,
                                                  "second-database")));

  osmscout::StyleConfigRef styleConfig=std::make_shared<osmscout::StyleConfig>(firstDatabase->GetTypeConfig());

  REQUIRE(styleConfig->Load(GetTestStyleSheet()));

  osmscout::GeoBox databaseBox;

  REQUIRE(firstDatabase->GetBoundingBox(databaseBox));

  osmscout::MapDataBudgetRef budget=std::make_shared<osmscout::MapDataBudget>();

  REQUIRE(budget->SetTotalBudget(64*1024*1024));
  REQUIRE(budget->SetFloor(2*1024*1024));
  budget->SetSettlingTime(std::chrono::milliseconds(0));

  auto firstService=std::make_shared<osmscout::MapService>(firstDatabase,budget);
  auto secondService=std::make_shared<osmscout::MapService>(secondDatabase,budget);

  REQUIRE(budget->GetContributorCount()==2);

  osmscout::AreaSearchParameter searchParameter;
  osmscout::Magnification       magnification=testMagnification;

  std::list<osmscout::TileRef>  firstTiles;
  std::list<osmscout::TileRef>  secondTiles;

  firstService->LookupTiles(magnification,databaseBox,firstTiles);
  secondService->LookupTiles(magnification,databaseBox,secondTiles);

  REQUIRE(firstService->LoadMissingTileData(searchParameter,*styleConfig,firstTiles));
  REQUIRE(secondService->LoadMissingTileData(searchParameter,*styleConfig,secondTiles));

  // The budget holds the usage of both services, and it stays within the budget
  REQUIRE(firstService->GetAccountedWeight()>0);
  REQUIRE(secondService->GetAccountedWeight()>0);
  REQUIRE(budget->GetUsageWeight()==firstService->GetAccountedWeight()+secondService->GetAccountedWeight());
  REQUIRE(budget->GetUsageWeight()<=budget->GetTotalBudgetWeight());

  // A service that is gone no longer takes part in the budget
  secondService.reset();

  REQUIRE(budget->GetContributorCount()==1);
  REQUIRE(budget->GetUsageWeight()==firstService->GetAccountedWeight());

  firstDatabase->Close();
  secondDatabase->Close();
}
