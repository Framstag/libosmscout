# Tasks

Spec references: `location-search-api` — requirement `Location search API on OSMScoutClient`
(modified) and requirement `JNI bridge for location search` (modified).

## 1. Reproduce

- [x] 1.1 Add a failing section to `Tests/src/SearchForLocationByStringTest.cpp` searching "August
  Warkner Platz Eving" against the committed `August-Warkner-Platz` location in
  `Tests/LocationTest.olt`; verify the `LocationLookupTest` target fails in exactly this section
  (spec: location-search-api — hyphen-joined location found by words spelled apart).

- [x] 1.2 Add a hyphen-joined POI to `Tests/LocationTest.olt` and a failing section searching its
  words spelled apart; verify the POI path reproduces the miss too
  (spec: location-search-api — hyphen-joined name found by words spelled apart).

- [x] 1.3 Add a failing section to `Tests/src/SearchForLocationByFormTest.cpp` for the hyphen-joined
  street via form components; verify it fails before the fix
  (spec: location-search-api — form search finds a hyphen-joined street).

## 2. Matcher

- [x] 2.1 Implement the word-aware matcher class plus its factory in
  `libosmscout/include/osmscout/util/StringMatcher.h` /
  `libosmscout/src/osmscout/util/StringMatcher.cpp`, substring path first, existing classes
  unchanged; verify the host build compiles
  (spec: location-search-api — separator-insensitive matching; design D2).

- [x] 2.2 Add `Tests/src/StringMatcherTest.cpp` covering both separator directions, slash, en dash,
  transliteration/sharp-s across the separator, reordered and interrupted runs, full versus partial
  coverage, unchanged substring cases, and the fast path for single-word patterns; verify the new
  test binary passes
  (spec: location-search-api — boundary, quality and preservation scenarios).

- [x] 2.3 Register the new test binary in **both** build systems — `Tests/CMakeLists.txt` and
  `Tests/meson.build` — and verify it is built and run by each
  (spec: location-search-api — boundary, quality and preservation scenarios).

- [x] 2.4 Verify the evaluation order is pinned behaviourally: let a substring hit continue into the
  word matching (mutation check) and verify `StringMatcherTest` fails, then restore and verify the
  suite passes again
  (spec: location-search-api — substring and prefix results unchanged).

- [x] 2.5 Give the sections that exercise the word-aware matcher a shared setup — one `SearchForString()`
  helper that takes an optional matcher factory and one `WordMatchingMatcher()` helper — so no two
  sections repeat the parameter construction and their assertions
  (spec: location-search-api — hyphen-joined name found by words spelled apart; verify: the new
  sections are a call, their expectations and nothing else, and the sections that share an outcome
  are asserted once instead of per spelling).

## 3. Search path

- [x] 3.1 Use the new matcher factory for the query-string search, the admin-region resolution and
  the form search in `libosmscout-client-java/src/OSMScoutClient.cpp`
  (spec: location-search-api — JNI bridge requirement).

- [x] 3.2 Use the same factory for the classification of a free-text text-index hit as a match or a
  candidate, so every name comparison on the search path follows one rule
  (spec: location-search-api — a free-text hit is classified with the same matching; design D3).

- [x] 3.3 Verify no name comparison on the search path still builds the previous factory — all four
  sites go through the shared factory helper and a grep for the previous factory type in
  `libosmscout-client-java/src/OSMScoutClient.cpp` returns no hit
  (spec: location-search-api — JNI bridge requirement).

- [x] 3.4 Turn the reproduction sections green and add the negative sections (reordered query,
  interrupted query) plus a preservation section for an existing space-separated name; verify the
  whole `LocationLookupTest` target passes
  (spec: location-search-api — reordered/interrupted words do not match; substring and prefix
  results unchanged).

- [x] 3.5 Leave the text-index keys unchanged and record the consequence: the index still misses a
  separator-joined name, no database needs to be re-imported
  (spec: location-search-api — substring and prefix results unchanged; design D4).

## 4. Suite and landing

- [x] 4.1 Run the full host test suite (all targets) and verify no test regresses and no new
  warnings appear
  (spec: location-search-api — substring and prefix results unchanged).

- [x] 4.2 Configure and build with CMake and with Meson and verify both complete without errors or
  new warnings, in particular for the touched translation units `StringMatcher.cpp` and
  `OSMScoutClient.cpp`.

- [ ] 4.3 Verify the client-Java library and all three Android ABIs still compile against this
  revision
  (spec: location-search-api — JNI bridge requirement).

- [ ] 4.4 Run the C++ test suite with the database-driven search tests enabled against a local
  database extract and verify the search-level scenarios pass there as well.

## 5. Conventions and documentation

- [ ] 5.1 Check every new or modified line against the documents in `guidelines/` and verify the
  touched files satisfy them, in particular the naming, comment and include-order rules.

- [ ] 5.2 Search the documentation and `AGENTS.md` for a description of the search matching and
  verify it either already accounts for the new rule or is updated in this change.

## Verification Notes

- Host verification: CMake Release build with the Java client enabled (`OSMSCOUT_BUILD_CLIENT_JAVA=ON`)
  and the Meson build both complete without errors. `ctest -j 4` reports 134 of 134 tests passing,
  `StringMatcherTest` included, and `meson test -C build-meson` reports the same. No warning is
  reported for `StringMatcher.cpp`, `StringMatcher.h`, `OSMScoutClient.cpp` or the Java client
  translation unit in either build; the warnings both builds do report come from pre-existing code
  (`libosmscout-client-qt`, generated Qt moc files, and the Doxygen configuration) and are untouched
  by this change.
- Mutation check (task 2.4): replacing the word-match verdict with a non-match makes both
  `StringMatcherTest` and `LocationLookupTest` fail, so the new tests pin the added rule and not just
  the pre-existing substring behavior. Letting a substring hit continue into the word matching
  instead — the evaluation order the fast path claims — also makes `StringMatcherTest` fail, so the
  order is pinned by results and not by a measurement. Both changes were reverted and the suite
  passes again.
- The fast path's cost claim (a substring hit does not pay for splitting the candidate into words)
  is not asserted. It was first pinned with a test-local `operator new`/`delete` counter, which had
  to go: replacing the global allocation functions fails to link against the memory sanitizer runtime
  (six `multiple definition` errors against `libclang_rt.msan_cxx`), and on MinGW the counter stayed
  at zero for both paths, so the assertion compared `0 < 0`. Both findings came from continuous
  integration, were reproduced locally for the sanitizer case, and are recorded in the design's risk
  table. Pinning the cost would need an allocation hook in the library, not a test-local replacement.
- Test duplication: the search-level sections that were added first repeated an eight-line parameter
  construction each, and two of them asserted the same outcome for two spellings, which the quality
  gate reports as new duplicated code. They now go through the shared helpers of task 2.5, and the
  two spellings of the POI query are one section looping over both.
- The change ships as one branch off `origin/master` (`fix-compound-name-matching`) containing the
  matcher, the four search-path sites, the test data, the tests and these artifacts. It is the first
  selectively extracted part of the `naviveylin-local` branch (PR #1773); the test-only refactor of
  `Tests/src/StringUtilsTest.cpp` and the readability rework of the search-scope resolution in
  `OSMScoutClient.cpp` stay behind there.
- The debug `log.Info()` traces that PR #1773 carries in `OSMScoutClient.cpp`, its duplicated
  comment separator and its reworded `BuildPoiEntry` comment are deliberately not extracted — they
  are unrelated to this change.
- Task 4.3 and 4.4 need an Android toolchain and a local database extract, neither of which is
  available here; both remain open rather than claimed.
