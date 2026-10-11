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
  derives those two from exactly these legs; this change does not alter them.
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
