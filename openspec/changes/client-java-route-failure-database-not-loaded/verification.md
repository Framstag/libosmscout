# Verification

Change: `client-java-route-failure-database-not-loaded` — a calculation that outruns the database open says so.

## 1. What was implemented

- `libosmscout-client-java/src/route_failure_kind.h`: `kRouteFailureDatabaseNotLoaded=7`, the
  `RouteFailureCause::DatabaseNotLoaded` member beside `RoutingDataUnavailable`, its branch in
  `RouteFailureKind`, and the header paragraph on why the not-yet-loaded list has a kind of its own.
- `libosmscout-client-java/java/com/framstag/libosmscout/client/RouteCallback.java`: the
  `KIND_DATABASE_NOT_LOADED = 7` constant, documented as distinct from `KIND_ROUTING_DATA_UNAVAILABLE`.
- `libosmscout-client-java/src/OSMScoutClient.cpp`: the two `databases.empty()` branches
  (`calculateRouteWithObjectsWithProfile` and `calculateRouteWithObjectsAsync`) now report
  `kRouteFailureDatabaseNotLoaded` and say "No databases loaded yet"; the branches where the routing service
  cannot be opened keep `kRouteFailureRoutingDataUnavailable`.
- `Tests/src/RouteFailureKindTest.cpp`: the cause array grows to eight, the constant and mapping assertions
  gain the new kind, and the "these seven values" comment is corrected to eight.

## 2. CMake build and test (exact commands and results)

- `cmake --build build --target RouteFailureKindTest` — `[1/4] Building CXX object
  Tests/CMakeFiles/RouteFailureKindTest.dir/Unity/unity_0_cxx.cxx.o`, `[2/4] Linking CXX executable
  Tests/RouteFailureKindTest-1.1.1`, `[3/4] Creating executable symlink Tests/RouteFailureKindTest`, exit 0.
- `ctest -R RouteFailureKindTest --output-on-failure` (cwd `build`) — `1/1 Test #74: RouteFailureKindTest
  ... Passed 0.00 sec`, `100% tests passed out of 1`.
- `cmake --build build --target osmscout_client_java` — `[1/3] Compiling Java sources and generating JNI
  headers`, `[2/3] Building CXX object
  libosmscout-client-java/CMakeFiles/osmscout_client_java.dir/Unity/unity_0_cxx.cxx.o`, `[3/3] Linking CXX
  shared library libosmscout-client-java/libosmscout_client_java.so`, exit 0, no error from
  `OSMScoutClient.cpp`.
- `bash scripts/check-jni-signatures.sh` — `JNI signatures match: 60 native declarations checked, 5 dead JNI
  functions reported above.` The five reported dead JNI functions are pre-existing and unrelated; this change
  adds no JNI function, changes no Java declaration and changes no `NewObject` descriptor.

## 3. OpenSpec

- `openspec validate client-java-route-failure-database-not-loaded --strict` — `Change
  'client-java-route-failure-database-not-loaded' is valid`.

The reference commit `f61ff8789` put its delta under `route-calculation`, modifying the `RouteCallback Java
interface` requirement (its constant list plus a new scenario). On master the failure-kind reporting lives in
`routing-failure-reporting`, so the reporting requirement is attached there (an `## ADDED Requirements`
entry), and `route-calculation`'s `RouteCallback Java interface` requirement is modified only to add
`KIND_DATABASE_NOT_LOADED` to its constant list (every existing scenario kept). Both requirement texts stay
under the validator's limits.

## 4. What could not be run, and why

- **Not run: the Java route path itself.** Every route-measuring Java test needs a routable database; `maps/`
  holds format v26 databases while the library expects v27, and there is no routable database on this machine
  (and importing one is out of scope). The JNI part is therefore compile-verified
  (`cmake --build build --target osmscout_client_java`) plus the JNI signature check
  (`scripts/check-jni-signatures.sh`); the kind mapping is exercised by the host `RouteFailureKindTest`.
- **Not run: the full C++ suite.** `build/` is shared with other branches and holds stale binaries, so a bare
  `ctest` would report failures of other branches; only the exact target `RouteFailureKindTest` was rebuilt
  immediately before it ran.

## 5. Diff hygiene

The staged set is exactly `libosmscout-client-java/src/route_failure_kind.h`,
`libosmscout-client-java/java/com/framstag/libosmscout/client/RouteCallback.java`,
`libosmscout-client-java/src/OSMScoutClient.cpp`, `Tests/src/RouteFailureKindTest.cpp` and this change's own
directory. `build/`, `build-meson/`, `maps/` and other `openspec/changes/*` directories are never staged.
