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
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <catch2/catch_test_macros.hpp>

#include "search_scope.h"

TEST_CASE("A hierarchy depth normalizes onto the admin_level scale")
{
  // The depth counts the root as 1, so the root is the scale's 0; the scale
  // then advances by two per level, matching OSM admin_level.
  REQUIRE(naviveylin::NormalizeDepthToAdminLevel(0)==0);
  REQUIRE(naviveylin::NormalizeDepthToAdminLevel(1)==0);  // root
  REQUIRE(naviveylin::NormalizeDepthToAdminLevel(2)==2);  // country
  REQUIRE(naviveylin::NormalizeDepthToAdminLevel(3)==4);  // state
  REQUIRE(naviveylin::NormalizeDepthToAdminLevel(4)==6);  // county / district
  REQUIRE(naviveylin::NormalizeDepthToAdminLevel(5)==8);  // municipality / city
  REQUIRE(naviveylin::NormalizeDepthToAdminLevel(6)==10); // suburb
}

TEST_CASE("Scope expansion stops at the cap and at an unknown parent level")
{
  // An unknown parent level never expands, whatever the cap is.
  REQUIRE_FALSE(naviveylin::ShouldExpandScope(0,5));
  REQUIRE_FALSE(naviveylin::ShouldExpandScope(0,0));

  // The cap itself is still enterable, and anything finer than it is.
  REQUIRE(naviveylin::ShouldExpandScope(5,5));
  REQUIRE(naviveylin::ShouldExpandScope(6,5));
  REQUIRE(naviveylin::ShouldExpandScope(10,5));

  // Anything coarser than the cap is not.
  REQUIRE_FALSE(naviveylin::ShouldExpandScope(4,5));
  REQUIRE_FALSE(naviveylin::ShouldExpandScope(2,5));

  // The comparison against the cap is inclusive, also for a cap other than the
  // one the search uses.
  REQUIRE(naviveylin::ShouldExpandScope(8,8));
  REQUIRE_FALSE(naviveylin::ShouldExpandScope(7,8));
}

TEST_CASE("The search cap admits a district parent but not a state parent")
{
  /*
   * search_scope.h pins the cap at level 5 (Regierungsbezirk / district) so that
   * the common "one up, one down" case is covered: a kreisfreie Stadt and the
   * towns around it share a district parent. Changing the cap is a product
   * decision about result volume, so it is pinned here on purpose.
   */
  REQUIRE(naviveylin::kMaxSearchRegionLevel==5);

  /*
   * Read against a real chain, with the depth normalized the way the search
   * normalizes it: the state (depth 3, level 4) is too coarse to expand into,
   * the district (depth 4, level 6) and the city (depth 5, level 8) are fine.
   */
  REQUIRE_FALSE(naviveylin::ShouldExpandScope(naviveylin::NormalizeDepthToAdminLevel(3),
                                              naviveylin::kMaxSearchRegionLevel));
  REQUIRE(naviveylin::ShouldExpandScope(naviveylin::NormalizeDepthToAdminLevel(4),
                                        naviveylin::kMaxSearchRegionLevel));
  REQUIRE(naviveylin::ShouldExpandScope(naviveylin::NormalizeDepthToAdminLevel(5),
                                        naviveylin::kMaxSearchRegionLevel));
}
