# Tasks

## 1. The speed a routing profile is parametrized with (spec: routing-profile)

- [x] 1.1 In `libosmscout-client-java/java/com/framstag/libosmscout/client/RoutingProfile.java`, add
  `public final double maxSpeedKmh` documented as the vehicle's usual maximum speed in km/h with `0` meaning
  the routing engine's built-in profile for the vehicle (car 120, bicycle 30, pedestrian 10), add the
  `(Vehicle, double)` constructor and give the full constructor `maxSpeedKmh` as its fifth parameter; the
  no-arg, vehicle-only and avoid-flags constructors keep their signatures and pass `0.0`. Verify: the class
  offers the five arities of the requirement, the three older ones delegate with `0.0`, and no existing
  caller has to change.
- [x] 1.2 In `libosmscout-client-java/src/OSMScoutClient.cpp`, add the trailing `double maxSpeedKmh = 0.0`
  parameter to `CreateRoutingProfile(...)`, resolve the applied speed per vehicle (car 120, bicycle 30,
  pedestrian 10 built-in) and pass it to `ParametrizeForCar(*tc, speedMap, carMaxSpeed)` /
  `ParametrizeForBicycle` / `ParametrizeForFoot`; name the applied speed in the three log lines. Verify: the
  car's per-road-type speed map is unchanged and the resolved value is the ceiling handed to
  `ParametrizeForCar`, the bicycle and foot profiles are parametrized with the resolved flat speed, and each
  log line names the speed in km/h.
- [x] 1.3 Factor the "usable speed or built-in default" decision into the new dependency-free header
  `libosmscout-client-java/src/routing_profile_speed.h` (`naviveylin::ResolveProfileMaxSpeedKmh`), with the
  reason for the 1..300 band and for falling back instead of failing, and use it from
  `CreateRoutingProfile(...)` instead of duplicating the condition. Verify: the header includes no JNI and no
  libosmscout header, `OSMScoutClient.cpp` includes it, and the condition is written in exactly one place.

## 2. The bridge reads the field (spec: routing-profile)

- [x] 2.1 In `calculateRouteWithObjectsWithProfile` (the JNI function behind
  `calculateRouteAsync(..., RoutingProfile, ...)`), read the profile's `maxSpeedKmh` with
  `env->GetFieldID(profileCls, "maxSpeedKmh", "D")` and `GetDoubleField`; a null field id means an older
  profile class, so clear the failed lookup's pending exception and keep the built-in default. Verify: the
  read is guarded, `env->ExceptionClear()` runs in the null branch, and no JNI signature changed (the field
  is read, not passed).
- [x] 2.2 Carry the value into the background thread's capture list and into the `RoutingProfileBuilder`
  lambda the routing service opens with, the second `RoutingProfileBuilder` lambda of the same call, and the
  `profiles.push_back(CreateRoutingProfile(...))` of the post-processing path. Verify: all three profile
  sites of that path are parametrized with the same value, and none of them still calls
  `CreateRoutingProfile(tc, vehicle, avoidTolls, avoidFerries, avoidUnpaved)` without it.
- [x] 2.3 Leave the legacy `calculateRouteWithObjectsAsync`/`calculateRouteAsync` path byte-identical: it
  takes no `RoutingProfile` and builds its own car-only speed map with a hardcoded `120.0` in two places.
  Verify: `git diff` contains no hunk in that JNI function, and the inline `profileBuilder` lambda that takes
  no captures is unchanged (see `design.md` D7).

## 3. Verification (spec: routing-profile)

- [x] 3.1 Cover the speed decision with a host test: `Tests/src/RoutingProfileSpeedTest.cpp` asserts the
  applied speed for `0`, a negative value, NaN, +infinity, -infinity, just below 1, just above 300, exactly
  1, exactly 300 and a normal value. Verify: the test compiles and reports the decision for every one of
  those inputs, with the built-in default of the vehicle where the request is unusable.
- [x] 3.2 Register the test in both build systems, mirroring the sibling host tests:
  `Tests/CMakeLists.txt` (`osmscout_test_project(NAME RoutingProfileSpeedTest ... INCLUDES
  ${PROJECT_SOURCE_DIR}/libosmscout-client-java/src)`) and `Tests/meson.build` (the same shape as
  `FramePixelLayoutTest`, including its `test(...)` line). Verify: CMake builds and ctest runs the target in
  the main build, Meson builds it in the Meson build. See `verification.md`.
- [x] 3.3 Cover the Java constructors with `JavaScout/src/test/java/com/framstag/libosmscout/client/
  RoutingProfileTest.java` (JUnit 5, pure Java, no native library): the default and vehicle-only
  constructors report `0.0`; `(Vehicle.BICYCLE, 20.0)` carries the vehicle and the speed with all avoid
  flags false; `(Vehicle.PEDESTRIAN, true, false, true, 4.0)` carries the speed and the flags;
  `(Vehicle.CAR, true, true, true)` reports `0.0`. Verify: the test runs against the freshly installed
  client jar and passes.
- [x] 3.4 Run `openspec validate client-java-routing-profile-max-speed --strict` and verify the change
  validates.
- [x] 3.5 Verify the JNI declaration surface is unchanged: `bash scripts/check-jni-signatures.sh` still
  matches, and the change adds no native declaration.
- [x] 3.6 Verify the applied speed end to end through a real route calculation. RUN 2026-10-16: done on the
  v27 database `maps/nordrhein-westfalen` — log `evidence/routing-speed-probe-nrw-2026-10-16.log`, probe
  source `evidence/RoutingSpeedProbe.java`. A real route (51.514227/7.465279 -> 51.520/7.480) is calculated
  once with `RoutingProfile(Vehicle.CAR, 77.0)` and once with the built-in profile: the bridge logs `created
  car routing profile (max speed 77.00000 km/h)` (three times, once per profile site) for the first and
  `(max speed 120.00000 km/h)` for the second, each followed by `route distance=1699.3774322313407m
  duration=122.35517512065651s waypoints=113`. Still unverified: the applied speed does not change this
  city route's published duration (both report 122.355 s), so the ceiling's effect on the road-type speeds
  is observed only through the log line.

## 4. Documentation and change hygiene

- [x] 4.1 Verify the field's meaning is documented where a consumer reads it: `maxSpeedKmh` states that `0`
  means the built-in profile, which vehicle's default applies, and that the value is a ceiling for a car and
  a flat speed for bicycle and pedestrian.
- [x] 4.2 Verify the diff touches only `libosmscout-client-java/java/com/framstag/libosmscout/client/
  RoutingProfile.java`, `libosmscout-client-java/src/OSMScoutClient.cpp`, the helper
  `libosmscout-client-java/src/routing_profile_speed.h`, the new host test and its registration in
  `Tests/CMakeLists.txt` and `Tests/meson.build`, the Java test, plus this change's own directory. Verify:
  `git status --short` lists nothing else as staged or modified.
- [x] 4.3 Verify the legacy asynchronous path is recorded as out of scope, so it is not silently forgotten:
  `proposal.md` names it, and `design.md` Non-Goals and D7 name it, and no code of it is touched.
