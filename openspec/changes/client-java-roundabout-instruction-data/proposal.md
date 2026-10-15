# Proposal

## Why

A roundabout manoeuvre reaches Java as text and a turn type, and the two quantities a caller needs are not
in it. The exit *taken* is only in the English sentence the bridge composes ("Take exit 3 onto Hauptstrasse",
"Exit 3"), and the ring's circulation direction is in no field and in no text at all: `route.oss`-styled
icons aside, a consumer cannot tell a clockwise ring from a counter-clockwise one, cannot pick the correct
exit diagram, and has to parse a translated-by-nobody English string to learn which exit is meant.
libosmscout itself has both as structure — `RoundaboutLeaveDescription::GetExitCount()` (the ordinal of the
exit taken: `RoutePostprocessor::roundaboutCrossingCounter` counts the exits passed) and
`IsClockwise()` on both the enter and the leave description — and the bridge drops them on the floor.

## What Changes

- A Java `RouteInstruction` SHALL carry the roundabout manoeuvre as structure: `roundaboutExitNumber` (the
  1-based ordinal of the exit taken, 0 if the instruction is not a roundabout exit), `roundaboutClockwise`
  (the ring's circulation direction, `true` by default), and `nextNextRoundaboutExitNumber` (the same
  ordinal for the "next next" hint's own manoeuvre, 0 if absent or not a roundabout exit).
- The bridge SHALL populate them from `RoundaboutEnterDescription::IsClockwise()` and
  `RoundaboutLeaveDescription::GetExitCount()`/`IsClockwise()`.
- The instruction's existing text fields (`description`, `shortDescription`) SHALL NOT change, so a consumer
  that reads only strings observes no difference.
- The next-next hint's ordinal SHALL be its own field: the hint's roundabout can be a different ring from
  the one the current instruction leaves.
- The full constructor SHALL take the three values, and the two shorter constructor arities SHALL keep older
  Java callers compiling with the "no roundabout" defaults.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `turn-by-turn-instructions`: a Java `RouteInstruction` gains the roundabout manoeuvre as structure (exit
  ordinal, circulation direction, and the next-next hint's own ordinal) and keeps a constructor arity that
  does not name them.

## Impact

Affected files and modules:

- `libosmscout-client-java/java/com/framstag/libosmscout/client/RouteInstruction.java` — the three fields,
  the full constructor's three trailing parameters, and the two shorter overloads that keep older callers
  compiling.
- `libosmscout-client-java/src/OSMScoutClient.cpp` — the `JavaRouteInstruction` struct's three fields, the
  `OnRoundaboutEnter`/`OnRoundaboutLeave` callbacks that set them, the next-next hint that copies the
  following manoeuvre's ordinal, and the `NewObject` call and `instructionCtor` descriptor that pass them to
  Java.
- `JavaScout/src/test/java/com/framstag/libosmscout/client/RouteInstructionTest.java` — the constructor
  cases for the new values and for the defaults of the shorter arities.

No database, map, style or file format change, so no `FileFormatVersion.md` version bump applies. No new
dependency. The JNI entry point signatures are unchanged: only the `RouteInstruction` constructor descriptor
the bridge looks up gains three characters (`IZI`), which is what the Java class in the same change declares.

Consumers: a client that renders a roundabout manoeuvre (the Android app's instruction card) can pick the
exit diagram and the circulation direction without parsing the description text.
