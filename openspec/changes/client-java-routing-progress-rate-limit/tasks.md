# Tasks

## 1. Throttling the progress callback (spec: route-calculation)

- [x] 1.1 In `libosmscout-client-java/src/OSMScoutClient.cpp`, give the routing progress callback the state
  it needs: the last handed-over percentage and the time it was handed over, plus a documented rate-limit
  interval. Verified: the state is the single data member `naviveylin::RoutingProgressThrottle throttle` of
  `JavaRoutingProgress`; the interval is `kMinProgressReportInterval` (100 ms) in the dependency-free header
  `libosmscout-client-java/src/routing_progress_throttle.h`, next to the completion cap
  `kMaxProgressPercent` (99). One throttle per calculation: the only constructions of the class are the two
  route entry points (`std::make_shared<JavaRoutingProgress>` in the coordinate-based and the object-based
  `calculateRouteAsync`), one object per call.
- [x] 1.2 Hand a report over only when the percentage is greater than the last handed-over one and either no
  report was handed over yet or the interval has passed. Verified: `RoutingProgressThrottle::ShouldReport()`
  returns false for an unchanged (`percent <= lastPercent`) and for a lower percentage, and for a change
  inside `kMinProgressReportInterval` of the last handover; the host test
  `Tests/src/RoutingProgressThrottleTest.cpp` covers all four cases, including that a dropped report leaves
  the state alone (the interval is measured from the last handover, not from the last call).
- [x] 1.3 Keep the percentage capped below completion and hand the first report over immediately. Verified:
  `ProgressPercent()` caps at `kMaxProgressPercent` (99) and the class doc states that completion arrives as
  the calculation's result; `ShouldReport()` accepts the first report whatever the time (the host test
  accepts `0` and `5` at the calculation's start).
- [x] 1.4 Verify the JNI attach and the Java call happen only for an accepted report, so a dropped report
  costs no JNI crossing. Verified: in `JavaRoutingProgress::Progress()` the `ShouldReport()` decision and
  its early `return` precede `AttachCurrentThread(&env, jvm)` and `CallVoidMethod(...)`, so both are reached
  only for an accepted report.

## 2. Verification (spec: route-calculation)

Option (b) of the design's open question was chosen: the decision is extracted into a dependency-free
header and covered by a host test. A route probe would have needed a routable v27 map database, and none is
available on this machine: the shipped `maps/*` databases carry type-config format version 26 while
`osmscout::TypeConfig::FILE_FORMAT_VERSION` is 27 (`Cannot open db '…/arnsberg-regbez'`, `types.dat` actual
26, expected 27), and importing a fresh database is out of scope for a 100 ms throttle.

- [x] 2.1 Verify the callback count on a long calculation: the number of progress callbacks is bounded by the
  calculation's wall-clock duration and the interval, not by the graph size, and is greater than one.
  Verified on the host: `RoutingProgressThrottleTest` feeds an edge-storm loop (one edge per simulated 10 ms,
  every one of them with a changed percentage, over 500 ms) through the rule and gets exactly six
  handed-over reports — the report at 0 ms and one per 100 ms interval — with 51 relaxed edges in the
  calculation, so the count follows the interval and the duration and not the edge count.
- [x] 2.2 Verify the reported values: each reported percentage is at least the previous one and below
  completion, and the first report does not wait for the interval. Verified on the host:
  `RoutingProgressThrottleTest` asserts that a lower and an unchanged percentage are dropped, that the
  accepted sequence is non-decreasing, that the first report is accepted at the start time, and that
  `ProgressPercent()` never returns 100 (the cap is reached at 99, also for a progress beyond the overall
  distance and for an unknown overall distance, which reports 0).
- [x] 2.3 Verify the outcome is unchanged: the route result, the duration and the cancellation and error
  callbacks behave exactly as before. Verified by inspection of the diff and by the Java suite: the only
  hunks in `OSMScoutClient.cpp` are the include, the throttle member and `Progress()`, so the result path,
  `onSuccess`, `onError` and `onCancel` are unchanged; `cd JavaScout && mvn test -Dnative.lib.dir=build/libosmscout-client-java`
  reports `Tests run: 230, Failures: 0, Errors: 0, Skipped: 19`, the same result as on master for a machine
  without a v27 map database. A live route could not be run here — see 2.1.
- [x] 2.4 The decision WAS extracted into a helper: `libosmscout-client-java/src/routing_progress_throttle.h`
  (option b) and the host test `Tests/src/RoutingProgressThrottleTest.cpp`, registered in `Tests/CMakeLists.txt`
  and `Tests/meson.build` next to the `SearchScopeTest` precedent. Verified: the header includes only
  `<chrono>`, takes the time as a parameter and the percentage as a value, so the test needs no routing
  engine, no JNI environment and no map database; `ctest -R RoutingProgressThrottleTest` and
  `meson test -C build-meson "Check routing progress throttle"` both pass with 28 assertions in 5 test cases.

## 3. Build and regression verification (spec: route-calculation)

- [x] 3.1 Build the CMake build with the Java client library and the jar, and verify it compiles without
  errors and without warnings from the touched file. Verified: `cmake --build build --target osmscout_client_java`
  relinks `libosmscout_client_java.so` with no warning from `OSMScoutClient.cpp`;
  `cmake --build build --target RoutingProgressThrottleTest` builds the host test with no warning.
- [x] 3.2 Build the Meson build the same way and verify it compiles without errors. Verified:
  `meson compile -C build-meson osmscout_client_java libosmscoutclientjava` relinks the shared library and
  the JAR; the 69 javadoc warnings are pre-existing (this change touches no Java source).
- [x] 3.3 Run the Java and C++ suites and verify no existing test regresses. Verified: the Java suite reports
  `Tests run: 230, Failures: 0, Errors: 0, Skipped: 19` (the 19 are the map-database-driven tests, which skip
  without a v27 database). The C++ suite reports `99% tests passed, 1 tests failed out of 107` with
  `-E PerformanceTest`; the one failure is `DatabaseOpenTest` from a stale binary of another branch
  (`undefined symbol: osmscout::IsOpenableDatabaseDirectory` — a symbol that does not exist anywhere in this
  work tree), and after `cmake --build build --target DatabaseOpenTest` the test passes, so the suite is green.
- [x] 3.4 Run `openspec validate "client-java-routing-progress-rate-limit" --strict` and verify the change
  validates. Verified: exits 0, `Change 'client-java-routing-progress-rate-limit' is valid`.

## 4. Documentation and change hygiene

- [x] 4.1 Verify the callback's new meaning is documented where a consumer reads it: the value is a sampled,
  non-decreasing percentage, the first report is immediate and completion arrives as the result. Verified at
  the decision point: the header's class doc states the non-decreasing, changed-driven, at-most-one-per-interval
  and first-report-immediate rules, `kMaxProgressPercent`'s comment states that completion is not progress but
  the calculation's result, and `JavaRoutingProgress::Progress()` carries the same summary. No Java source text
  changed: the proposal's Impact section declares "No Java source change", and the existing Java javadoc
  (`Called periodically during route calculation`, `percent (0-100)`) already describes the throttled
  behaviour better than the old per-edge behaviour did. Recorded as a residual risk in `verification.md`.
- [x] 4.2 Verify the diff touches only the progress callback in
  `libosmscout-client-java/src/OSMScoutClient.cpp` and, if extracted, a helper header and its test. Verified:
  the diff is `OSMScoutClient.cpp`, `libosmscout-client-java/src/routing_progress_throttle.h`,
  `Tests/src/RoutingProgressThrottleTest.cpp`, `Tests/CMakeLists.txt`, `Tests/meson.build` and this change
  directory; no map, database, style or Java file changed.
- [x] 4.3 Verify the routing engine's own progress reporting is untouched. Verified: the diff contains no hunk
  under `libosmscout/`, `libosmscout-client/` or any other directory; `osmscout::RoutingProgress` and its
  callers are unchanged, and the bridge still reports once per relaxed edge to the throttle.
