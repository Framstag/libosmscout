# Tasks

## 1. Bridge plumbing (spec: turn-by-turn-instructions)

- [x] 1.1 Give the bridge's instruction struct the per-step time (`timeTo`, seconds) next to the distance in `libosmscout-client-java/src/OSMScoutClient.cpp`. Verify: the struct carries both, documented as metres and seconds.
- [x] 1.2 Track the node times in the collector: keep the previous and the current node time, advance them in `BeforeNode`, and derive the per-step time as the whole-second difference. Verify: the derivation uses the same node times the route description uses, not the instruction distance.
- [x] 1.3 Set the per-step time on every instruction kind the collector produces (start, target reached, turn, roundabout enter/leave, motorway enter/change/leave). Verify: no instruction is left with an unset time while a time is known.
- [x] 1.4 Extend the Java constructor lookup's signature string and the object construction by the time argument, keeping the two in step. Verify: the signature string's argument list matches the Java constructor the test exercises.

## 2. Java data class (spec: turn-by-turn-instructions)

- [x] 2.1 Add `timeTo` (double, seconds) to `RouteInstruction` in `libosmscout-client-java/java/com/framstag/libosmscout/client/RouteInstruction.java`, document it in the field comment and in the constructor Javadoc, and include it in the text representation. Verify: the field is final and documented as the time of the segment.
- [x] 2.2 Give the full constructor the parameter between the distance and the turn type, and have the short constructor report `0.0` while keeping its own signature. Verify: the short constructor still compiles for existing callers and yields an unknown time.

## 3. Tests (spec: turn-by-turn-instructions)

- [x] 3.1 Add `JavaScout/src/test/java/com/framstag/libosmscout/client/RouteInstructionTest.java`: the full constructor carries the time and the next-next hint alongside it; the short constructor reports no time; the text representation mentions the time; null text fields fall back to empty strings. Verify: `mvn test -Dtest=RouteInstructionTest` runs the cases without a native library or a map database.
- [x] 3.2 Record the testing gap for the bridge path: the per-step time is derived inside the JNI translation unit, and a Java-level test for it would need a map database and a route, which the repository's Java CI does not provide (its database-driven tests are skipped there). Verify: the gap is stated in the design and in the pull request rather than silently left uncovered.

## 4. Build and regression verification (spec: turn-by-turn-instructions)

- [x] 4.1 Build the Java client library in the CMake build and verify it compiles without errors and without warnings from the touched file.
- [x] 4.2 Build the Java client shared library and the jar in the Meson build and verify both compile without errors.
- [x] 4.3 Run the JavaScout test suite with Maven and verify it passes, including the new test class.
- [x] 4.4 Run the C++ suite and verify no existing test regresses.
- [x] 4.5 Run `openspec validate "client-route-instruction-step-time" --strict` and verify the change validates.

## 5. Documentation and change hygiene

- [x] 5.1 Verify the field is documented where a Java caller sees it (field comment, constructor Javadoc) and that the capability spec states what the value is for the full instruction list and for the next instruction.
- [x] 5.2 Verify nothing outside the instruction path changed: no change to `TurnType`, the navigation listener, the navigation engine, the route description or a build-system file.
- [x] 5.3 Verify the change is a re-derivation rather than a cherry-pick, and that the other two topics of the downstream source commit are not repeated (they are already upstream).

## 6. Verification evidence

- [x] 6.1 CMake: `ninja osmscout_client_java java_jar` compiles the bridge without errors and without compiler warnings from the touched file.
- [x] 6.2 Meson: `meson compile osmscout_client_java libosmscoutclientjava` compiles and repackages the jar without errors.
- [x] 6.3 Maven: `mvn test -Dtest=RouteInstructionTest` passes 5 cases; the full JavaScout suite runs 169 tests with 0 failures and 10 skipped (the database- and native-dependent ones).
- [x] 6.4 C++: `ctest -j 4 --exclude-regex PerformanceTest` passes 90/90 under `xvfb-run` with `QT_QPA_PLATFORM=offscreen`.
- [x] 6.5 `openspec validate "client-route-instruction-step-time" --strict` reports the change as valid.
- [x] 6.6 The ported Java file is byte-identical to the downstream version of the same class; the bridge is re-derived because the downstream file also carries a distance refinement that upstream has no caller for.
