# client-java-avoid-unpaved

## Why

`RoutingProfile.avoidUnpaved` is accepted by the JNI bridge and changes nothing: `CreateRoutingProfile`
answers it with a log line and builds a profile whose every road type has one flat speed for all grades.
The profile therefore prices a solid road and a soft track identically, and a caller asking to avoid
unpaved roads silently gets a route that uses them. The shipped `routing-profile` requirement "RoutingProfile
bridged through JNI" already says the avoid flags are passed to the routing parameter, so the capability is
declared and not delivered.

The Android app itself passes the flag as `false` today, so nothing user-visible breaks; JavaScout's
"Avoid tolls" neighbour shows what a declared-but-inert restriction looks like when it does reach a user.
The routing index already carries the road grade (`ObjectVariantData` has `grade`), so the capability can be
delivered without touching the importer, the type configuration or any stored database.

## What Changes

- A routing profile created with `avoidUnpaved` prices every routable type of its vehicle with a per-grade
  speed table instead of one speed per type: grade 1 keeps the type's speed, and the price of a kilometre
  rises strictly towards grade 5 while staying finite.
- A profile created without the flag keeps one speed for all grades, unchanged.

## Impact

- Affected capability: `routing-profile`.
- Affected code: `libosmscout-client-java/src/OSMScoutClient.cpp` (profile creation), plus the change's
  probe under `openspec/changes/client-java-avoid-unpaved/evidence/`.
- Not affected: the importer, the type configuration, the routing index format, and every stored database —
  the grade is already in the index and already read by the profile's cost function.
- The Java API does not change: `RoutingProfile`'s fields and constructors stay as they are.
