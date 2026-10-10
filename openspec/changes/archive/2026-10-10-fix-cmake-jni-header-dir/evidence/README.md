# Evidence — fix-cmake-jni-header-dir

| file | what it carries |
|---|---|
| `red-case-java_compile-target.log` | HEAD run, 2026-10-10T14:44:58+02:00: after writing the sentinel stale header and forcing the rule, `cmake --build build --target java_compile` ran `[1/1] Compiling Java sources and generating JNI headers`, rc=0, and the sentinel survived — `RED: stale sentinel header survived the header-generating target`. |
| `headers-current.txt` | the latent state: the header directory held exactly the four headers the current sources produce (`MapDownloadManager`, `NavigationController`, `OSMScoutClientBuilder`, `OSMScoutClient` — one per class declaring a native method; `LocationEntry` and `RouteEntry` are named by `java/meson.build`'s `native_headers()` but declare none), no stale file; `build/` is git-ignored, so the defect is invisible to the tree. |
| `green-case-java_compile-target.log` | after the fix, 2026-10-10T14:45:57+02:00: the same construction, sentinel gone — `GREEN: stale sentinel header removed, directory mirrors the current sources`. This file carries the case for tasks 2.2 and 2.4. |
| `case-after-comment-rewording.log` | the same case re-run on the final tree at 2026-10-10T15:12:58+02:00, after the comment wording of the fix hunk was corrected to "removed or renamed class"; the `rm -rf` command line is unchanged. Sentinel gone, four current headers present. |
| `green-build-osmscout_client_java.log` | `cmake --build build --target osmscout_client_java java_jar` rc=0; the C++ client's header exists (17407 B). Its last line, `82646 40 files`, is the summary row `unzip -l` prints (total uncompressed size in bytes, then entry count), not a count of classes or headers; no claim rests on it. |
| `green-build-osmscout_client_java-forced.log` | the library target with the C++ source touched: `Building CXX object … OSMScoutClient.cpp.o` then linking, rc=0 — the client compiles against the emptied-and-regenerated header directory. |
| `probe-java_compile-target.log` | probe, 2026-10-10T14:48:20+02:00: with the added `rm -rf ${JNI_HEADERS_DIR}` commented out and marked `TEMP-REVERT-PROBE`, the sentinel survived again — the case fails for the right reason. |
| `probe-restore.log` | the marker removed, the fix active: sentinel gone again, `git diff -- libosmscout-client-java/CMakeLists.txt` shows the intended hunk alone, marker grep empty. |
| `gate-phaseD-corrected.log` + `gate-run.sh` | the CMake build gate: (a) sentinel gone after `java_compile` rc=0, (b) all four current headers present, (c) full `cmake --build build -j 16` rc=0 with `OSMScoutClient.cpp.o` rebuilt and the shared library linked; the touched C++ source's content is unchanged. |
| `gate-ctest.log` | the full CMake test gate, `ctest -j 2 --output-on-failure` with `QT_QPA_PLATFORM=offscreen TESTS_TOP_DIR=…/Tests TESTS_TMP_DIR=…/Tests`, window 2026-10-10T15:11:29 to 15:14:22: `100% tests passed out of 149`, `Total Test time (real) = 173.16 sec`. Caveat: both gate runs started before the 15:12:27 comment edit of `libosmscout-client-java/CMakeLists.txt`; they cover the fix itself (the `rm -rf` was already in place, and `gate-phaseD-corrected.log` at 15:02 proves the rule works), and only `case-after-comment-rewording.log` (15:12:58) postdates the comment. |
| `gate-meson.log` + `gate-meson.sh` | the Meson gate, `meson compile -C build-meson` and `meson test -C build-meson --timeout-multiplier 2 --print-errorlogs`, 2026-10-10T15:11:51 onward: `meson compile rc=0`, `Ok: 149`, `Fail: 0`, `meson test rc=0`; a parity check, since Meson does not read `CMakeLists.txt`. Its final block is a re-run of the script's `find` only (15:21:07), because the 15:11 pattern `-path "*header*"` matched nothing; it lists the four headers. |

The superseded first gate log (`gate-phaseD.log`) was deleted; it carried two artifacts of the interactive
shell (a word-splitting `b FAIL` line and an empty `${PIPESTATUS}` rc) and `gate-phaseD-corrected.log` is the
reproducible replacement.

## Deliberately skipped

- `uncrustify` on `CMakeLists.txt`: the file is CMake, not C++ (`-l CPP` would misparse it); the change adds
  comment and command lines matching the adjacent class-directory style.

## Observations not caused by this change

- `libosmscout-map-opengl/src/osmscoutmapopengl/MapPainterOpenGL.cpp:454` warns about an unused-but-set
  `lineOffset` in the full build; it is in another module and was not touched here.
- The Meson-side divergence named in `design.md`'s Non-Goals is grounded here: `meson compile` generates the
  same four headers into `build-meson/libosmscout-client-java/java/` (2026-10-10T15:11), and
  `libosmscout-client-java/java/meson.build:1` shows no cleaning step for that directory. The header list is
  retained in `gate-meson.log`'s appended `find` block (15:21:07).
