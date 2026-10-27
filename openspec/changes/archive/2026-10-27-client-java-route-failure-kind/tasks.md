# Tasks

## 1. The kind as one decision (spec: routing-failure-reporting)

- [x] 1.1 Add `libosmscout-client-java/src/route_failure_kind.h`: the `KIND_*` values mirroring
  `com.framstag.libosmscout.client.RouteCallback`, the bridge's own cause vocabulary
  (`RouteFailureCause`: `Unknown`, `NoStartNode`, `NoTargetNode`, `NoRouteFound`, `DataFailure`,
  `RoutingDataUnavailable`, `EngineFailure`) and `RouteFailureKind(cause)`. Dependency-free (no libosmscout
  include, no JNI), in the style of the sibling helpers of that directory.
- [x] 1.2 Document in the header why the bridge has its own vocabulary instead of the router's enum, why the
  numbering has to stay in step with the Java interface, and why there is deliberately no "route too long"
  kind.
- [x] 1.3 Add `Tests/src/RouteFailureKindTest.cpp`: every cause maps to its kind, `Unknown` maps to
  `KIND_UNKNOWN`, the constants are the numbers the Java interface declares (write them down in the test), and
  the mapping is total (no cause without a branch).
- [x] 1.4 Register the test in `Tests/CMakeLists.txt` and `Tests/meson.build` next to the sibling client-java
  helper tests, with the include path of `libosmscout-client-java/src`. Verify: both build systems build and
  run it.

## 2. The Java side (spec: route-calculation)

- [x] 2.1 Add the seven `KIND_*` constants and the default `onErrorDetail(int kind, String detail, double
  nearestRoadDistanceMeters)` to `RouteCallback.java`, documenting the kind's meaning, that `detail` is for a
  log and not display text, that the wording for the user is the caller's, that a kind the interface does not
  define MUST be treated as `KIND_UNKNOWN`, and that the distance is `Double.NaN` where unknown. The default
  SHALL delegate to `onError(detail)`.

## 3. The bridge (spec: routing-failure-reporting)

- [x] 3.1 In `libosmscout-client-java/src/OSMScoutClient.cpp` extend the cached callback methods with
  `onErrorDetail` and look it up with the descriptor `(ILjava/lang/String;D)V`; a class that does not expose
  it (an older Java side) SHALL fall back to `onError` with the detail, so no error is raised for the missing
  method.
- [x] 3.2 Report the failure at every site that reports a route error: the kind from the routing library's
  `RoutingError` (through the cause vocabulary and `RouteFailureKind`), the English detail, and the
  nearest-road distance of the failing end — taken from the resolution result where one exists, and a
  non-finite value where it does not.
- [x] 3.3 A cancellation SHALL be reported as a cancellation, not as an error kind (the routing library
  names it `RoutingError::Aborted`).
- [x] 3.4 Keep every existing `onError(String)` behaviour: a callback without `onErrorDetail` sees exactly
  what it saw before.

## 4. Verification

- [x] 4.1 Build `RouteFailureKindTest` in both build systems and run it; every case passes.
- [x] 4.2 Build the native client library and the jar (`osmscout_client_java`, `java_jar`); no errors and no
  new warnings from the touched translation unit.
- [x] 4.3 **Check the callback descriptor by hand**: after the jar is built, run
  `javap -s -p -classpath /home/tim/projects/libosmscout/build/libosmscout-client-java/libosmscoutclientjava.jar com.framstag.libosmscout.client.RouteCallback`
  and compare the `onErrorDetail` descriptor character for character with the string in the bridge's
  `GetMethodID` call, and the argument order in the `CallVoidMethod` call. `scripts/check-jni-signatures.sh`
  does NOT cover a callback method ID; a wrong descriptor is a runtime abort.
- [x] 4.4 Run `scripts/check-jni-signatures.sh` and verify it passes.
- [x] 4.5 Install the freshly built jar into the local Maven repository and compile JavaScout against it
  (`mvn install:install-file` then `./JavaScout/mvnw -f JavaScout/pom.xml clean test-compile`); verify main and
  test sources compile unchanged (the new method is a default method).
- [x] 4.6 State in `verification.md` what could not be verified: the route path that reports the kind needs a
  v27 routing database and a live route callback; say whether such a probe was run and what it showed, or why
  not. Do not invent a test that cannot run.
- [x] 4.7 Run `openspec validate "client-java-route-failure-kind" --strict`.

## 5. Change hygiene

- [x] 5.1 Verify the diff against the prerequisite branch touches only the files in the proposal's Impact
  section.
- [x] 5.2 Verify this branch is the prerequisite's branch plus this change only (no `libosmscout/` routing
  file of the prerequisite is modified here).
