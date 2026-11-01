# Proposal

## Why

`openDatabases` publishes the database-set change to the DB thread's job queue and returns; that job clears
and repopulates the instance list the routers read. A route calculation issued in between therefore found an
empty list and reported `kRouteFailureRoutingDataUnavailable` — the same kind a database that genuinely cannot
be opened reports. A caller could not tell a retry-clearable condition (the open has not been registered yet)
from a map-data one (the database cannot be opened and a different or re-downloaded map is needed).

## What Changes

- The not-yet-loaded case SHALL get its own failure kind, `7` (`KIND_DATABASE_NOT_LOADED` /
  `kRouteFailureDatabaseNotLoaded`), at both route-calculation entry points
  (`calculateRouteWithObjectsWithProfile` and `calculateRouteWithObjectsAsync`).
- The branches where the routing service cannot be opened SHALL keep the unusable-routing-data kind.
- The `RouteCallback` interface SHALL carry the new constant, and the `RouteFailureCause` vocabulary and its
  translation SHALL gain the matching member, so the number stays identical on both sides of the boundary.

## Capabilities

### New Capabilities

<!-- None. -->

### Modified Capabilities

- `routing-failure-reporting`: one added requirement — a calculation that outruns the database open reports
  the not-yet-loaded kind, not unusable routing data.
- `route-calculation`: the `RouteCallback Java interface` requirement's constant list gains
  `KIND_DATABASE_NOT_LOADED` (every existing scenario kept).

## Impact

Affected files and modules:

- `libosmscout-client-java/src/route_failure_kind.h` — the `kRouteFailureDatabaseNotLoaded` constant, the
  `RouteFailureCause::DatabaseNotLoaded` member and its `RouteFailureKind` branch, and why the not-yet-loaded
  list has a kind of its own.
- `libosmscout-client-java/java/com/framstag/libosmscout/client/RouteCallback.java` — the
  `KIND_DATABASE_NOT_LOADED` constant.
- `libosmscout-client-java/src/OSMScoutClient.cpp` — the two `databases.empty()` branches (the
  `...WithProfile...` and `...Async...` entry points) report the new kind and say "yet"; the routing-service
  branches keep `kRouteFailureRoutingDataUnavailable`.
- `Tests/src/RouteFailureKindTest.cpp` — the cause list, the constant assertions and the mapping assertion
  gain the new member.

No database, map, style or file format change, so no `FileFormatVersion.md` version bump applies. No new
dependency. No JNI signature change and no `NewObject` descriptor change: this unit adds no native method and
no constructor parameter. The route path that reports the kind needs a v27 routing database and a live route
callback, so it is compile-verified plus the host test; it is not asserted by a route test here.

Reference: `f61ff8789` (`client-java: a calculation that outruns the database open says so`), change
`2026-10-29-client-java-database-not-loaded-kind` (archived).
