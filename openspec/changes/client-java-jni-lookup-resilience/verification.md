# Verification

Change: `client-java-jni-lookup-resilience` — the Java client's JNI bridge clears the exception a failed
class or method lookup leaves pending, and builds every route instruction before it allocates the array it
delivers.

## 1. What was implemented

`libosmscout-client-java/src/OSMScoutClient.cpp` (the only source file touched):

- Three static helpers before `GetNavigationListenerMethods`: `FindClassOrClear(JNIEnv *, const char *)`,
  `GetMethodOrClear(JNIEnv *, jclass, const char *, const char *)` and
  `GetStaticMethodOrClear(...)`. Each returns the raw call's value and calls `env->ExceptionClear()` when
  that value is null, with the comment block that explains the pending exception, the
  `JNI DETECTED ERROR IN APPLICATION: JNI FindClass called with pending exception` abort and the
  2026-10-14 emulator measurement.
- Every lookup inside `GetNavigationListenerMethods` converted: the eleven listener `GetMethodID` calls,
  the `FindClass` of `NavigationPosition`, `NavigationState`, `RouteInstruction`, `TurnType` and
  `LaneTurn`, and the six constructor / static method ids taken on those classes
  (`positionCtor`, `stateValueOf`, `instructionCtor`, `turnTypeValueOf`, `laneTurnFromId`). No raw
  `env->FindClass` / `env->GetMethodID` / `env->GetStaticMethodID` is left in the function.
- `DispatchRouteInstructions` builds every instruction into a `std::vector<jobject> built`
  (`built.reserve(instructions.size())`) first, returns early when `built.empty()`, and only then
  allocates the array with `static_cast<jsize>(built.size())` and fills it, deleting each local ref.
- The comment above `methods.instructionCtor` gains the reachability clause (a mismatch leaves
  `instructionCtor` null, `CreateJavaRouteInstruction` reports that by returning null,
  `DispatchRouteInstructions` answers by sending no instructions, and the path is only reachable because
  the lookup clears its own pending exception); the base's roundabout-descriptor sentence is kept.

The hunks were ported from the reference commit (`3b29204e3` on `origin/naviveylin-local`) and its net
file diff against `origin/master`; no file was taken wholesale (`git checkout <ref> -- <file>` and
`git apply` were not used). The ported regions are byte-identical to the reference:

```
$ git diff --no-index --stat <(git show origin/naviveylin-local:libosmscout-client-java/src/OSMScoutClient.cpp) libosmscout-client-java/src/OSMScoutClient.cpp
 1 file changed, 13 insertions(+), 356 deletions(-)
```

The 13 insertions are the routing-profile lines another unit owns (`maxSpeedKmh`, `ResolveMaxSpeedKmh`,
`ParametrizeFor…`); the 356 deletions are that unit's other branch work (the type-icon entry point, the
roundabout fields it already merged and this base's newer master material). **No line of the lookup
wrappers, the converted lookups or the dispatch differs from the reference.**

## 2. CMake build

- `cmake --build build --target osmscout_client_java java_jar` (the target rebuilt immediately before the
  checks):

```
[1/5] Compiling Java sources and generating JNI headers
[2/5] Packaging Java classes into jar
[3/5] Building CXX object libosmscout-client-java/CMakeFiles/osmscout_client_java.dir/src/OSMScoutClient.cpp.o
[4/5] Linking CXX shared library libosmscout-client-java/libosmscout_client_java.so
```

Exit 0, no error, and no warning from the touched file (the warnings in the transcript belong to the
CMake re-configure: pre-existing `find_package_handle_standard_args` author warnings for Glib2/GObject/
Pango/Pangocairo/Pangoft2 and the `Skip …` demo messages). A second run reports `ninja: no work to do.`

## 3. Meson build

- `meson compile -C debug osmscout_client_java`:

```
[5/7] Compiling C++ object libosmscout-client-java/src/libosmscout_client_java.so.1.1.1.p/OSMScoutClient.cpp.o
[6/7] Linking target libosmscout-client-java/src/libosmscout_client_java.so.1.1.1
```

Exit 0.
- `meson compile -C debug libosmscoutclientjava`:

```
69 warnings
[2/2] Creating JAR libosmscout-client-java/java/libosmscoutclientjava.jar
```

Exit 0. The 69 warnings are the pre-existing javadoc warnings of other classes (e.g.
`PoiCategories.java:66: warning: no @return`), not of this change.
- **NOT RUN: the bare `meson compile -C debug`** (whole tree). It stalls the shell tool; the two targets
  this change touches were compiled explicitly instead (above), which is what the meson-parity check
  needs.

## 4. `scripts/check-jni-signatures.sh`

```
$ bash scripts/check-jni-signatures.sh
WARNING: JNI function without a Java declaration: getMaxSpeedAt (jdouble,jdouble)
WARNING: JNI function without a Java declaration: searchLocationByForm (jstring,jstring,jstring,jstring,jint)
WARNING: JNI function without a Java declaration: reloadBasemap ()
WARNING: JNI function without a Java declaration: getAddressAt (jdouble,jdouble)
WARNING: JNI function without a Java declaration: getDatabaseBoundingBox (jstring)
JNI signatures match: 57 native declarations checked, 5 dead JNI functions reported above.
```

Exit 0, and the count is **57** — unchanged, as it must be: this change adds no native declaration, adds
no Java declaration and touches no Java file. The five reported functions are pre-existing and unrelated
to this diff.

## 5. OpenSpec

- `openspec validate client-java-jni-lookup-resilience --strict` — `Change
  'client-java-jni-lookup-resilience' is valid`. The ADDED requirement descriptions stay below strict
  mode's 500-character limit; the design detail lives in `design.md`.

## 6. C++ test suite

`ctest --test-dir build -j 8 -E Performance --output-on-failure`

```
100% tests passed out of 116

Total Test time (real) =  19.98 sec
```

116 of 116 passed, 0 failed. This is the count of the base commit (the brief's "117 of current master" is
the count of `master`, which also carries the type-icon unit); the count is unchanged from the base, and
no failure and no regression was observed.

## 7. What could not be run, and what is covered instead

- **NOT RUN: a device or emulator check of the defect itself** — mutate the `instructionCtor` descriptor
  so it matches no constructor, run a navigation session, and observe that no instruction list is
  delivered instead of a process abort. Reasons: (a) the helpers wrap `JNIEnv`, and a pending exception
  only aborts a process with a live JVM, so no host test can exercise the defect — there is no JNI
  environment in the Catch2 suite and a fake `JNIEnv` would test the fake; (b) there is no device or
  emulator on this machine; (c) `maps/` holds format v26 databases while the library expects v27, so no
  routable database can be opened here, no database may be imported for a check, and no navigation
  session can be started locally.
- The evidence that this is a real defect is **the reference branch's own measurement, not one taken
  here**: on 2026-10-14, on `emulator-5554`, with the instruction-constructor descriptor mutated to a
  signature no constructor has, the abort landed on the *next* lookup — the `FindClass` for `TurnType` —
  and the instrumentation reported "Process crashed", not a failed case; with the clear in place the same
  mutation delivers no instruction list. It is recorded in the comment above the wrappers
  (`OSMScoutClient.cpp`) and in the reference commit message `3b29204e3`.
- **Covered instead**: the build (sections 2 and 3), the unchanged JNI signature check (section 4) and the
  full C++ suite (section 6). The spec scenarios about a mismatching descriptor, a missing class and a
  complete set of ids are satisfied by inspection of the converted lookups and the build-then-allocate
  dispatch, not by a runtime test.
- **NOT RUN: the bare `meson compile -C debug`** — see section 3.

## 8. The risk this change cannot cover here

A regression in the **success path**: a lookup that used to succeed now returning null. The change's own
reason for why it cannot happen is structural — the wrappers return exactly what the raw JNI call
returned, with the same `JNIEnv` and the same arguments, and the only statement added on the success path
is a null test whose branch is not taken; the ids, the `NewGlobalRef` calls and the descriptors are
unchanged. It is stated in the PR body because the build and the suite cannot distinguish the wrappers
from the raw calls.

## 9. Diff hygiene

`git status --short` after the commit lists only the pre-existing untracked entries
(`maps/repository/`, `openspec/changes/add-type-descriptions-de-en/`) — nothing of this change is left
uncommitted and no other unit's file is touched. The commit stages only
`libosmscout-client-java/src/OSMScoutClient.cpp` and
`openspec/changes/client-java-jni-lookup-resilience/`; `git add -A`, `git add .` and `git commit -a` were
not used.
