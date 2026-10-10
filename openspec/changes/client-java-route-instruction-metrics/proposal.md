# Proposal

## Why

A Java route's step list shows values that belong to the wrong thing. A step's distance and time are derived
from the route node preceding its manoeuvre rather than from the instruction preceding it, so on a city
route a row can read "14 m" and "2 s" and the rows never add up to the route's distance or duration. An
instruction carries no length of its own leg, so a client cannot relate a remaining distance to the leg it
lies on, and a calculated route publishes no per-instruction position, so a step cannot be placed on the
map.

## What Changes

- Every instruction SHALL carry the length of the leg that ends at its manoeuvre, alongside its travel time.
- An instruction's travel time SHALL be the travel time of that leg, measured from the instruction preceding
  it, and SHALL NOT be the time of a single geometry edge between two route nodes.
- The per-step values of a route description SHALL be the values of that step's own leg, so the steps sum to
  the route's distance and duration.
- A calculated route SHALL publish, per instruction line, the manoeuvre's position, the leg's distance and
  the leg's time, index-aligned with the instruction lines of the description.
- The next instruction ahead of the current position SHALL report the remaining time of its leg, consistent
  with the remaining distance it already reports.
- The first instruction of a list that was rebuilt from a position SHALL report unknown leg values rather
  than a cumulative value that belongs to another leg.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `route-calculation`: the Java `RouteEntry` gains per-instruction positions and per-step leg values.
- `turn-by-turn-instructions`: a Java `RouteInstruction` gains its leg's length, and its time becomes the
  leg's own travel time (remaining time for the next instruction).

## Impact

Affected files and modules:

- `libosmscout-client-java/java/com/framstag/libosmscout/client/RouteEntry.java` — the four per-instruction
  arrays and their documentation.
- `libosmscout-client-java/java/com/framstag/libosmscout/client/RouteInstruction.java` — the leg length as a
  field and as a full-constructor parameter; the short constructor keeps its signature.
- `libosmscout-client-java/src/OSMScoutClient.cpp` — both route-building paths' description callbacks, the
  instruction collector, the next-instruction builder and the marshalling of the arrays.
- `JavaScout/src/test/java/com/framstag/libosmscout/client/RouteInstructionTest.java` — the constructor test
  follows the new full-constructor argument list.

No database, map or style file format change, so no `FileFormatVersion.md` version bump applies. No new
dependency. No JNI signature change (the value types are built field by field). The short
`RouteInstruction` constructor keeps its signature, so a Java caller that uses it is unaffected.

Depends on `client-java-route-length`: the published route length is computed from the per-step legs this
change introduces, so this change is applied first or the two are reviewed together.
