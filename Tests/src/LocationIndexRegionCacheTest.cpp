/*
  LocationIndexRegionCacheTest - a test program for libosmscout
  Copyright (C) 2026  Tim Teulings

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation; either version 2.1 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <cstddef>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include <osmscout/db/Database.h>
#include <osmscout/db/LocationIndex.h>

/*
 * The database is built and opened by the main() of LocationServiceTest.cpp, which imports
 * Tests/LocationTest.olt into the test's temporary directory.
 */
extern osmscout::DatabaseRef database;

namespace {
  /**
   * Records the admin regions a traversal shows it, in visit order, so that a case can compare
   * two traversals of the same index and count the regions the index visited. It follows the
   * complete tree and changes nothing.
   */
  class RegionRecordingVisitor : public osmscout::AdminRegionVisitor
  {
  public:
    std::vector<std::pair<osmscout::FileOffset,std::string>> regions;

  public:
    Action Visit(const osmscout::AdminRegion& region) override
    {
      regions.emplace_back(region.regionOffset,region.name);

      return visitChildren;
    }
  };
}

/*
 * The number the region cache is for: a lookup of the location service walks the complete region
 * tree of the index, so every further lookup deserializes every region again unless the index
 * retains what it read. The case reads the count of region entries the index deserialized
 * (LocationIndex::GetRegionReadCount()) and requires the second traversal of the same tree to
 * read none.
 */
TEST_CASE("A second traversal of the region tree reads no region entry", "[LocationIndexRegionCache]")
{
  osmscout::LocationIndexRef index=database->GetLocationIndex();

  REQUIRE(index);

  index->FlushCache(); // the two traversals below start with nothing retained

  RegionRecordingVisitor firstTraversal;

  REQUIRE(index->VisitAdminRegions(firstTraversal));

  size_t readsOfFirstTraversal=index->GetRegionReadCount();

  std::cout << "first traversal: " << firstTraversal.regions.size() << " regions visited, "
            << readsOfFirstTraversal << " region entries read" << std::endl;

  REQUIRE(firstTraversal.regions.size()>1); // the fixture carries a region tree, not a single region
  REQUIRE(readsOfFirstTraversal==firstTraversal.regions.size());

  RegionRecordingVisitor secondTraversal;

  REQUIRE(index->VisitAdminRegions(secondTraversal));

  size_t readsOfSecondTraversal=index->GetRegionReadCount()-readsOfFirstTraversal;

  std::cout << "second traversal: " << secondTraversal.regions.size() << " regions visited, "
            << readsOfSecondTraversal << " region entries read" << std::endl;

  // the regions a visitor sees do not change, whether the index read them or retained them
  REQUIRE(secondTraversal.regions==firstTraversal.regions);

  REQUIRE(readsOfSecondTraversal==0);
}

/*
 * The invariant the cache adds beyond the lookups it serves: the entries it retains are released
 * again, so the memory footprint the class documents stays achievable. A traversal after a flush
 * reads exactly the regions it visits.
 */
TEST_CASE("A traversal after a flushed cache reads the regions it visits", "[LocationIndexRegionCache]")
{
  osmscout::LocationIndexRef index=database->GetLocationIndex();

  REQUIRE(index);

  index->FlushCache();

  RegionRecordingVisitor warmUpTraversal;

  REQUIRE(index->VisitAdminRegions(warmUpTraversal));

  index->FlushCache(); // release what the warm-up traversal retained

  RegionRecordingVisitor afterFlushTraversal;

  REQUIRE(index->VisitAdminRegions(afterFlushTraversal));

  size_t readsAfterFlush=index->GetRegionReadCount();

  std::cout << "traversal after a flush: " << afterFlushTraversal.regions.size()
            << " regions visited, " << readsAfterFlush << " region entries read" << std::endl;

  REQUIRE(afterFlushTraversal.regions==warmUpTraversal.regions);

  REQUIRE(readsAfterFlush==afterFlushTraversal.regions.size());
}
