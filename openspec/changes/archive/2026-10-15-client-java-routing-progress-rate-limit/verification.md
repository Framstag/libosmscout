# Verification — client-java-routing-progress-rate-limit

Machine: local development machine, CMake + Ninja (`build/`, Release, `OSMSCOUT_BUILD_CLIENT_JAVA=ON`,
`OSMSCOUT_BUILD_TESTS=ON`), Meson (`build-meson/`), JDK 17, Maven 3.x, Catch2.

Baseline: `git rev-parse master origin/master` → `0205b359e62a669cd4a84686478e2e49dbd9f927` for both; the
branch `client-java-routing-progress-rate-limit` was created from `origin/master`.

## 1. The change (tasks 1.1–1.4)

`git diff --stat` over the change (plus the two new files):

```
 Tests/CMakeLists.txt                           |  3 +++
 Tests/meson.build                              |  9 ++++++++
 libosmscout-client-java/src/OSMScoutClient.cpp | 30 +++++++++++++++++++-------
 3 files changed, 34 insertions(+), 8 deletions(-)
 libosmscout-client-java/src/routing_progress_throttle.h   (new, 109 lines)
 Tests/src/RoutingProgressThrottleTest.cpp                 (new, 144 lines)
```

| Check | Result |
|---|---|
| 1.1 state lives in the callback object, one per calculation | the only state is the member `naviveylin::RoutingProgressThrottle throttle` of `JavaRoutingProgress`; the interval is `kMinProgressReportInterval` (100 ms) in the header. `grep -n JavaRoutingProgress` finds the class plus exactly two constructions (`std::make_shared<JavaRoutingProgress>` at the coordinate-based and the object-based route entry point), one object per `calculateRouteAsync` call |
| 1.2 only a changed percentage, at most once per interval | `ShouldReport()` returns false when `percent <= lastPercent`, and when a report was handed over and `(now - lastReport) < kMinProgressReportInterval`; a dropped report changes nothing, so the interval is measured from the last handover |
| 1.3 cap below completion, first report immediate | `ProgressPercent()` returns `kMaxProgressPercent` (99) for progress at or beyond the overall distance and 0 for an unknown/negative overall distance; `ShouldReport()` accepts the first report whatever `now` is |
| 1.4 dropped report costs no JNI crossing | in `Progress()` the decision and its `return` precede `AttachCurrentThread(&env, jvm)` and `env->CallVoidMethod(callback, methods.onProgress, percent)`; both are reached only for an accepted report |

## 2. Host test of the decision (tasks 2.1, 2.2, 2.4)

```
cd /home/tim/projects/libosmscout
cmake --build build --target RoutingProgressThrottleTest
./build/Tests/RoutingProgressThrottleTest
cd build && ctest -R RoutingProgressThrottleTest --output-on-failure
```

Result: `All tests passed (28 assertions in 5 test cases)`; `1/1 Test #63: RoutingProgressThrottleTest … Passed 0.01 sec`.

Meson: `meson test -C build-meson "Check routing progress throttle"` →
`1/1 libosmscout:Check routing progress throttle OK 0.01s`, `Ok: 1  Fail: 0`.

The five cases and what they pin down:

| Case | Assertion |
|---|---|
| The first progress report is handed over at once | `ShouldReport(0, t0)` and `ShouldReport(5, t0)` are both accepted, so the first report waits for nothing |
| An unchanged or lower percentage is not reported again | `5` again after 100 ms and after 10 s is dropped; `4` and `-1` are dropped |
| A change inside the rate-limit interval is dropped, a later one is taken | `6` at 99 ms dropped, `6` at 100 ms accepted, `7` at 150 ms dropped, `7` at 200 ms accepted, `8` at 250 ms and 299 ms dropped, `8` at 300 ms accepted |
| An edge storm hands over at most one report per interval | 51 relaxed edges over a simulated 500 ms, every edge with a changed percentage → exactly six reports (0 ms plus one per 100 ms), the count is `<= duration / interval + 1`, the accepted percentages are non-decreasing |
| Progress never reports completion | the cap is 99 for `1000/1000` and for `1500/1000`, `0` for a zero or negative overall distance, no accepted value reaches 100, the highest accepted value is 99 |

## 3. Build, signature parity and regressions (tasks 3.1–3.4)

| Command | Result |
|---|---|
| `cmake --build build --target osmscout_client_java` | rc=0 — `Building CXX object …/OSMScoutClient.cpp.o`, `Linking CXX shared library …/libosmscout_client_java.so`; no warning from the touched file |
| `cmake --build build --target RoutingProgressThrottleTest` | rc=0 — compiles and links; no warning |
| `bash scripts/check-jni-signatures.sh` | exit 0 — `JNI signatures match: 55 native declarations checked, 6 dead JNI functions reported above.` (the six are the pre-existing dead list) |
| `cd build && ctest -R JniSignatureParityTest --output-on-failure` | `1/1 Test #46: JniSignatureParityTest … Passed 0.13 sec` |
| `meson compile -C build-meson osmscout_client_java libosmscoutclientjava` | rc=0 — relinks `libosmscout_client_java.so.1.1.1` and `libosmscoutclientjava.jar`; the 69 javadoc warnings are pre-existing (no Java source changed) |
| `meson test -C build-meson "Check JNI signature parity"` | `1/1 libosmscout:Check JNI signature parity OK 0.13s`, `Ok: 1  Fail: 0` |
| `cd build && ctest -j 2 -E "PerformanceTest" --output-on-failure` | `99% tests passed, 1 tests failed out of 107` — see below |
| `cmake --build build --target DatabaseOpenTest` then `ctest -R DatabaseOpenTest` | `1/1 Test #3: DatabaseOpenTest … Passed 0.05 sec` |
| `cd JavaScout && mvn test -Dnative.lib.dir=/home/tim/projects/libosmscout/build/libosmscout-client-java` | `Tests run: 230, Failures: 0, Errors: 0, Skipped: 19` — the same result as on master on a machine without a v27 map database; the 19 are the map-database-driven tests |
| `openspec validate client-java-routing-progress-rate-limit --strict` | exit 0 — `Change 'client-java-routing-progress-rate-limit' is valid` |

The single C++ failure is not a regression and not from this change: the stale binary
`build/Tests/DatabaseOpenTest` (built from another branch in this shared `build/`) dies with
`symbol lookup error: … undefined symbol: _ZN8osmscout27IsOpenableDatabaseDirectoryERKNSt10filesystem7__cxx114pathE`,
and `IsOpenableDatabaseDirectory` does not occur anywhere in this work tree
(`grep -rn IsOpenableDatabaseDirectory Tests/src libosmscout/include libosmscout/src` finds nothing). After
rebuilding that one target from this tree the test passes, so the suite is green.

## 4. Why the rule is covered by a host test and not by a route probe (task 2.4)

The design's open question — whether to extract the decision — was settled in favour of extraction (D5),
because a probe through the Java bridge is not runnable on this machine:

- The shipped map databases carry type-config format version **26** while the library expects **27**:
  `od -An -tu1 -N4 maps/arnsberg-regbez/types.dat` → `26 0 0 0` (same for `maps/Dortmund`,
  `maps/iceland`, `maps/nordrhein-westfalen`, `maps/provence-alpes-cote-d-azur`), and
  `libosmscout/include/osmscout/TypeConfig.h:1044` has `FILE_FORMAT_VERSION=27` with
  `MIN_FORMAT_VERSION = MAX_FORMAT_VERSION = FILE_FORMAT_VERSION`, so there is no compatibility path.
  Opening one reports `File '…/maps/arnsberg-regbez/types.dat' does not have the expected format version!
  Actual 26, expected: 27` and `Cannot open db '…/maps/arnsberg-regbez'!`.
- The databases in the local build directories (`debug/`, `build-meson/`) are version 27 but incomplete:
  `nodes.dat` is 93 bytes, no `ways.idx` exists anywhere outside `maps/`, and opening them reports
  `Can't use db …, some mandatory files are missing.` They are the remains of interrupted imports.
- Importing a fresh database is out of scope for a 100 ms throttle, and a timing-dependent test that needs a
  database would be skipped on CI and flaky where it is not.

The extracted header makes the rule deterministic instead: the time is a parameter, the percentage is a
value, and the header includes only `<chrono>`, so no routing engine, no JNI environment and no map database
is involved.

## 5. Residual risks

- The Java-side javadoc of `RouteCallback.onProgress` (`Called periodically during route calculation`,
  `percent (0-100)`) was deliberately left unchanged: the proposal's Impact section declares "No Java source
  change". Its wording already matches the throttled behaviour better than the old per-edge behaviour did,
  but it does not name the new rules — a caller reads them in the header and in `Progress()`'s doc comment.
- The route path itself (a live calculation delivering reports to a Java callback) was verified by inspection
  of the diff, not by execution, for the reason in section 4.
- The rate-limit interval is a fixed constant (100 ms); the design records that making it caller-configurable
  is deferrable.
