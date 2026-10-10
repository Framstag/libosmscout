# Tasks

## 1. Red case on HEAD

- [x] 1.1 Record the failing sentinel run before the fix: write
  `com_framstag_libosmscout_client_OSMScoutClient_removedMethod.h` into
  `build/libosmscout-client-java/java/headers`, force the rule with
  `rm -f build/libosmscout-client-java/java/headers/com_framstag_libosmscout_client_OSMScoutClient.h`,
  run `cmake --build build --target java_compile`, and confirm the sentinel survives —
  `evidence/red-case-java_compile-target.log` carries `RED: stale sentinel header survived the
  header-generating target`, 2026-10-10T14:44:58+02:00.

## 2. Fix and its verification

- [x] 2.1 In `libosmscout-client-java/CMakeLists.txt`, add
  `COMMAND ${CMAKE_COMMAND} -E rm -rf ${JNI_HEADERS_DIR}` before `make_directory ${JNI_HEADERS_DIR}` in the
  `java_compile` custom command, with a comment matching the class-directory comment; verify
  `cmake --build build --target java_compile` re-runs the rule and the build directory reconfigures
  cleanly. (spec: JNI header generation / A header of a removed source is not kept)
- [x] 2.2 Verify scenario "Headers generated before C++ compile": build
  `cmake --build build --target osmscout_client_java` and confirm the rule ran and
  `build/libosmscout-client-java/java/headers/com_framstag_libosmscout_client_OSMScoutClient.h` exists
  before the C++ compile; retain `evidence/green-build-osmscout_client_java.log`. (spec: JNI header
  generation / Headers generated before C++ compile)
- [x] 2.3 Verify scenario "All Java sources produce headers": after the fix the header directory holds the
  same four headers as before (`MapDownloadManager`, `NavigationController`, `OSMScoutClientBuilder`,
  `OSMScoutClient`) — one per class that declares a native method; `LocationEntry` and `RouteEntry` are in
  `java/meson.build`'s `native_headers()` list but declare none, so they emit no header — i.e. no header the
  current sources produce was lost; retain `evidence/headers-current.txt`. (spec: JNI header generation /
  All Java sources produce headers)
- [x] 2.4 Verify scenario "A header of a removed source is not kept": repeat task 1.1's construction after
  the fix and confirm the sentinel is gone while the four current headers are regenerated; retain
  `evidence/green-case-java_compile-target.log`. (spec: JNI header generation / A header of a removed
  source is not kept)

## 3. Probe

- [x] 3.1 Prove the case discriminates: re-comment out the added `rm -rf ${JNI_HEADERS_DIR}` command with a
  `TEMP-REVERT-PROBE` marker, rerun task 2.4's construction and confirm the sentinel survives for the right
  reason, then restore the fix and confirm the sentinel is gone and `git diff --
  libosmscout-client-java/CMakeLists.txt` is the fix hunk alone; retain `evidence/probe-java_compile-target.log`.

## 4. Gate

- [x] 4.1 Run the CMake build gate `cmake --build build -j "$(nproc)"` and confirm it compiles the affected
  targets with no warning from the touched file; the log is `evidence/gate-phaseD-corrected.log`. (spec: JNI
  header generation)
- [x] 4.2 Confirm this change's own diff touches the CMake file only:
  `git diff --stat -- libosmscout-client-java/CMakeLists.txt` shows the 3-line hunk. The tree-wide
  `git diff --stat` additionally lists `TODO.md`, which is loop bookkeeping and not this change. `uncrustify`
  does not apply — the touched file is CMake, not C++ (see `evidence/README.md`).
- [x] 4.3 Run the full CMake test gate `cd build && QT_QPA_PLATFORM=offscreen TESTS_TOP_DIR="$PWD/../Tests"
  TESTS_TMP_DIR="$PWD/Tests" ctest -j 2 --output-on-failure` and retain it as `evidence/gate-ctest.log`; the
  run reports `100% tests passed out of 149` and `Total Test time (real) = 173.16 sec`, window
  2026-10-10T15:11:29 to 15:14:22. Caveat: both gate runs started before the 15:12:27 comment edit of
  `libosmscout-client-java/CMakeLists.txt`; they cover the fix itself — the `rm -rf` was already in place, and
  `evidence/gate-phaseD-corrected.log` (15:02) proves the rule works — and only
  `evidence/case-after-comment-rewording.log` (15:12:58) postdates the comment. The gate was not re-run for a
  comment-only edit that changes no executed line and whose re-run would overwrite the retained logs. (spec:
  JNI header generation)
- [x] 4.4 Run the Meson gate `meson compile -C build-meson` and `meson test -C build-meson
  --timeout-multiplier 2 --print-errorlogs`; retain `evidence/gate-meson.log`. It reports `meson compile rc=0`,
  `Ok: 149`, `Fail: 0`, `meson test rc=0` (started 2026-10-10T15:11:51). The Meson build does not read
  `CMakeLists.txt` (named deviation, `design.md` Non-Goals), so this run is a parity check. (spec: JNI header
  generation)

## 5. Backlog

- [x] 5.1 Leave the `TODO.md` entry's metadata line at `fixed-by` for the archived change; if the change is
  not archived in this iteration, report it as a removal candidate instead of editing the entry.

## Workflow follow-up

- Archive the change after the project's review requirements are satisfied, then re-check the archived
  evidence with `evidence-check.sh`.
