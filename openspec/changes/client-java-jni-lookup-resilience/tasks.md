# Tasks

## 1. The lookup wrappers (spec: client-java-jni-method-lookup)

- [x] 1.1 `libosmscout-client-java/src/OSMScoutClient.cpp`: add the three static helpers
  `FindClassOrClear(JNIEnv *, const char *name)`, `GetMethodOrClear(JNIEnv *, jclass, const char *name,
  const char *sig)` and `GetStaticMethodOrClear(...)` directly before `GetNavigationListenerMethods`,
  each returning the raw call's value and calling `env->ExceptionClear()` when it is null, with the
  comment block that explains the pending exception, the `JNI DETECTED ERROR IN APPLICATION` abort and
  the 2026-10-14 emulator measurement. Verify: the three definitions precede `GetNavigationListenerMethods`
  and `git diff` shows the comment block byte-identical to the reference's (including its escaped quotes).
- [x] 1.2 Convert all eleven listener `GetMethodID` calls in `GetNavigationListenerMethods` to
  `GetMethodOrClear`. Verify: `grep -n "env->GetMethodID\|env->FindClass\|env->GetStaticMethodID"` over
  the function's line range matches only the three helpers' own bodies.
- [x] 1.3 Convert the `FindClass` of `NavigationPosition`, `NavigationState`, `RouteInstruction`, `TurnType`
  and `LaneTurn` to `FindClassOrClear`, and the six constructor / static method ids taken on them
  (`positionCtor`, `stateValueOf`, `instructionCtor`, `turnTypeValueOf`, `laneTurnFromId`) to
  `GetMethodOrClear` / `GetStaticMethodOrClear`. Verify: no raw lookup is left in the function (1.2's
  grep), and the cached ids, the `NewGlobalRef` calls and the descriptors are unchanged.
- [x] 1.4 Keep the failure a refusal: no call site gains an error path, a return value or a log.
  Verify: the diff of `GetNavigationListenerMethods` adds no `return`, no log call and no change to the
  `if (cls)` guards or to the trailing `return true`.

## 2. The instruction dispatch (spec: client-java-jni-method-lookup)

- [x] 2.1 `DispatchRouteInstructions`: build every instruction into a `std::vector<jobject> built` first
  (`built.reserve(instructions.size())`), return early when `built.empty()`, and only then allocate the
  array with `static_cast<jsize>(built.size())` and set its elements from `built`, deleting each local
  ref. Verify: the function allocates no array before the loop, returns before the `NewObjectArray` call
  when nothing was built, and deletes exactly one local ref per element.
- [x] 2.2 Port the comment above the vector that says why the array is built first (an instruction the
  bridge cannot build yields null, an input-sized array would carry null entries the listener
  dereferences far from here, and no buildable instruction means no dispatch at all). Verify: the comment
  text is the reference's, byte-identical.
- [x] 2.3 Extend the comment above `methods.instructionCtor` with the reachability clause (a mismatch
  leaves `instructionCtor` null, `CreateJavaRouteInstruction` reports it by returning null,
  `DispatchRouteInstructions` answers by sending no instructions, and that path is only reachable because
  the lookup clears its own pending exception). Verify: the clause is present and the base's
  roundabout-descriptor sentence is kept unchanged.
- [x] 2.4 `DispatchNextRouteInstruction` is untouched: it dispatches one instruction and already skips a
  null one. Verify: the diff contains no hunk inside that function.

## 3. Scope (spec: client-java-jni-method-lookup)

- [x] 3.1 Touch no Java file. Verify: `git status --short` lists exactly one modified source file, and no
  `.java` path appears in `git diff --stat`.
- [x] 3.2 Keep the routing-profile speed lookup out. Verify: `grep -n "maxSpeedKmh\|speedField"` over the
  touched file exits 1 (no match) and neither string appears in `git diff`.
- [x] 3.3 Keep the reference's dispatch and lookup hunks identical to the branch's, without copying the
  file. Verify: `git diff --no-index <(git show origin/naviveylin-local:<file>) <file>` shows only the
  routing-profile lines the other unit owns (13 lines), i.e. every lookup and dispatch line matches.
- [x] 3.4 Verify no version bump and no new dependency apply. Verify: no file under `libosmscout/` is
  touched, no version constant changes, and `libosmscout-client-java/CMakeLists.txt` / `src/meson.build`
  are absent from the diff.

## 4. Verification

- [x] 4.1 Build the CMake targets of this change. Verify: `cmake --build build --target
  osmscout_client_java java_jar` exits 0, compiles the touched translation unit
  (`Building CXX object libosmscout-client-java/CMakeFiles/osmscout_client_java.dir/src/OSMScoutClient.cpp.o`),
  links `libosmscout_client_java.so` and packages the jar, with no warning from the touched file.
- [x] 4.2 Build the Meson targets of this change. Verify: `meson compile -C debug osmscout_client_java`
  compiles `OSMScoutClient.cpp` and links the native library, and `meson compile -C debug
  libosmscoutclientjava` creates the jar, both exit 0 (the jar step reports the 69 pre-existing javadoc
  warnings of other classes).
- [x] 4.3 RUN 2026-10-16: the bare `meson compile -C debug` of the whole tree completed — `637` targets,
  `0` compile errors, exit 0, revision `06b197a35`, log
  `evidence/meson-compile-whole-tree-2026-10-16.log`. The earlier "stalls the shell tool" note is
  superseded: the same command ran to completion in the background (`meson` 1.12.1, ninja, build dir
  `debug/`), which is the meson-parity check this task asks for.
- [x] 4.4 Check the JNI signatures. Verify: `bash scripts/check-jni-signatures.sh` exits 0 and reports
  `JNI signatures match: 57 native declarations checked, 5 dead JNI functions reported above.` — the same
  count as before this change (no native declaration is added), and the five reported functions are
  pre-existing.
- [x] 4.5 Run `openspec validate client-java-jni-lookup-resilience --strict`. Verify: `Change
  'client-java-jni-lookup-resilience' is valid`. (The ADDED requirement descriptions stay under strict
  mode's 500-character limit; the design detail lives in `design.md`.)
- [x] 4.6 Run the C++ suite from `build/`. Verify: `ctest --test-dir build -j 8 -E Performance
  --output-on-failure` reports `100% tests passed out of 116` (the count of this base; no failure to
  report).
- [ ] 4.7 NOT RUN: a device or emulator check of the defect itself (mutate the `instructionCtor`
  descriptor so it matches no constructor, run a navigation session, observe no instruction list instead
  of a process abort). Reason: the helpers wrap `JNIEnv`, and only a live JVM aborts on a pending
  exception, so no host test can exercise it; there is no device or emulator here, and `maps/` holds
  format v26 while the library expects v27, so no navigation session can be started locally either.
  RE-CHECKED 2026-10-16: a routable v27 database now exists (`maps/nordrhein-westfalen`) and a navigation
  session can be started, so that half of the reason no longer applies — but the defect itself stays not
  host-runnable, because the helpers wrap `JNIEnv` and only a live JVM aborts on a pending exception, and
  mutating the descriptor means changing compiled source. The task stays open; see `verification.md`. The
  evidence that this is a real defect is the branch's own 2026-10-14 emulator measurement (recorded in
  `verification.md`), labelled as not taken here. What is covered instead: the build (4.1, 4.2), the
  unchanged JNI signature check (4.4) and the full C++ suite (4.6).

## 5. Change hygiene

- [x] 5.1 Stage only this change. Verify: the commit stages `libosmscout-client-java/src/OSMScoutClient.cpp`
  and `openspec/changes/client-java-jni-lookup-resilience/` and nothing else; `git add -A`, `git add .`
  and `git commit -a` are not used, and `openspec/changes/add-type-descriptions-de-en/` (another
  session's unit) is never staged or edited.
- [x] 5.2 Verify the stacked base. Verify: `git log --oneline -2` shows this branch's parent as
  `client-java: carry a roundabout instruction's exit ordinal and circulation direction` (PR #1888), and
  the PR is opened with `--base client-java-roundabout-instruction-data`, its body starting with
  `Stacked on #1888; retarget to master once #1888 merges.`
- [x] 5.3 Record the coverage gap in the PR body. Verify: the body states that a regression in the
  success path (a lookup that used to succeed now returning null) is the risk this change cannot cover
  here, and gives the change's own reason — the wrappers return exactly what the raw call returned apart
  from clearing the exception.
