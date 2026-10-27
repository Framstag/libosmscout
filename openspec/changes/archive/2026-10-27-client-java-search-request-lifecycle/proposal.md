# Proposal

## Why

The bridge kept one file-static breaker for every search. A newer search aborted whichever search was
running, regardless of the surface that asked for it — in a projection session the car's search template
stopped the phone's places search — and no caller could cancel a search without starting another. The one
explicit cancel was parameterless: `cancelSearch()` aborted the *current* search, whoever owns it.

A caller that cannot name the request it means cannot cancel safely. The Kotlin caller is a coroutine whose
cancellation handler runs after the blocking JNI frame returned, by which time a successor search may
already own the breaker: the late cancellation then stops the live search — the defect this change removes.

## What Changes

- A search request SHALL be identified by a generation, and SHALL belong to a slot — one per search surface
  (places, poi, route, car, car-poi), so a surface's searches supersede only each other.
- `beginSearch(slot)` SHALL return the request's generation; `cancelSearch(generation)` SHALL cancel it and
  report whether it was live, so cancelling a superseded generation is a no-op by construction.
- One abort predicate SHALL cover every way a search stops: a live request leaves the live set when a newer
  request of its slot begins, when it is cancelled, or when it ends.
- The search entry points SHALL carry the request's generation, and the convenience overloads SHALL begin
  and end the place (or poi) slot's request themselves.
- An aborted search SHALL return an empty result array rather than a partial list, and its diagnostics line
  SHALL carry the slot, the generation, the abort state and the duration.
- The JavaScout callers SHALL be ported to the generation API in the same change.
- The lifecycle SHALL be a dependency-free registry, so a host test can exercise a supersede, a
  cancellation and a slot's independence without a database or a fake client.

**Breaking Java API**: the parameterless `cancelSearch()` and the boolean-cancel
`searchLocations(String, int, String, boolean)` overload are replaced by `beginSearch(String)` /
`cancelSearch(long)` and generation-carrying entry points. Every in-repo caller is updated in this change;
an out-of-tree caller must port its cancellation the same way.

## Capabilities

### New Capabilities

<!-- None. -->

### Modified Capabilities

- `search-cancellation`: the requirement "Search can be cancelled" is rewritten to the generation model
  (every scenario name kept), and the per-slot registry is added as a requirement of its own.
- `location-search-api`: one added requirement, the generation-carrying location search entry points and the
  convenience path that manages a request itself.
- `poi-search-api`: one added requirement, the generation-carrying POI search entry point.
- `location-search-ui`: one added requirement, the overlay cancels exactly the request it started.

## Impact

Affected files and modules:

- `libosmscout-client-java/src/search_breaker.h` — new dependency-free header: the slot registry,
  generations and the abort predicate (live-set membership).
- `libosmscout-client-java/src/OSMScoutClient.cpp` — the registry and its `osmscout::Breaker` adapter, the
  generation on every search entry point, the per-database abort poll of the free-text pass, the empty
  result of an aborted search, and the diagnostics line.
- `libosmscout-client-java/java/com/framstag/libosmscout/client/OSMScoutClient.java` — `beginSearch`,
  `cancelSearch(long)`, the `SEARCH_SLOT_*` constants, the generation-carrying declarations and the
  convenience overloads; the parameterless cancel and the boolean-cancel overload are removed.
- `JavaScout/src/main/java/com/framstag/libosmscout/SearchOverlay.java` — the place search begins its
  request, the Cancel button cancels that generation.
- `JavaScout/src/main/java/com/framstag/libosmscout/FavLocationDialog.java` — the favourite picker's search
  uses the convenience overload (the three-value form it used is gone).
- `JavaScout/src/test/java/com/framstag/libosmscout/client/OSMScoutClientAdminRegionScopeTest.java` — the
  handle-scoped search begins the place slot's request, runs the generation-carrying overload and cancels
  its generation in a `finally`.
- `JavaScout/src/test/java/com/framstag/libosmscout/client/SearchReproTest.java` — the region-name searches
  use the convenience overload (the boolean-cancel form is gone).
- `Tests/src/SearchBreakerTest.cpp` — new host test for the registry.
- `Tests/CMakeLists.txt`, `Tests/meson.build` — register the test in both build systems.

No Java declaration is added for the bridge's `searchLocationByForm`, which master does not declare either
(TODO.md item 310: a JNI function without a Java declaration); its C++ parameter list gains the generation
like the other search entry points, so the two sides agree once it is declared. No database, style or file
format change, so no `FileFormatVersion.md` version bump applies. No new dependency.
