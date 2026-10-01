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

#include <cmath>

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

TEST_CASE("The search cap admits a district parent but not a state parent"){
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

TEST_CASE("A scoped search admits a position inside the extent and rejects one outside")
{
  // Dortmund's own neighbourhood as the region's bounding box.
  const auto box=naviveylin::BoxFromCorners(51.40, 7.30, 51.70, 7.70);

  REQUIRE(box.isSet);
  REQUIRE(naviveylin::IsInsideGeoBox(box, 51.5136, 7.4653));   // the city centre
  REQUIRE(naviveylin::IsInsideGeoBox(box, 51.40, 7.30));       // the corner is inside
  REQUIRE(naviveylin::IsInsideGeoBox(box, 51.70, 7.70));       // and so is its opposite

  REQUIRE_FALSE(naviveylin::IsInsideGeoBox(box, 64.1466, -21.9426)); // Reykjavik
  REQUIRE_FALSE(naviveylin::IsInsideGeoBox(box, 51.40, 7.29));       // a step west
  REQUIRE_FALSE(naviveylin::IsInsideGeoBox(box, 51.71, 7.50));       // a step north
}

TEST_CASE("The extent normalizes the corner order and clamps the latitude")
{
  const auto reversed=naviveylin::BoxFromCorners(51.70, 7.70, 51.40, 7.30);
  const auto ordered=naviveylin::BoxFromCorners(51.40, 7.30, 51.70, 7.70);

  REQUIRE(reversed.minLat==ordered.minLat);
  REQUIRE(reversed.maxLat==ordered.maxLat);
  REQUIRE(reversed.minLon==ordered.minLon);
  REQUIRE(reversed.maxLon==ordered.maxLon);

  // The pole clamp keeps a box usable even when the fallback radius runs past it.
  const auto polar=naviveylin::BoxFromCorners(89.9, 10.0, 90.1, 20.0);

  REQUIRE(polar.maxLat==90.0);
  REQUIRE(naviveylin::IsInsideGeoBox(polar, 90.0, 15.0));
}

TEST_CASE("An unavailable extent admits every position instead of emptying the search")
{
  const auto unset=naviveylin::UnsetGeoBox();

  REQUIRE_FALSE(unset.isSet);
  REQUIRE(naviveylin::IsInsideGeoBox(unset, 51.5136, 7.4653));
  REQUIRE(naviveylin::IsInsideGeoBox(unset, 64.1466, -21.9426));
  REQUIRE(naviveylin::IsInsideGeoBox(unset, 0.0, 0.0));
}

TEST_CASE("A non-finite position is never inside a set extent")
{
  const auto box=naviveylin::BoxFromCorners(51.40, 7.30, 51.70, 7.70);
  const double nan=std::nan("");

  REQUIRE_FALSE(naviveylin::IsInsideGeoBox(box, nan, 7.4653));
  REQUIRE_FALSE(naviveylin::IsInsideGeoBox(box, 51.5136, nan));
  REQUIRE_FALSE(naviveylin::IsInsideGeoBox(box, nan, nan));
}

TEST_CASE("The node-region fallback box contains its point at the documented size")
{
  const auto box=naviveylin::BoxAroundPoint(51.5136, 7.4653);

  REQUIRE(box.isSet);
  REQUIRE(naviveylin::IsInsideGeoBox(box, 51.5136, 7.4653));
  REQUIRE(naviveylin::IsInsideGeoBox(box, 51.5136+naviveylin::kNodeRegionFallbackDegrees,
                                     7.4653));
  REQUIRE_FALSE(naviveylin::IsInsideGeoBox(box, 51.5136+2.0*naviveylin::kNodeRegionFallbackDegrees,
                                          7.4653));

  // An explicit half-size is honoured, which is what a caller with a known
  // region radius would pass.
  const auto tight=naviveylin::BoxAroundPoint(51.5136, 7.4653, 0.01);

  REQUIRE(naviveylin::IsInsideGeoBox(tight, 51.5136+0.01, 7.4653));
  REQUIRE_FALSE(naviveylin::IsInsideGeoBox(tight, 51.5136+0.02, 7.4653));
}

TEST_CASE("A region's bounding box never drops a position inside the region")
{
  /*
   * The property the scope filter relies on: the extent derived from a region
   * is a superset of that region, so filtering by it can only admit a position
   * the region does not contain — it can never drop one the region does
   * contain. A smaller box standing in for an object inside the region is
   * therefore always contained.
   */
  const auto region=naviveylin::BoxFromCorners(51.40, 7.30, 51.70, 7.70);
  const auto insideTheRegion=naviveylin::BoxAroundPoint(51.5136, 7.4653, 0.001);

  REQUIRE(naviveylin::GeoBoxContains(region, insideTheRegion));
  REQUIRE_FALSE(naviveylin::GeoBoxContains(insideTheRegion, region));

  // An unavailable extent is contained by nothing, and contains nothing but itself.
  const auto unset=naviveylin::UnsetGeoBox();

  REQUIRE_FALSE(naviveylin::GeoBoxContains(region, unset));
  REQUIRE(naviveylin::GeoBoxContains(unset, unset));
}

