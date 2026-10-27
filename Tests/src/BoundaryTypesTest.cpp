/*
  BoundaryTypesTest - the types the import pipeline treats as administrative boundaries

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
 * Tests for the resolver that decides which types the import pipeline treats as administrative
 * boundaries:
 *
 *  - the shipped full type definition declares a type per admin level the pipeline evaluates, and all
 *    of them have to reach the set - a level-8 area of a map built with it has to become a region,
 *  - a type definition that predates the municipality and suburb types declares only the four types
 *    every map carries; the resolver has to accept it without aborting, because the admin levels those
 *    types cover fall back to the generic boundary type, which such a definition declares.
 */

#include <cstdlib>
#include <filesystem>
#include <string>

#include <osmscout/TypeConfig.h>
#include <osmscout/TypeInfoSet.h>

#include <osmscout/util/Progress.h>

#include <osmscoutimport/ImportProgress.h>

#include <osmscoutimport/private/BoundaryTypes.h>

#include <catch2/catch_test_macros.hpp>

namespace {

  /**
   * Path of a file below the repository's stylesheet directory. The directory is found through
   * TESTS_TOP_DIR, the environment the test target is registered with, so that neither build system has
   * to bake an absolute path into the binary.
   */
  std::filesystem::path StylesheetPath(const std::string& filename)
  {
    const char  * topDirEnv=getenv("TESTS_TOP_DIR");
    std::string topDir=topDirEnv!=nullptr ? topDirEnv : "..";

    return std::filesystem::path(topDir) / ".." / "stylesheets" / filename;
  }

  osmscout::TypeConfigRef LoadTypeConfig(const std::string& filename)
  {
    auto typeConfig=std::make_shared<osmscout::TypeConfig>();

    REQUIRE(typeConfig->LoadFromOSTFile(StylesheetPath(filename).string()));

    return typeConfig;
  }

  bool Contains(const osmscout::TypeConfigRef& typeConfig,
                const osmscout::TypeInfoSet& types,
                const std::string& name)
  {
    osmscout::TypeInfoRef type=typeConfig->GetTypeInfo(name);

    REQUIRE(type);

    return types.IsSet(type);
  }
}

/*
 * The reported defect: the stylesheet splits two admin levels out of the generic boundary type, so a
 * level-8 boundary resolves to a type of its own and a level-10 one likewise. Both have to reach the set
 * of types the pipeline treats as boundaries, or the area is dropped before the location index sees it.
 */
TEST_CASE("The shipped type definition resolves every boundary type it declares", "[BoundaryTypes]")
{
  osmscout::TypeConfigRef  typeConfig=LoadTypeConfig("map.ost");
  osmscout::ImportProgress progress;
  osmscout::TypeInfoSet    types=osmscout::ResolveBoundaryTypes(*typeConfig,progress);

  REQUIRE(Contains(typeConfig,types,"boundary_country"));
  REQUIRE(Contains(typeConfig,types,"boundary_state"));
  REQUIRE(Contains(typeConfig,types,"boundary_county"));
  REQUIRE(Contains(typeConfig,types,"boundary_municipality"));
  REQUIRE(Contains(typeConfig,types,"boundary_suburb"));
  REQUIRE(Contains(typeConfig,types,"boundary_administrative"));
}

/*
 * A type definition older than the two types - the shipped basemap definition is one - declares only the
 * four types every map carries. Its level-8 and level-10 boundaries resolve to the generic boundary type,
 * which it declares as well, so the import has to run: resolving the two names must not require them.
 */
TEST_CASE("A type definition without the municipality and suburb types resolves without them", "[BoundaryTypes]")
{
  osmscout::TypeConfigRef  typeConfig=LoadTypeConfig("basemap.ost");
  osmscout::ImportProgress progress;
  osmscout::TypeInfoSet    types=osmscout::ResolveBoundaryTypes(*typeConfig,progress);

  REQUIRE(Contains(typeConfig,types,"boundary_country"));
  REQUIRE(Contains(typeConfig,types,"boundary_state"));
  REQUIRE(Contains(typeConfig,types,"boundary_county"));
  REQUIRE(Contains(typeConfig,types,"boundary_administrative"));

  REQUIRE(typeConfig->GetTypeInfo("boundary_municipality")==nullptr);
  REQUIRE(typeConfig->GetTypeInfo("boundary_suburb")==nullptr);
}
