# Proposal

## Why

A calculated route's published length is not a length of that route. It is the start/target air-line estimate
the router computes for its cost limit and its progress denominator, so it under-counts: measured against the
polyline the same call publishes, it read 0.748× on a ~70 km route, 0.795× on a 20 km one and 0.552× on a
1.5 km one. A caller therefore shows a route length that neither matches the drawn geometry nor the step list
it displays next to it.

## What Changes

- A calculated route's published length SHALL be a length of that route and SHALL NOT be the start/target
  air-line estimate the router uses for its cost limit and its progress denominator.
- When the route's description produced an own total — the cumulative distance at its last node, i.e. the
  sum of its steps — the published length SHALL be that total, so a step list and the published total agree
  by construction. (The per-step legs published by the companion change
  `client-java-route-instruction-metrics` are that same quantity.)
- When the description produced no length, the published length SHALL be the length of the route geometry the
  same call publishes, and the route SHALL still report a usable length rather than nothing.
- The route's estimated duration SHALL be derived from the published length.
- The router's own under-count SHALL NOT be addressed here; it is a defect of the routing side and needs its
  own change.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `route-calculation`: the length a calculated route reports becomes a length of that route, with the
  description's own total first and the published geometry as the fallback.

## Impact

Affected files and modules:

- `libosmscout-client-java/src/OSMScoutClient.cpp` — both route-building paths (calculate and async
  calculate), the description branch that takes the description's own total, the polyline fallback, and the
  success log that named the air-line estimate as a length.
- `libosmscout-client-java/src/route_length.h` — the dependency-free helper that decides the published
  length, with a host test under `Tests/`.

No database, map, style or file format change, so no `FileFormatVersion.md` version bump applies. No new
dependency. No JNI signature change. No Java source change: the fields keep their names and types, only the
value of `distance` changes (and `duration` follows it).

Depends on `client-java-route-instruction-metrics`: the description's own total is the sum of the per-step
legs that change publishes. When that change is not applied, the description's total is still preferred, and
the geometry fallback applies unchanged.

Consumers: every caller of a calculated route; a route that previously reported an under-counted length
reports a larger, geometry-consistent one.
