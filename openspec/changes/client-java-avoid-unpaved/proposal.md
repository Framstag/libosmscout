# Proposal

## Why

`RoutingProfile.avoidUnpaved` is accepted by the JNI bridge and changes nothing: `CreateRoutingProfile`
answers it with a log line and builds a profile whose every road type has one flat speed for all grades.
The profile therefore prices a solid road and a soft track identically, and a caller asking to avoid unpaved
roads silently gets a route that uses them. The shipped `routing-profile` requirement "RoutingProfile
bridged through JNI" already says the avoid flags are passed to the routing parameter, so the capability is
declared and not delivered.

The Android app itself passes the flag as `false` today, so nothing user-visible breaks; the "avoid tolls"
neighbour shows what a declared-but-inert restriction looks like when it does reach a user. The routing
index already carries the road grade (`ObjectVariantData` has `grade`), so the capability can be delivered
without touching the importer, the type configuration or any stored database.

## What Changes

- A routing profile created with `avoidUnpaved` SHALL price every routable type of its vehicle with a
  per-grade speed table instead of one speed per type: grade 1 keeps the type's speed, and the price of a
  kilometre rises strictly towards grade 5 while staying finite.
- The table SHALL be applied after the vehicle parametrization, so the junction penalty and the raised cost
  limit that the parametrization sets stay in place and a route that needs a high-grade way is still
  calculated instead of reported as not found.
- A degraded type SHALL keep a floor speed, so avoiding an unpaved way never means being unable to route.
- A profile created without the flag SHALL keep one speed for all grades, unchanged.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `routing-profile`: a profile created with `avoidUnpaved` prices each of its vehicle's routable types by
  grade instead of by one flat speed. The new requirement is added; the existing "RoutingProfile bridged
  through JNI" behaviour is unchanged.

## Impact

Affected files and modules (`libosmscout-client-java/`):

- `libosmscout-client-java/src/OSMScoutClient.cpp` — the car speed map and the resolved vehicle speed
  hoisted out of the vehicle switch, the `ApplyAvoidUnpavedGradeSpeeds` pass, and the replaced log-only
  `avoidUnpaved` branch.
- `libosmscout-client-java/src/routing_profile_speed.h` — the dependency-free per-grade table and its floor
  (`kAvoidUnpavedGradeFactor`, `kAvoidUnpavedMinSpeedKmh`, `AvoidUnpavedGradeSpeedsKmh`), so the arithmetic
  can be pinned on the host.
- `Tests/src/RoutingProfileSpeedTest.cpp` — the added cases for the table, the strict ordering and the floor.

Not affected: the importer, the type configuration, the routing index format, and every stored database —
the grade is already in the index and already read by the profile's cost function. No new dependency, no JNI
signature change (the flag is read with `GetFieldID`/`GetBooleanField`), and no file format or type
configuration change, so no `FileFormatVersion.md` version bump applies. The Java API does not change:
`RoutingProfile`'s fields and constructors stay as they are.

Out of scope: the `avoidTolls` handling of the same bridge, which is covered by its own change.
