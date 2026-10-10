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
#include <vector>

#include "routing_progress_throttle.h"

namespace {

using Throttle = naviveylin::RoutingProgressThrottle;
using TimePoint = std::chrono::steady_clock::time_point;

/** A time point `millis` after the start of an imaginary calculation. */
TimePoint At(long long millis)
{
  return TimePoint{} + std::chrono::milliseconds(millis);
}

} // namespace

TEST_CASE("The first progress report is handed over at once")
{
  // Nothing was reported yet, so the first percentage does not wait for the
  // rate-limit interval and a short calculation still shows progress.
  Throttle zeroStart;
  REQUIRE(zeroStart.ShouldReport(0, At(0)));

  Throttle movedStart;
  REQUIRE(movedStart.ShouldReport(5, At(0)));
}

TEST_CASE("An unchanged or lower percentage is not reported again")
{
  Throttle throttle;

  REQUIRE(throttle.ShouldReport(5, At(0)));

  // A repeated value carries no news, however long the interval since.
  REQUIRE_FALSE(throttle.ShouldReport(5, At(100)));
  REQUIRE_FALSE(throttle.ShouldReport(5, At(10000)));

  // A lower value cannot happen for one calculation, and would be dropped.
  REQUIRE_FALSE(throttle.ShouldReport(4, At(10000)));
  REQUIRE_FALSE(throttle.ShouldReport(-1, At(10000)));
}

TEST_CASE("A change inside the rate-limit interval is dropped, a later one is taken")
{
  Throttle throttle;

  REQUIRE(throttle.ShouldReport(5, At(0)));

  REQUIRE_FALSE(throttle.ShouldReport(6, At(99)));
  REQUIRE(throttle.ShouldReport(6, At(100)));

  REQUIRE_FALSE(throttle.ShouldReport(7, At(150)));
  REQUIRE(throttle.ShouldReport(7, At(200)));

  // A dropped report leaves the state alone, so the next interval is measured
  // from the last handover and not from the last call.
  REQUIRE_FALSE(throttle.ShouldReport(8, At(250)));
  REQUIRE_FALSE(throttle.ShouldReport(8, At(299)));
  REQUIRE(throttle.ShouldReport(8, At(300)));
}

TEST_CASE("An edge storm hands over at most one report per interval")
{
  Throttle throttle;

  // One edge per simulated 10 ms, each with a changed percentage: the whole
  // calculation runs 500 ms, so the rule admits the report at 0 ms and one per
  // interval after it.
  unsigned long accepted = 0;
  int lastAccepted = -1;
  bool nonDecreasing = true;

  for (long long millis = 0; millis <= 500; millis += 10) {
    int percent = naviveylin::ProgressPercent(static_cast<double>(millis), 1000.0);

    if (throttle.ShouldReport(percent, At(millis))) {
      accepted++;

      if (percent < lastAccepted) {
        nonDecreasing = false;
      }
      lastAccepted = percent;
    }
  }

  REQUIRE(accepted == 6);   // 0 ms, then 100 ms, 200 ms, 300 ms, 400 ms, 500 ms
  REQUIRE(accepted <= 500 / naviveylin::kMinProgressReportInterval.count() + 1);
  REQUIRE(nonDecreasing);
}

TEST_CASE("Progress never reports completion")
{
  // The reported percentage is capped below completion while the router covers
  // the whole route; 100 % is delivered as the calculation's result.
  REQUIRE(naviveylin::kMaxProgressPercent == 99);
  REQUIRE(naviveylin::ProgressPercent(0.0, 1000.0) == 0);
  REQUIRE(naviveylin::ProgressPercent(500.0, 1000.0) == 50);
  REQUIRE(naviveylin::ProgressPercent(990.0, 1000.0) == 99);
  REQUIRE(naviveylin::ProgressPercent(1000.0, 1000.0) == 99);
  REQUIRE(naviveylin::ProgressPercent(1500.0, 1000.0) == 99);

  // An unknown or negative overall distance cannot produce a percentage.
  REQUIRE(naviveylin::ProgressPercent(100.0, 0.0) == 0);
  REQUIRE(naviveylin::ProgressPercent(100.0, -1.0) == 0);

  // Feeding the conversion through the rule keeps every handed-over value
  // below completion.
  Throttle throttle;
  int maxAccepted = 0;

  for (long long meters = 0; meters <= 1200; meters += 100) {
    int percent = naviveylin::ProgressPercent(static_cast<double>(meters), 1000.0);

    if (throttle.ShouldReport(percent, At(meters))) {
      maxAccepted = percent > maxAccepted ? percent : maxAccepted;
    }
  }

  REQUIRE(maxAccepted < 100);
  REQUIRE(maxAccepted == 99);
}
