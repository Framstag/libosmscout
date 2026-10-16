# Tasks

## 1. The Java model (spec: turn-by-turn-instructions)

- [x] 1.1 `libosmscout-client-java/java/com/framstag/libosmscout/client/RouteInstruction.java`: add the
  three fields `roundaboutExitNumber` (int), `roundaboutClockwise` (boolean) and
  `nextNextRoundaboutExitNumber` (int), with the javadoc that names the exit ordinal as the exit *taken* and
  warns against calling it `exitCount`. Verify: `javap -s` on the built jar lists the three as `public final
  int` / `boolean` / `int` (descriptors `I`, `Z`, `I`), and the javadoc names
  `RoundaboutLeaveDescription::GetExitCount()` and `RoutePostprocessor::roundaboutCrossingCounter` as the
  reason it is the ordinal.
- [x] 1.2 The full constructor takes the three values as trailing parameters and assigns them. Verify: the
  constructor has 14 parameters ending `int roundaboutExitNumber, boolean roundaboutClockwise,
  int nextNextRoundaboutExitNumber`, its body assigns all three, and its descriptor ends
  `...Ljava/lang/String;IZI)V`.
- [x] 1.3 Keep two shorter arities that delegate with the "no roundabout" defaults. Verify: the 13-parameter
  constructor delegates with `nextNextRoundaboutExitNumber = 0` and the 11-parameter one (the arity that
  predates the roundabout fields) with `0, true, 0`; the Java tests construct through both and read the
  defaults.
- [x] 1.4 Keep the instruction's text fields and `toString()` unchanged. Verify: the diff of the Java file
  adds no assignment to `description`/`shortDescription` and does not touch `toString()`; the text values a
  test passes in are the values it reads back.

## 2. The JNI bridge (spec: turn-by-turn-instructions)

- [x] 2.1 `libosmscout-client-java/src/OSMScoutClient.cpp`, `struct JavaRouteInstruction`: add
  `roundaboutExitNumber{0}`, `roundaboutClockwise{true}` and `nextNextRoundaboutExitNumber{0}` with the
  comments that explain that the leave description's exit count *is* the ordinal and that the hint's ordinal
  is its own field. Verify: the three members are declared with those defaults, beside the fields they
  belong to (the manoeuvre fields with the instruction, the hint's ordinal with the hint).
- [x] 2.2 `OnRoundaboutEnter`: name the formerly ignored `roundaboutEnterDesc` parameter and set
  `instr.roundaboutClockwise = roundaboutEnterDesc->IsClockwise()`. Verify: the parameter is no longer
  commented out, the assignment is guarded by a null check, and `roundaboutExitNumber` stays 0 because
  entering a ring takes no exit.
- [x] 2.3 `OnRoundaboutLeave`: set `instr.roundaboutExitNumber` from `GetExitCount()` and
  `instr.roundaboutClockwise` from `IsClockwise()`. Verify: the two assignments follow the text composition,
  which is byte-identical to before, and both are guarded by the same null check the text uses.
- [x] 2.4 `GenerateNextRouteInstruction`: copy the following manoeuvre's own ordinal into the hint. Verify:
  `next.nextNextRoundaboutExitNumber = it->roundaboutExitNumber;` sits in the `hasNextNext` branch that fills
  the rest of the hint (distance, turn type, description, short description) and the master-side abscissa and
  remaining-time logic above it is untouched.
- [x] 2.5 `CreateJavaRouteInstruction`: pass the three values in the Java constructor's parameter order, and
  update the `instructionCtor` descriptor in `GetNavigationListenerMethods` to the full constructor. Verify:
  the `NewObject` argument list ends `instr.roundaboutExitNumber, instr.roundaboutClockwise ? JNI_TRUE :
  JNI_FALSE, instr.nextNextRoundaboutExitNumber`, and the descriptor string equals the `javap -s` descriptor
  of the 14-parameter constructor character for character (task 4.4).
- [x] 2.6 Leave the rest of the file alone. Verify: the diff of the file is six hunks and every one of them is
  an addition of the material above (plus the descriptor comment); the newer master work in the file — the
  abscissa/remaining-time logic, `legDistance`, `nodeTimeSeconds`, `FillLeg` — appears nowhere in the diff.

## 3. Tests (spec: turn-by-turn-instructions)

- [x] 3.1 `JavaScout/src/test/java/com/framstag/libosmscout/client/RouteInstructionTest.java`: a case for the
  full constructor carrying the roundabout values. Verify: the case constructs through the 14-parameter
  constructor with `roundaboutExitNumber=2`, `roundaboutClockwise=false`,
  `nextNextRoundaboutExitNumber=1` and asserts all three plus the unchanged `description` /
  `shortDescription` text.
- [x] 3.2 A case for the 13-parameter overload: it defaults the next-next ordinal. Verify: the case
  constructs through the 13-parameter constructor, reads `roundaboutExitNumber` and `roundaboutClockwise`
  back as given and `nextNextRoundaboutExitNumber` as 0, with the hint's other fields still carried.
- [x] 3.3 A case for the arity that predates the roundabout fields: it defaults all three. Verify: the case
  constructs through the 11-parameter constructor and reads `0`, `true` and `0` (task 5.2 records that this
  is the overload the brief labels "12-parameter").
- [x] 3.4 Run the Java tests. Verify: `mvn -o -f JavaScout/pom.xml test -Dtest=RouteInstructionTest
  -Dnative.lib.dir=<abs>/build/libosmscout-client-java` reports `Tests run: 8, Failures: 0, Errors: 0,
  Skipped: 0` and `BUILD SUCCESS`.

## 4. Verification

- [x] 4.1 Build both CMake targets of this change. Verify: `cmake --build build --target
  osmscout_client_java java_jar` exits 0, compiles and links `OSMScoutClient.cpp` and packages the jar, with
  no warning from the touched file.
- [x] 4.2 Build both Meson targets of this change. Verify: `meson compile -C debug osmscout_client_java`
  compiles and links the native library and `meson compile -C debug libosmscoutclientjava` creates the jar,
  both exit 0 (the jar step reports the 69 pre-existing javadoc warnings of the other classes).
- [ ] 4.3 NOT RUN: the bare `meson compile -C debug` of the whole tree. Reason: it stalled the shell tool and
  was abandoned; the two targets this change touches were compiled explicitly instead (4.2), which is what
  the meson-parity check needs.
- [x] 4.4 Check the JNI constructor descriptor against the built jar. Verify: `javap -s -classpath
  build/libosmscout-client-java/libosmscoutclientjava.jar
  com.framstag.libosmscout.client.RouteInstruction` prints the descriptor of the 14-parameter constructor,
  and a `diff` of that line against the string in `GetNavigationListenerMethods` is empty — recorded in
  `verification.md`, because `scripts/check-jni-signatures.sh` does not check `<init>` descriptors.
- [x] 4.5 Run `bash scripts/check-jni-signatures.sh`. Verify: exit 0, `JNI signatures match: 57 native
  declarations checked, 5 dead JNI functions reported above.` — this change adds no JNI function and changes
  no Java native declaration, and the five reported functions are pre-existing.
- [x] 4.6 Run `openspec validate client-java-roundabout-instruction-data --strict`. Verify: `Change
  'client-java-roundabout-instruction-data' is valid`.
- [x] 4.7 Run the C++ suite from `build/`. Verify: `ctest --test-dir build -j 8 -E Performance
  --output-on-failure` reports `100% tests passed out of 116` (no failure to report).
- [x] 4.8 Record what the brief's Maven install step could not do. NOT RUN AS SPECIFIED:
  `mvn -o -q -f libosmscout-client-java/pom.xml install -DskipTests` fails with `POM file
  libosmscout-client-java/pom.xml specified with the -f/--file command line argument does not exist` (exit
  1) — this tree has no Maven project for the client library. Verify: the freshly built jar was installed
  into the local repository with the mechanism the repository documents
  (`mvn -o install:install-file -Dfile=build/libosmscout-client-java/libosmscoutclientjava.jar ...`),
  and the JavaScout test then ran against it offline (3.4).
- [ ] 4.9 NOT RUN: an end-to-end navigation test on a roundabout (a fix on the ring, the engine's enter and
  leave callbacks). Reason: `maps/` holds format v26 databases while the library expects v27, so no routable
  database can be opened here and no database may be imported for a check, and there is no display. The model
  is covered by the Java tests and the JNI contract by the descriptor check.

## 5. Change hygiene

- [x] 5.1 Verify the diff touches only the three source files and this change's directory. Verify:
  `git status --short` before staging lists the three modified files and the four files of this change
  directory, nothing else.
- [x] 5.2 Record one wording difference from the task brief. The brief calls the two shorter overloads
  "12- and 13-parameter"; the arity that predates the roundabout fields carries 11 parameters, so the
  shortest delegating constructor is the 11-parameter one and its test (3.3) is the 11-argument call. Verify:
  `javap -s` lists the four constructors as 5, 14, 13 and 11 parameters, and the delegation values are
  `0, true, 0` for the shortest one.
- [x] 5.3 Keep the sibling change's subject out of this one, including the comment it edits. Verify: the
  comment above `instructionCtor` states the descriptor requirement only (two lines, no reference to the
  dispatch or to another change), and `DispatchRouteInstructions` is untouched — the build-then-allocate
  dispatch and the pending-exception handling belong to `client-java-jni-lookup-resilience`, which is stacked
  on this change and names this one as its base.
- [x] 5.4 Verify no version bump and no new dependency apply. Verify: no file under `libosmscout/` and no
  file-format or type-config version constant is touched; the values come from the in-memory route
  description, and the Java side gains no import.
