# Proposal

## Why

A turn-by-turn instruction reaches Java with a distance but no time. The route description the bridge
already walks carries a time for every node - it is what the "[1.2 km, 5 min]" summary suffix is computed
from - so the estimate exists and is thrown away at the bridge. A client that shows "in 300 m" can
therefore not say how long that is, and cannot estimate when the manoeuvre will be reached without
re-deriving the times from a speed it does not have.

## What Changes

- A turn-by-turn instruction carries the estimated time of its own segment, in seconds, alongside the
  distance. The time comes from the route description's per-node times and is the same per-step time the
  description itself reports.
- The full instruction list and the next instruction both carry it, so a client can show a time for the
  next manoeuvre and for every step of the route.
- A segment without a time in the route description reports zero rather than failing, so a client can
  treat zero as "unknown".
- **BREAKING** for Java code that constructs the full `RouteInstruction` constructor directly: the
  constructor gains the per-step time as a parameter, between the distance and the turn type. The
  short constructor (distance, turn type, street, description, short description) keeps its signature and
  reports an unknown time.

## Capabilities

### New Capabilities
<!-- None: the change extends an existing capability. -->

### Modified Capabilities
- `turn-by-turn-instructions`: the Java `RouteInstruction` data class gains the per-step time field, and
  the contract states what that time is for the full instruction list and for the next instruction.

## Impact

Affected files and modules:

- `libosmscout-client-java/src/OSMScoutClient.cpp` — the bridge's instruction struct gains the time, the
  collector derives the per-step time from consecutive node times, and the constructor lookup and the
  object construction pass it through.
- `libosmscout-client-java/java/com/framstag/libosmscout/client/RouteInstruction.java` — the field, the
  full constructor parameter, the short constructor's default and the text representation.
- `JavaScout/src/test/java/com/framstag/libosmscout/client/RouteInstructionTest.java` — new test for the
  data class contract.

No change to the favorites or map data formats, the type config, any database file or a build-system
file, so no `FileFormatVersion.md` version bump applies. No new dependency. `TurnType`, the navigation
listener callbacks and their default implementations are untouched.

This change is re-derived from the downstream branch, not cherry-picked: the source commit mixes three
topics, and the other two (the database-aware admin region search scope and the brand/operator POI
fields) have already landed upstream.
