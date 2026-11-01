#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <locale>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_session.hpp>
#include <catch2/catch_test_macros.hpp>

#include <osmscoutimport/Import.h>
#include <osmscoutimport/ImportProgress.h>

#include <osmscout-test/PreprocessOLT.h>

#include <osmscout/Area.h>
#include <osmscout/Node.h>
#include <osmscout/Way.h>

#include <osmscout/db/Database.h>
#include <osmscout/db/LocationIndex.h>
#include <osmscout/location/Location.h>

#include <osmscoutclient/DBInstance.h>

#include "search_scope.h"
#include "search_scope_extent.h"

/**
 * The derivation behind the cross-database scope filter, over a real region
 * object (issue #20): the box a scoped search measures free-text hits against
 * comes from the object that represents the scope region, in the database that
 * resolved it. SearchScopeTest.cpp covers the box and position decision, but it
 * takes the box as an input; nothing reached the derivation itself — loading the
 * region's object and turning it into a box — because that lived inside the JNI
 * translation unit, which the host build cannot compile. These cases reach it.
 *
 * Test data: Tests/LocationTest.olt, imported into TESTS_TMP_DIR at start-up, the
 * way LocationServiceTest.cpp imports it. The committed database under
 * Tests/data/testregion cannot answer here: its location index is empty (a
 * 77-byte header, measured 2026-10-29), so it holds no admin region to derive an
 * extent from. The import is full, not eco: an eco import is allowed to skip the
 * indexes a case asserts against (Build.md §4f).
 */
osmscout::DatabaseRef   database;
osmscout::DBInstanceRef dbInstance;

namespace {

/**
 * Collects the admin regions the index holds, in visit order, so a case can pick
 * one by the kind of object that represents it.
 */
class RegionCollectingVisitor : public osmscout::AdminRegionVisitor
{
public:
  std::vector<osmscout::AdminRegion> regions;

public:
  Action Visit(const osmscout::AdminRegion& region) override
  {
    regions.push_back(region);

    return visitChildren;
  }
};

std::vector<osmscout::AdminRegion> Regions()
{
  REQUIRE(database);

  osmscout::LocationIndexRef index=database->GetLocationIndex();

  REQUIRE(index);

  RegionCollectingVisitor visitor;

  REQUIRE(index->VisitAdminRegions(visitor));

  return visitor.regions;
}

/**
 * The extent of the object that represents the region, read here rather than
 * derived by the code under test, so a case compares two independent reads.
 */
std::optional<osmscout::GeoBox> ObjectBox(const osmscout::AdminRegion& region)
{
  if (region.object.GetType()==osmscout::RefType::refArea) {
    osmscout::AreaRef area;

    if (!database->GetAreaByOffset(region.object.GetFileOffset(),area)) {
      return std::nullopt;
    }

    return area->GetBoundingBox();
  }

  if (region.object.GetType()==osmscout::RefType::refWay) {
    osmscout::WayRef way;

    if (!database->GetWayByOffset(region.object.GetFileOffset(),way)) {
      return std::nullopt;
    }

    return way->GetBoundingBox();
  }

  if (region.object.GetType()==osmscout::RefType::refNode) {
    osmscout::NodeRef node;

    if (!database->GetNodeByOffset(region.object.GetFileOffset(),node)) {
      return std::nullopt;
    }

    osmscout::GeoBox box;

    box.Include(node->GetCoords());

    return box;
  }

  return std::nullopt;
}

/**
 * Every region represented by an outline — an area or a way — whose object this
 * database can load. That is the input the derivation has to handle in the
 * common case, and the one a fixture can assert a box for. The case walks all of
 * them on purpose: the first one the index lists is the outermost region, whose
 * box is the database's own box, so a derivation that loaded the wrong object
 * would still agree with it.
 */
std::vector<osmscout::AdminRegion> OutlineRegions()
{
  std::vector<osmscout::AdminRegion> outlineRegions;

  for (const auto& region : Regions()) {
    if (region.object.GetType()!=osmscout::RefType::refArea &&
        region.object.GetType()!=osmscout::RefType::refWay) {
      continue;
    }

    if (!ObjectBox(region).has_value()) {
      continue;
    }

    outlineRegions.push_back(region);
  }

  return outlineRegions;
}

/**
 * Of the outline regions, the one whose box spans the least — the region whose
 * extent is most specific, so a point a degree outside it is certainly outside.
 */
osmscout::AdminRegion SmallestOutlineRegion()
{
  std::optional<osmscout::AdminRegion> smallest;
  double                              smallestSpan=-1.0;

  for (const auto& region : OutlineRegions()) {
    const auto box=ObjectBox(region);

    if (!box.has_value()) {
      continue;
    }

    const double span=(box->GetMaxCoord().GetLat()-box->GetMinCoord().GetLat())+
                      (box->GetMaxCoord().GetLon()-box->GetMinCoord().GetLon());

    if (!smallest.has_value() || span<smallestSpan) {
      smallest=region;
      smallestSpan=span;
    }
  }

  REQUIRE(smallest.has_value());

  return *smallest;
}

}

/*
 * The one derivation a scoped search rests on: the extent of an area or way
 * region is the box the object spans, so the region's own outline decides which
 * hits survive. A regression that loaded the wrong object, or lost a corner,
 * changes these four numbers for at least one region of the tree.
 */
TEST_CASE("A scope extent is the bounding box of the region's own object")
{
  const auto regions=OutlineRegions();

  REQUIRE(regions.size()>1); // a region tree, not a single region

  for (const auto& region : regions) {
    INFO("region: " << region.name);

    const naviveylin::GeoBox extent=
      naviveylin::DeriveScopeExtent(dbInstance,
                                    std::make_shared<osmscout::AdminRegion>(region));

    REQUIRE(extent.isSet);

    const std::optional<osmscout::GeoBox> objectBox=ObjectBox(region);

    REQUIRE(objectBox.has_value());

    REQUIRE(extent.minLat==Catch::Approx(objectBox->GetMinCoord().GetLat()));
    REQUIRE(extent.minLon==Catch::Approx(objectBox->GetMinCoord().GetLon()));
    REQUIRE(extent.maxLat==Catch::Approx(objectBox->GetMaxCoord().GetLat()));
    REQUIRE(extent.maxLon==Catch::Approx(objectBox->GetMaxCoord().GetLon()));

    // The region's own area is inside the extent it produced: the filter drops
    // only what the region cannot contain.
    REQUIRE(naviveylin::IsInsideGeoBox(extent,
                                       objectBox->GetCenter().GetLat(),
                                       objectBox->GetCenter().GetLon()));
  }
}

/*
 * The filter itself, applied the way the search applies it: a hit inside the
 * derived extent is kept, one outside it in either direction is dropped. This is
 * the pair the item was filed for — a hit from a second database that lies
 * outside the scope's box must not reach the caller's list.
 */
TEST_CASE("A hit inside the derived extent is kept and one outside is dropped")
{
  const osmscout::AdminRegion region=SmallestOutlineRegion();

  const naviveylin::GeoBox extent=
    naviveylin::DeriveScopeExtent(dbInstance,
                                  std::make_shared<osmscout::AdminRegion>(region));

  REQUIRE(extent.isSet);

  const double insideLat=(extent.minLat+extent.maxLat)/2.0;
  const double insideLon=(extent.minLon+extent.maxLon)/2.0;

  REQUIRE(naviveylin::IsInsideGeoBox(extent,insideLat,insideLon));

  // One degree beyond either edge: no installed data spans that from the region.
  REQUIRE_FALSE(naviveylin::IsInsideGeoBox(extent,extent.minLat-1.0,insideLon));
  REQUIRE_FALSE(naviveylin::IsInsideGeoBox(extent,extent.maxLat+1.0,insideLon));
  REQUIRE_FALSE(naviveylin::IsInsideGeoBox(extent,insideLat,extent.minLon-1.0));
  REQUIRE_FALSE(naviveylin::IsInsideGeoBox(extent,insideLat,extent.maxLon+1.0));
}


/*
 * The fail-open rule: an extent that cannot be established must degrade to the
 * unscoped behaviour and never empty the search. Both outcomes are a fact about
 * the derivation, not about the filter, so they are asserted here.
 *
 * The node-represented region — the derivation's third branch, whose extent is the
 * fallback box around a point — is not reachable from this fixture: the import
 * writes no node objects (a 93-byte `nodes.dat`, measured 2026-10-29), so there is
 * no node to represent a region with. The fallback box itself is covered as the
 * pure function it is (`BoxAroundPoint`, Tests/src/SearchScopeTest.cpp).
 */
TEST_CASE("A region without a loadable object leaves the extent unset")
{
  // No region at all: the caller passes an empty handle when the scope lookup
  // found nothing.
  REQUIRE_FALSE(naviveylin::DeriveScopeExtent(dbInstance,
                                              osmscout::AdminRegionRef()).isSet);

  // A region whose object offset does not resolve in this database: the extent
  // stays unset instead of becoming an empty box that drops every hit.
  osmscout::AdminRegionRef unresolvable=std::make_shared<osmscout::AdminRegion>();

  unresolvable->name="unresolvable";
  unresolvable->object=osmscout::ObjectFileRef(0,osmscout::RefType::refArea);

  const naviveylin::GeoBox extent=naviveylin::DeriveScopeExtent(dbInstance,unresolvable);

  REQUIRE_FALSE(extent.isSet);

  // Unset admits everything, including a position nowhere near the fixture.
  REQUIRE(naviveylin::IsInsideGeoBox(extent,0.0,0.0));
}

namespace {

class PreprocessorFactory : public osmscout::PreprocessorFactory
{
public:
  std::unique_ptr<osmscout::Preprocessor> GetProcessor(const std::string& /*filename*/,
                                                       osmscout::PreprocessorCallback& callback) const override
  {
    return std::unique_ptr<osmscout::Preprocessor>(new osmscout::test::PreprocessOLT(callback));
  }
};

}

int main(int argc,char* argv[])
{
  osmscout::ImportParameter importParameter;
  osmscout::ImportProgress  progress;
  std::list<std::string>    mapfiles;

  try {
    std::locale::global(std::locale(""));
  }
  catch (const std::runtime_error& e) {
    progress.Error("Cannot set locale: \""+std::string(e.what())+"\"");
    return 1;
  }

  char* testsTopDirEnv=getenv("TESTS_TOP_DIR");

  if (testsTopDirEnv==nullptr) {
    std::cerr << "Expected environment variable 'TESTS_TOP_DIR' not set" << std::endl;
    return 1;
  }

  std::string testsTopDir=testsTopDirEnv;

  if (testsTopDir.empty() || !osmscout::IsDirectory(testsTopDir)) {
    std::cerr << "Environment variable 'TESTS_TOP_DIR' is empty or does not point to a directory" << std::endl;
    return 77;
  }

  char* testsTmpDirEnv=getenv("TESTS_TMP_DIR");

  std::string testsTmpDir;

  if (testsTmpDirEnv==nullptr) {
    std::cerr << "Environment variable 'TESTS_TMP_DIR' not set, using current dir" << std::endl;
    testsTmpDir=".";
  }
  else {
    testsTmpDir=testsTmpDirEnv;
  }

  if (testsTmpDir.empty() || !osmscout::IsDirectory(testsTmpDir)) {
    std::cerr << "Environment variable 'TESTS_TMP_DIR' is empty or does not point to a directory" << std::endl;
    return 77;
  }

  // An own subdirectory, so this import cannot collide with the one
  // LocationServiceTest makes in the same temporary directory when the suite runs
  // its cases in parallel, and removed first so a stale database cannot answer.
  std::filesystem::path databaseDir=std::filesystem::path(testsTmpDir)/"search-scope-extent";

  std::error_code errorCode;

  std::filesystem::remove_all(databaseDir,errorCode);
  std::filesystem::create_directories(databaseDir,errorCode);

  mapfiles.emplace_back(osmscout::AppendFileToDir(testsTopDir,"LocationTest.olt"));

  importParameter.SetTypefile(osmscout::AppendFileToDir(testsTopDir,"../stylesheets/map.ost"));
  importParameter.SetMapfiles(mapfiles);
  importParameter.SetDestinationDirectory(databaseDir.string());
  importParameter.SetPreprocessorFactory(std::make_shared<PreprocessorFactory>());

  try {
    osmscout::Importer importer(importParameter);

    if (!importer.Import(progress)) {
      progress.Error("Import failed!");
      return 1;
    }
  }
  catch (const osmscout::IOException& e) {
    progress.Error("Import failed: "+e.GetDescription());
    return 1;
  }

  // The coordinate spill file is a temporary of the import (the repository's own
  // generation script deletes it as well); dropping it keeps the temporary
  // directory at the size of the database instead of several hundred megabytes.
  std::filesystem::remove(databaseDir/"coord.dat",errorCode);

  osmscout::DatabaseParameter dbParameter;

  database=std::make_shared<osmscout::Database>(dbParameter);

  if (!database->Open(databaseDir.string())) {
    std::cerr << "Cannot open db" << std::endl;
    return 77;
  }

  // Only the database matters to the derivation. The services a DBInstance can
  // additionally hold belong to a client session; a derivation must not need
  // them, and the test states that by leaving them empty.
  dbInstance=std::make_shared<osmscout::DBInstance>(databaseDir.string(),
                                                    database,
                                                    nullptr,
                                                    nullptr,
                                                    nullptr,
                                                    nullptr);

  int result=Catch::Session().run(argc,argv);

  // The instance closes the database it was handed, so it goes first.
  dbInstance.reset();
  database.reset();

  return result;
}
