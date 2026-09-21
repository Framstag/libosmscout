# Tasks

Spec: `specs/client-java-style-switching/spec.md`. Design: `design.md` D1–D5.
Origin: `naviveylin-local` (commit `9f99f7edf`) — this change re-creates that work as an
independent, upstreamable patch on `master`.

## 1. Change artifacts and branch

- [x] 1.1 Create the branch `client-style-load-resilience` from current `master` and commit the four
      artifacts of this change (proposal, spec delta, design, tasks). Verify: `openspec validate
      client-style-load-resilience` passes. (spec: all three deltas) — Done: branch `client-style-load-resilience`
      off `master` `c91dd4abc`; artifacts committed as `a33661fc8`; `openspec validate` reports valid.
- [x] 1.2 Re-check every `file:line` in design.md's Context table against the branch's checkout and
      correct the drifted ones. Verify: each referenced line still shows the quoted fact. — Done: all eight
      references confirmed on the branch (`DBInstance.cpp:28`/`:73`, `DBThread.cpp:61`/`:440`/`:447`/`:567`/`:596`,
      `StyleConfig.cpp:1830`, `OSMScoutClient.cpp:796`/`:851`/`:1273`/`:1371`); no drift, no edit needed.

## 2. Client configuration lifecycle (install on clean parse)

- [x] 2.1 Add the fallback configuration parameter to `DBInstance::LoadStyle`
      (`libosmscout-client/include/osmscoutclient/DBInstance.h`,
      `libosmscout-client/src/osmscoutclient/DBInstance.cpp`): the candidate configuration is adopted
      only when the parse reported no errors; on a rejected parse the previously installed
      configuration stays, and the fallback is installed only when nothing was ever installed.
      Verify: compiling the client plus the test added in 4.1, which fails against the unpatched
      `LoadStyle`. (spec: Client switches active style at runtime; A database always has a usable style
      configuration) — Done: `DBInstance.h`/`DBInstance.cpp` adopt the candidate only on a clean parse,
      keep the installed configuration on a rejection and install `fallback` only when none was ever
      installed. `ninja -C build libosmscout_client.so` clean. Revert check recorded under 4.1.
- [x] 2.2 Install the already constructed safe configuration (`emptyStyleConfig`) instead of leaving a
      database without one, on every path that can fail to load — database open, `LoadStyleInternal`
      and `LoadBasemap`. Verify: no code path in `DBThread.cpp` assigns a null style configuration
      anymore. (spec: A database always has a usable style configuration) — Done: the database-open path
      (`OnDatabaseListChanged`) and `LoadBasemap()` install `emptyStyleConfig` on a rejected load and on
      an invalid type config; the `nullptr` assignments in `DBThread.cpp` are gone.
- [x] 2.3 Report the load outcome per database and which stylesheet is active afterwards: add
      `activeStyleSheetFilename` / `lastStyleLoadSucceeded` with
      `GetActiveStyleSheetFilename()` / `WasLastStyleLoadSuccessful()` to
      `libosmscout-client/include/osmscoutclient/DBThread.h` and
      `libosmscout-client/src/osmscoutclient/DBThread.cpp`, documented. Verify: the accessors are filled
      on both the success and the failure branch, and the header documents the contract. (design D4) —
      Done: both members declared next to `stylesheetFilename` (so the ctor init order stays valid),
      accessors are read-locked; `LoadStyleInternal` sets `lastStyleLoadSucceeded=succeeded &&
      styleErrors.empty()` and stores `activeStyleSheetFilename=file` only on success. Deviation from
      the origin patch, for D4: on a newly scanned database the active file name is kept when one is
      already known, instead of being cleared (clearing would make `getActiveStyleSheet()` report the
      configured file while another style is genuinely active).
- [x] 2.4 Apply the basemap rule: a rejected basemap stylesheet keeps the map rendering and only drops
      the basemap layer, and is reported as a failed load. Verify: review of `LoadBasemap()` shows no
      path that leaves the basemap database without a configuration. (spec: Basemap stylesheet fails to
      load) — Done: both the rejected-stylesheet and invalid-type-config branches of `LoadBasemap()`
      install `emptyStyleConfig` and mark the load failed with a warning.

## 3. Java-facing reporting and render safety

- [x] 3.1 Add the JNI entry point `wasLastStyleLoadSuccessful` and make `getActiveStyleSheet` report the
      actually installed stylesheet (falling back to the configured one before any successful load) in
      `libosmscout-client-java/src/OSMScoutClient.cpp`. Verify: both symbols are wired to the accessors
      added in 2.3. (spec: Client switches active style at runtime; Session start with an unloadable
      style) — Done: `getActiveStyleSheet` returns the file name of `GetActiveStyleSheetFilename()` when
      set, else the configured file; `wasLastStyleLoadSuccessful` returns
      `WasLastStyleLoadSuccessful()`. Java/JNI header regeneration and the native library build both
      succeed, so the symbol signatures match.
- [x] 3.2 Rework `loadStyleSheet` to decide on the new load-outcome flag instead of comparing the error
      count before and after, and keep restoring the persisted selection on failure. Verify: the
      explicit switch still returns `false` for an unknown style and for an unparsable stylesheet.
      (spec: Switching to an unknown style fails; Switching to an unloadable stylesheet fails) — Done:
      `previousErrorCount` removed; the failure branch is now `!WasLastStyleLoadSuccessful()`. The
      unknown-name path still returns `false` before the load (`std::filesystem::exists` check), and the
      unparsable path restores `previousFile` and returns `false`.
- [x] 3.3 Keep the render batch from painting a database without a configuration and document the guard
      as a safety net, not a normal path. Verify: review of the render path shows no per-frame lookup or
      allocation added. (spec: Rendering never uses a style configuration from a failed load) — Done:
      the existing `GetStyleConfig()` guard in `loadDbData` is kept and now logs which database was
      skipped; no new lookup or allocation was added to the render path.
- [x] 3.4 Declare `wasLastStyleLoadSuccessful()` and update the javadoc of `getActiveStyleSheet()` and
      `loadStyleSheet()` in
      `libosmscout-client-java/java/com/framstag/libosmscout/client/OSMScoutClient.java`. Verify: the
      documented behaviour matches the tests of group 4. (spec: Client switches active style at runtime;
      design D4) — Done: the declaration plus javadoc were added; `getActiveStyleSheet` documents that a
      failed stylesheet is never reported as active and `loadStyleSheet` points at the new flag.

## 4. Tests

- [x] 4.1 Add `Tests/src/StyleLoadResilienceTest.cpp` covering: a rejected stylesheet keeps the
      previously active configuration; a first failed load installs the fallback; valid → rejected →
      valid recovers; a batch with one fallback-configured and one healthy database paints without a
      fault. Use a stylesheet with a missing module plus a syntax error — not an invalid colour literal,
      which asserts in the colour helper instead of reporting a parse error. Verify: revert check — the
      test fails against the unpatched client. (spec: A database always has a usable style
      configuration; Rendering never uses a style configuration from a failed load) — Done:
      `Tests/src/StyleLoadResilienceTest.cpp` with 4 cases: rejected keeps previous, first-load fallback
      installs the fallback, valid → rejected → valid recovery, batch painting with a fallback-configured
      database. `ctest -R StyleLoadResilienceTest` passed. Revert check performed: mutating
      `if (!styleConfig && fallback)` to `if (fallback)` in `DBInstance::LoadStyle` makes "Rejected
      stylesheet keeps the previously active configuration" fail at `GetStyleConfig() == active`
      (1 failed / 35 assertions), and the mutation was reverted afterwards.
- [x] 4.2 Register the new test in `Tests/CMakeLists.txt` (with
      `TESTS_TOP_DIR=${CMAKE_CURRENT_SOURCE_DIR}`) and `Tests/meson.build`. Add the entries to the
      current `master` files — do not copy the older NaviVeylin versions of those two files, which drop
      `BasemapCheckTest` and revert the `JsonWriterTest` linking. Verify: both files list exactly the
      new test and nothing else changed. (spec: A database always has a usable style configuration) —
      Done: `Tests/CMakeLists.txt` gains the test plus the `TESTS_TOP_DIR` property;
      `Tests/meson.build` gains the executable and the `env:` test entry. `git diff` on both files shows
      only those additions — `BasemapCheckTest` and the `JsonWriterTest`/`DbJsonWriterTest` registrations
      are untouched.
- [x] 4.3 Assert the diagnostic channel: the error list is populated after a failed load on a non-switch
      path. Verify: the test of 4.1 asserts a non-empty error list for the direct load. (design D4) —
      Done: "Rejected stylesheet keeps the previously active configuration" asserts
      `errors.empty() == false` after a direct `DBInstance::LoadStyle` call.
- [x] 4.4 Confirm no existing test relied on the old behaviour (database left without a configuration).
      Verify: the full client test target is green in both build systems; any changed expectation is
      explained in the task notes. (spec: Rendering never uses a style configuration from a failed load) —
      Done: CMake `ctest -j4` 125/126 and Meson `meson test` 126/126. The single CMake failure is
      `MapPainterAreaVisibilityCullTest`, which is pre-existing and unrelated: this change touches no file
      under `libosmscout-map/` (`git diff master -- libosmscout-map` is empty), the failing assertion
      expects a pixel-based border tolerance while `master` still computes millimetres — exactly what the
      open PR #1825 (`fix-area-cull-pixel-tolerance`) corrects. That test's file and expectations were not
      modified here.

## 5. Build and verification gates

- [x] 5.1 Build the client library and the new test target in the CMake configuration and verify it
      compiles without errors and without new warnings in the touched files. Verify: build log clean for
      `DBInstance.cpp`, `DBThread.cpp`, `OSMScoutClient.cpp`, `StyleLoadResilienceTest.cpp`. — Done:
      `ninja -C build libosmscout_client.so libosmscout_client_java.so StyleLoadResilienceTest` — no
      warnings. The Java/JNI header regeneration step also ran, so the new native declaration matches the
      generated header. Note: `clang-tidy` is not part of this repository's CI, and the local
      `scripts/format-check.sh check` reports 791 of 890 tracked sources as unformatted **including
      unmodified `master` files** (verified against `master:DBInstance.cpp` and `master:DBThread.h`), so it
      is not a usable gate in this checkout; the new test file itself is reported clean.
- [x] 5.2 Run the tests in the CMake configuration to completion and record the counts. Verify: the new
      test and every test linking `OSMScout::Client` pass. — Done: `xvfb-run -a ctest -j4` → 125/126 passed,
      21.8 s. `FavoriteLocationServiceTest` and `StyleLoadResilienceTest` both pass; the style-related tests
      (`CheckStyleSheet-*`, `StyleConfigSymbolsTest`, `StyleConfigVisibilityBoundsTest`) pass. Only
      `MapPainterAreaVisibilityCullTest` fails (pre-existing, see 4.4).
- [x] 5.3 Configure and build the touched sources in the Meson configuration as well. Verify: the Meson
      build compiles the client library and the new test, and existing Meson tests still pass. — Done:
      `meson compile -C build-meson` built `libosmscout_client_java.so`, `StyleLoadResilienceTest` and the
      rest (110 targets) with no errors; `meson test --timeout-multiplier 2 -C build-meson` → 126/126 OK.
      `Check style load resilience` and `Check FavoriteLocationService` OK.

## 6. Documentation and pull request

- [x] 6.1 Check `guidelines/` and the client documentation for statements about stylesheet loading or the
      render configuration that the change invalidates, and update them if so. Verify: no guideline
      contradicts the implemented behaviour.
- [x] 6.2 Record the `**BREAKING**` assessment in the pull-request description: no API is removed or
      renamed; `DBInstance::LoadStyle` gains a defaulted parameter and `DBThread` gains two accessors.
      Verify: the PR body names both signature changes explicitly. — Done: the PR body has a `BREAKING`
      section stating none, and names both additive changes (`DBInstance::LoadStyle` defaulted
      `fallback` parameter, `DBThread::GetActiveStyleSheetFilename()` / `WasLastStyleLoadSuccessful()`).
- [x] 6.3 Open the pull request against `master` with the change's proposal, spec delta and design
      linked, and no unrelated file in the diff. Verify: `git diff master...client-style-load-resilience
      --stat` lists only the files named in the proposal's Impact section. — Done: pushed the branch to
      `origin` and opened **PR #1828** (`fix: keep the active style when a stylesheet fails to load`),
      linking this change. `git diff master --name-only` lists exactly the nine source/build/test files of
      the proposal's Impact section plus this change's five artifacts.
