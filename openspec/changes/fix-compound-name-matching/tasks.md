# Tasks

## 1. Reproduce

- [ ] 1.1 Add a failing section to `Tests/src/SearchForLocationByStringTest.cpp` searching "August Warkner Platz Eving" against the committed `August-Warkner-Platz` location in `Tests/LocationTest.olt`; verify the `LocationLookupTest` target fails in exactly this section (spec: location-search-api — hyphen-joined location found by words spelled apart)
- [ ] 1.2 Add a hyphen-joined POI to `Tests/LocationTest.olt` and a failing section searching its words spelled apart; verify the POI path reproduces the miss too (spec: location-search-api — hyphen-joined name found by words spelled apart)
- [ ] 1.3 Add a failing section to `Tests/src/SearchForLocationByFormTest.cpp` for the hyphen-joined street via form components; verify it fails before the fix (spec: location-search-api — form search finds a hyphen-joined street)

## 2. Matcher

- [ ] 2.1 Implement the word-aware matcher class plus its factory in `libosmscout/include/osmscout/util/StringMatcher.h` / `libosmscout/src/osmscout/util/StringMatcher.cpp`, substring path first, existing classes unchanged; verify the host build compiles (spec: location-search-api — separator-insensitive matching; design D2)
- [ ] 2.2 Add `Tests/src/StringMatcherTest.cpp` (registered in `Tests/CMakeLists.txt`) covering both separator directions, slash, en dash, transliteration/sharp-s across the separator, reordered and interrupted runs, full versus partial coverage, unchanged substring cases, and the fast path for single-word patterns; verify the new test binary passes (spec: location-search-api — boundary, quality and preservation scenarios)
- [ ] 2.3 Verify the fast-path assertion fails when the substring step is skipped for single-word patterns (mutation check) and restore; verify the suite passes again (spec: location-search-api — substring and prefix results unchanged)

## 3. Search parameters

- [ ] 3.1 Use the new matcher factory for the query-string search, the admin-region resolution and the form search in `libosmscout-client-java/src/OSMScoutClient.cpp`; verify no call site still uses the previous factory (spec: location-search-api — JNI bridge requirement)
- [ ] 3.2 Turn the reproduction sections green and add the negative sections (reordered query, interrupted query) plus a preservation section for an existing space-separated name; verify the whole `LocationLookupTest` target passes (spec: location-search-api — reordered/interrupted words do not match; substring and prefix results unchanged)

## 4. Suite and landing

- [ ] 4.1 Run the full host test suite (all targets) and verify no test regresses and no new warnings appear (spec: location-search-api — substring and prefix results unchanged)
- [ ] 4.2 Verify all three Android ABIs still compile the app build with this submodule revision and verify the client-Java library builds for each ABI (spec: location-search-api — JNI bridge requirement)
- [ ] 4.3 Commit the work on `naviveylin-local` (matcher, parameters, test data, tests, these artifacts), push it, and verify the working tree is clean (spec: location-search-api — the change ships as a pinned submodule revision)
