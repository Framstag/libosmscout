# Design

## Context

`RouteStep` (`route-step-model`) is one record per described route node, carrying that node's manoeuvre data
— direction, motorway junction and destination, roundabout exit ordinal and circulation, way type, max speed,
lane, per-step distance and time — as values, not prose. The route list and the live guidance already read it.
The description-line walk and the `RouteInstruction` class predate it and duplicate a subset of it.

## D1 — The description walk is removed, not kept alongside

The walk and the step collection read the same `RouteDescription`. Keeping both means two producers of the
same facts in one translation unit, which is exactly the disagreement the step model exists to prevent. The
walk is removed; `steps` is the only manoeuvre output of a calculated route.

## D2 — The published length comes from the last step

`RouteEntry.distance` was the description walk's own cumulative total, with the geometry length as fallback.
The step list carries each node's cumulative `distanceFromStart`, so the route's length is the largest of
those — the description's last node, read without a second walk. `LastStepDistanceMeters` is a small helper
next to `route_length.h`; the geometry fallback is kept.

## D3 — The prose class is deleted, `TurnType` stays

`RouteInstruction` exists only to carry the walk's output. With the walk gone it has no producer. `TurnType`
is still the step model's turn value and stays.

## Risks

- A Java consumer that read `descriptions` or the five arrays loses them. This is the intended BREAKING
  change; the values remain available on the steps.
- The end-to-end Java route path cannot be exercised on a host without a usable map database; the change is
  compile-verified and the host step tests are run.
