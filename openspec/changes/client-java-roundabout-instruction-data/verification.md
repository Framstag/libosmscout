# Verification

Change: `client-java-roundabout-instruction-data` — a Java `RouteInstruction` carries a roundabout
manoeuvre's exit ordinal and circulation direction, and the next-next hint's own exit ordinal.

## 1. What was implemented

`libosmscout-client-java/src/OSMScoutClient.cpp`:

- `struct JavaRouteInstruction` gains `roundaboutExitNumber{0}`, `roundaboutClockwise{true}` and (with the
  hint) `nextNextRoundaboutExitNumber{0}`.
- `OnRoundaboutEnter` names its formerly ignored description and reads `IsClockwise()` from it;
  `OnRoundaboutLeave` reads `GetExitCount()` (the exit taken — `RoutePostprocessor::roundaboutCrossingCounter`
  counts the exits passed) and `IsClockwise()`.
- `GenerateNextRouteInstruction` copies the following manoeuvre's own ordinal into the hint.
- `CreateJavaRouteInstruction` passes the three values in the Java parameter order, and
  `GetNavigationListenerMethods` looks the constructor up with the 14-parameter descriptor.

`libosmscout-client-java/java/com/framstag/libosmscout/client/RouteInstruction.java`: the three
`public final` fields, the full 14-parameter constructor, and the two delegating arities (13 and 11
parameters) that keep the older callers compiling with the `0 / true / 0` defaults.

`JavaScout/src/test/java/com/framstag/libosmscout/client/RouteInstructionTest.java`: three cases for the full
constructor, the 13-parameter overload and the 11-parameter one.

The hunks were ported from the reference commits (`98817dc58`, `863bb9d91` on `origin/naviveylin-local`) onto
current master; no file was taken wholesale (`git checkout <ref> -- <file>` and `git apply` were not used).
The roundabout material in `OSMScoutClient.cpp` is identical to the reference: the descriptor string was
compared byte for byte (`grep -o '(DDD.*IZI)V'` on both sides, identical), and the reference's other change to
this file — the build-then-allocate dispatch — is deliberately not part of this unit (see section 8). The Java
file's parent blob on master is the reference's parent, so porting its hunks reproduced the reference content
exactly: `diff` of the working file against `origin/naviveylin-local:<file>` is empty.

## 2. CMake build

- `cmake --build build --target osmscout_client_java java_jar` — on the final source state:
  `[1/2] Building CXX object
  libosmscout-client-java/CMakeFiles/osmscout_client_java.dir/src/OSMScoutClient.cpp.o`,
  `[2/2] Linking CXX shared library libosmscout-client-java/libosmscout_client_java.so`. Exit 0, no error, no
  warning from the touched file. (The first run of the same command also packaged the Java classes:
  `[12/29] Packaging Java classes into jar`.)

## 3. Meson build

- `meson compile -C debug osmscout_client_java` — on the final source state:
  `[2/3] Compiling C++ object
  libosmscout-client-java/src/libosmscout_client_java.so.1.1.1.p/OSMScoutClient.cpp.o`,
  `[3/3] Linking target libosmscout-client-java/src/libosmscout_client_java.so.1.1.1`. Exit 0.
- `meson compile -C debug libosmscoutclientjava` — `[1/2] Compiling Java sources for
  libosmscout-client-java/java/libosmscoutclientjava.jar`, `[2/2] Creating JAR
  libosmscout-client-java/java/libosmscoutclientjava.jar`. Exit 0. The 69 javadoc warnings it reports are
  pre-existing warnings of other classes (e.g. `BasemapManager.java:450: warning: no @throws`), not of this
  change.
- **NOT RUN: the bare `meson compile -C debug`** (whole tree). It stalled the shell tool and was abandoned;
  the two targets this change touches were compiled explicitly instead (above), which is what the
  meson-parity check needs.

## 4. JNI constructor descriptor check (done by hand, no script covers it)

```
$ javap -s -classpath build/libosmscout-client-java/libosmscoutclientjava.jar com.framstag.libosmscout.client.RouteInstruction
  public com.framstag.libosmscout.client.RouteInstruction(double, double, double, com.framstag.libosmscout.client.TurnType, java.lang.String, java.lang.String, java.lang.String, double, com.framstag.libosmscout.client.TurnType, java.lang.String, java.lang.String, int, boolean, int);
    descriptor: (DDDLcom/framstag/libosmscout/client/TurnType;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;DLcom/framstag/libosmscout/client/TurnType;Ljava/lang/String;Ljava/lang/String;IZI)V
```

The string in `GetNavigationListenerMethods`:

```
(DDDLcom/framstag/libosmscout/client/TurnType;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;DLcom/framstag/libosmscout/client/TurnType;Ljava/lang/String;Ljava/lang/String;IZI)V
```

`diff` of the two strings (each extracted from its own source) is **empty** — identical character for
character. The jar also lists three further constructors, the 5-argument one and the two delegating arities
(descriptors `...Ljava/lang/String;)V` and `...Ljava/lang/String;IZ)V`), i.e. the arities a Java caller keeps.

The `NewObject` argument order matches the Java parameter order:

| Java parameter | `NewObject` argument |
|---|---|
| `double distanceTo` | `instr.distanceTo` |
| `double timeTo` | `instr.timeTo` |
| `double legDistance` | `instr.legDistance` |
| `TurnType turnType` | `turnTypeObj` |
| `String streetName` | `streetNameJ` |
| `String description` | `descriptionJ` |
| `String shortDescription` | `shortDescJ` |
| `double nextNextDistanceTo` | `instr.nextNextDistanceTo` |
| `TurnType nextNextTurnType` | `nnTurnTypeObj` |
| `String nextNextDescription` | `nnDescJ` |
| `String nextNextShortDescription` | `nnShortDescJ` |
| `int roundaboutExitNumber` | `instr.roundaboutExitNumber` |
| `boolean roundaboutClockwise` | `instr.roundaboutClockwise ? JNI_TRUE : JNI_FALSE` |
| `int nextNextRoundaboutExitNumber` | `instr.nextNextRoundaboutExitNumber` |

This check matters because `scripts/check-jni-signatures.sh` checks native method declarations, not
`<init>` descriptors, and a wrong descriptor is a runtime failure, not a compile error.

## 5. `scripts/check-jni-signatures.sh`

```
$ bash scripts/check-jni-signatures.sh
WARNING: JNI function without a Java declaration: getMaxSpeedAt (jdouble,jdouble)
WARNING: JNI function without a Java declaration: searchLocationByForm (jstring,jstring,jstring,jstring,jint)
WARNING: JNI function without a Java declaration: reloadBasemap ()
WARNING: JNI function without a Java declaration: getAddressAt (jdouble,jdouble)
WARNING: JNI function without a Java declaration: getDatabaseBoundingBox (jstring)
JNI signatures match: 57 native declarations checked, 5 dead JNI functions reported above.
```

Exit 0. This change adds no JNI entry point, so the count is unchanged and the five reported functions are
pre-existing (unrelated to this diff). The Java class this change edits is a model class, not a native
declaration, so no Java declaration had to be added.

## 6. OpenSpec

- `openspec validate client-java-roundabout-instruction-data --strict` — `Change
  'client-java-roundabout-instruction-data' is valid`. The ADDED requirement descriptions stay below the
  strict mode's 500-character limit; the design detail lives in `design.md`.

## 7. C++ test suite

`ctest --test-dir build -j 8 -E Performance --output-on-failure`

```
100% tests passed out of 116

Total Test time (real) =  24.59 sec
```

116 of 116 passed, 0 failed. `JniSignatureParityTest` and `RoutePostprocessorTest` are among them (both
passed). No failure to report and no regression observed.

## 8. Java tests, and what the brief's Maven step could not do

- **NOT RUN AS SPECIFIED**: `mvn -o -q -f libosmscout-client-java/pom.xml install -DskipTests` —
  `POM file libosmscout-client-java/pom.xml specified with the -f/--file command line argument does not
  exist`, exit 1. This tree has no Maven project for the client library (`libosmscout-client-java/` holds the
  Java sources, the JNI implementation and a CMake/Meson build, but no `pom.xml`), so there is nothing for
  that command to install. `JavaScout/pom.xml` consumes the artifact
  `net.sf.libosmscout:libosmscout-client-java:1.0-SNAPSHOT` from the local Maven repository.
- The freshly built jar was therefore put into the local repository the way the repository documents it for
  this layout (`libosmscout-client-java/AGENTS.md`, Pitfalls):
  `mvn -o -q install:install-file -Dfile=build/libosmscout-client-java/libosmscoutclientjava.jar
  -DgroupId=net.sf.libosmscout -DartifactId=libosmscout-client-java -Dversion=1.0-SNAPSHOT
  -Dpackaging=jar` — exit 0, offline. (The local repository previously held a stale jar, whose `RouteInstruction`
  did not have the new arity.)
- `mvn -o -f JavaScout/pom.xml test -Dtest=RouteInstructionTest
  -Dnative.lib.dir=/home/tim/projects/libosmscout/build/libosmscout-client-java`:

  ```
  [INFO] Tests run: 8, Failures: 0, Errors: 0, Skipped: 0, Time elapsed: 0.065 s -- in com.framstag.libosmscout.client.RouteInstructionTest
  [INFO] Tests run: 8, Failures: 0, Errors: 0, Skipped: 0
  [INFO] BUILD SUCCESS
  ```

  Eight cases: the five pre-existing ones plus the three added by this change (full constructor with the
  roundabout values, the 13-parameter overload defaulting the next-next ordinal, the 11-parameter overload
  defaulting `0 / true / 0`). `RouteInstructionTest` is a pure-Java test and needs no native library.

## 9. What could not be run

- **NOT RUN: an end-to-end navigation test on a roundabout** (a position on the ring, the engine's enter and
  leave callbacks reaching Java). Every route-measuring test needs a routable database; `maps/` holds format
  v26 databases while this library expects v27, so no routable database can be opened on this machine, no
  database may be imported for a check, and there is no display. The model side is covered by the Java tests
  (section 8), the bridge's data flow by the descriptor check (section 4) and the build (sections 2 and 3);
  the requirement scenarios that describe the bridge marshalling a description are satisfied by inspection of
  the callbacks, not by a runtime test.
- **NOT RUN: the bare `meson compile -C debug`** — see section 3.

## 10. Two differences of wording from the task brief, and what was done

- The brief calls the two shorter constructor overloads "12- and 13-parameter". The constructor that predates
  the roundabout fields carries **11** parameters (`javap -s` lists the four arities as 5, 14, 13 and 11), so
  the shortest delegating overload is the 11-parameter one and its Java test is the 11-argument call. The
  behaviour the brief asks for — a caller written before the roundabout fields keeps compiling and observes
  `0 / true / 0` — is what is implemented and tested.
- The brief asked to keep the reference's comment above `instructionCtor` including its clause about
  `DispatchRouteInstructions` sending no instructions rather than an array with null entries. That
  build-then-allocate dispatch belongs to the sibling change `client-java-jni-lookup-resilience` (stacked on
  this one, and it states that it edits this same descriptor and comment), so on this branch alone the clause
  would describe code that is not here. The comment records only the fact this unit is responsible for: the
  descriptor must match the full Java constructor exactly. `DispatchRouteInstructions` is untouched.

## 11. Diff hygiene

`git status --short` before staging lists the three modified source files and this change's directory
(`proposal.md`, `spec.md`, `design.md`, `tasks.md`, `verification.md`), plus the pre-existing untracked
`maps/repository/` and the other units' change directories, which are not staged. The commit stages only the
three source paths and this change directory.
