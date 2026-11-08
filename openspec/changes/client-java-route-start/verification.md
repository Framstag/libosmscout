# Verification

Change: `client-java-route-start` — the published route starts where the caller asked.

## 1. What was implemented

`libosmscout-client-java/src/OSMScoutClient.cpp`, both route-building paths
(`calculateRouteWithObjectsWithProfile` and `calculateRouteWithObjectsAsync`):

- A `startApproachMeters` value beside `totalDistance`, `0.0` meaning "the start was on the network".
- After the description is postprocessed, the approach from the requested start to the description's first
  (snapped) node is measured; when it exceeds the tolerance, the first description node takes the requested
  position and every later node's cumulative distance carries the approach. The same block is applied in both
  functions so the two copies of the pipeline cannot drift.
- The published polyline gains one leading vertex (the requested start) when the start was snapped; the routed
  vertices are shifted by one and are otherwise unchanged. An on-network start publishes the routed geometry
  with no duplicate leading vertex.
- The decision is logged in metres and the tolerance, never a coordinate.

The rule itself is a dependency-free helper, `libosmscout-client-java/src/route_start.h`
(`naviveylin::RouteStartApproachMeters`, `ShouldPrependRouteStart`, `RouteStartNodeDistanceMeters`,
`kRouteStartSnapToleranceMeters`), following the existing `route_length.h` pattern.

## 2. CMake build and test (exact commands and results)

- `cmake --build build --target RouteStartTest` — `[1/4] Building CXX object
  Tests/CMakeFiles/RouteStartTest.dir/Unity/unity_0_cxx.cxx.o`, `[2/4] Linking CXX executable
  Tests/RouteStartTest-1.1.1`, `[3/4] Creating executable symlink Tests/RouteStartTest`, exit 0.
- `cd build && ctest -R RouteStartTest --output-on-failure` — `1/1 Test #76: RouteStartTest ... Passed 0.00
  sec`, `100% tests passed out of 1`.
- `cmake --build build --target osmscout_client_java` — `[11/12] Building CXX object
  libosmscout-client-java/CMakeFiles/osmscout_client_java.dir/Unity/unity_0_cxx.cxx.o`, `[12/12] Linking CXX
  shared library libosmscout-client-java/libosmscout_client_java.so`, exit 0, no error and no warning from
  `OSMScoutClient.cpp`.
- `bash scripts/check-jni-signatures.sh` — `JNI signatures match: 60 native declarations checked, 5 dead JNI
  functions reported above.` The five reported dead JNI functions are pre-existing and unrelated; this change
  adds no JNI function, changes no Java declaration and changes no `NewObject` descriptor.

## 3. OpenSpec

- `openspec validate client-java-route-start --strict` — `Change 'client-java-route-start' is valid`.

The reference comments cite capability `osmscout-jni`, which does not exist in this repository. The requirement
was mapped onto the real capability `route-description` (no existing requirement covered the start rule, so it
is an `## ADDED Requirements` entry). The requirement description was kept below the validator's
500-character limit; the reference's two-scenario split was kept and its `osmscout-jni` references were
repointed at `route-description`.

## 4. What could not be run, and why

- **Not run: the Java route path itself.** Every route-measuring Java test needs a routable database; `maps/`
  holds format v26 databases while the library expects v27, and there is no routable database on this machine
  (and importing one is out of scope). The JNI part is therefore compile-verified
  (`cmake --build build --target osmscout_client_java`) plus the JNI signature check
  (`scripts/check-jni-signatures.sh`); the start rule is exercised by the host `RouteStartTest`.
- **Not run: the full C++ suite.** `build/` is shared with other branches and holds stale binaries, so a bare
  `ctest` would report failures of other branches; only the exact target `RouteStartTest` was rebuilt
  immediately before it ran.

## 5. Diff hygiene

The staged set is exactly `libosmscout-client-java/src/route_start.h`,
`libosmscout-client-java/src/OSMScoutClient.cpp`, `Tests/src/RouteStartTest.cpp`, `Tests/CMakeLists.txt`,
`Tests/meson.build` and this change's own directory. `Tests/CMakeLists.txt` and `Tests/meson.build` are shared;
only this unit's added `RouteStartTest` lines were staged. `build/`, `build-meson/`, `maps/` and other
`openspec/changes/*` directories are never staged.
