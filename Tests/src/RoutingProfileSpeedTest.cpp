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

#include <limits>

#include "routing_profile_speed.h"

TEST_CASE("Zero requests the vehicle's built-in speed")
{
  // 0 is the documented "use the routing engine's built-in profile" request, not a speed.
  REQUIRE(naviveylin::ResolveProfileMaxSpeedKmh(0.0, 120.0)==120.0);
  REQUIRE(naviveylin::ResolveProfileMaxSpeedKmh(0.0, 30.0)==30.0);
  REQUIRE(naviveylin::ResolveProfileMaxSpeedKmh(0.0, 10.0)==10.0);
}

TEST_CASE("A negative speed selects the built-in default")
{
  REQUIRE(naviveylin::ResolveProfileMaxSpeedKmh(-1.0, 120.0)==120.0);
  REQUIRE(naviveylin::ResolveProfileMaxSpeedKmh(-300.0, 30.0)==30.0);
}

TEST_CASE("A non-finite speed selects the built-in default")
{
  // A NaN or an infinity is not a speed; neither can parametrize a profile.
  REQUIRE(naviveylin::ResolveProfileMaxSpeedKmh(std::numeric_limits<double>::quiet_NaN(), 120.0)==120.0);
  REQUIRE(naviveylin::ResolveProfileMaxSpeedKmh(std::numeric_limits<double>::infinity(), 30.0)==30.0);
  REQUIRE(naviveylin::ResolveProfileMaxSpeedKmh(-std::numeric_limits<double>::infinity(), 10.0)==10.0);
}

TEST_CASE("A speed just below 1 km/h selects the built-in default")
{
  REQUIRE(naviveylin::ResolveProfileMaxSpeedKmh(0.999, 120.0)==120.0);
}

TEST_CASE("A speed just above 300 km/h selects the built-in default")
{
  REQUIRE(naviveylin::ResolveProfileMaxSpeedKmh(300.001, 120.0)==120.0);
}

TEST_CASE("The band bounds 1 and 300 km/h are usable speeds")
{
  REQUIRE(naviveylin::ResolveProfileMaxSpeedKmh(1.0, 120.0)==1.0);
  REQUIRE(naviveylin::ResolveProfileMaxSpeedKmh(300.0, 120.0)==300.0);
}

TEST_CASE("A usable speed is used as it was requested")
{
  // A lorry-limited car: the ceiling is the requested speed, not the built-in default.
  REQUIRE(naviveylin::ResolveProfileMaxSpeedKmh(90.0, 120.0)==90.0);
  // A slow cargo bike and a hiker: the flat speed of the vehicle's profile.
  REQUIRE(naviveylin::ResolveProfileMaxSpeedKmh(20.0, 30.0)==20.0);
  REQUIRE(naviveylin::ResolveProfileMaxSpeedKmh(4.0, 10.0)==4.0);
}

TEST_CASE("The avoid-unpaved grade table is the published 1.0/0.85/0.6/0.3/0.1")
{
  REQUIRE(naviveylin::kAvoidUnpavedGradeFactor[0]==1.0);
  REQUIRE(naviveylin::kAvoidUnpavedGradeFactor[1]==0.85);
  REQUIRE(naviveylin::kAvoidUnpavedGradeFactor[2]==0.6);
  REQUIRE(naviveylin::kAvoidUnpavedGradeFactor[3]==0.3);
  REQUIRE(naviveylin::kAvoidUnpavedGradeFactor[4]==0.1);
}

TEST_CASE("A degraded type keeps its base speed at grade 1")
{
  // highway_unclassified at 50 km/h: the solid grade keeps the whole type speed, so a profile with
  // avoidUnpaved prices grade 1 exactly like one without the flag.
  const auto speeds=naviveylin::AvoidUnpavedGradeSpeedsKmh(50.0);
  REQUIRE(speeds[0]==50.0);
}

TEST_CASE("The grade speeds of a type fall strictly from grade 1 to grade 5")
{
  // 50 km/h: 50, 42.5, 30, 15, 5 — every step is slower, so a kilometre of it costs more.
  const auto speeds=naviveylin::AvoidUnpavedGradeSpeedsKmh(50.0);
  REQUIRE(speeds[0] > speeds[1]);
  REQUIRE(speeds[1] > speeds[2]);
  REQUIRE(speeds[2] > speeds[3]);
  REQUIRE(speeds[3] > speeds[4]);
}

TEST_CASE("A degraded road keeps a floor speed instead of becoming unusable")
{
  // A 2 km/h type is below the floor on every grade, so each grade keeps the floor.
  const auto slow=naviveylin::AvoidUnpavedGradeSpeedsKmh(2.0);
  for (const auto& speed : slow) {
    REQUIRE(speed==naviveylin::kAvoidUnpavedMinSpeedKmh);
  }

  // 10 km/h: grade 5 would be 1 km/h, which the floor lifts to 3.
  const auto speeds=naviveylin::AvoidUnpavedGradeSpeedsKmh(10.0);
  REQUIRE(speeds[4]==naviveylin::kAvoidUnpavedMinSpeedKmh);
  REQUIRE(speeds[4] > 0.0);
}
