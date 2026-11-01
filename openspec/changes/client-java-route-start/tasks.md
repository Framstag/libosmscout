# Tasks

## 1. The start rule in both route-building paths (spec: route-description)

- [x] 1.1 Add the dependency-free helper `libosmscout-client-java/src/route_start.h`
  (`naviveylin::RouteStartApproachMeters`, `ShouldPrependRouteStart`, `RouteStartNodeDistanceMeters`, the snap
  tolerance constant), following the existing `route_length.h` shape. Verify: the header includes no
  libosmscout header and is covered by `Tests/src/RouteStartTest.cpp`.
- [x] 1.2 In `libosmscout-client-java/src/OSMScoutClient.cpp`, apply the rule in
  `calculateRouteWithObjectsWithProfile`: declare `startApproachMeters` beside `totalDistance`, and after the
  description is postprocessed move the first description node to the requested position and add the approach
  to every later node's cumulative distance. Verify: the block is guarded by `!Nodes().empty()` and only runs
  when the approach exceeds the tolerance.
- [x] 1.3 Apply the same rule in the second route-building path, `calculateRouteWithObjectsAsync`, so the two
  copies of the pipeline cannot drift. Verify: the same block appears in both functions.
- [x] 1.4 Make the published polyline lead with the requested start when the start was snapped: one extra
  leading vertex, the routed vertices shifted by one, no extra vertex otherwise. Verify: `prependStart` is
  `startApproachMeters > 0.0` and the arrays are sized and filled accordingly in both functions.

## 2. Diagnostics (spec: route-description)

- [x] 2.1 Log the decision once per snapped start with the approach in metres and the tolerance, never a
  coordinate. Verify: the log line names metres, the tolerance and the decision only.

## 3. Verification (spec: route-description)

- [x] 3.1 Cover the start rule with a host test. The Java route path needs a routable database and this machine
  has none (`maps/` holds format v26 while the library expects v27), so the rule was factored into
  `libosmscout-client-java/src/route_start.h` and covered by `Tests/src/RouteStartTest.cpp`. Verify: the test
  reports the approach of a snapped and an on-network start, the strict tolerance boundary, the non-finite
  case and the node-distance offset.
- [x] 3.2 Register the test in `Tests/CMakeLists.txt` and `Tests/meson.build` beside the other
  dependency-free helper tests. Verify: `cmake --build build --target RouteStartTest` compiles and links.
- [x] 3.3 Build the JNI translation unit that carries the hunk:
  `cmake --build build --target osmscout_client_java` compiles and links `libosmscout_client_java.so` with no
  error from `OSMScoutClient.cpp`.
- [x] 3.4 Run `bash scripts/check-jni-signatures.sh` and verify the Java native declarations still match. The
  change adds no JNI function and no `NewObject` descriptor, so the checked count is unchanged.
- [x] 3.5 Run `openspec validate client-java-route-start --strict` and verify the change validates.

## 4. Documentation and change hygiene

- [x] 4.1 Record in `verification.md` that the Java route path could not be exercised (no routable database)
  and that the JNI part is compile-verified plus the signature check.
- [x] 4.2 Verify the diff touches only `libosmscout-client-java/src/route_start.h`,
  `libosmscout-client-java/src/OSMScoutClient.cpp`, `Tests/src/RouteStartTest.cpp`, `Tests/CMakeLists.txt`,
  `Tests/meson.build`, plus this change's own directory.
