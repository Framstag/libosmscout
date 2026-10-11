# Verification

Change: `client-java-route-length` — the length a calculated route publishes becomes a length of that route.

## 1. What was implemented

`libosmscout-client-java/src/OSMScoutClient.cpp`, both route-building paths
(`calculateRouteWithObjectsWithProfile` and `calculateRouteWithObjectsAsync`):

- `totalDistance = result.GetOverallDistance().AsMeter()` is gone. That figure is the start/target air-line
  estimate the router computes for its cost limit and its progress denominator.
- The description callback keeps the largest cumulative node distance it sees; that is the description's own
  total (the cumulative distance at its last node, i.e. the sum of its steps).
- When no description produced a total, the published length is the great-circle length of the polyline the
  same call publishes, measured with the library's `GetEllipsoidalDistance` over the already built point
  arrays; a polyline of one or zero points yields zero and is never indexed.
- The duration is computed from `totalDistance` after the decision, so it follows the published length.
- The async success log now reads `route OK, air-line estimate=` instead of naming that figure a distance.

The decision itself is a dependency-free helper, `libosmscout-client-java/src/route_length.h`
(`naviveylin::RouteLengthMeters`), following the existing `search_scope.h` /
`admin_region_hierarchy.h` pattern.

## 2. CMake build and test (exact commands and results)

- `cmake --build build --target osmscout_client_java` — `[3/4] Linking CXX shared library
  libosmscout-client-java/libosmscout_client_java.so`, no error, no warning from `OSMScoutClient.cpp`.
- `cmake --build build --target java_jar` — `[1/1] Packaging Java classes into jar`.
- `cmake --build build --target RouteLengthTest` — compiles `Tests/src/RouteLengthTest.cpp` and links
  `Tests/RouteLengthTest-1.1.1`.
- `cd build && ctest -R RouteLengthTest --output-on-failure` — `1/1 Test #63: RouteLengthTest ...
  Passed 0.01 sec`, `100% tests passed out of 1`.
- `./build/Tests/RouteLengthTest` — `All tests passed (6 assertions in 4 test cases)`.
- `bash scripts/check-jni-signatures.sh` — `JNI signatures match: 55 native declarations checked, 6 dead JNI
  functions reported above.` The six reported dead JNI functions are pre-existing and unrelated; this change
  adds no JNI function, changes no Java declaration and changes no signature.

## 3. Meson build and test (exact commands and results)

- `meson setup --reconfigure build-meson` — regenerated (the new test target has to be picked up).
- `meson compile -C build-meson RouteLengthTest` — `[2/2] Linking target Tests/RouteLengthTest`.
- `meson test -C build-meson "Check route length" --print-errorlogs` — `1/1 libosmscout:Check route length
  OK`, `Fail: 0`.
- `meson compile -C build-meson osmscout_client_java libosmscoutclientjava` — links
  `libosmscout-client-java/src/libosmscout_client_java.so.1.1.1` and creates
  `libosmscout-client-java/java/libosmscoutclientjava.jar`, no error from the touched file.

## 4. OpenSpec

- `openspec validate client-java-route-length --strict` — `Change 'client-java-route-length' is valid`.

The requirement text was shortened below the validator's 500-character limit; the sum-of-legs phrasing moved
into the scenario, where it is true standalone: the published total is the cumulative distance at the
description's last node, which is the sum of its steps (and, once `client-java-route-instruction-metrics`
lands, the sum of the per-step leg distances it publishes). Every scenario name of the ADDED requirement is
kept.

## 5. Testing option chosen, and what could not be run

- **Chosen: the dependency-free helper plus a host Catch2 test.** The length rule is a pure choice among one
  description total, one geometry length and zero; factoring it out of the JNI translation unit makes it
  testable on a host with no database, no map and no JNI. The geometry length itself is not re-implemented in
  the helper — the caller measures it with the library's own `GetEllipsoidalDistance`, so the published
  number and the drawn polyline use one formula.
- **Not run: the Java measurement test the original task 3.1 described.** Every route-measuring Java test
  needs a routable database; `maps/` holds format v26 databases while the library expects v27, so no
  routable database can be opened on this machine (and a v27 import is out of scope). Task 3.1 was rewritten
  to the host test that was run.
- **Not run: the Java and C++ suites (task 3.5).** `mvn -o -f JavaScout/pom.xml test
  -Dnative.lib.dir=.../build/libosmscout-client-java` fails to compile its own test sources against the
  `libosmscout-client-java` artifact installed in the local Maven repository:

  ```
  RouteInstructionTest.java:[15,40] no suitable constructor found for RouteInstruction(double,double,
  com.framstag.libosmscout.client.TurnType,java.lang.String,java.lang.String,java.lang.String,double,
  com.framstag.libosmscout.client.TurnType,java.lang.String,java.lang.String)
  ```

  The installed artifact's `RouteInstruction` constructor has a different arity from the working tree — a
  leftover of a sibling branch, unrelated to this change (this change touches no Java file and no
  constructor). The C++ suite was not run either: `build/` is shared with the other branches and holds stale
  binaries, which abort with `undefined symbol` or `malloc.c:3948` unless the exact target is rebuilt
  immediately before running it, so a full `ctest` here would report failures of other branches. Only
  `RouteLengthTest` (rebuilt directly before it ran) was executed.
- Known unrelated flake in the JavaScout suite
  (`OSMScoutClientBasemapConfigTest.testClearingBasemapLookupDirectoryUnloads`) is untouched and not fixed.

## 6. Diff hygiene

`git status --porcelain` before committing lists only the files of this change; `build/`, `build-meson/`,
`build-asan/`, `maps/repository/` and other `openspec/changes/*` directories are never staged.
