# Tasks

## 1. Red case and root cause

- [x] 1.1 Re-confirmed in the tree: `libosmscout/src/osmscout/util/StringMatcher.cpp` carries
      `#include <algorithm>` at line 24 on HEAD, below the project includes at lines 20 and 22, and uses no
      name from `<algorithm>`: the only `std::` names of the file are `std::string` (13, with
      `std::string::npos` 3), `std::vector` (2) and `std::make_shared` (3), all of them provided by
      `libosmscout/include/osmscout/util/StringMatcher.h` (lines 23-25) and
      `libosmscout/include/osmscout/util/String.h` (`evidence/symbol-audit.log`). One root cause: the header
      was included without need and left below the project includes.
- [x] 1.2 Red case, recorded: the case "Check include order of util/StringMatcher.cpp" of
      `Tests/src/HeaderCheckTest.cpp` fails on HEAD - `REQUIRE( violations.size()==0 )`, expansion `1 == 0`,
      test cases: 1 | 1 failed, assertions: 3 | 2 passed | 1 failed, exit code 42, run at 2026-10-16T11:42:06Z
      (UTC). Retained as `evidence/red-head-check.log`, with the pins of the file before the fix and of the
      delivered check revision. Parent requirement: source-include-order / "Standard library includes precede
      project includes".
- [x] 1.3 The check is new and lives in the existing `HeaderCheckTest` binary, which both build systems
      already compile and register (`Tests/CMakeLists.txt:166`, `Tests/meson.build:394`), so no test
      registration changed. The case reads `libosmscout/src/osmscout/util/StringMatcher.cpp` under
      `SOURCE_ROOT` and reports the number it measures.

## 2. The single fix

- [x] 2.1 `libosmscout/src/osmscout/util/StringMatcher.cpp`: the `#include <algorithm>` line and the blank
      line that followed it are deleted; the include block keeps the own header and
      `<osmscout/util/String.h>`. Removal, not relocation: the symbol audit eliminates the file's need for the
      header, so moving it into the standard-library block would keep the unused dependency the entry names.
      Verifies spec source-include-order / "Standard library includes precede project includes".

## 3. Cases and probes

- [x] 3.1 `Tests/src/HeaderCheckTest.cpp`: helper `IncludesAfterProjectHeaders` (line 471) and the case "Check
      include order of util/StringMatcher.cpp" (line 510) assert that the file lists no standard-library
      include behind a project include other than its own header, print
      `STD_INCLUDES_AFTER_PROJECT=<n>` and fail while the number is not 0. Case: "Check include order of
      util/StringMatcher.cpp" - scenario "The checked file lists no standard library include after its
      project headers". The whole binary reports `All tests passed (15 assertions in 3 test cases)`
      (`evidence/after-fix-check.log`).
- [x] 3.2 Probe (`TEMP-REVERT-PROBE`): the include of HEAD was put back into `StringMatcher.cpp` and the case
      run against the binary of the delivered check revision - it fails with exactly the red assertion above
      (`1 == 0`, exit 42, run at 2026-10-16T11:42:11Z), log `evidence/probe-revert-check.log` with the sha256
      of the mutated file. The mutation was then removed; the restored file hashes to the pin of
      `evidence/sha256sums.txt` and no marker remains in the tree (checked by `evidence-check.sh` C4).
      The case guards an improvement's number, so the probe moves the production include block.

## 4. Gate (code delta, both build systems)

- [x] 4.1 `cmake --build build -j "$(nproc)"` - exit 0; the log names the rebuilt object
      `libosmscout/CMakeFiles/OSMScout.dir/src/osmscout/util/StringMatcher.cpp.o` and the relinked
      `Tests/HeaderCheckTest` (`evidence/gate-cmake-build.log`). The 24 warnings the log carries are all
      Doxygen `Doxyfile` tags of the documentation target, none of them from a C++ source.
- [x] 4.2 `cd build && QT_QPA_PLATFORM=offscreen TESTS_TOP_DIR="$PWD/../Tests" TESTS_TMP_DIR="$PWD/Tests"
      ctest -j 8 -E 'NumberSetPerformanceTest' --output-on-failure` - exit 0, `100% tests passed out of 158`,
      `HeaderCheckTest` passed (`evidence/gate-cmake-ctest.log`). The exclusion `NumberSetPerformanceTest` is
      named because the change is not about what that test measures (set/queue throughput).
- [x] 4.3 `meson compile -C debug` - exit 0, rebuilding `src_osmscout_util_StringMatcher.cpp.o` and
      `Tests/HeaderCheckTest`; then `meson test -C debug --timeout-multiplier 2 --print-errorlogs` - exit 0,
      `Ok:                159`, `Fail:              0` (`evidence/gate-meson-build.log`,
      `evidence/gate-meson-test.log`).
- [x] 4.4 `Tests/src/HeaderCheckTest.cpp` and `libosmscout/src/osmscout/util/StringMatcher.cpp`:
      `uncrustify -q -c .uncrustify -l CPP -f <file>` (`evidence/uncrustify.log`). The drift of the test file
      is the two hunk ranges `@@ -10,25 +10,25 @@` and `@@ -53,413 +53,422 @@`; the second range runs to
      `Tests/src/HeaderCheckTest.cpp:465`, two lines into the comment of the block this change adds, so the
      added test block (lines 464-532) is itself unreformatted. The 58 diff lines of `StringMatcher.cpp` are
      the pre-existing drift `TODO.md` §88 records for that file.

## 5. Evidence and bookkeeping

- [x] 5.1 Every cited number lives in a log under `evidence/` with its command, UTC timestamp, exit code and
      the sha256 pins (`evidence/sha256sums.txt`, which pins the two touched files and two of the evidence
      files listed in it). `evidence-check.sh` was run live before the gate and once more before this report;
      the latter run is retained verbatim as `evidence/evidence-check-live.log`:

      ```
      ok   C1 §references: 7 checked, 0 exempted as archived pointers
      ok   C2 tallies: 3 confirmed against 14 carrier(s), 0 exempted (not retained), 1 unparsable, 0 drift
      ok   C3 /tmp pointers: none cited
      ok   C4 markers: no probe marker in any source file under .
      ok   C5 file:line: 270 resolved and in range, 0 unresolved or ambiguous
      ----
      evidence-check: PASS
      ```

      (run at 2026-10-16T11:48:19Z (UTC), exit code 0; the one unparsable line is the `test cases: 1 | 1 failed`
      form of the red log, whose `FAILED` text, expansion and exit code are quoted verbatim there).
- [x] 5.2 `guidelines/FileFormatVersion.md`: no type configuration, database file format or serialized layout
      is touched, so no version bump. `guidelines/CodeStyles.md` §include order: the section the fix restores,
      cited by `proposal.md` and by the check's comment.
- [x] 5.3 `TODO.md` §89: the metadata line states `**status:** fixed-by \`fix-stringmatcher-unused-include\`` once
      the change is archived. The include-order drift of the rest of the tree that phase A measured (113 files,
      `evidence/residue-out-of-order-includes.log`) is filed as a new entry §106.
- [x] 5.4 Note, no edit: `Tests/src/HeaderCheckTest.cpp` itself carries the same class of pre-existing
      uncrustify drift as the files `TODO.md` §88 lists (`evidence/uncrustify.log`); extending that entry's
      list is its own bookkeeping, not this change's.

## 6. Residue

- [x] 6.1 Pending, no host seam: none. The change is host-testable end to end.
