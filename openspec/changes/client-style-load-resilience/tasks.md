# Tasks

Spec: `specs/client-java-style-switching/spec.md`. Design: `design.md` D1–D5.
Origin: `naviveylin-local` (commit `9f99f7edf`) — this change re-creates that work as an
independent, upstreamable patch on `master`.

## 1. Change artifacts and branch

- [ ] 1.1 Create the branch `client-style-load-resilience` from current `master` and commit the four
      artifacts of this change (proposal, spec delta, design, tasks). Verify: `openspec validate
      client-style-load-resilience` passes. (spec: all three deltas)
- [ ] 1.2 Re-check every `file:line` in design.md's Context table against the branch's checkout and
      correct the drifted ones. Verify: each referenced line still shows the quoted fact.

## 2. Client configuration lifecycle (install on clean parse)

- [ ] 2.1 Add the fallback configuration parameter to `DBInstance::LoadStyle`
      (`libosmscout-client/include/osmscoutclient/DBInstance.h`,
      `libosmscout-client/src/osmscoutclient/DBInstance.cpp`): the candidate configuration is adopted
      only when the parse reported no errors; on a rejected parse the previously installed
      configuration stays, and the fallback is installed only when nothing was ever installed.
      Verify: compiling the client plus the test added in 4.1, which fails against the unpatched
      `LoadStyle`. (spec: Client switches active style at runtime; A database always has a usable style
      configuration)
- [ ] 2.2 Install the already constructed safe configuration (`emptyStyleConfig`) instead of leaving a
      database without one, on every path that can fail to load — database open, `LoadStyleInternal`
      and `LoadBasemap`. Verify: no code path in `DBThread.cpp` assigns a null style configuration
      anymore. (spec: A database always has a usable style configuration)
- [ ] 2.3 Report the load outcome per database and which stylesheet is active afterwards: add
      `activeStyleSheetFilename` / `lastStyleLoadSucceeded` with
      `GetActiveStyleSheetFilename()` / `WasLastStyleLoadSuccessful()` to
      `libosmscout-client/include/osmscoutclient/DBThread.h` and
      `libosmscout-client/src/osmscoutclient/DBThread.cpp`, documented. Verify: the accessors are filled
      on both the success and the failure branch, and the header documents the contract. (design D4)
- [ ] 2.4 Apply the basemap rule: a rejected basemap stylesheet keeps the map rendering and only drops
      the basemap layer, and is reported as a failed load. Verify: review of `LoadBasemap()` shows no
      path that leaves the basemap database without a configuration. (spec: Basemap stylesheet fails to
      load)

## 3. Java-facing reporting and render safety

- [ ] 3.1 Add the JNI entry point `wasLastStyleLoadSuccessful` and make `getActiveStyleSheet` report the
      actually installed stylesheet (falling back to the configured one before any successful load) in
      `libosmscout-client-java/src/OSMScoutClient.cpp`. Verify: both symbols are wired to the accessors
      added in 2.3. (spec: Client switches active style at runtime; Session start with an unloadable
      style)
- [ ] 3.2 Rework `loadStyleSheet` to decide on the new load-outcome flag instead of comparing the error
      count before and after, and keep restoring the persisted selection on failure. Verify: the
      explicit switch still returns `false` for an unknown style and for an unparsable stylesheet.
      (spec: Switching to an unknown style fails; Switching to an unloadable stylesheet fails)
- [ ] 3.3 Keep the render batch from painting a database without a configuration and document the guard
      as a safety net, not a normal path. Verify: review of the render path shows no per-frame lookup or
      allocation added. (spec: Rendering never uses a style configuration from a failed load)
- [ ] 3.4 Declare `wasLastStyleLoadSuccessful()` and update the javadoc of `getActiveStyleSheet()` and
      `loadStyleSheet()` in
      `libosmscout-client-java/java/com/framstag/libosmscout/client/OSMScoutClient.java`. Verify: the
      documented behaviour matches the tests of group 4. (spec: Client switches active style at runtime;
      design D4)

## 4. Tests

- [ ] 4.1 Add `Tests/src/StyleLoadResilienceTest.cpp` covering: a rejected stylesheet keeps the
      previously active configuration; a first failed load installs the fallback; valid → rejected →
      valid recovers; a batch with one fallback-configured and one healthy database paints without a
      fault. Use a stylesheet with a missing module plus a syntax error — not an invalid colour literal,
      which asserts in the colour helper instead of reporting a parse error. Verify: revert check — the
      test fails against the unpatched client. (spec: A database always has a usable style
      configuration; Rendering never uses a style configuration from a failed load)
- [ ] 4.2 Register the new test in `Tests/CMakeLists.txt` (with
      `TESTS_TOP_DIR=${CMAKE_CURRENT_SOURCE_DIR}`) and `Tests/meson.build`. Add the entries to the
      current `master` files — do not copy the older NaviVeylin versions of those two files, which drop
      `BasemapCheckTest` and revert the `JsonWriterTest` linking. Verify: both files list exactly the
      new test and nothing else changed. (spec: A database always has a usable style configuration)
- [ ] 4.3 Assert the diagnostic channel: the error list is populated after a failed load on a non-switch
      path. Verify: the test of 4.1 asserts a non-empty error list for the direct load. (design D4)
- [ ] 4.4 Confirm no existing test relied on the old behaviour (database left without a configuration).
      Verify: the full client test target is green in both build systems; any changed expectation is
      explained in the task notes. (spec: Rendering never uses a style configuration from a failed load)

## 5. Build and verification gates

- [ ] 5.1 Build the client library and the new test target in the CMake configuration and verify it
      compiles without errors and without new warnings in the touched files. Verify: build log clean for
      `DBInstance.cpp`, `DBThread.cpp`, `OSMScoutClient.cpp`, `StyleLoadResilienceTest.cpp`.
- [ ] 5.2 Run the tests in the CMake configuration to completion and record the counts. Verify: the new
      test and every test linking `OSMScout::Client` pass.
- [ ] 5.3 Configure and build the touched sources in the Meson configuration as well. Verify: the Meson
      build compiles the client library and the new test, and existing Meson tests still pass.

## 6. Documentation and pull request

- [ ] 6.1 Check `guidelines/` and the client documentation for statements about stylesheet loading or the
      render configuration that the change invalidates, and update them if so. Verify: no guideline
      contradicts the implemented behaviour.
- [ ] 6.2 Record the `**BREAKING**` assessment in the pull-request description: no API is removed or
      renamed; `DBInstance::LoadStyle` gains a defaulted parameter and `DBThread` gains two accessors.
      Verify: the PR body names both signature changes explicitly.
- [ ] 6.3 Open the pull request against `master` with the change's proposal, spec delta and design
      linked, and no unrelated file in the diff. Verify: `git diff master...client-style-load-resilience
      --stat` lists only the files named in the proposal's Impact section.
