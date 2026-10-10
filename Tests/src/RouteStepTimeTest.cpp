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

#include <chrono>
#include <optional>
#include <string>

#include "route_step_time.h"

namespace {

using std::chrono::seconds;

std::string FormatSeconds(long long stepSeconds)
{
  auto timeText = naviveylin::FormatRouteStepTime(seconds(stepSeconds));

  return timeText.has_value() ? *timeText : std::string();
}

}

TEST_CASE("A step of minutes is printed in whole minutes")
{
  REQUIRE(FormatSeconds(60)==std::string("1 min"));
  REQUIRE(FormatSeconds(75)==std::string("1 min"));
  REQUIRE(FormatSeconds(45 * 60)==std::string("45 min"));
  REQUIRE(FormatSeconds(59 * 60 + 59)==std::string("59 min"));
}

TEST_CASE("A step of an hour or more is printed in hours and remaining minutes")
{
  REQUIRE(FormatSeconds(60 * 60)==std::string("1 h 0 min"));
  REQUIRE(FormatSeconds(60 * 60 + 5 * 60 + 59)==std::string("1 h 5 min"));
  REQUIRE(FormatSeconds(2 * 60 * 60 + 45 * 60)==std::string("2 h 45 min"));
}

TEST_CASE("A step below a minute is printed in whole seconds")
{
  // The defect this rule fixes: a city route's steps all printed "0 min".
  REQUIRE(FormatSeconds(1)==std::string("1 s"));
  REQUIRE(FormatSeconds(45)==std::string("45 s"));
  REQUIRE(FormatSeconds(59)==std::string("59 s"));

  REQUIRE(FormatSeconds(1)!=std::string("0 min"));
  REQUIRE(FormatSeconds(45)!=std::string("0 min"));
}

TEST_CASE("A step below a second carries no time")
{
  REQUIRE(naviveylin::FormatRouteStepTime(seconds(0))==std::nullopt);
  REQUIRE(naviveylin::FormatRouteStepTime(seconds(-5))==std::nullopt);

  // No zero-valued time on a step that does exist.
  REQUIRE(FormatSeconds(0)!=std::string("0 s"));
  REQUIRE(FormatSeconds(0)!=std::string("0 min"));
}

TEST_CASE("The unit boundaries are inclusive at the second, minute and hour")
{
  // The exact boundaries are what the three branches switch on.
  REQUIRE(FormatSeconds(1)==std::string("1 s"));
  REQUIRE(FormatSeconds(60)==std::string("1 min"));
  REQUIRE(FormatSeconds(60 * 60)==std::string("1 h 0 min"));
}
