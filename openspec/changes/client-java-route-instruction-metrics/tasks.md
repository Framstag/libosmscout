# Tasks

## 1. Description callbacks (spec: route-calculation, turn-by-turn-instructions)

- [x] 1.1 In `libosmscout-client-java/src/OSMScoutClient.cpp`, make the description callback advance its
  per-step reference when it emits a line, in both the calculate and the reroute path, so a step's distance
  and time are the leg ending at its manoeuvre. Verify: the step rows sum to the route's distance and
  duration within rounding.
- [x] 1.2 Collect the emitted line's position, leg distance and leg time in the callback. Verify: the four
  per-instruction vectors grow together, one entry per emitted instruction line.
- [x] 1.3 Add the alignment check: publish the four arrays only when they align one-to-one with the
  instruction lines, and drop all four otherwise. Verify: a description that produces no instruction lines
  yields four null arrays and an otherwise unchanged `RouteEntry`.

## 2. Instruction value (spec: turn-by-turn-instructions)

- [x] 2.1 Give `JavaRouteInstruction` a leg-length field and fill it in the collector, one call per emitted
  instruction (`FillLeg`). Verify: the first instruction of a rebuilt list carries 0.0 and not a cumulative
  distance.
- [x] 2.2 Make the next-instruction builder report the remaining time of its leg, interpolated at the
  position with the same progress the remaining distance uses. Verify: the time shrinks as the manoeuvre is
  approached and never contradicts the remaining distance.
- [x] 2.3 Add the field to `libosmscout-client-java/java/com/framstag/libosmscout/client/RouteInstruction.java`
  as a full-constructor parameter, and keep the short constructor's signature by passing 0.0. Verify: the JNI
  constructor descriptor and the `NewObject` argument list follow the Java parameter order.
- [x] 2.4 Add the four arrays to
  `libosmscout-client-java/java/com/framstag/libosmscout/client/RouteEntry.java`, documented, including the
  alignment rule and the null case.

## 3. Tests (spec: route-calculation, turn-by-turn-instructions)

- [x] 3.1 Extend `JavaScout/src/test/java/com/framstag/libosmscout/client/RouteInstructionTest.java` for the
  new full-constructor argument list and the leg length. Verify: the test compiles and passes.
- [ ] 3.2 Add a Java test that reads the per-instruction arrays off a real route: the arrays are index-aligned
  with the instruction lines, their distances and times sum to the route's totals, and a route that cannot be
  aligned reports null arrays. Verify: with a routable map directory the test reports the sums, and reports a
  skip without one. — PROBED 2026-10-16 on the v27 database `maps/nordrhein-westfalen` (log
  `evidence/route-metrics-probe-nrw-2026-10-16.log`, probe `evidence/RouteMetricsProbe.java`): alignment holds
  (array length equals the instruction-line count, 9/9, 17/17, 12/12), the distances sum to `distance`
  exactly, but **the times do not sum to `duration`** (`157.0` vs `122.355` s; `494.0` vs `357.430`; `3742.0`
  vs `6890.579`), because `duration` is the fixed 50 km/h estimate of `OSMScoutClient.cpp:6973`, which is what
  the merged `client-java-route-length` change asks for. The clause is therefore not satisfied and the task
  stays open; the null-array case cannot be forced from Java. See `verification.md`.
- [ ] 3.3 Verify the step values on a city route: no step reports a value that belongs to a single geometry
  edge. Verify: the smallest reported step is larger than the largest single geometry edge of that route, or
  the route is too short for the case to be meaningful and the test says so. — PROBED 2026-10-16: the stated
  check fails on all four routes tried (`21.885` vs `85.726` m; `11.138` vs `171.272`; `52.759` vs `829.774`;
  `11.158` vs `1164.749`). A long straight edge is simply longer than the shortest leg between two manoeuvres,
  so the check does not pin the property; the task stays open. Log and probe as for 3.2.

## 4. Build and regression verification (specs: route-calculation, turn-by-turn-instructions)

- [x] 4.1 Build the CMake build with the Java client library, the jar and the tests, and verify it compiles
  without errors and without warnings from the touched files.
- [x] 4.2 Build the Meson build the same way and verify it compiles without errors.
- [x] 4.3 Run the Java tests and verify the new cases pass and no existing Java test regresses.
- [x] 4.4 Run the C++ suite and verify no existing test regresses. RUN 2026-10-16: the full C++ suite passed
  `100% tests passed out of 158` (exit 0). The note stands that no C++ test exercises this change; the JNI
  parity test (`meson test "Check JNI signature parity"`) was run alongside. Logs and revision:
  `verification.md`, "Host verification 2026-10-16".
- [x] 4.5 Run `openspec validate "client-java-route-instruction-metrics" --strict` and verify the change
  validates.

## 5. Documentation and change hygiene

- [x] 5.1 Verify the new and changed fields are documented in the Java value types: `legDistance` (0 =
  unknown), the four arrays (alignment, null case) and the corrected meaning of `timeTo`.
- [x] 5.2 Verify the short `RouteInstruction` constructor is unchanged in signature and result.
- [x] 5.3 Verify the diff touches only the files listed in the proposal's Impact section, and that no
  database, map or style file changed.
