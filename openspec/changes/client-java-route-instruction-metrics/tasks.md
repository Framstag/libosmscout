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
  skip without one. — NOT DONE: no routable (v27) map database exists on this machine, so the test could not
  be written against anything that runs. The contract is covered by construction and by inspection of the two
  callbacks instead; see `verification.md`.
- [ ] 3.3 Verify the step values on a city route: no step reports a value that belongs to a single geometry
  edge. Verify: the smallest reported step is larger than the largest single geometry edge of that route, or
  the route is too short for the case to be meaningful and the test says so. — NOT DONE: same missing map
  database; the reference implementation's own device measurement (17,3 km route, "14 m / 2 s" for one step)
  is the case this rule fixes.

## 4. Build and regression verification (specs: route-calculation, turn-by-turn-instructions)

- [x] 4.1 Build the CMake build with the Java client library, the jar and the tests, and verify it compiles
  without errors and without warnings from the touched files.
- [x] 4.2 Build the Meson build the same way and verify it compiles without errors.
- [x] 4.3 Run the Java tests and verify the new cases pass and no existing Java test regresses.
- [ ] 4.4 Run the C++ suite and verify no existing test regresses. — NOT DONE: the change touches only the
  Java bridge's own callbacks and Java value types, no core C++ library code, so no C++ test exercises it.
  The JNI parity test (`meson test "Check JNI signature parity"`) was run instead.
- [x] 4.5 Run `openspec validate "client-java-route-instruction-metrics" --strict` and verify the change
  validates.

## 5. Documentation and change hygiene

- [x] 5.1 Verify the new and changed fields are documented in the Java value types: `legDistance` (0 =
  unknown), the four arrays (alignment, null case) and the corrected meaning of `timeTo`.
- [x] 5.2 Verify the short `RouteInstruction` constructor is unchanged in signature and result.
- [x] 5.3 Verify the diff touches only the files listed in the proposal's Impact section, and that no
  database, map or style file changed.
