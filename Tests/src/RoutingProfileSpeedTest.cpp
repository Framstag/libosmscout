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
