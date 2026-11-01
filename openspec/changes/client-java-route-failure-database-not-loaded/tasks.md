# Tasks

## 1. The not-yet-loaded kind (spec: routing-failure-reporting)

- [x] 1.1 Add `inline constexpr int kRouteFailureDatabaseNotLoaded=7;` and
  `RouteFailureCause::DatabaseNotLoaded` (with its `RouteFailureKind` branch) to
  `libosmscout-client-java/src/route_failure_kind.h`, and document why the not-yet-loaded list has a kind of
  its own. Verify: the cause→kind mapping stays total and the switch gains one case.
- [x] 1.2 Add `int KIND_DATABASE_NOT_LOADED = 7;` to
  `libosmscout-client-java/java/com/framstag/libosmscout/client/RouteCallback.java`, documenting that it is
  distinct from `KIND_ROUTING_DATA_UNAVAILABLE`. Verify: the number matches the native constant.
- [x] 1.3 In `libosmscout-client-java/src/OSMScoutClient.cpp`, report the new kind in the two
  `databases.empty()` branches (`calculateRouteWithObjectsWithProfile` and
  `calculateRouteWithObjectsAsync`) and say "yet" in the message. Verify: the two branches agree and the
  branches where the routing service cannot be opened keep `kRouteFailureRoutingDataUnavailable`.

## 2. The host test (spec: routing-failure-reporting)

- [x] 2.1 Add the cause to `Tests/src/RouteFailureKindTest.cpp`: the cause array grows to eight, the constant
  assertions gain `kRouteFailureDatabaseNotLoaded==7`, and the reverse mapping gains
  `RouteFailureKind(DatabaseNotLoaded)==7`. Verify: the test compiles and passes.
- [x] 2.2 Update the test's "these seven values" comment to eight, so it matches the constants the interface
  now declares.

## 3. Verification (spec: routing-failure-reporting)

- [x] 3.1 `cmake --build build --target RouteFailureKindTest` compiles and links; run
  `ctest -R RouteFailureKindTest --output-on-failure` and verify every case passes.
- [x] 3.2 `cmake --build build --target osmscout_client_java` compiles and links `OSMScoutClient.cpp` with no
  error.
- [x] 3.3 Run `bash scripts/check-jni-signatures.sh`; the change adds no JNI native method and no `NewObject`
  descriptor, so the checked count is unchanged.
- [x] 3.4 Run `openspec validate client-java-route-failure-database-not-loaded --strict` and verify the change
  validates.

## 4. Change hygiene

- [x] 4.1 Record in `verification.md` that the Java route path could not be exercised (no routable v27
  database) and that the JNI part is compile-verified plus the signature check.
- [x] 4.2 Verify the diff touches only `libosmscout-client-java/src/route_failure_kind.h`,
  `libosmscout-client-java/java/com/framstag/libosmscout/client/RouteCallback.java`,
  `libosmscout-client-java/src/OSMScoutClient.cpp`, `Tests/src/RouteFailureKindTest.cpp`, plus this change's
  own directory.
