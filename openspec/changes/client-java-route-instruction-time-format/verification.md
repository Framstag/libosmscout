# Verification — client-java-route-instruction-time-format

Machine: local development machine, CMake + Ninja (`build/`, Release, `OSMSCOUT_BUILD_CLIENT_JAVA=ON`,
`OSMSCOUT_BUILD_TESTS=ON`), Meson (`build-meson/`), JDK 17, Maven 3.x, Catch2.

Baseline: `git rev-parse master origin/master` → `0205b359e62a669cd4a84686478e2e49dbd9f927` for both; the
branch `client-java-route-instruction-time-format` was created from `origin/master`.

## 1. The change (tasks 1.1–1.3, 3.2)

`git diff --stat` over the change (plus the two new files):

```
 Tests/CMakeLists.txt                           |  3 +++
 Tests/meson.build                              |  9 ++++++++
 libosmscout-client-java/src/OSMScoutClient.cpp | 31 +++++++++-----------------
 3 files changed, 23 insertions(+), 20 deletions(-)
 libosmscout-client-java/src/route_step_time.h  (new, 68 lines)
 Tests/src/RouteStepTimeTest.cpp                (new, 83 lines)
```

| Check | Result |
|---|---|
| 1.1 unit rule | `naviveylin::FormatRouteStepTime(std::chrono::seconds)` in `route_step_time.h` returns `nullopt` below one whole second, `"<h> h <m> min"` from an hour, `"<m> min"` from a minute and `"<s> s"` below a minute |
| 1.1 distance and separator unchanged | the distance part (`>= 1.0` km, `> 0.01` m) is byte-identical to before; the comma is still written only for `segDist > 0.01`, with the former `time - prevTime > Duration::zero()` test replaced by `timeText.has_value()`, i.e. by the very condition that prints the time part — so the separator rule "both parts present" is now expressed once instead of twice |
| 1.2 both paths | the only two `AppendDistanceTime` bodies in the file (`DescCallback` of `calculateRouteWithObjectsWithProfile` and of `calculateRouteWithObjectsAsync`) both call the helper; `grep -n AppendDistanceTime libosmscout-client-java/src/OSMScoutClient.cpp` finds exactly two definitions and no remaining `" min"` time formatting (`grep -n 'min"'` finds no hit in the file) |
| 1.3 comment names the defect | the sub-minute branch carries `A step shorter than a minute used to print "0 min" (owner finding, 2026-10-03) … do not reduce this branch back to minutes`; the header's file comment names the defect as well |
| 3.2 diff scope | the diff touches the include list and the two `AppendDistanceTime` bodies of `OSMScoutClient.cpp`, the new header, the new test and the two test registrations; no numeric value, no JNI or Java signature, no style/type-config file |
| per-step numbers untouched | `CollectCallback::SegmentTimeSeconds()` publishes `JavaRouteInstruction.timeTo` and is not in the diff |

## 2. Host test of the rule (tasks 2.1, 2.2)

```
cd /home/tim/projects/libosmscout
cmake --build build --target RouteStepTimeTest
cd build && ctest -R RouteStepTimeTest --output-on-failure
```

Result: `All tests passed (19 assertions in 5 test cases)`; `1/1 Test #63: RouteStepTimeTest … Passed 0.00 sec`
(the number is the one ctest assigns; in the full-suite run below the same test is listed as `#59`).

Meson: `meson test -C build-meson "Check route step time"` →
`1/1 libosmscout:Check route step time OK 0.00s`, `Ok: 1  Fail: 0`.

The five cases and what they pin down:

| Case | Assertion |
|---|---|
| A step of minutes is printed in whole minutes | `60 s → "1 min"`, `75 s → "1 min"` (whole minutes), `45 min → "45 min"`, `59 min 59 s → "59 min"` |
| A step of an hour or more is printed in hours and remaining minutes | `1 h → "1 h 0 min"`, `1 h 5 min 59 s → "1 h 5 min"`, `2 h 45 min → "2 h 45 min"` |
| A step below a minute is printed in whole seconds | `1 s → "1 s"`, `45 s → "45 s"`, `59 s → "59 s"`, and none of them equals `"0 min"` — the defect |
| A step below a second carries no time | `FormatRouteStepTime(0 s)` and `(-5 s)` are `nullopt`, and `0 s` formats neither as `"0 s"` nor as `"0 min"` |
| The unit boundaries are inclusive at the second, minute and hour | exactly `1 s → "1 s"`, exactly `60 s → "1 min"`, exactly `3600 s → "1 h 0 min"` |

## 3. Build, signature parity and regressions (tasks 2.4–2.7)

| Command | Result |
|---|---|
| `cmake --build build --target osmscout_client_java` | rc=0 — `Building CXX object …/OSMScoutClient.cpp.o`, `Linking CXX shared library …/libosmscout_client_java.so`; no warning from the touched file |
| `cmake --build build --target RouteStepTimeTest` | rc=0 — compiles and links; no warning |
| `bash scripts/check-jni-signatures.sh` | exit 0 — `JNI signatures match: 55 native declarations checked, 6 dead JNI functions reported above.` (the six are the pre-existing dead list) |
| `meson compile -C build-meson osmscout_client_java libosmscoutclientjava` | rc=0 — `Compiling C++ object …/libosmscout_client_java.so.1.1.1.p/meson-generated_osmscout_client_java-unity0.cpp.o`, `Linking target libosmscout_client_java.so.1.1.1` |
| `meson test -C build-meson "Check JNI signature parity"` | `1/1 libosmscout:Check JNI signature parity OK 0.17s`, `Ok: 1  Fail: 0` |
| `cd build && QT_QPA_PLATFORM=offscreen ctest -j 2 -E "PerformanceTest"` | `100% tests passed out of 107` in 40.37 s (includes the new `RouteStepTimeTest` and the sibling host test `SearchScopeTest`) |
| `cmake --build build --target SearchScopeTest` then `ctest -R SearchScopeTest` | `ninja: no work to do.`, `1/1 Test #62: SearchScopeTest … Passed 0.00 sec` — the sibling host test that shares the include path is unaffected |
| `cd JavaScout && mvn -o test -Dnative.lib.dir=/home/tim/projects/libosmscout/build/libosmscout-client-java` | `BUILD SUCCESS`, `Tests run: 230, Failures: 0, Errors: 0, Skipped: 19` — the same 19 skips as the recorded master baseline (the map-database-driven tests; the shipped databases are format version 26) |
| `openspec validate client-java-route-instruction-time-format --strict` | exit 0 — `Change 'client-java-route-instruction-time-format' is valid` |

## 4. Why the two paths are compared by construction (task 2.2)

The spec scenario "Both description paths print the same times" compares two real line lists, which needs a
calculated route and therefore a routable map database. That is not available here:

- The shipped map databases carry type-config format version **26** while the library expects **27**, so
  opening one fails and a probe through the Java bridge cannot be run.
- Importing a fresh database is out of scope for a display fix.

The change therefore removes the possibility of drift instead of testing it: both `DescCallback`s call the one
helper `naviveylin::FormatRouteStepTime`, so the time column of a calculated and of a recalculated route is
produced by the same code for the same per-step duration. The test above pins that helper's behaviour, and the
diff shows both call sites.

## 5. Residual risks

- A consumer that parses the time column and expects a fixed unit sees `s`, `min` and `h` — the column already
  varied (hours and minutes, minutes) before the change.
- A step below a second now prints an empty time column. That matches the existing shape (a line can already
  print without a distance part, and the comma appears only when both parts are present), but a caller that
  assumed a non-empty time part would see an empty one.
- The route's total duration keeps the existing minute rule (a non-goal of the change), so the line's per-step
  unit and the route's total unit can differ.
- The rule is verified on the host; the two description paths are not compared on a live route for the reason
  in section 4.
