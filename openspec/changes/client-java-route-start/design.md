# Design

## Context

See `proposal.md` — Why. The relevant current state:

- `libosmscout-client-java/src/OSMScoutClient.cpp`: both route-building paths snap the requested start with
  `ResolveRoutePosition`, which returns the routable node nearest the coordinate; the coordinate itself is not
  kept past that call. The description is transformed from the route data, so its first node is the snapped
  node, and `TransformRouteDataToPoints` publishes the routed polyline, whose first vertex is that same node.
- The published polyline is built from `pointsResult.GetPoints()->points` after the job; its vertex arrays are
  the only place the caller-supplied `start` of the JNI function is still in scope next to the routed points.
- Both entry points are separate copies of the same pipeline (`AGENTS.md`: a change to one belongs in both),
  so the rule is factored into one header applied twice rather than written out twice.

## Goals / Non-Goals

**Goals:**

- Make the published route begin at the requested start: its polyline, its description's first node, its
  per-step cumulative distances and its total all speak of one start.
- Leave the routed geometry and the step list otherwise untouched.
- Publish an on-network start unchanged, with no duplicate leading vertex.

**Non-Goals:**

- Fixing the router's snapping (a routing-side concern; the snap is what routing uses to find the node).
- Re-routing from the requested coordinate along real ways; the approach is the straight segment the caller
  drew to the network, not a road.
- Changing the route geometry's later vertices, the description text or the per-step values.
- Adding a new field: the existing geometry arrays and distance values are corrected.

## Decisions

**D1 — The published route begins at the requested start.**
When the approach exceeds the snap tolerance, the description's first node takes the requested position and the
published polyline gains the requested start as its leading vertex in front of the routed points.
Alternatives:
- *Leave the geometry at the snapped node and only change the description*: the drawn line and the first step
  would disagree, which is the class of defect this change removes.
- *Snap only the description, keep the polyline*: a leading step from a position the polyline does not reach.
Chosen because the polyline is the geometry the caller draws and measures, so it has to lead with the start.

**D2 — The approach is counted once, in the later distances and the total.**
Every description node after the first carries the approach in its cumulative distance; the first node keeps
zero. The first leg's delta, the per-step sums and the description total therefore all include the approach by
construction, and the published total (which the companion change `client-java-route-length` takes from the
description) agrees with the drawn line.
Alternatives:
- *Add the approach to the total only*: the per-step legs would not sum to the total.
- *Add it to the first node's distance too*: step 0 would own a leg that no later delta accounts for.
Chosen because it keeps the published numbers internally consistent.

**D3 — A great-circle approach with a strict one-metre tolerance decides the prepend.**
The approach is the haversine length on a sphere, and it is prepended only when it is strictly greater than the
snap tolerance. A non-finite coordinate yields an approach of zero, so an unusable start never invents a
segment.
Alternatives:
- *Use the library's ellipsoidal distance*: the caller measures the drawn polyline with it, but the helper is
  deliberately dependency-free so it can be host-tested; the two differ by far less than the tolerance.
- *Prepend on any non-zero approach*: a start within metres of the node would get a near-zero segment and a
  duplicate-leading-vertex artefact.
Chosen because the tolerance is a decision bound, the geometry is unchanged for a start the router already
treated as on-network, and the rule stays testable without a database.

## Sequence diagram

```
JNI entry (start,dest)          routing service                 description                 polyline
     |                              |                              |                          |
     | ResolveRoutePosition(start) -| snaps start to a node        |                          |
     | CalculateRoute ------------->|                              |                          |
     | TransformRouteDataToRouteDescription --------------------->| first node = snapped     |
     |                                                             |                          |
     | approach = great-circle(start, first node)                                                |
     | approach > tolerance?                                                                     |
     |   yes: first node <- start;  later nodes' distances += approach                           |
     |        startApproachMeters = approach -----------------------------------------------------> leading vertex = start
     |   no:  description left untouched --------------------------------------------------------> routed vertices only
     |---------------------------------------------------------------------------------------------> RouteEntry
```

## Risks / Trade-offs

- *A drawn route can now leave the network for its first segment* → it is the straight segment the caller asked
  to depart from; the exact routed geometry, order and step list are otherwise unchanged.
- *The published total and the polyline grow by the approach* → that is the distance the caller draws and the
  description now covers, so the numbers stay consistent with each other.
- *The approach is spherical while the drawn length is ellipsoidal* → the difference is far below the
  tolerance's own margin; the metres the helper returns are the metres the published numbers use.
- *A missing or empty description* → the block is guarded by `!Nodes().empty()` and the polyline prepend is
  driven by `startApproachMeters`, so nothing is indexed into an empty list.

## Migration Plan

Geometry-and-values-only change to the published route: no JNI signature, no constructor, no type and no data
change. Rollback is a revert of the commits, which restores the snapped-node start.

## Open Questions

- Whether the router should offer a routed approach along real ways instead of the straight segment: a
  routing-side capability, deferrable, no caller asked for it.
