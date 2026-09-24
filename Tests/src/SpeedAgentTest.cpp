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

#include <chrono>
#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <osmscout/GeoCoord.h>
#include <osmscout/navigation/Agents.h>
#include <osmscout/navigation/SpeedAgent.h>
#include <osmscout/util/Bearing.h>
#include <osmscout/util/Distance.h>
#include <osmscout/util/Geometry.h>
#include <osmscout/util/Time.h>

/*
 * Tests for the speed agent's position-difference fallback (spec navigation-speed-agent).
 *
 * The fallback has to separate a standing vehicle whose fix jitters from a vehicle that
 * really moves, and it must not swallow the speed of a walker: walking is 3 to 5 km/h, which
 * is 0.8 to 1.4 m per 1 s fix - less than the jitter of a bad fix. The tests state both
 * sides of that contract, at 1 Hz and at 4 Hz, and they feed the agent directly, so they
 * need neither a database nor a GPS device.
 */

namespace {

  using osmscout::GeoCoord;
  using osmscout::Timestamp;

  constexpr double LAT_DEGREE_METERS = 111320.0;
  // Length of a longitude degree at the 51.5 deg latitude of the fixtures.
  constexpr double LON_DEGREE_METERS = 111320.0 * 0.6225;

  constexpr double FIXTURE_LAT = 51.5;
  constexpr double FIXTURE_LON = 7.4;

  /**
   * A fixed jitter pattern of 1 m amplitude. A stationary receiver repeats a bounded pattern
   * around its true position, and a fixed pattern keeps the test independent of the random
   * number generator of the standard library.
   */
  const double     JITTER[][2] = {
    { 1.0,  0.0}, {-0.9,  0.3}, { 0.8, -0.6}, {-0.9, -0.9},
    { 0.6,  0.9}, {-0.7,  0.5}, { 0.9, -0.4}, {-1.0, -0.2},
    { 0.5,  0.8}, {-0.6, -0.9}, { 1.0,  0.2}, {-0.8,  0.6}
  };

  constexpr size_t JITTER_COUNT = sizeof(JITTER)/sizeof(JITTER[0]);

  Timestamp At(uint64_t seconds)
  {
    return Timestamp(std::chrono::seconds(seconds));
  }

  GeoCoord Offset(const GeoCoord& coord, double eastMeters, double northMeters)
  {
    return GeoCoord(coord.GetLat() + northMeters / LAT_DEGREE_METERS,
                    coord.GetLon() + eastMeters / LON_DEGREE_METERS);
  }

  /**
   * Feed one fix without a speed reported by the receiver (negative), so that the agent uses
   * its position-difference fallback, and return the speed it published (NaN when it published
   * none).
   */
  double FeedFix(osmscout::SpeedAgent& agent,
                 const Timestamp& timestamp,
                 const GeoCoord& fix)
  {
    auto messages = agent.Process(std::make_shared<osmscout::GPSUpdateMessage>(timestamp,
                                                                               fix,
                                                                               -1.0,
                                                                               osmscout::Meters(5)));

    for (const auto& message : messages) {
      if (auto* speed = dynamic_cast<osmscout::CurrentSpeedMessage*>(message.get());
          speed != nullptr) {
        return speed->speed;
      }
    }

    return std::numeric_limits<double>::quiet_NaN();
  }

  /**
   * Drive the agent over `fixCount` fixes: the true position moves at `speedKmh` (0 = standing
   * still) and every fix is disturbed by the jitter pattern when `jitter` is set. Returns the
   * published speeds in order.
   */
  std::vector<double> Drive(double speedKmh, double rateHz, bool jitter, size_t fixCount)
  {
    osmscout::SpeedAgent agent;
    std::vector<double>  speeds;

    Timestamp            timestamp = At(10000);
    GeoCoord             truePosition(FIXTURE_LAT, FIXTURE_LON);

    for (size_t i = 0; i < fixCount; i++) {
      GeoCoord fix = truePosition;

      if (jitter) {
        fix = Offset(fix, JITTER[i % JITTER_COUNT][0], JITTER[i % JITTER_COUNT][1]);
      }

      speeds.push_back(FeedFix(agent, timestamp, fix));

      truePosition = osmscout::GetEllipsoidalDistance(truePosition,
                                                      osmscout::Bearing::Degrees(0.0),
                                                      osmscout::Meters((speedKmh / 3.6) / rateHz));
      timestamp = timestamp + std::chrono::duration_cast<Timestamp::duration>(
        std::chrono::duration<double>(1.0 / rateHz));
    }

    return speeds;
  }

  /**
   * The last speed the agent published. The fallback only runs on a segment of at least one
   * second, so at fix rates above 1 Hz it publishes on every such segment and stays quiet in
   * between.
   */
  double LastPublished(const std::vector<double>& speeds)
  {
    for (auto speed = speeds.rbegin(); speed != speeds.rend(); speed++) {
      if (!std::isnan(*speed)) {
        return *speed;
      }
    }

    return std::numeric_limits<double>::quiet_NaN();
  }
}

TEST_CASE("Speed agent does not report standstill GPS jitter as speed")
{
  // 1 m jitter per 1 s fix is 3.6 km/h of phantom speed for the position-difference fallback.
  // Once the fix history covers the gate window, every fix of a standing vehicle reports 0.
  const auto speeds = Drive(0.0, 1.0, true, 15);

  REQUIRE(speeds.size() == 15);

  // The first fix only seeds the previous position, and the gate needs its window of fixes
  // before it decides: the first four published speeds come from the plain fallback.
  for (size_t i = 5; i < speeds.size(); i++) {
    REQUIRE(speeds[i] == Catch::Approx(0.0));
  }
}

TEST_CASE("Speed agent reports walking speed")
{
  SECTION("3 km/h at 1 Hz")
  {
    const auto speeds = Drive(3.0, 1.0, false, 15);

    REQUIRE(speeds.back() == Catch::Approx(3.0).margin(0.4));
  }

  SECTION("4 km/h at 1 Hz")
  {
    const auto speeds = Drive(4.0, 1.0, false, 15);

    REQUIRE(speeds.back() == Catch::Approx(4.0).margin(0.4));
  }

  SECTION("5 km/h at 1 Hz")
  {
    const auto speeds = Drive(5.0, 1.0, false, 15);

    REQUIRE(speeds.back() == Catch::Approx(5.0).margin(0.4));
  }

  // The gate is a displacement per window, so walking must be reported at slow fix rates too:
  // at 0.5 Hz every fix is already a 2 s segment.
  SECTION("4 km/h at 0.5 Hz")
  {
    const auto speeds = Drive(4.0, 0.5, false, 12);

    REQUIRE(LastPublished(speeds) == Catch::Approx(4.0).margin(0.4));
  }

  // Walking on top of jitter is still movement: it must not collapse to a standing vehicle.
  // The reported magnitude is inflated by the jitter, because the fallback sums the segment
  // distances - that is a property of the fallback and not of the stationary gate.
  SECTION("4 km/h at 1 Hz with 1 m jitter")
  {
    const auto speeds = Drive(4.0, 1.0, true, 15);

    REQUIRE(LastPublished(speeds) > 1.5);
    REQUIRE(LastPublished(speeds) < 12.0);
  }
}

TEST_CASE("The stationary gate has a floor: movement below it is reported as standing")
{
  // The gate compares the net displacement per window against a floor meant to sit above fix
  // jitter, so movement that accumulates less displacement than that floor over the window is
  // indistinguishable from a standing receiver. At 1 Hz, 2 km/h covers about 2.8 m in the five
  // second window, which is below the floor - the limit is deliberate and documented at the gate.
  const auto speeds = Drive(2.0, 1.0, false, 15);

  REQUIRE(speeds.size() == 15);
  REQUIRE(LastPublished(speeds) == Catch::Approx(0.0));
}

TEST_CASE("Speed agent prefers the speed reported by the receiver")
{
  osmscout::SpeedAgent agent;

  auto                 drive = [&agent](const Timestamp& timestamp, double reportedSpeedMetersPerSecond) {
                                 auto messages = agent.Process(
                                   std::make_shared<osmscout::GPSUpdateMessage>(timestamp,
                                                                                GeoCoord(FIXTURE_LAT, FIXTURE_LON),
                                                                                reportedSpeedMetersPerSecond,
                                                                                osmscout::Meters(5)));

                                 for (const auto& message : messages) {
                                   if (auto* speed = dynamic_cast<osmscout::CurrentSpeedMessage*>(message.get());
                                       speed != nullptr) {
                                     return speed->speed;
                                   }
                                 }

                                 return std::numeric_limits<double>::quiet_NaN();
                               };

  REQUIRE(drive(At(10000), 4.0) == Catch::Approx(14.4).margin(0.1));

  // A receiver that reports a standing vehicle keeps the fallback quiet as well.
  REQUIRE(drive(At(10001), 0.0) == Catch::Approx(0.0));
}
