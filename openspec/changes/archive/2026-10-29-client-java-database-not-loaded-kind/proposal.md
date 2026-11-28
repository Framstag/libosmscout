# Proposal — client-java-database-not-loaded-kind

## Why

`openDatabases` publishes the database-set change to the DB thread's job queue and returns. The job clears and
repopulates the instance list the routers read, so a route calculation that starts in between finds an empty
list and reports `KIND_ROUTING_DATA_UNAVAILABLE` — the same kind a database that cannot be opened reports. The
two facts differ for a caller: one is cleared by trying again, the other by changing the map data.

Measured 2026-10-13 on a 416-point sweep of Dortmund's old town
(`nordrhein-westfalen-27-20260820-0826`, `emulator-5556`): the first two points failed with
`calculateRouteAsync: no databases loaded`, the 414 after them did not, and the sweep's own client had called
`openDatabases` immediately before the run.

## What Changes

- One additional failure kind reports *the database list was not populated yet*.
- The two `databases.empty()` branches of the JNI route calculation report that kind; the kind for genuinely
  unusable routing data keeps the branches where the routing service cannot be opened.
- The Java mirror of the kinds carries the new constant.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `route-calculation` — the `RouteCallback` failure kinds gain the not-yet-loaded kind and the scenario that
  reports it.

## Impact

- `libosmscout-client-java/src/route_failure_kind.h` — the kind constant, the cause and its mapping.
- `libosmscout-client-java/src/OSMScoutClient.cpp` — the two `databases.empty()` branches of
  `calculateRouteWithObjectsWithProfile` and `calculateRouteWithObjectsAsync`.
- `libosmscout-client-java/java/com/framstag/libosmscout/client/RouteCallback.java` — the Java mirror.
- `Tests/src/RouteFailureKindTest.cpp` — the mapping row and the mirrored number.
