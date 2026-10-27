# Tasks

## 1. The dedup bookkeeping

- [x] 1.1 In `libosmscout-client-java/src/OSMScoutClient.cpp` (`DoSearchLocations`), give the free-text
      loop its own per-database offset set next to `seenOffsets`, and use it for the in-loop duplicate
      check and insert. Verify: the loop no longer writes into the set the structured-dedup pass reads.
- [x] 1.2 Keep the structured-dedup pass unchanged: it still collects the structured results' object
      offsets per database and erases the free-text entries that match them. Verify: the diff of that pass
      is empty.
- [x] 1.3 Keep both sets keyed by database and the comment that explains why (the same offset in two
      databases denotes two different objects). Verify: no key type changes.

## 2. Regression test

- [x] 2.1 Add `JavaScout/src/test/java/com/framstag/libosmscout/client/OSMScoutClientFreeTextResultTest.java`
      with the two cases of the design: a bare street name an unscoped search resolves only through the
      text index yields entries with `matchedComponent = "freeText"`, and a search where structured and
      text index both report streets returns no label/position pair twice. Skip both without
      `-Dsearch.test.db.dir`. Verify: the first case fails before 1.1 and passes after it.
- [x] 2.2 Verify the test is not vacuous: run it against the fix reverted (or against the pre-fix binary)
      and record the failure in `verification.md`.

## 3. Build and regression verification

- [x] 3.1 Build the native client library and the jar in CMake (`osmscout_client_java`, `java_jar`) with no
      error and no new warning from the touched translation unit.
- [x] 3.2 Build the same targets through Meson (or an existing Meson build directory) with no error.
- [x] 3.3 Run the new JavaScout test against a v27 database
      (`-Dsearch.test.db.dir=<...>/maps/nordrhein-westfalen`) and verify both cases pass.
- [x] 3.4 Run the whole JavaScout suite with the same properties and compare with the documented baseline
      (the §91 full-suite flakes of `OSMScoutClientStyleTest` / `OSMScoutClientBasemapConfigTest` are
      neither fixed nor blocking; they are documented in `TODO.md`).
- [x] 3.5 Run the C++ suite (`ctest -j 8`) and verify no test regresses.
- [x] 3.6 Run `scripts/check-jni-signatures.sh` and verify it still reports the unchanged declaration list.
- [x] 3.7 Run `openspec validate "client-java-free-text-hits-survive" --strict`.

## 4. Evidence and hand-off

- [x] 4.1 Record in `verification.md` the pre-fix instrumented measurement (`resultsTxt`, `beforeDedup`,
      `freeText=0`), the post-fix measurement of the same query, and the commands with their output.
- [x] 4.2 Note in `verification.md` that `client-java-search-scope-extent` task 4.4 becomes observable with
      this fix, and that the fix does not touch the extent filter.
