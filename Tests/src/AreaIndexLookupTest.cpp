/*
  AreaIndexLookupTest - a test program for libosmscout
  Copyright (C) 2026  Tim Teulings

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

#include <osmscout/db/AreaIndex.h>
#include <osmscout/db/Database.h>

#include <osmscout/GeoCoord.h>
#include <osmscout/OSMScoutTypes.h>
#include <osmscout/TypeConfig.h>
#include <osmscout/TypeInfoSet.h>

#include <osmscout/util/Distance.h>
#include <osmscout/util/GeoBox.h>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <utility>
#include <vector>

/**
 * What a lookup on an area index returns, and what it is allowed to cost: the offsets and the
 * loaded types follow the request, and the work follows the requested types rather than the
 * entries the index carries.
 *
 * Test data: the database directory committed under `Tests/data/testregion`, from
 * TESTS_TOP_DIR/data/testregion.
 */
namespace {

std::filesystem::path TestsTopDir()
{
  const char *topDir=std::getenv("TESTS_TOP_DIR");

  REQUIRE(topDir!=nullptr);

  return topDir;
}

std::filesystem::path TestDataDir()
{
  return TestsTopDir()/"data"/"testregion";
}

struct TestDatabase
{
  osmscout::DatabaseRef   database;
  osmscout::TypeConfigRef typeConfig;
  osmscout::GeoBox        boundingBox;

  explicit TestDatabase(const std::filesystem::path& dataDir)
  {
    osmscout::DatabaseParameter parameter;

    database=std::make_shared<osmscout::Database>(parameter);
    REQUIRE(database->Open(dataDir.string()));

    typeConfig=database->GetTypeConfig();
    REQUIRE(typeConfig!=nullptr);
    REQUIRE(typeConfig->GetTypeCount()>0);

    REQUIRE(database->GetBoundingBox(boundingBox));
  }
};

/**
 * The offset order is not part of the contract: the lookup collects the offsets of an object
 * referenced by several types in a deduplicating container, so the order of the returned
 * vector is that container's, not the request's or the file's. Comparisons therefore sort.
 */
std::vector<osmscout::FileOffset> Sorted(std::vector<osmscout::FileOffset> offsets)
{
  std::ranges::sort(offsets);

  return offsets;
}

std::vector<osmscout::TypeInfoRef> Types(const osmscout::TypeInfoSet& types)
{
  std::vector<osmscout::TypeInfoRef> result;

  for (const auto& type : types) {
    result.push_back(type);
  }

  return result;
}

struct LookupResult
{
  std::vector<osmscout::FileOffset>  offsets;         //!< Sorted
  std::vector<osmscout::TypeInfoRef> loadedTypes;     //!< By type index
  size_t                             examinedEntries; //!< Entries the lookup examined
  size_t                             entryCount;      //!< Entries the index carries
};

LookupResult Resolve(const TestDatabase& db,
                     const osmscout::AreaIndex& index,
                     const osmscout::GeoBox& box,
                     const osmscout::TypeInfoSet& request)
{
  std::vector<osmscout::FileOffset> offsets;
  osmscout::TypeInfoSet             loadedTypes(*db.typeConfig);

  REQUIRE(index.GetOffsets(box,
                           request,
                           offsets,
                           loadedTypes));

  return LookupResult{.offsets=Sorted(std::move(offsets)),
                      .loadedTypes=Types(loadedTypes),
                      .examinedEntries=index.GetExaminedEntryCount(),
                      .entryCount=index.GetEntryCount(),};
}

/**
 * The types an index carries an entry for, read from the index itself: a lookup reports a
 * requested type as loaded exactly when the index file carries an entry for it, whether or
 * not that entry resolves an offset. Deriving the requests this way keeps the test
 * independent of which types the committed database happened to index.
 */
std::vector<osmscout::TypeInfoRef> CarriedTypes(const TestDatabase& db,
                                                const osmscout::AreaIndex& index,
                                                size_t count)
{
  std::vector<osmscout::TypeInfoRef> result;

  for (const auto& type : db.typeConfig->GetTypes()) {
    if (type->IsInternal()) {
      continue;
    }

    osmscout::TypeInfoSet request(*db.typeConfig);
    request.Set(type);

    if (!Resolve(db,index,db.boundingBox,request).loadedTypes.empty()) {
      result.push_back(type);

      if (result.size()==count) {
        break;
      }
    }
  }

  return result;
}

/** A non-internal type the index carries no entry for. */
osmscout::TypeInfoRef NotCarriedType(const TestDatabase& db,
                                     const osmscout::AreaIndex& index)
{
  for (const auto& type : db.typeConfig->GetTypes()) {
    if (type->IsInternal()) {
      continue;
    }

    osmscout::TypeInfoSet request(*db.typeConfig);
    request.Set(type);

    if (Resolve(db,index,db.boundingBox,request).loadedTypes.empty()) {
      return type;
    }
  }

  return nullptr;
}

osmscout::TypeInfoSet RequestOf(const TestDatabase& db,
                                const std::vector<osmscout::TypeInfoRef>& types)
{
  osmscout::TypeInfoSet request(*db.typeConfig);

  for (const auto& type : types) {
    request.Set(type);
  }

  return request;
}

/**
 * The center and radius of a box inside the committed test region. It resolves a few areas of
 * the requested types, so the assertions below pin the exact offsets of a small result: a
 * lookup that loses or adds an offset fails here.
 */
constexpr double kBoxCenterLat=50.47254;
constexpr double kBoxCenterLon=14.53186;
constexpr double kBoxRadiusKm=1.0;

/** The far corner of a box no entry of the committed test region intersects. */
constexpr double kOutsideBoxMax=0.1;

osmscout::GeoBox BoxInsideRegion()
{
  return osmscout::GeoBox::BoxByCenterAndRadius(osmscout::GeoCoord(kBoxCenterLat,kBoxCenterLon),
                                                osmscout::Distance::Of<osmscout::Kilometer>(kBoxRadiusKm));
}

/** A box no entry of the committed test region intersects; the test region lies in Europe. */
osmscout::GeoBox BoxOutsideRegion()
{
  return {osmscout::GeoCoord(0.0,0.0),osmscout::GeoCoord(kOutsideBoxMax,kOutsideBoxMax)};
}

}

TEST_CASE("A narrow area-way index request resolves the offsets of the requested types")
{
  TestDatabase db(TestDataDir());

  const osmscout::AreaIndex& index=*db.database->GetAreaWayIndex();

  auto carriedTypes=CarriedTypes(db,index,2);

  REQUIRE(carriedTypes.size()==2);

  osmscout::TypeInfoSet request=RequestOf(db,carriedTypes);

  LookupResult region=Resolve(db,index,db.boundingBox,request);
  LookupResult box=Resolve(db,index,BoxInsideRegion(),request);

  // A requested type is reported as loaded because the index carries an entry for it.
  REQUIRE(region.loadedTypes==Types(request));
  REQUIRE(box.loadedTypes==Types(request));
  REQUIRE(!region.offsets.empty());

  // Every offset the smaller box resolves is one the region box resolves as well.
  REQUIRE(std::ranges::includes(region.offsets,
                                box.offsets));

  // The offsets the committed index resolves for the requested types.
  REQUIRE(box.offsets==std::vector<osmscout::FileOffset>{38105,39348});
}

TEST_CASE("A narrow area-route index request resolves the offsets of the requested types")
{
  TestDatabase db(TestDataDir());

  const osmscout::AreaIndex& index=*db.database->GetAreaRouteIndex();

  auto carriedTypes=CarriedTypes(db,index,2);

  REQUIRE(carriedTypes.size()==2);

  osmscout::TypeInfoSet request=RequestOf(db,carriedTypes);

  LookupResult region=Resolve(db,index,db.boundingBox,request);
  LookupResult box=Resolve(db,index,BoxInsideRegion(),request);

  // A requested type is reported as loaded because the index carries an entry for it.
  REQUIRE(region.loadedTypes==Types(request));
  REQUIRE(box.loadedTypes==Types(request));

  // Every offset the smaller box resolves is one the region box resolves as well.
  REQUIRE(std::ranges::includes(region.offsets,
                                box.offsets));

  // The offsets the committed index resolves for the requested types.
  REQUIRE(region.offsets==std::vector<osmscout::FileOffset>{4,114,231,1130,1359,1407,1793,1838,1991,
                                                            2381,2977,4090,4181,4392});
}

TEST_CASE("A lookup examines only the entries of the types its request names")
{
  TestDatabase db(TestDataDir());

  const osmscout::AreaIndex& index=*db.database->GetAreaWayIndex();

  auto carriedTypes=CarriedTypes(db,index,2);

  REQUIRE(carriedTypes.size()==2);

  osmscout::TypeInfoSet request=RequestOf(db,carriedTypes);

  LookupResult lookup=Resolve(db,index,db.boundingBox,request);

  // The number of examined entries is the number of requested types the index carries, and
  // the index carries entries the request does not name as well.
  REQUIRE(lookup.examinedEntries==Types(request).size());
  REQUIRE(lookup.examinedEntries<lookup.entryCount);
}

TEST_CASE("Adding indexed types outside the request does not enlarge the lookup")
{
  TestDatabase db(TestDataDir());

  const osmscout::AreaIndex& index=*db.database->GetAreaWayIndex();

  auto carriedTypes=CarriedTypes(db,index,2);

  REQUIRE(carriedTypes.size()==2);

  auto notCarriedType=NotCarriedType(db,index);

  REQUIRE(notCarriedType!=nullptr);

  // One request names one carried type, one names two, and one names a type the index does
  // not carry. The examined count is the number of requested types the index carries in each
  // case and does not follow the number of entries the index holds - which is what the
  // scenario asks for. (The identity, not a second database, establishes it: the repository
  // commits one database, so the two-index comparison the scenario describes is not
  // reproducible here.)
  LookupResult one=Resolve(db,
                           index,
                           db.boundingBox,
                           RequestOf(db,std::vector<osmscout::TypeInfoRef>{carriedTypes.front()}));
  LookupResult two=Resolve(db,
                           index,
                           db.boundingBox,
                           RequestOf(db,carriedTypes));
  LookupResult none=Resolve(db,
                            index,
                            db.boundingBox,
                            RequestOf(db,std::vector<osmscout::TypeInfoRef>{notCarriedType}));

  REQUIRE(one.examinedEntries==1);
  REQUIRE(two.examinedEntries==2);
  REQUIRE(none.examinedEntries==0);

  // All three requests are served by the same index, so the entry count is the same for them
  // while the examined count follows the request.
  REQUIRE(one.entryCount==two.entryCount);
  REQUIRE(two.entryCount==none.entryCount);

  REQUIRE(none.loadedTypes.empty());
  REQUIRE(none.offsets.empty());
}

TEST_CASE("A named type that resolves no offset is still reported as loaded")
{
  TestDatabase db(TestDataDir());

  const osmscout::AreaIndex& index=*db.database->GetAreaWayIndex();

  auto carriedTypes=CarriedTypes(db,index,1);

  REQUIRE(carriedTypes.size()==1);

  osmscout::TypeInfoSet request=RequestOf(db,carriedTypes);

  LookupResult lookup=Resolve(db,index,BoxOutsideRegion(),request);

  // The entry is examined because the request names its type, the box resolves no offset
  // through it, and the type is reported as loaded either way.
  REQUIRE(lookup.examinedEntries==1);
  REQUIRE(lookup.offsets.empty());
  REQUIRE(lookup.loadedTypes==Types(request));
}
