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

#include "route_start.h"

TEST_CASE("The approach of a start that is the snapped node is zero")
{
  // The ordinary case: the caller's start is already the node the router uses, so
  // there is nothing to prepend and no length to add.
  REQUIRE(naviveylin::RouteStartApproachMeters(52.5,13.4,52.5,13.4)==0.0);
}

TEST_CASE("The approach is the great-circle length between the two positions")
{
  // A tenth of a degree of latitude is 11.1 km — 1852 m per arc-minute on the sphere
  // this header measures on — whatever the longitude. The bound is loose enough for
  // the sphere-versus-ellipsoid difference and tight enough to catch a formula that
  // mixes the two radii or forgets the cosine.
  const double northSouth=naviveylin::RouteStartApproachMeters(52.5,13.4,52.6,13.4);

  REQUIRE(northSouth>11000.0);
  REQUIRE(northSouth<11200.0);

  // Symmetry: the approach does not depend on which end is asked first.
  REQUIRE(naviveylin::RouteStartApproachMeters(52.6,13.4,52.5,13.4)==northSouth);

  // A longitude-only step at a high latitude is shorter by cos(latitude): at 60°
  // north a tenth of a degree of longitude is half of the equatorial one.
  const double atEquator=naviveylin::RouteStartApproachMeters(0.0,13.4,0.0,13.5);
  const double atSixty=naviveylin::RouteStartApproachMeters(60.0,13.4,60.0,13.5);

  REQUIRE(atSixty>atEquator*0.49);
  REQUIRE(atSixty<atEquator*0.51);
}

TEST_CASE("A start inside the snap tolerance is not prepended")
{
  // Below the tolerance the start counts as being on the network: no extra first
  // point, no zero-length leading segment.
  REQUIRE_FALSE(naviveylin::ShouldPrependRouteStart(0.0));
  REQUIRE_FALSE(naviveylin::ShouldPrependRouteStart(0.4));

  // Exactly at the tolerance is still "on the network" — the comparison is strict, so
  // the boundary belongs to the unchanged geometry (spec `osmscout-jni` — a start on
  // the network is published unchanged).
  REQUIRE_FALSE(naviveylin::ShouldPrependRouteStart(naviveylin::kRouteStartSnapToleranceMeters));

  // Beyond it the requested start is published, so a drawn route can differ from the
  // routed one — and does have to say how far.
  REQUIRE(naviveylin::ShouldPrependRouteStart(1.5));
  REQUIRE(naviveylin::ShouldPrependRouteStart(198.0));
}

TEST_CASE("A start that is not a position has no approach")
{
  // Non-finite input is not a position: it must not invent an approach, because the
  // prepend decision and the published length both rest on this number.
  REQUIRE(naviveylin::RouteStartApproachMeters(std::nan(""),13.4,52.5,13.4)==0.0);
  REQUIRE(naviveylin::RouteStartApproachMeters(52.5,13.4,52.5,INFINITY)==0.0);
  REQUIRE_FALSE(naviveylin::ShouldPrependRouteStart(
    naviveylin::RouteStartApproachMeters(std::nan(""),13.4,52.5,13.4)));
}

TEST_CASE("The offset leaves the start node at zero and adds the approach to every later node")
{
  const double approach=200.0;

  // The start node keeps its own value — zero for a description that begins at the
  // route's start — so step 0 still reads as the route's start and owns no leg.
  REQUIRE(naviveylin::RouteStartNodeDistanceMeters(0,0.0,approach)==0.0);

  // Every later node carries the approach, so the first leg's delta, the description
  // total and the per-step sums all include it (spec `osmscout-jni` — a snapped start
  // does not split one route into two lengths).
  REQUIRE(naviveylin::RouteStartNodeDistanceMeters(1,0.0,approach)==200.0);
  REQUIRE(naviveylin::RouteStartNodeDistanceMeters(2,350.0,approach)==550.0);
  REQUIRE(naviveylin::RouteStartNodeDistanceMeters(7,72000.0,approach)==72200.0);

  // Without an approach the rule is the identity on every node, which is what makes
  // an on-network start publish byte-identical geometry.
  REQUIRE(naviveylin::RouteStartNodeDistanceMeters(0,0.0,0.0)==0.0);
  REQUIRE(naviveylin::RouteStartNodeDistanceMeters(3,350.0,0.0)==350.0);
}
