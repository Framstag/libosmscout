# Verification

Evidence for `client-java-free-text-hits-survive`. Every command ran from the repository root in
`/home/tim/projects/libosmscout` unless the working directory is named. Checked-in source touched:
`libosmscout-client-java/src/OSMScoutClient.cpp` (one function), plus the new JavaScout test.

## 1. The defect, measured before the fix

A temporarily instrumented bridge (three `osmscout::log.Info()` lines, reverted afterwards; the tree carried
no other change) on the v27 database `maps/nordrhein-westfalen`, query `Hauptstraße`:

```
search probe: resultsTxt=6 refs=3076 buildFail=0      <- the text index finds candidates
search probe: beforeDedup=50                          <- the loop built a full page
search probe: structured=0 freeText=0 scopeExtentSet=false query=Hauptstraße
```

The dedup pass that follows the loop removed all 50 entries: the loop had inserted each hit's offset into
`seenOffsets[db]`, and the pass erased every free-text entry whose offset is in that same set
(`OSMScoutClient.cpp`, before the fix). `freeText=0` in every call, scoped and unscoped.

## 2. The regression test, failing before the fix

`JavaScout/src/test/java/com/framstag/libosmscout/client/OSMScoutClientFreeTextResultTest.java`, run against
the pre-fix binary (the fix copied aside, `git checkout --` of the touched file, library rebuilt):

```
cd JavaScout && xvfb-run -a mvn -o test \
  -Dnative.lib.dir=…/build/libosmscout-client-java \
  -Dsearch.test.db.dir=…/maps/nordrhein-westfalen \
  -Dtest=OSMScoutClientFreeTextResultTest
```

```
[ERROR] Tests run: 2, Failures: 2, Errors: 0, Skipped: 0
[ERROR]   OSMScoutClientFreeTextResultTest.freeTextOnlyHitIsReturned:97 the text index holds streets named
          'Hauptstraße', so the result list must not be empty ==> expected: <true> but was: <false>
[ERROR]   OSMScoutClientFreeTextResultTest.objectFoundByBothSearchesAppearsOnce:117 a scoped search for a
          bare street name must include text-index hits ==> expected: <true> but was: <false>
```

That is the test's teeth: both cases fail on the unfixed bridge.

## 3. The fix

`DoSearchLocations` keeps two per-database offset sets: `seenTextOffsets` collapses the duplicates the text
index reports for one object (it names an object once per matched name), `seenOffsets` holds the offsets the
structured results reported and stays the set the dedup pass compares against. The diff of the function is
`12 insertions, 4 deletions`; the structured-dedup pass itself is unchanged, and both sets stay keyed by
database, so the per-database identity rule (the same offset in two databases is two objects) is kept.

## 4. Post-fix measurements

Test, same command as section 2:

```
[INFO] Tests run: 2, Failures: 0, Errors: 0, Skipped: 0, Time elapsed: 3.309 s
[INFO] BUILD SUCCESS
```

Search probe of `client-java-search-scope-extent` (`evidence` of that change, current API, post-fix binary),
query `Hauptstraße` on `maps/nordrhein-westfalen`:

| call | hits |
|---|---|
| unscoped | 50 (repeat 50) |
| scoped to `Innenstadt West` / `Regierungsbezirk Arnsberg` | 53 (repeat 53) |
| same native call with `NO_ADMIN_REGION` | 50 |
| unscoped span | lat `50.383 .. 52.274`, lon `6.252 .. 8.775` (the whole database) |
| scoped span | lat `50.876 .. 51.695`, lon `7.159 .. 8.455` (inside the scope's region) |

The free-text part now reaches the caller, and — as a side effect that task 4.4 of
`client-java-search-scope-extent` needed — the extent filter of an active scope now applies to real hits,
which is why the scoped set is confined to the region's extent.

A separate throwaway probe answered the ordering question the test raised: the first search after
`openDatabase` already returns text-index hits (`call=1 total=50 freeText=50`), so no warm-up call is needed.
The test builds one client for the class because the builder returns the process-wide instance — a client
closed by one case would be handed to the next already closed (that produced a false failure in an earlier
revision of the test, not in the bridge).

## 5. Builds and suites

- `cmake --build build --target osmscout_client_java java_jar` — compiles the touched translation unit and
  relinks `libosmscout_client_java.so`, no warning from the file.
- JavaScout suite, whole module, `-Dsearch.test.db.dir=…/maps/nordrhein-westfalen`:
  - run 1: `Tests run: 269, Failures: 1, Errors: 0, Skipped: 18` — the one failure
    `OSMScoutClientBasemapConfigTest.testBasemapLookupDirectoryWithoutDatabaseKeepsRendering:373`
    ("a directory without an openable basemap must not stop map rendering"), the documented full-suite flake
    of TODO.md §91; in isolation that class is `Tests run: 14, Failures: 0, Errors: 0, Skipped: 0`.
  - run 2: `Tests run: 269, Failures: 0, Errors: 0, Skipped: 18`, `BUILD SUCCESS`. 269 = the 267 of the
    baseline plus the two cases this change adds.
- C++ suite:
  `QT_QPA_PLATFORM=offscreen TESTS_TOP_DIR=../Tests TESTS_TMP_DIR=./tmp-suite-2026-10-27 ctest -j 8
  --output-on-failure` → `100% tests passed out of 165`, 66.47 s.
- `bash scripts/check-jni-signatures.sh` → `JNI signatures match: 60 native declarations checked, 5 dead JNI
  functions reported above.` The five dead functions are the pre-existing list; this change adds no
  declaration, no Java method and no export.
- `ninja -C build-meson-check libosmscout-client-java/src/libosmscout_client_java.so.1.1.1` — the Meson
  build compiles the touched translation unit and relinks the library, no error
  (`[212/214] Compiling C++ object …/OSMScoutClient.cpp.o`, `[213/214] Linking target …`). The Meson
  target is addressed by its ninja path because `meson compile -C … libosmscoutclientjava` does not know
  the introspected target names of this directory (the same note as in
  `client-java-render-label-wrap-bounds`).
- `openspec validate "client-java-free-text-hits-survive" --strict` → `Change
  'client-java-free-text-hits-survive' is valid`.

## 6. What this change does not do

- It does not touch the extent filter of `client-java-search-scope-extent`; it only removes the reason that
  filter could never be observed. That change's task 4.4 is verified against the fixed bridge, in that
  change's evidence.
- It does not deduplicate entries the structured search reports twice for one object. That is the structured
  search's own result and outside this merge; the regression test asserts the cross-source rule only.
- No Meson test run was made: the change has no C++ test target of its own; its regression test lives in
  the JavaScout suite, which is run against the CMake-built native library (section 5).
