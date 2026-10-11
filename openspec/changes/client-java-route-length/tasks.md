# Tasks

## 1. Length source in both route-building paths (spec: route-calculation)

- [x] 1.1 In `libosmscout-client-java/src/OSMScoutClient.cpp`, stop reading the router's overall distance into
  the published length on both the calculate and the async calculate path. Verify: no assignment of that
  figure reaches `totalDistance` (the only remaining reads are the async success log, which names it an
  air-line estimate).
- [x] 1.2 Keep the description's own total as the first source. On master the per-step leg arrays do not exist
  yet (`client-java-route-instruction-metrics` adds them), so the total is the cumulative distance at the
  description's last node, which is the sum of its steps. Verify: `totalDistance` is set from the description
  callback's largest cumulative node distance whenever the description produced one.
- [x] 1.3 Add the geometry fallback: when no length was decided and the call publishes more than one geometry
  point, publish the great-circle length of that polyline; otherwise publish zero. Verify: a route without a
  description still reports a length greater than zero, and a single-point route reports zero without
  faulting (the loop is guarded by `count > 1`).
- [x] 1.4 Derive the estimated duration from the published length. Verify: both paths compute the duration
  from `totalDistance` after the length has been decided, so it cannot contradict the length it is published
  with.

## 2. Diagnostics (spec: route-calculation)

- [x] 2.1 Correct the comments and the success log that named the router's figure a route length or an
  accumulated distance. Verify: no remaining text calls that figure a length; it is named as the start/target
  air-line estimate used for the cost limit and the progress denominator.

## 3. Verification (spec: route-calculation)

- [x] 3.1 Cover the length decision with a host test. The Java test the original version of this task asked
  for needs a routable database, and this machine has none (`maps/` holds format v26 while the library
  expects v27), so the decision was factored into the dependency-free
  `libosmscout-client-java/src/route_length.h` and covered by `Tests/src/RouteLengthTest.cpp` instead.
  Verify: the test reports the decision for a description total, a geometry length and neither, and records
  the measured air-line estimate of the ~70 km route as a value the decision can never return.
- [x] 3.2 Verify the step list agrees with the published total where a description exists. The per-step leg
  arrays do not exist standalone, so the agreement is structural: the published total is the cumulative
  distance at the description's last node, which is exactly the distance the description's steps cover (with
  `client-java-route-instruction-metrics`, the per-step legs sum to it). Verify: by inspection of the
  callback and the rewritten requirement; no Java-side summing is asserted because the arrays are not part
  of this change.
- [x] 3.3 Build the CMake build with the Java client library and the jar, and verify it compiles without
  errors and without warnings from the touched file.
- [x] 3.4 Build the Meson build the same way and verify it compiles without errors.
- [ ] 3.5 Run the Java and C++ suites and verify no existing test regresses. NOT RUN: the JavaScout test
  sources do not compile against the `libosmscout-client-java` artifact installed in the local Maven
  repository (its `RouteInstruction` constructor arity differs from the working tree, a leftover of a
  sibling branch), and the shared `build/` directory holds stale binaries of other branches, so neither
  suite can be run meaningfully here. See `verification.md` section 5.
- [x] 3.6 Run `openspec validate "client-java-route-length" --strict` and verify the change validates.

## 4. Documentation and change hygiene

- [x] 4.1 Verify the corrected meaning of the published length is documented where a consumer reads it
  (`RouteEntry.distance` is documented as the total route distance in meters, which now holds), and that the
  router's air-line estimate is documented as what it is where it is still used (the async success log and
  the comments at both call sites).
- [x] 4.2 Verify the diff touches only `libosmscout-client-java/src/OSMScoutClient.cpp`, the helper
  `libosmscout-client-java/src/route_length.h`, the new host test and its registration in
  `Tests/CMakeLists.txt` and `Tests/meson.build`, plus this change's own directory.
- [x] 4.3 Verify the router's own under-count is recorded as out of scope (a note for the backlog), so it is
  not silently forgotten: `design.md` Non-Goals and decision D4 name it, and no routing code is touched.
