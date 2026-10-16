# Verification

Evidence for `client-java-route-instruction-metrics`. Every command was run from the repository root
unless the working directory is named. The change is a Java-bridge and Java-value-type change only: no
core C++ library, database, map or style file is touched.

## 1. Build

- [x] 1.1 CMake: `cmake --build build --target osmscout_client_java java_jar` compiles
  `libosmscout-client-java/src/OSMScoutClient.cpp` and packages the Java classes into
  `build/libosmscout-client-java/libosmscoutclientjava.jar` without errors and without warnings from the
  touched files. A second invocation reports `ninja: no work to do`.
- [x] 1.2 The freshly built jar was installed into the local Maven repository so JavaScout compiles and
  runs against it:
  `mvn -o install:install-file -Dfile=build/libosmscout-client-java/libosmscoutclientjava.jar
  -DgroupId=net.sf.libosmscout -DartifactId=libosmscout-client-java -Dversion=1.0-SNAPSHOT -Dpackaging=jar`
  → `BUILD SUCCESS`.
- [x] 1.3 Meson: `meson compile -C build-meson osmscout_client_java libosmscoutclientjava` builds
  `libosmscout-client-java/src/libosmscout_client_java.so.1.1.1` and the jar without errors.

## 2. JNI constructor descriptor (the trap that `check-jni-signatures.sh` does not catch)

- [x] 2.1 `javap -s -cp build/libosmscout-client-java/java/classes
  com.framstag.libosmscout.client.RouteInstruction` against the **compiled** class reports:

  ```
  public com.framstag.libosmscout.client.RouteInstruction(double, com.framstag.libosmscout.client.TurnType, java.lang.String, java.lang.String, java.lang.String);
    descriptor: (DLcom/framstag/libosmscout/client/TurnType;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V
  public com.framstag.libosmscout.client.RouteInstruction(double, double, double, com.framstag.libosmscout.client.TurnType, java.lang.String, java.lang.String, java.lang.String, double, com.framstag.libosmscout.client.TurnType, java.lang.String, java.lang.String);
    descriptor: (DDDLcom/framstag/libosmscout/client/TurnType;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;DLcom/framstag/libosmscout/client/TurnType;Ljava/lang/String;Ljava/lang/String;)V
  ```

  Exact `javap` output; the full-constructor descriptor is
  `(DDDLcom/framstag/libosmscout/client/TurnType;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;DLcom/framstag/libosmscout/client/TurnType;Ljava/lang/String;Ljava/lang/String;)V`.
- [x] 2.2 The `GetMethodID` descriptor in `OSMScoutClient.cpp` was changed from `(DD…` to `(DDD…` and is
  character-for-character equal to the compiled full-constructor descriptor above.
- [x] 2.3 The `NewObject` argument list in `CreateJavaRouteInstruction` follows the Java parameter order
  `distanceTo, timeTo, legDistance, turnType, streetName, description, shortDescription,
  nextNextDistanceTo, nextNextTurnType, nextNextDescription, nextNextShortDescription`; the added argument
  `instr.legDistance` is the third one, directly after `instr.timeTo`.
- [x] 2.4 The short constructor keeps its signature: `javap` still reports
  `(DLcom/framstag/libosmscout/client/TurnType;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V`,
  and it still delegates with `0.0` for both per-step values.
- [x] 2.5 `bash scripts/check-jni-signatures.sh` → `JNI signatures match: 55 native declarations checked,
  6 dead JNI functions reported above.` The six reported functions are pre-existing
  (`getMaxSpeedAt`, `searchLocationByForm`, `reloadBasemap`, `getAddressAt`, `getDatabaseBoundingBox`,
  `setMapDpi`).
- [x] 2.6 `meson test -C build-meson "Check JNI signature parity"` → `1/1 OK`, `Ok: 1`, `Fail: 0`.

## 3. Java tests

- [x] 3.1 `cd JavaScout && mvn -o test -Dnative.lib.dir=/home/tim/projects/libosmscout/build/libosmscout-client-java
  -Dtest=RouteInstructionTest` → `Tests run: 5, Failures: 0, Errors: 0, Skipped: 0`, `BUILD SUCCESS`. The
  test now calls the eleven-argument full constructor and asserts `legDistance` (a leg length whose value is
  independent of the distance still to travel, and `0.0` from the short constructor).
- [x] 3.2 `cd JavaScout && mvn -o test -Dnative.lib.dir=/home/tim/projects/libosmscout/build/libosmscout-client-java`
  (full offline suite):

  | run | result |
  |-----|--------|
  | 1 | `Tests run: 230, Failures: 1, Errors: 0, Skipped: 19` |
  | 2 | `Tests run: 230, Failures: 1, Errors: 0, Skipped: 19` |
  | 3 | `Tests run: 230, Failures: 0, Errors: 0, Skipped: 19` |
  | 4 | `Tests run: 230, Failures: 1, Errors: 0, Skipped: 19` |

  Run 3 reproduces the documented master baseline (0 failures / 19 skipped). Every failure is the same
  single test, `OSMScoutClientBasemapConfigTest.testClearingBasemapLookupDirectoryUnloads:357` — "the map
  must keep rendering after the basemap is unloaded ==> expected: not <null>".
- [x] 3.3 That test is unrelated to this change: it builds a client, opens a map fixture, loads
  stylesheets, sets and clears a basemap lookup directory and renders — it never calculates a route, never
  receives a `RouteEntry` and never constructs a `RouteInstruction`. It passes in isolation on every
  attempt: `-Dtest=OSMScoutClientBasemapConfigTest` → `Tests run: 14, Failures: 0, Errors: 0, Skipped: 0`
  three times in a row, and the eight classes that run before it in suite order (the prefix up to and
  including `OSMScoutClientBasemapConfigTest`) pass together, 47 tests / 0 failures / 3 skipped. It is a
  load-dependent flake of a render/basemap race on this shared machine, pre-existing and out of scope:
  it was not fixed here. A pristine-master comparison was attempted and abandoned because the shell
  wrapper stalled on the chained command that would have swapped the sources; the code-path argument above
  is what the evidence rests on.

## 4. What is asserted by construction rather than by a run

No routable map database exists on this machine (`maps/` is format v26 while the library expects v27), so
the route-level checks (tasks 3.2 and 3.3) could not be executed and no test was invented for them. The
contract is instead held by construction and by inspection of the two `DescCallback`s:

- [x] 4.1 Alignment: in both `calculateRouteWithObjectsWithProfile` and
  `calculateRouteWithObjectsAsync`, `instructionLats`/`instructionLons` are appended in `NextLine()` and
  `instructionDistances`/`instructionTimes` are appended in `AppendDistanceTime()`. `AppendDistanceTime()`
  is called exactly once per emitted line (`OnStart`, `OnTurn`, `OnTargetReached`) and every one of those
  calls is preceded by exactly one `NextLine()`, so all four vectors have exactly `lineCount` entries — the
  number of instruction lines, the `"--- Route ---"` header of `BeforeRoute()` not counted.
- [x] 4.2 The alignment check after `GenerateDescription` compares all four sizes with `descCb.lineCount`
  and clears all four together (and logs through `osmscout::log`), so a mismatch yields four `null` Java
  arrays and an otherwise unchanged `RouteEntry`; the marshalling additionally re-checks the four sizes and
  leaves the arrays `null` when they are empty.
- [x] 4.3 Sums: `prevDistance`/`prevTime` start at 0 and now advance at the end of `AppendDistanceTime`,
  i.e. once per emitted line, while `BeforeNode` only records the current node. The first emitted line is
  the route start node (distance 0), so `instructionDistances[0]` is the zero-length start leg and the
  entries telescope to the distance of the last emitted node, the route's own total; `instructionTimes`
  telescopes the same way to the description's total time. The equality with the *published*
  `RouteEntry.distance`/`duration` is completed by the companion change `client-java-route-length`, which
  derives those two from exactly these legs; this change does not alter them. (Measured on the v27 database
  on 2026-10-16: the distance half holds exactly; the time half does not — `instructionTimes` sums to the
  description's own time, while the published `duration` is the 50 km/h estimate of
  `client-java-route-length`. See "Host verification 2026-10-16 (map-dependent items)".)
- [x] 4.4 `RouteEntry`'s four arrays are documented in the Java source (`instructionLats`,
  `instructionLons`, `instructionDistances`, `instructionTimes`), `RouteInstruction.legDistance` documents
  `0.0` as unknown, and `RouteInstruction.timeTo` documents the leg semantics and the remaining-time
  behaviour of the next instruction.
- [x] 4.5 Instruction leg values: `CollectCallback::FillLeg` is called once per emitted instruction and is
  the only place that sets `legDistance`/`timeTo`/`nodeTimeSeconds`; it measures the leg from the previously
  emitted instruction and marks an unknown leg (`legStartKnown == false`, i.e. a walk that starts mid-leg)
  as `0.0`/`0.0` instead of a cumulative-from-the-route-start number. The next-instruction builder reports
  `next.nodeTimeSeconds - timeAtPosition` with `timeAtPosition` interpolated at the current position with
  the same `abscissa` and the same `nextNode` the remaining distance uses.

## 5. Change hygiene

- [x] 5.1 `openspec validate "client-java-route-instruction-metrics" --strict` reports the change as valid
  (all pre-existing scenario names of the two MODIFIED requirements kept).
- [x] 5.2 The diff touches only the four files named in the proposal's Impact section plus this change's
  own `openspec/changes/client-java-route-instruction-metrics/` directory; `git diff --stat` lists exactly
  those files, and no database, map or style file changed.
- [x] 5.3 The description line text is unchanged: `AppendDistanceTime` still renders the same
  `"  [x km, y min]"` / `"  [x m, y min]"` columns; the seconds value the new arrays carry is computed
  separately from the same `time - prevTime` difference and does not feed the text.

## Host verification 2026-10-16

Run to close the suite item of the task list (the item now reads `RUN 2026-10-16`). Verified revision:
`38aaf7a18` on branch `todo-backlog-loop-2026-10-16`, whose only delta from `master` (`a79e239c9`) is `TODO.md`, so it carries
`master`'s code. The client jar was rebuilt from that revision
(`build/libosmscout-client-java/libosmscoutclientjava.jar`, sha256 `b6cb25802c513f1a2e448848129d7087acd4efb159eefc02f77714c9b4a2f3af`).

```
cmake --build build -j "$(nproc)"                                  # exit 0, no target failed
QT_QPA_PLATFORM=offscreen TESTS_TOP_DIR=../Tests TESTS_TMP_DIR=./tmp-suite \
  ctest -j 8 --output-on-failure                                   # 100% tests passed out of 158, exit 0, 23.68 s
cd JavaScout && ./mvnw install:install-file -Dfile=../build/libosmscout-client-java/libosmscoutclientjava.jar \
  -DgroupId=net.sf.libosmscout -DartifactId=libosmscout-client-java -Dversion=1.0-SNAPSHOT -Dpackaging=jar -q
./mvnw package -DskipTests                                         # BUILD SUCCESS
xvfb-run ./mvnw test                                               # Tests run: 205, Failures: 0, Errors: 0, Skipped: 38
```

Retained logs: `evidence/cpp-suite-2026-10-16.log`. Each names its own command and the revision it ran on.

Two facts about the environment, so the numbers can be reproduced:
- a reused `build/` directory whose test objects predate the installed Catch2 fails to link
  (`undefined reference to typeinfo for Catch::ITransientExpression`) until the test translation units are
  recompiled. That is a build-directory artefact, not a source defect; no source file was changed for it.
- the first full-suite run used the reused `build/Tests` as `TESTS_TMP_DIR` and failed `OptimizeAreaWayIdsTest`
  on its stale temp state (`Tests/src/OptimizeAreaWayIdsTest.cpp:383`, see the retained
  `ctest-full.log` beside this change's evidence). The same revision passes that test with a fresh temp
  directory, which is what the numbers above use. The dependence of the test on its temp directory is filed
  in `TODO.md`.

Not covered by this run, and still open in `tasks.md` on 2026-10-16 (now for a measured reason): 3.2 and
3.3, the values read off a real route.

## Host verification 2026-10-16 (map-dependent items)

Tasks 3.2 and 3.3 were probed on real routes of the v27 database `maps/nordrhein-westfalen`. **They stay
open:** the alignment and distance clauses hold, the time clause does not, and the min-leg check of 3.3
fails on every route tried. Log: `evidence/route-metrics-probe-nrw-2026-10-16.log`; probe source
`evidence/RouteMetricsProbe.java`.

```bash
# compiled once into build/loop-probes-2026-10-16
java -Djava.library.path=build/libosmscout-client-java \
  -cp build/libosmscout-client-java/libosmscoutclientjava.jar:build/loop-probes-2026-10-16 \
  RouteMetricsProbe maps/nordrhein-westfalen stylesheets
```

| route | instruction lines | array length | sum(distance) vs `distance` | sum(time) vs `duration` | min reported leg | max single geometry edge | 3.3 holds |
|---|---|---|---|---|---|---|---|
| city 51.514227/7.465279 -> 51.520/7.480 | 9 | 9 | `1699.377` vs `1699.377` (diff `-2.3e-13`) | `157.0` vs `122.355` (diff `+34.645`) | `21.885` m | `85.726` m | no |
| 51.514227/7.465279 -> 51.500/7.500 | 17 | 17 | `4964.306` vs `4964.306` (diff `0.0`) | `494.0` vs `357.430` (diff `+136.570`) | `11.138` m | `171.272` m | no |
| 51.514227/7.465279 -> 50.938/6.960 (Cologne) | 12 | 12 | `95702.492` vs `95702.492` (diff `0.0`) | `3742.0` vs `6890.579` (diff `-3148.579`) | `52.759` m | `829.774` m | no |
| 51.514227/7.465279 -> 51.960/7.630 (Münster) | 17 | 17 | `69942.457` vs `69942.457` | `2874.0` vs `5035.857` | `11.158` m | `1164.749` m | no |

What this means:

- **3.2, alignment clause: satisfied.** The four arrays are non-null, all the same length, and that length
equals the number of description lines that are not the `--- Route ---` header (9/9, 17/17, 12/12).
- **3.2, distance clause: satisfied.** The leg distances telescope to the published `distance` exactly
  (diffs at the last bit or `0.0`).
- **3.2, time clause: NOT satisfied.** `sum(instructionTimes)` never equals the published `duration`
  (`157.0` vs `122.355`, `494.0` vs `357.430`, `3742.0` vs `6890.579`). `duration` is not derived from the
  description's times: `OSMScoutClient.cpp:6973` sets it to `totalDistance / 50 km/h`, which is what the
  merged companion change `client-java-route-length` asks for (its spec: "The estimated duration SHALL be
  derived from the published length"). The metrics spec's scenario "Per-step leg values sum to the route's
  totals ... their entries SHALL add up to `distance` and `duration`" is therefore **contradicted by the
  implementation of the two merged changes**: the published duration is a fixed-speed estimate, not the
  description's total time.
- **3.2, null-array clause: not exercisable from Java.** The arrays are dropped only when the description
  cannot be aligned, which needs a `DescCallback` misalignment the JNI call path never produces; the probe
  cannot force it.
- **3.3: NOT satisfied as stated.** The smallest reported leg is smaller than the largest single geometry
  edge of the same route on all four routes (`21.885 < 85.726`, `11.138 < 171.272`, `52.759 < 829.774`,
  `11.158 < 1164.749`). A long straight edge on the route is simply longer than the shortest leg between two
  manoeuvres, which is normal where instructions sit close together, so this check cannot pin the property
  the task means (a leg must not be a single geometry edge). The leg distances do telescope to the route
  total, which is the property the alignment check above pins, but the clause as written was not observed.

Nothing here is a code change; both tasks stay open.
