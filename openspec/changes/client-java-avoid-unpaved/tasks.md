# Tasks — client-java-avoid-unpaved

## 1. Bridge (spec: Grade-aware speeds when avoidUnpaved is set)

- [x] 1.1 Hoist the car speed map and the resolved vehicle speed out of the vehicle switch, so the graded
  pass can reuse them for every vehicle.
- [x] 1.2 Add the graded speed pass: parametrize the vehicle first, then re-price every routable type with
  the per-grade table and its floor.
- [x] 1.3 Replace the log-only `avoidUnpaved` branch with the call and a log line that names the applied
  table and floor.
- [x] 1.4 Compile the translation unit (`cmake --build build --target osmscout_client_java`).

## 2. Host test (rule: prefer a host-testable helper over an un-runnable probe)

- [x] 2.1 Factor the per-grade table, the floor and the base-speed-to-speeds arithmetic into the
  dependency-free `routing_profile_speed.h` as `kAvoidUnpavedGradeFactor`, `kAvoidUnpavedMinSpeedKmh` and
  `AvoidUnpavedGradeSpeedsKmh`, without changing what the bridge computes.
- [x] 2.2 Pin the table, the strict grade ordering, the 3 km/h floor and the grade-1 identity in
  `RoutingProfileSpeedTest`.
- [x] 2.3 Build and run the test (`RoutingProfileSpeedTest`).

## 3. Specification

- [x] 3.1 Add the capability's scenarios to `specs/routing-profile/spec.md`.
- [x] 3.2 Run `openspec validate client-java-avoid-unpaved --strict`.

## 4. Verification

- [x] 4.1 Rebuild and run `RoutingProfileSpeedTest`; record the run in `verification.md`.
- [x] 4.2 Run `bash scripts/check-jni-signatures.sh` (this change adds no JNI native method).
- [x] 4.3 Record that the end-to-end route cannot be exercised here (no routable database; `maps/` is v26
  against a v27 importer) and that no database was imported for it.
