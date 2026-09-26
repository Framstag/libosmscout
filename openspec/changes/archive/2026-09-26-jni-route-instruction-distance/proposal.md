# Proposal

## Why

A Java client that navigates a route is told the wrong distance for the next manoeuvre and gets no usable "arrive" instruction at all. Two defects in the bridge from the core route description to Java: the remaining distance to the next manoeuvre is derived from the straight-line distance between the current fix and the route node behind it, although the position agent already reports how far along the current route segment the fix has progressed (and `PositionAgent::Position::abscissa` documents itself as existing for exactly that consumer); and the arrival instruction carries the distance 0 instead of the distance of its own route node, which makes it indistinguishable from "at the start of the route", so the bridge's own distance-ordered search for the next instruction skips it and a client is left without an arrival instruction after the last manoeuvre.

## What Changes

- The next route instruction's remaining distance SHALL be derived from how far the position has progressed along its current route segment, so a segment that folds back or a fix that lies beside the route no longer overstates the distance already travelled. When the position reports no progress for its segment, the previous straight-line estimate SHALL be kept, and the reported distance SHALL never be negative.
- Every instruction SHALL carry the distance of its own route node from the start of the route, including the arrival instruction, so a client can distinguish an arrival that is still ahead from the route's start and so the arrival instruction is found and delivered after the last manoeuvre.
- Instruction builders that do not consume the position's progress SHALL keep working unchanged, so this is a compatible extension of the instruction-builder contract rather than a break for existing users of the navigation agents.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `turn-by-turn-instructions`: the distances of the next instruction and of the arrival instruction become well defined; the arrival instruction stops reporting 0.

## Impact

Affected modules and files:

- `libosmscout/include/osmscout/navigation/RouteInstructionAgent.h` — passes the position's progress to the instruction builder when the builder can consume it.
- `libosmscout/include/osmscout/navigation/PositionAgent.h` — consumed, not changed: it already carries the progress of the snapped position.
- `libosmscout-client-java/src/OSMScoutClient.cpp` — the Java instruction builder uses the reported progress for the remaining distance and reports the arrival instruction's own distance.
- `Tests/src/NavigationAgentTest.cpp` — extends the existing synthetic navigation-agent tests, which need neither a map nor a GPS device.
- `JavaScout/src/test/java/com/framstag/libosmscout/client/` — a navigation test for the Java-visible distances, in the style of the existing navigation tests (routable map provided by the environment).
- Consumers: every user of `RouteInstructionAgent` with a progress-aware instruction builder (the Java client today); the Java client's next-turn distance and arrival instruction change.
- No new dependency, no database or file-format change, no change to the rendering path.
