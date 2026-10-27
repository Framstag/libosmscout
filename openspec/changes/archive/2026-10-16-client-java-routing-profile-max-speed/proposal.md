# Proposal

## Why

A Java caller can choose the vehicle and the avoid flags of a routing profile, and nothing else. The
vehicle's speed is whatever the bridge hardcodes: a car is parametrized with the fixed per-road-type speed
map and a 120 km/h ceiling, a bicycle with a flat 30 km/h and a pedestrian with a flat 10 km/h. A client
that knows the vehicle it actually routes for — a slow cargo bike, a lorry-limited car, a hiker on a
mountain route — has no way to say so, so its durations and its chosen route are those of the wrong vehicle,
and a caller cannot even correct a bicycle speed the user configured in the app.

## What Changes

- `RoutingProfile` SHALL carry the vehicle's usual maximum speed in km/h as `double maxSpeedKmh`, with `0`
  meaning "use the routing engine's built-in profile for the vehicle" (car 120, bicycle 30, pedestrian 10).
- The class SHALL offer the two constructor arities that name the speed (vehicle-plus-speed, and
  vehicle-plus-avoid-flags-plus-speed) while every earlier arity keeps compiling and sets `0`.
- The JNI bridge SHALL read the field and parametrize the created profile with it: for a car it is a ceiling
  on top of the unchanged per-road-type speed map, for bicycle and pedestrian it is the flat speed of every
  routable way type.
- A value the engine cannot use (`0`, non-finite, below 1 km/h or above 300 km/h) SHALL select the vehicle's
  built-in default rather than failing the calculation, and a profile class that does not declare the field
  (an older caller) SHALL behave the same way, with the failed lookup's pending exception cleared.
- The log line that reports the created profile SHALL name the applied speed, so a caller can see which
  speed was actually used.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `routing-profile`: a `RoutingProfile` gains the vehicle's maximum speed, the Java data class gains the
  constructor arities that name it, and the JNI bridge parametrizes the routing profile with it.

## Impact

Affected files and modules:

- `libosmscout-client-java/java/com/framstag/libosmscout/client/RoutingProfile.java` — the field, the
  documented meaning of `0`, the vehicle-plus-speed constructor and the full constructor's fifth parameter;
  the no-arg and vehicle-only constructors and the avoid-flags constructor keep their signatures and pass `0`.
- `libosmscout-client-java/src/OSMScoutClient.cpp` — the `maxSpeedKmh` field lookup in the profile-taking
  route call, the maximum-speed resolution that maps an unusable value to the vehicle's built-in default,
  `CreateRoutingProfile`'s new parameter and its three `Parametrize*` calls, the two lambdas that must carry
  the value into the background thread, and the profile log lines.
- `libosmscout-client-java/src/routing_profile_speed.h` — a new dependency-free helper holding the
  "usable speed or built-in default" decision, so it can be unit-tested on the host.
- `Tests/src/RoutingProfileSpeedTest.cpp` and its registration in `Tests/CMakeLists.txt` and
  `Tests/meson.build`.
- `JavaScout/src/test/java/com/framstag/libosmscout/client/RoutingProfileTest.java` — the constructor cases
  for the speed and the defaults.

No database, map, style or file format change, so no `FileFormatVersion.md` version bump applies. No new
dependency. No JNI signature change: the `RoutingProfile` field is read with `GetFieldID`/`GetDoubleField`,
not passed as a parameter.

Out of scope: the legacy `calculateRouteWithObjectsAsync`/`calculateRouteAsync` path takes no
`RoutingProfile` at all and builds its own car-only speed map with a hardcoded 120 km/h ceiling. It is not
touched here — giving that path a profile is its own change.

Consumers: a caller of `calculateRouteAsync(…, RoutingProfile, …)` that names a speed gets that speed's
durations and route choices.
