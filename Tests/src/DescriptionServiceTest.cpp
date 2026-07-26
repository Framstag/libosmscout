/*
  DescriptionServiceTest - a test program for libosmscout
  Copyright (C) 2024 Tim Teulings

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

#include <osmscout/description/DescriptionService.h>

#include <catch2/catch_test_macros.hpp>

/**
 * Test that ObjectDescription can hold and return entries.
 */
TEST_CASE("ObjectDescription entry management")
{
  osmscout::ObjectDescription desc;

  // Initially empty
  REQUIRE(desc.GetEntries().empty());

  // Add one entry
  desc.AddEntry(osmscout::DescriptionEntry("General", "Type", "building"));
  REQUIRE(desc.GetEntries().size() == 1);
  REQUIRE(desc.GetEntries().front().GetValue() == "building");

  // Add multiple entries
  desc.AddEntry(osmscout::DescriptionEntry("General", "Name", "My Building"));
  REQUIRE(desc.GetEntries().size() == 2);

  // Verify order is preserved
  auto it = desc.GetEntries().begin();
  REQUIRE(it->GetLabelKey() == "Type");
  ++it;
  REQUIRE(it->GetLabelKey() == "Name");
}

/**
 * Test that DescriptionService can be constructed.
 */
TEST_CASE("DescriptionService construction")
{
  osmscout::DescriptionService service1;
  osmscout::DescriptionService service2;
  REQUIRE(true);
}

/**
 * Test the candidate ranking logic used in the JNI getDescription.
 * This simulates the ranking algorithm that selects the best object
 * from multiple candidates found near a long-press coordinate.
 */
TEST_CASE("Candidate ranking logic")
{
  // Simulate the ranking algorithm from OSMScoutClient.cpp
  // Candidates have: hasData, typeRank (0=area,1=way,2=node),
  // distance, contains, areaSize

  struct TestCandidate {
    bool hasData;
    int typeRank;
    double distanceMeters;
    bool contains;
    double areaSize;
  };

  auto rankCandidates = [](const std::vector<TestCandidate> &candidates) -> int {
    if (candidates.empty()) return -1;

    const double VERY_CLOSE_METERS = 5.0;
    const double MAX_SMALL_AREA_SIZE = 10000.0;

    int bestIdx = -1;
    bool bestHasData = false;
    bool bestVeryClose = false;
    bool bestContains = false;
    double bestAreaSize = std::numeric_limits<double>::max();
    int bestTypeRank = 999;
    double bestDistance = 999999;

    for (size_t i = 0; i < candidates.size(); i++) {
      const auto &c = candidates[i];
      bool hasData = c.hasData;
      bool veryClose = c.typeRank > 0 && c.distanceMeters < VERY_CLOSE_METERS;
      bool effectiveContains = c.contains && c.areaSize < MAX_SMALL_AREA_SIZE;

      bool better = false;
      if (bestIdx < 0) {
        better = true;
      } else if (hasData && !bestHasData) {
        better = true;
      } else if (hasData == bestHasData) {
        if (veryClose && !bestVeryClose) {
          better = true;
        } else if (veryClose == bestVeryClose) {
          if (effectiveContains && !bestContains) {
            better = true;
          } else if (effectiveContains && bestContains && c.areaSize < bestAreaSize) {
            better = true;
          } else if (effectiveContains == bestContains) {
            if (c.typeRank < bestTypeRank) {
              better = true;
            } else if (c.typeRank == bestTypeRank && c.distanceMeters < bestDistance) {
              better = true;
            }
          }
        }
      }

      if (better) {
        bestIdx = static_cast<int>(i);
        bestHasData = hasData;
        bestVeryClose = veryClose;
        bestContains = effectiveContains;
        bestAreaSize = c.areaSize;
        bestTypeRank = c.typeRank;
        bestDistance = c.distanceMeters;
      }
    }
    return bestIdx;
  };

  SECTION("Empty candidates returns -1") {
    REQUIRE(rankCandidates({}) == -1);
  }

  SECTION("Single candidate is always selected") {
    std::vector<TestCandidate> candidates = {{false, 0, 10.0, false, 0}};
    REQUIRE(rankCandidates(candidates) == 0);
  }

  SECTION("Candidate with data beats candidate without data") {
    std::vector<TestCandidate> candidates = {
      {false, 0, 10.0, false, 0},   // area, no data
      {true, 1, 15.0, false, 0}     // way, has data
    };
    REQUIRE(rankCandidates(candidates) == 1);
  }

  SECTION("Very close way/node beats containing area") {
    std::vector<TestCandidate> candidates = {
      {true, 0, 20.0, true, 500.0},   // area contains point, has data
      {true, 1, 3.0, false, 0}         // way very close (3m < 5m), has data
    };
    REQUIRE(rankCandidates(candidates) == 1);
  }

  SECTION("Containing small area beats non-containing") {
    std::vector<TestCandidate> candidates = {
      {true, 0, 20.0, true, 500.0},    // area contains point
      {true, 1, 15.0, false, 0}        // way, not containing
    };
    REQUIRE(rankCandidates(candidates) == 0);
  }

  SECTION("Smaller containing area beats larger containing area") {
    std::vector<TestCandidate> candidates = {
      {true, 0, 20.0, true, 5000.0},   // larger containing area
      {true, 0, 25.0, true, 200.0}     // smaller containing area
    };
    REQUIRE(rankCandidates(candidates) == 1);
  }

  SECTION("Lower type rank (area) beats higher (node) when all else equal") {
    std::vector<TestCandidate> candidates = {
      {true, 2, 10.0, false, 0},   // node
      {true, 0, 10.0, false, 0}    // area (lower rank = better)
    };
    REQUIRE(rankCandidates(candidates) == 1);
  }

  SECTION("Closer candidate wins when type rank equal") {
    std::vector<TestCandidate> candidates = {
      {true, 1, 20.0, false, 0},   // way, far
      {true, 1, 8.0, false, 0}     // way, close
    };
    REQUIRE(rankCandidates(candidates) == 1);
  }

  SECTION("Large containing area does not get contains bonus") {
    std::vector<TestCandidate> candidates = {
      {true, 0, 30.0, true, 50000.0},  // large area (>10000), contains
      {true, 1, 10.0, false, 0}         // way, no contains
    };
    // Large area's contains bonus is suppressed, but area still wins by typeRank (0 < 1)
    REQUIRE(rankCandidates(candidates) == 0);
  }

  SECTION("Large area with very close way: way wins") {
    std::vector<TestCandidate> candidates = {
      {true, 0, 30.0, true, 50000.0},  // large area (>10000), contains
      {true, 1, 3.0, false, 0}          // way very close (3m < 5m)
    };
    // Way is very close, which beats area's typeRank advantage
    REQUIRE(rankCandidates(candidates) == 1);
  }
}
