# Proposal

## Why

A route calculation that produced no route reached Java as one English sentence in `onError(String)`. A
caller could therefore neither pick wording for the user's language nor decide whether the failure is
something the user can act on — "you are not on a road network" is actionable, "no connection was found" is
not, and a cancellation is not a failure at all. The reason now exists in the routing library
(`routing-failure-reporting`, the prerequisite of this change); nothing carried it across the bridge.

## What Changes

- `RouteCallback` SHALL carry the failure kinds as `KIND_*` constants and a new
  `onErrorDetail(int kind, String detail, double nearestRoadDistanceMeters)` with a default implementation
  that delegates to `onError(detail)`, so an implementation written before it existed keeps working
  unchanged.
- A kind the interface does not define (a newer native library, or one that knows a cause this version does
  not) SHALL be treated as `KIND_UNKNOWN` by the caller.
- The bridge SHALL translate the routing library's `RoutingError` into the kind through one dependency-free
  function a host test can exercise, and SHALL report the failure's kind, an English detail for a log, and
  how far the failing end of the route is from the nearest usable road.
- The kinds SHALL be numbered identically on both sides of the boundary — the integer is what crosses.
- There SHALL be no "route too long" kind: the router reports one error for a search that exhausted its
  alternatives, and a caller cannot tell from it whether the connection is longer than the profile's cost
  limit allows or the endpoints' parts of the network do not join.

## Capabilities

### New Capabilities

<!-- None. -->

### Modified Capabilities

- `route-calculation`: the `RouteCallback Java interface` requirement is rewritten to carry the failure kind
  (its scenario name kept).
- `routing-failure-reporting`: one added requirement, the kind and the distance the bridge reports for a
  failed calculation (that capability is declared by the change this one is stacked on).

## Impact

Affected files and modules:

- `libosmscout-client-java/src/route_failure_kind.h` — new dependency-free header: the kind constants, the
  bridge's own cause vocabulary and its translation to a kind.
- `libosmscout-client-java/java/com/framstag/libosmscout/client/RouteCallback.java` — the `KIND_*` constants
  and the `onErrorDetail` default method.
- `libosmscout-client-java/src/OSMScoutClient.cpp` — the kind and the nearest-road distance reported at every
  route-failure site, and the `onErrorDetail` method lookup.
- `Tests/src/RouteFailureKindTest.cpp` — new host test for the translation (pure, no database).
- `Tests/CMakeLists.txt`, `Tests/meson.build` — register the test in both build systems.

This change is **stacked on `routing-failure-reason`** (#1905): it reports the `RoutingError` that change
introduces. No file format change, so no `FileFormatVersion.md` version bump applies. No new dependency. No
existing Java method changes signature — `onErrorDetail` is a default method, so an implementor compiles
unchanged.

The failure kind is asserted by a host test; the rendered/route path that reports it needs a v27 routing
database and is not asserted by a test here.
