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

#include "route_length.h"

TEST_CASE("The description's own total is the published length")
{
  // 97 416 m description total against a 97 283 m drawn polyline: the
  // description's own total wins, so the step list sums to the published total.
  REQUIRE(naviveylin::RouteLengthMeters(97416.0, 97283.0)==97416.0);

  // A description total without any geometry at all is still the length.
  REQUIRE(naviveylin::RouteLengthMeters(1487.0, 0.0)==1487.0);
}

TEST_CASE("A zero description total falls back to the published geometry's length")
{
  // No description total, but a polyline was published: its length is the
  // route's length.
  REQUIRE(naviveylin::RouteLengthMeters(0.0, 97283.0)==97283.0);
}

TEST_CASE("No usable length at all yields zero")
{
  // Neither a description total nor geometry to measure: zero, not a fault.
  REQUIRE(naviveylin::RouteLengthMeters(0.0, 0.0)==0.0);
}

TEST_CASE("The router's air-line estimate is never a published length")
{
  /*
   * The router's overall distance for this route (the start/target air-line
   * estimate it uses for its cost limit and its progress denominator) was
   * 72 771 m against a 97 283 m drawn polyline - 0.748x. The estimate is not an
   * input of the decision at all, so no combination of a missing description
   * total and the published geometry can yield it.
   */
  const double airLineEstimateMeters = 72771.0;
  const double geometryLengthMeters = 97283.0;

  REQUIRE(naviveylin::RouteLengthMeters(0.0, geometryLengthMeters)!=airLineEstimateMeters);
  REQUIRE(naviveylin::RouteLengthMeters(0.0, geometryLengthMeters)>airLineEstimateMeters);
}
