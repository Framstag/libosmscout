# Design

## Context

See `proposal.md` — Why. The relevant current state:

- `libosmscout-client-java/java/com/framstag/libosmscout/client/RoutingProfile.java`: a data class with
  `Vehicle vehicle` and the three avoid flags, and three constructors (no-arg, vehicle-only,
  vehicle-plus-avoid-flags). Nothing names a speed.
- `libosmscout-client-java/src/OSMScoutClient.cpp`, `CreateRoutingProfile(...)`: the vehicle's speed is
  hardcoded — a car gets the fixed per-road-type speed map topped at `120.0`, a bicycle a flat `30.0` and a
  pedestrian a flat `10.0` — and the three `osmscout::log.Warn()` lines report only that a profile was
  created, not what it was parametrized with.
- The profile-taking route call (`calculateRouteWithObjectsWithProfile`, the JNI function for
  `calculateRouteAsync(..., RoutingProfile, ...)`): reads the profile's fields with
  `GetFieldID`/`GetBooleanField`, captures them into the background thread that does the routing, and builds
  the routing profile in three places — the `RoutingProfileBuilder` lambda that the routing service opens
  with, the second `RoutingProfileBuilder` inside the same call, and the `profiles.push_back(...)` of the
  route post-processing pass.
- The library already interprets a speed per vehicle: `ParametrizeForCar(tc, speedMap, maxSpeed)` applies
  `maxSpeed` as a ceiling on top of the per-road-type map, `ParametrizeForBicycle(tc, speed)` and
  `ParametrizeForFoot(tc, speed)` use `speed` for every routable way type.
- The legacy `calculateRouteWithObjectsAsync`/`calculateRouteAsync` JNI function takes no `RoutingProfile`
  at all; it builds its own car-only speed map with a hardcoded `120.0` in two places.

## Goals / Non-Goals

**Goals:**

- Let a caller state the vehicle's usual maximum speed, and have the engine parametrize the profile with it.
- Keep every existing constructor arity compiling, and keep its meaning: no speed named means the engine's
  built-in profile for the vehicle.
- A value the engine cannot use selects the vehicle's built-in default instead of failing the calculation.
- Make the applied speed observable in the log, so a caller can see which speed was actually used.

**Non-Goals:**

- Giving the legacy `calculateRouteWithObjectsAsync`/`calculateRouteAsync` path a profile: it takes none and
  is left byte-identical. That is its own change.
- Changing the three built-in defaults (car 120, bicycle 30, pedestrian 10) or the per-road-type speed map.
- Validating the speed in Java: the bridge is where the engine's contract is applied, and a profile object
  can come from any caller of the class.
- Any file format, database or style change, so no `FileFormatVersion.md` bump applies.

## Decisions

**D1 — The value means what the library's parametrization already means.**
For a car the value is a ceiling on top of the unchanged per-road-type speed map; for bicycle and pedestrian
it is the flat speed of every routable way type. No new interpretation is invented in the bridge.
Alternatives:
- *A flat speed for every vehicle, including the car*: would replace the road-type map (motorway 110,
  residential 20) with one number and change every existing car route.
- *A fraction/percentage of the built-in profile*: keeps the map for the car, but a caller that knows "this
  lorry does 90" would have to do the arithmetic, and the bicycle's flat speed has no profile to scale.
Chosen because the library's three `Parametrize*` calls already define the two readings, and `0` can
therefore stay the one sentinel for "built-in".

**D2 — An unusable value selects the built-in default, it never fails the calculation.**
"Usable" is a finite value in 1..300 km/h. `0` is the documented sentinel for the built-in profile, a
negative or non-finite value is not a speed, and a value above 300 km/h is above every road vehicle — as a
flat bicycle or foot speed it would make the vehicle arbitrarily fast. Outside that band the request is
ignored and the vehicle's built-in default is applied.
Alternatives:
- *Reject the calculation with an error*: the bridge's only error channel on this path reports routing
  failures through `onError`; a refused route for a speed the caller mis-typed is a worse outcome than the
  vehicle's own default.
- *Clamp to the band bound*: a caller that asked for 500 km/h would silently get 300, which is a speed nobody
  asked for, and a caller that asked for 0.5 km/h would get 1.
- *Pass the value through unvalidated*: a NaN ceiling or a flat 500 km/h speed would corrupt the cost
  function and the reported duration.
Chosen because the requirement is that a bad value degrades to the built-in profile, and because the band is
the one place the decision is written down.

**D3 — The decision lives in a dependency-free header and is unit-tested on the host.**
`libosmscout-client-java/src/routing_profile_speed.h` holds
`naviveylin::ResolveProfileMaxSpeedKmh(requested, builtIn)`; `OSMScoutClient.cpp` calls it, and
`Tests/src/RoutingProfileSpeedTest.cpp` covers the decision without JNI, without a database and without a
map. The header follows `frame_pixel_layout.h` and `route_length.h`.
Alternatives:
- *A `static` function in the translation unit* (what the reference commit on the branch did): the band and
  the fallback would only be reachable through a JNI call that needs a routable database, and this checkout
  has none (`maps/` holds format v26 while the library expects v27, and importing one is out of scope); the
  reference's `ResolveMaxSpeedKmh` would be untestable here.
- *A Java-side check*: leaves the read path and the fallback untested, and does not help a caller that
  bypasses the check.
Chosen because factoring the decision out is what makes it verifiable on this machine, and because both
call-path helpers of the same bridge are already written that way.

**D4 — The speed is read from the existing Java field, not added as a JNI parameter.**
`env->GetFieldID(profileCls, "maxSpeedKmh", "D")`, then `GetDoubleField`. The native declaration of
`calculateRouteAsync` (and its descriptor, checked by `scripts/check-jni-signatures.sh`) is unchanged, and a
profile class that does not declare the field — an older caller — keeps working.
Alternatives:
- *A new native method or an extra JNI parameter*: changes the JNI signature, the Java declaration and the
  signature check, for a value that is already part of the profile object.
- *A `RouteEntry`-style accessor pair*: the other fields of the profile are read the same direct way; a
  getter only for the speed would be a second spelling of the same read.
Chosen because it keeps the change additive on both sides.

**D5 — A failed field lookup clears its pending exception.**
`GetFieldID` returns null and leaves a pending exception (`NoSuchFieldError`). While one is pending the next
JNI call aborts the process, which would make the null guard unreachable. The bridge therefore calls
`env->ExceptionClear()` in that branch and carries on with the built-in default.
Alternatives:
- *Ignore the null and continue*: the pending exception aborts on the next JNI call inside the same function
  (the global-ref creation, the callback lookup), i.e. a crash instead of a defaulted speed.
- *Fail the call*: an older profile class is not an error.
Chosen because it is the same hazard the roundabout-instruction change addresses for method lookups, and it
is what makes the "older caller" scenario true.

**D6 — Every profile this call builds gets the value.**
The value is captured into the background thread and passed into both `RoutingProfileBuilder` lambdas and
into the post-processing `profiles.push_back(CreateRoutingProfile(...))`.
Alternatives:
- *Only the routing service's builder*: the second builder (the point transform) would open with a different
  profile, and the post-processing pass would parametrize the post-processors' profile with the built-in
  speed — the description and the geometry could then disagree about the same route.
Chosen because the three sites build the same logical profile, and a value that reaches only one of them is
a value the caller cannot rely on.

**D7 — The legacy async path is out of scope.**
`calculateRouteWithObjectsAsync`/`calculateRouteAsync` takes no `RoutingProfile`; its own car-only speed map
with a hardcoded `120.0` (master lines ~6781 and ~6926) is left byte-identical.
Alternatives:
- *Retrofit the profile into that path*: a wider change to a different JNI entry point and its own Java
  declaration; `proposal.md` records it as its own change.
Chosen because the reference on the branch did not touch it either, and mixing the two would widen this PR.

## Sequence diagram

```
Java RoutingProfile               JNI calculateRouteWithObjectsWithProfile            engine
     |                                        |                                       |
     | maxSpeedKmh (0 = built-in) ----------->| GetFieldID("maxSpeedKmh","D")         |
     |                                        |   null? -> ExceptionClear(), 0.0      |
     |                                        | capture into the routing thread       |
     |                                        |  RoutingProfileBuilder #1 ----------->| CreateRoutingProfile(..., maxSpeedKmh)
     |                                        |  RoutingProfileBuilder #2 ----------->|     ResolveProfileMaxSpeedKmh(
     |                                        |  post-processing profiles.push_back ->|       requested, built-in for vehicle)
     |                                        |                                       |   car: ceiling on the speed map
     |                                        |                                       |   bicycle/foot: flat speed
     |                                        | log line names the applied speed <-----|
```

## Risks / Trade-offs

- *A caller that passed garbage now gets the built-in speed instead of an error* → documented, and the log
  line names the applied speed, so the substitution is observable.
- *The band 1..300 excludes a legitimate request* (a pedestrian slower than 1 km/h, a value above 300) →
  both are outside what a vehicle profile can use; the built-in default is applied and the log says so.
- *The log line text changes* → it is a log line; a consumer that parsed the old text sees a longer line, and
  the new one is what makes the applied speed observable (spec scenario "The applied speed is observable").
- *The JNI read itself is not covered by a host test* → the decision is (D3); the read path needs a routable
  database, which this machine does not have. Recorded as NOT RUN in `tasks.md` and `verification.md`.
- *A caller that relied on `RoutingProfile`'s binary layout* → the class gains a field; Java callers
  recompile against the client library, which is how the class is consumed.

## Migration Plan

Additive on both sides: a field with a documented default, two new constructor arities, and a trailing
parameter of `CreateRoutingProfile` with a default value. No serialization, no file format, no data change.
An older caller whose profile class lacks the field behaves exactly as before. Rollback is a revert of the
commits, which restores the three hardcoded speeds.

## Open Questions

- Whether a caller should be able to ask for "the built-in speed of some other vehicle" rather than a
  number: deferrable, no caller asked for it, and it would need a second sentinel or an enum.
- Whether an unusable value should be reported back to the caller (an `onError` note or a status on the
  result) instead of only logged: deferrable, and it is a question about the route call's error model.
- Whether Java should validate the range in the constructor so the caller learns early: deferrable; the
  bridge keeps its fallback either way, because a profile object can be built by any path.
