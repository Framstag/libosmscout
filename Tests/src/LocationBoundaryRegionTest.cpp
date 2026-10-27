/*
  LocationBoundaryRegionTest - administrative boundaries of every admin level as regions

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

/*
 * The reported defect: a map built from the shipped type definition carried no municipality region,
 * because the types the stylesheet declares for admin level 8 and 10 were missing from the set the
 * location index step scans. The fixture of this target (Tests/LocationTest.olt) carries a level-8
 * boundary under the county and a level-10 boundary nested in it, so both levels are checked as regions
 * of the imported database and as children of the region their fixture parent describes.
 *
 * The database is built and opened by the main() of LocationServiceTest.cpp, which imports
 * Tests/LocationTest.olt into the test's temporary directory.
 */

#include <string>
#include <unordered_map>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include <osmscout/OSMScoutTypes.h>
#include <osmscout/db/Database.h>
#include <osmscout/db/LocationIndex.h>
#include <osmscout/location/Location.h>

extern osmscout::DatabaseRef database;

namespace {

  /**
   * Records every admin region with its parent, so that a case can name a region and walk the hierarchy
   * the index serialized for it.
   */
  class RegionParentVisitor : public osmscout::AdminRegionVisitor
  {
  public:
    struct Entry
    {
      osmscout::FileOffset parentOffset;
      std::string          name;
    };

    std::unordered_map<osmscout::FileOffset,Entry> regions;

  public:

    Action Visit(const osmscout::AdminRegion& region) override
    {
      regions[region.regionOffset]={region.parentRegionOffset,region.name};

      return visitChildren;
    }
  };

  /**
   * Names of the region and of its ancestors, the region itself first, up to the root of the tree. The
   * walk ends at the offset the index uses for "no parent", which is not an entry of the index.
   */
  std::vector<std::string> RegionChain(const RegionParentVisitor& hierarchy,
                                       const std::string& name)
  {
    std::vector<std::string> chain;

    for (const auto& [offset,entry] : hierarchy.regions) {
      if (entry.name!=name) {
        continue;
      }

      osmscout::FileOffset current=offset;

      while (true) {
        auto it=hierarchy.regions.find(current);

        if (it==hierarchy.regions.end()) {
          break;
        }

        chain.push_back(it->second.name);

        current=it->second.parentOffset;
      }

      return chain;
    }

    return chain;
  }
}

/*
 * A level-8 boundary is the region a search row names for a town: without it the row falls back to the
 * county, which is what the report describes.
 */
TEST_CASE("A level-8 boundary is a region of the location index", "[LocationBoundaryRegion]")
{
  osmscout::LocationIndexRef index=database->GetLocationIndex();

  REQUIRE(index);

  RegionParentVisitor hierarchy;

  REQUIRE(index->VisitAdminRegions(hierarchy));

  std::vector<std::string> chain=RegionChain(hierarchy,"Lünen");

  REQUIRE(!chain.empty()); // the level-8 boundary of the fixture is no region at all

  REQUIRE(chain==std::vector<std::string> {"Lünen","Kreis Unna","Regierungsbezirk Arnsberg","Nordrhein-Westfalen"});
}

/*
 * A level-10 boundary is nested in the municipality, so the same boundary level the report names for a
 * suburb is checked one level deeper.
 */
TEST_CASE("A level-10 boundary is a region of the location index, nested in the municipality",
          "[LocationBoundaryRegion]")
{
  osmscout::LocationIndexRef index=database->GetLocationIndex();

  REQUIRE(index);

  RegionParentVisitor hierarchy;

  REQUIRE(index->VisitAdminRegions(hierarchy));

  std::vector<std::string> chain=RegionChain(hierarchy,"Gahmen");

  REQUIRE(!chain.empty()); // the level-10 boundary of the fixture is no region at all

  REQUIRE(chain==std::vector<std::string> {"Gahmen","Lünen","Kreis Unna","Regierungsbezirk Arnsberg",
                                           "Nordrhein-Westfalen"});
}
