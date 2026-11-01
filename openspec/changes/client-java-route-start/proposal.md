# Proposal

## Why

The Java bridge's two route entry points use the caller's start coordinate only to find a routable node and
then discard it: `ResolveRoutePosition` snaps the coordinate, `TransformRouteDataToPoints` publishes the
snapped node's geometry, and the description's first node is that same node. A start that lies off the
network — a station, a POI, a coordinate pasted into the search field, a GPS fix at the edge of the map data —
therefore publishes the polyline, the description total and the first step tens to hundreds of metres from
where the user asked to depart (measured on Andorra: 80 m for a hillside start, 57 m for a search result's
object position).

## What Changes

- A calculated route's published route SHALL begin at the requested start. When the requested start lies
  farther from the snapped routable node than the snap tolerance, the description's first node SHALL take the
  requested position, the approach from it to the snapped node SHALL become the polyline's first segment, and
  every later description node's cumulative distance and the published total SHALL include the approach once.
- A requested start within the snap tolerance SHALL publish the routed geometry unchanged, with no duplicate
  leading vertex.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `route-description`: a calculated route publishes its geometry and its description from the requested start
  position, with the approach to the snapped node counted once.

## Impact

Affected files and modules:

- `libosmscout-client-java/src/OSMScoutClient.cpp` — both route-building paths
  (`calculateRouteWithObjectsWithProfile` and `calculateRouteWithObjectsAsync`), each applying the same rule so
  the two copies of the pipeline cannot drift.
- `libosmscout-client-java/src/route_start.h` — the dependency-free helper that measures the approach, decides
  whether to prepend it and offsets a description node's cumulative distance, with a host test under `Tests/`.

No database, map, style or file format change, so no `FileFormatVersion.md` version bump applies. No new
dependency. No JNI signature change and no `NewObject` descriptor change: this unit adds no constructor
parameter and no native method. No Java source change: the published arrays simply gain one leading vertex and
the description's distances carry the approach.

Consumers: every caller of a calculated route whose start lies off the network; a route that previously
started at the snapped node now starts where the caller asked and reports the drawn geometry's length.
