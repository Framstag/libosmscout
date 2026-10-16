# Verification

Change: `client-java-routing-profile-max-speed` — a routing profile carries the vehicle's maximum speed.

## 1. What was implemented

- `libosmscout-client-java/java/com/framstag/libosmscout/client/RoutingProfile.java`: new
  `public final double maxSpeedKmh` (documented: the vehicle's usual maximum speed in km/h, `0` for the
  routing engine's built-in profile — car 120, bicycle 30, pedestrian 10 — a ceiling on top of the car's
  per-road-type speed map and the flat speed for bicycle and pedestrian); new `(Vehicle, double)`
  constructor; the full constructor takes `maxSpeedKmh` as its fifth parameter. The no-arg and vehicle-only
  constructors and the avoid-flags constructor keep their signatures and pass `0.0`.
- `libosmscout-client-java/src/routing_profile_speed.h` (new): `naviveylin::ResolveProfileMaxSpeedKmh(
  requestedKmh, builtInKmh)` — the requested speed when it is a finite value in 1..300 km/h, else the
  vehicle's built-in default. Dependency-free (no JNI, no libosmscout includes), following
  `frame_pixel_layout.h` and `route_length.h`.
- `libosmscout-client-java/src/OSMScoutClient.cpp`: `CreateRoutingProfile(...)` gains a trailing
  `double maxSpeedKmh = 0.0` and resolves the applied speed per vehicle through the helper, passing it to
  `ParametrizeForCar(*tc, speedMap, carMaxSpeed)` (the per-road-type speed map is unchanged),
  `ParametrizeForBicycle(*tc, bicycleMaxSpeed)` and `ParametrizeForFoot(*tc, footMaxSpeed)`; the three log
  lines now name the applied speed in km/h. `calculateRouteWithObjectsWithProfile` reads the profile's
  `maxSpeedKmh` with `GetFieldID(profileCls, "maxSpeedKmh", "D")`/`GetDoubleField`, clears the failed
  lookup's pending exception when the id is null, and carries the value into the background thread's capture
  list, both `RoutingProfileBuilder` lambdas and the post-processing `profiles.push_back(
  CreateRoutingProfile(...))`.
- The legacy `calculateRouteWithObjectsAsync`/`calculateRouteAsync` path is untouched (it takes no
  `RoutingProfile`; its two hardcoded `120.0` car speed maps are byte-identical).

## 2. CMake build and test (exact commands and results)

- `cmake --build build --target osmscout_client_java java_jar RoutingProfileSpeedTest` — CMake re-ran
  (the new test target was picked up), then: `[2/8] Building CXX object
  Tests/CMakeFiles/RoutingProfileSpeedTest.dir/src/RoutingProfileSpeedTest.cpp.o`,
  `[4/8] Linking CXX executable Tests/RoutingProfileSpeedTest-1.1.1`, `[6/8] Building CXX object
  libosmscout-client-java/CMakeFiles/osmscout_client_java.dir/src/OSMScoutClient.cpp.o`,
  `[7/8] Linking CXX shared library libosmscout-client-java/libosmscout_client_java.so`,
  `[3/8] Packaging Java classes into jar`. Exit 0; no warning from `OSMScoutClient.cpp`,
  `RoutingProfileSpeedTest.cpp` or `RoutingProfile.java` (the only CMake warnings are the project's
  pre-existing `FindGlib2`/`FindGobject`/`FindPango` author warnings).
- `ctest --test-dir build -R RoutingProfileSpeedTest --output-on-failure` — `1/1 Test #68:
  RoutingProfileSpeedTest ... Passed 0.02 sec`, `100% tests passed out of 1`.
- `ctest --test-dir build -j 8 -E Performance --output-on-failure` — `100% tests passed out of 117`, no
  failures (the new `RoutingProfileSpeedTest` is test #64 in that run and passed).
- `bash scripts/check-jni-signatures.sh` — `JNI signatures match: 57 native declarations checked, 5 dead JNI
  functions reported above.` Exit 0. The five reported dead JNI functions (`getMaxSpeedAt`,
  `searchLocationByForm`, `reloadBasemap`, `getAddressAt`, `getDatabaseBoundingBox`) are pre-existing and
  unrelated: this change adds no native declaration, changes no Java native method and reads the new value as
  a Java field, so no signature changes.

## 3. JavaScout test (exact commands and results)

- `mvn -o -q install:install-file -Dfile=build/libosmscout-client-java/libosmscoutclientjava.jar
  -DgroupId=net.sf.libosmscout -DartifactId=libosmscout-client-java -Dversion=1.0-SNAPSHOT -Dpackaging=jar`
  — no output (success); the freshly built jar is what the test compiles and runs against.
- `mvn -o -f JavaScout/pom.xml test -Dtest=RoutingProfileTest
  -Dnative.lib.dir=.../build/libosmscout-client-java` — `Tests run: 5, Failures: 0, Errors: 0, Skipped: 0
  -- in com.framstag.libosmscout.client.RoutingProfileTest`, `BUILD SUCCESS`. The Java test is pure Java and
  needs no native library.

## 4. Meson build and test (exact commands and results)

- `meson setup --reconfigure debug` — regenerated (the new test target has to be picked up).
- `meson compile -C debug RoutingProfileSpeedTest` — `[1/2] Compiling C++ object
  Tests/RoutingProfileSpeedTest.p/src_RoutingProfileSpeedTest.cpp.o`, `[2/2] Linking target
  Tests/RoutingProfileSpeedTest`.
- `meson test -C debug "Check routing profile speed" --print-errorlogs` — `1/1
  libosmscout:Check routing profile speed OK  0.01s`, `Ok: 1`, `Fail: 0`.
- `meson compile -C debug osmscout_client_java libosmscoutclientjava` —
  `[7/8] Compiling C++ object
  libosmscout-client-java/src/libosmscout_client_java.so.1.1.1.p/OSMScoutClient.cpp.o`,
  `[8/8] Linking target libosmscout-client-java/src/libosmscout_client_java.so.1.1.1`,
  `[6/8] Creating JAR libosmscout-client-java/java/libosmscoutclientjava.jar`. Exit 0.
- **Not run: the full Meson test suite and a bare `meson compile -C debug`.** Only the targets this change
  touches were compiled; the shared Meson build directory holds binaries of other branches, so a whole-suite
  run there would report failures that are not this change's. The CMake suite in section 2 is the suite that
  was run.
- The 69 javadoc warnings the Meson JAR step prints all come from `BasemapManager.java`,
  `CurrentRoadInfo.java` and `PoiCategories.java` (pre-existing); `RoutingProfile.java` produces none, which
  was confirmed by forcing its recompile (`touch` + `meson compile -C debug libosmscoutclientjava`) and
  reading the full warning list.

## 5. OpenSpec

- `openspec validate client-java-routing-profile-max-speed --strict` — `Change
  'client-java-routing-profile-max-speed' is valid`.

## 6. What could not be run, and why

- **Not run then (2026-10-16: superseded — see "Host verification 2026-10-16 (map-dependent items)" below): an end-to-end route calculation that observes the applied speed.** Every route through this JNI
  function needs a routable database, and this machine has none: `maps/` holds format v26 databases while
  the library expects v27, and importing one is out of scope. The log line that names the applied speed and
  the per-road-type ceiling therefore cannot be observed on a real route here. The decision that selects the
  speed is covered by `RoutingProfileSpeedTest` (section 2), the Java constructors by `RoutingProfileTest`
  (section 3), and the read path (`GetFieldID`/`GetDoubleField` plus `ExceptionClear`) by inspection.
- **Not run: the router's own speed behaviour against the per-road-type map with a caller's ceiling.** Same
  reason — a routable database is needed; the unchanged speed map is verified by inspection of the diff (the
  map's entries are untouched and only the ceiling argument changed).
- `scripts/format-check.sh` is red repo-wide on this checkout even for pristine master files, so it was not
  used as a gate; the touched code matches the surrounding style by hand.
- No unrelated flake was worked around or fixed. `PerformanceTest` was excluded from the suite run as
  documented (UI libraries leak on exit).

## 7. Diff hygiene

`git status --short` after the commit lists only the pre-existing untracked entries (`maps/repository/` and
the other `openspec/changes/client-java-*` directories); `build/`, `debug/` and the other branches' files are
never staged. Nothing was staged with `git add -A`, `git add .` or `git commit -a`.

## Host verification 2026-10-16 (map-dependent items)

Task 3.6 is now ticked: the applied speed is observed end to end on a real route of the v27 database
`maps/nordrhein-westfalen`.

```bash
# compiled once into build/loop-probes-2026-10-16
java -Djava.library.path=build/libosmscout-client-java \
  -cp build/libosmscout-client-java/libosmscoutclientjava.jar:build/loop-probes-2026-10-16 \
  RoutingSpeedProbe maps/nordrhein-westfalen stylesheets
```

Route 51.514227/7.465279 -> 51.520/7.480, calculated twice through
`calculateRouteWithObjectsAsync(..., RoutingProfile, ...)`. The bridge logs the applied speed at WARN level,
which the default osmscout logger writes to stderr, so the log line and its route are in one transcript:

```
  created car routing profile (max speed 77.00000 km/h)     <- RoutingProfile(Vehicle.CAR, 77.0)
  created car routing profile (max speed 77.00000 km/h)
  created car routing profile (max speed 77.00000 km/h)
PROBE: requested-77 vehicle=CAR maxSpeedKmh=77.0 -> route distance=1699.3774322313407m duration=122.35517512065651s waypoints=113
  created car routing profile (max speed 120.00000 km/h)    <- RoutingProfile(Vehicle.CAR) (built-in)
  created car routing profile (max speed 120.00000 km/h)
  created car routing profile (max speed 120.00000 km/h)
PROBE: built-in vehicle=CAR maxSpeedKmh=0.0 -> route distance=1699.3774322313407m duration=122.35517512065651s waypoints=113
```

The requested `77` and the built-in `120` each reach `CreateRoutingProfile` (three times per route, one per
profile site), on a route the routing service actually returned (113 waypoints). **What stayed unverified:**
the ceiling does not change this city route's published duration (both `122.355` s), because the road types it
uses stay below the ceiling; the speed's effect on the route is therefore observed only through the log line,
not through a changed duration. The bicycle/foot log lines were not run (no bicycle/foot route was calculated).

Log: `evidence/routing-speed-probe-nrw-2026-10-16.log` (command, UTC timestamp, exit 0, revision
`06b197a35`). Probe source: `evidence/RoutingSpeedProbe.java`.
