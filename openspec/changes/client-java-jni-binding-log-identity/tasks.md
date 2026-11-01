# Tasks

## 1. Remove the coordinate from the seven logging statements

Spec: `client-java-jni-binding` — Requirement "A bridge log line carries no coordinate"
(scenarios "A database check logs the database, not its box", "A successful lookup logs
the radius, not the coordinate", "A route end is traced by name").

- [x] 1.1 In `libosmscout-client-java/src/OSMScoutClient.cpp`, replace
  `[JNI] checking db box: <box>` with `[JNI] checking db box of <db->path>`
- [x] 1.2 Replace the two `ResolveRoutePosition: found via coord ...` lines with the
  search radius (`(1 km)` and `(5 km)`)
- [x] 1.3 Replace the start and destination `resolving ... at <coord>` and
  `FAILED to find routable node near ... <coord>` lines with the route end name only
- [x] 1.4 Confirm `git grep GetDisplayText -- libosmscout-client-java/src/OSMScoutClient.cpp`
  returns no match

## 2. Build and signature checks

- [x] 2.1 Build the exact target: `cmake --build build --target osmscout_client_java`
- [x] 2.2 Run `bash scripts/check-jni-signatures.sh`

## 3. Verification record

- [x] 3.1 Write `verification.md` with the commands and their observed results
