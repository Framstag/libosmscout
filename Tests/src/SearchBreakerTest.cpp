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

#include "search_breaker.h"

/*
 * The search-request lifecycle (NaviVeylin change "fix-search-cancellation",
 * spec `search-request-lifecycle`). The registry is the whole decision: which
 * request is live, which one a supersede or a cancellation takes out, and that
 * neither reaches across a slot boundary.
 */

TEST_CASE("Beginning a search returns a fresh generation for that slot")
{
  naviveylin::SearchBreakerRegistry registry;

  const auto first  = registry.Begin(naviveylin::kSearchSlotPlaces);
  const auto second = registry.Begin(naviveylin::kSearchSlotPlaces);

  REQUIRE(first != second);
  REQUIRE(first != naviveylin::SearchBreakerRegistry::kNoGeneration);
  REQUIRE(registry.LiveCount() == 1);
}

TEST_CASE("A superseded generation reads as aborted")
{
  naviveylin::SearchBreakerRegistry registry;

  const auto superseded = registry.Begin(naviveylin::kSearchSlotPlaces);
  const auto current    = registry.Begin(naviveylin::kSearchSlotPlaces);

  // The same predicate covers supersede and cancel: the older request is not
  // live any more.
  REQUIRE(registry.IsAborted(superseded));
  REQUIRE_FALSE(registry.IsAborted(current));
}

TEST_CASE("Cancelling the current generation aborts it")
{
  naviveylin::SearchBreakerRegistry registry;

  const auto generation = registry.Begin(naviveylin::kSearchSlotRoute);

  REQUIRE(registry.Cancel(generation));
  REQUIRE(registry.IsAborted(generation));
}

TEST_CASE("Cancelling a stale generation is a no-op")
{
  naviveylin::SearchBreakerRegistry registry;

  const auto superseded = registry.Begin(naviveylin::kSearchSlotPlaces);
  const auto current    = registry.Begin(naviveylin::kSearchSlotPlaces);

  // The late cancellation of a superseded request must not touch the live one:
  // this is the property that lets a caller cancel in a `finally` block.
  REQUIRE_FALSE(registry.Cancel(superseded));
  REQUIRE_FALSE(registry.IsAborted(current));
  REQUIRE(registry.LiveCount() == 1);
}

TEST_CASE("A cancellation without a successor aborts the running search")
{
  naviveylin::SearchBreakerRegistry registry;

  const auto generation = registry.Begin(naviveylin::kSearchSlotPoi);

  // No newer request replaces it, so the cancellation itself is what stops it —
  // the case a search abandoned by its surface relies on.
  REQUIRE(registry.Cancel(generation));
  REQUIRE(registry.IsAborted(generation));
  REQUIRE(registry.LiveCount() == 0);
}

TEST_CASE("One slot's search does not abort another slot's")
{
  naviveylin::SearchBreakerRegistry registry;

  const auto places = registry.Begin(naviveylin::kSearchSlotPlaces);
  const auto car    = registry.Begin(naviveylin::kSearchSlotCar);

  // Both are live: a car search must not stop the phone's, and vice versa.
  REQUIRE_FALSE(registry.IsAborted(places));
  REQUIRE_FALSE(registry.IsAborted(car));

  REQUIRE(registry.Cancel(places));
  REQUIRE(registry.IsAborted(places));
  REQUIRE_FALSE(registry.IsAborted(car));

  // A supersede inside the place slot leaves the car slot alone as well.
  const auto placesAgain = registry.Begin(naviveylin::kSearchSlotPlaces);

  REQUIRE_FALSE(registry.IsAborted(car));
  REQUIRE_FALSE(registry.IsAborted(placesAgain));
  REQUIRE(registry.LiveCount() == 2);
}

TEST_CASE("Ending a finished search idles its slot")
{
  naviveylin::SearchBreakerRegistry registry;

  const auto finished = registry.Begin(naviveylin::kSearchSlotPoi);
  registry.End(finished);

  // The search is over, so a later cancellation of its generation aborts
  // nothing — and the next search of the slot starts cleanly.
  REQUIRE(registry.IsAborted(finished));
  REQUIRE_FALSE(registry.Cancel(finished));
  REQUIRE(registry.LiveCount() == 0);

  const auto next = registry.Begin(naviveylin::kSearchSlotPoi);

  REQUIRE(next != finished);
  REQUIRE_FALSE(registry.IsAborted(next));
}

TEST_CASE("The registry reports a live request's own slot")
{
  naviveylin::SearchBreakerRegistry registry;

  const auto places = registry.Begin(naviveylin::kSearchSlotPlaces);
  const auto car    = registry.Begin(naviveylin::kSearchSlotCar);

  // The diagnostics line names the request's slot, so the entry point that serves several
  // surfaces cannot guess it from the call it is in.
  REQUIRE(registry.SlotOf(places) == std::string(naviveylin::kSearchSlotPlaces));
  REQUIRE(registry.SlotOf(car) == std::string(naviveylin::kSearchSlotCar));

  registry.End(car);
  REQUIRE(registry.SlotOf(car).empty());

  const auto superseded = registry.Begin(naviveylin::kSearchSlotPlaces);
  REQUIRE(registry.SlotOf(places).empty());
  REQUIRE(registry.SlotOf(superseded) == std::string(naviveylin::kSearchSlotPlaces));
}

TEST_CASE("The unknown-generation sentinel is always aborted")
{
  naviveylin::SearchBreakerRegistry registry;
  const auto running = registry.Begin(naviveylin::kSearchSlotPlaces);

  // A caller that never began a search cannot hold a live request by accident.
  REQUIRE(registry.IsAborted(naviveylin::SearchBreakerRegistry::kNoGeneration));
  REQUIRE_FALSE(registry.Cancel(naviveylin::SearchBreakerRegistry::kNoGeneration));
  REQUIRE_FALSE(registry.IsAborted(running));
}
