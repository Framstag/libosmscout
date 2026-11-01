# Proposal

## Why

The route calculation still walks the route description a second time to build a Java-only prose record:
description lines, per-instruction positions and per-step arrays on `RouteEntry`, plus a whole
`RouteInstruction` class, its builder and its JNI projection. The client library now publishes the same
manoeuvre facts as structured route steps (`RouteStep`, `route-step-model`), and both the route list and the
live guidance read steps, so the second walk produces nothing a client needs and every field it fills has a
structured equivalent.

Keeping it is not free: the walk runs on every calculation, the five `RouteEntry` arrays can disagree with
the steps, and the `RouteInstruction` class is a second, prose-shaped public surface that a client has to
migrate away from separately. Retiring it now, while the step model is the single source of manoeuvre data,
removes the duplicate and the risk of drift.

## What Changes

- The route calculation no longer walks the description a second time: `RouteEntry` carries the structured
  `RouteStep[] steps` only, and the published length is read from the last step's cumulative distance.
- `RouteEntry` loses `descriptions`, `instructionLats`, `instructionLons`, `instructionDistances` and
  `instructionTimes`.
- The Java `RouteInstruction` class, its JNI projection and its field tests are deleted. `TurnType` stays —
  the step model carries it.
- **BREAKING** for a Java consumer that reads those fields or the `RouteInstruction` class; every value they
  need is on the step model.

## Impact

- Affected capabilities: `turn-by-turn-instructions`, `route-calculation`, `route-visualization`.
- Affected code: `libosmscout-client-java/src/OSMScoutClient.cpp`, `RouteEntry.java`, the `RouteInstruction`
  class and its registrations in the CMake/Meson Java source lists, the JavaScout AGENTS notes and the
  JavaScout route-step test.
- Not affected: the C++ route step model, `TurnType`, the route geometry, and the live-guidance step
  callbacks.
