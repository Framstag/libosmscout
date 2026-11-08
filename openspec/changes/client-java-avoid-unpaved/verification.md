# Verification — client-java-avoid-unpaved

## What was run

| # | command | result |
|---|---------|--------|
| 1 | `cmake --build build --target osmscout_client_java` | passed — the translation unit compiles and links |
| 2 | `cmake --build build --target RoutingProfileSpeedTest` | passed |
| 3 | `ctest -R RoutingProfileSpeedTest --output-on-failure` (cwd `build`) | passed — 1/1 |
| 4 | `bash scripts/check-jni-signatures.sh` | passed — `JNI signatures match: 60 native declarations checked` |
| 5 | `openspec validate client-java-avoid-unpaved --strict` | passed — `Change 'client-java-avoid-unpaved' is valid` |

## Host test

The grade arithmetic is factored into the dependency-free `libosmscout-client-java/src/routing_profile_speed.h`
(`kAvoidUnpavedGradeFactor`, `kAvoidUnpavedMinSpeedKmh`, `AvoidUnpavedGradeSpeedsKmh`) and pinned by
`RoutingProfileSpeedTest`. The added cases:

- `The avoid-unpaved grade table is the published 1.0/0.85/0.6/0.3/0.1`
- `A degraded type keeps its base speed at grade 1`
- `The grade speeds of a type fall strictly from grade 1 to grade 5`
- `A degraded road keeps a floor speed instead of becoming unusable`

The helper computes exactly what the reference branch applied inline
(`max(kAvoidUnpavedMinSpeedKmh, baseSpeed * kAvoidUnpavedGradeFactor[i])`), so the host test pins the same
behaviour. `ApplyAvoidUnpavedGradeSpeeds` in the bridge keeps the part that needs a `TypeConfig` (walking the
types, the routability decision and the car base speed) and delegates the per-grade speed array.

## What could not be run

- **The end-to-end route.** `CreateRoutingProfile` runs inside a route calculation against a routable
  database. This checkout has none: `maps/` is import version 26 against the current v27 importer and holds
  no routing database, so no route can be calculated here. No database was imported to obtain one.
- **A measurement of the runtime cost per grade.** The reference branch measured the tables on a Bremen
  database through its probe; that probe needs a routable database and is not reproducible here, which is
  why the arithmetic is pinned on the host instead (rule: prefer a host-testable helper over an un-runnable
  probe). The 1.0/0.85/0.6/0.3/0.1 table and the 3 km/h floor are the reference's, unchanged.
- `scripts/format-check.sh` was not used — it is red repo-wide; the change matches the surrounding style.

## Files changed

- `libosmscout-client-java/src/OSMScoutClient.cpp`
- `libosmscout-client-java/src/routing_profile_speed.h`
- `Tests/src/RoutingProfileSpeedTest.cpp`
- `openspec/changes/client-java-avoid-unpaved/` (proposal, design, tasks, spec delta, this file)
