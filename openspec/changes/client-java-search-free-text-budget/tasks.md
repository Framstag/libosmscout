# Tasks

## 1. Budget counts kept candidates (spec: search-free-text)

- [x] 1.1 Add `naviveylin::FreeTextBudget(limit, extent)` to
  `libosmscout-client-java/src/search_scope.h`, whose `Admit(lat, lon)` counts only hits inside the
  extent and whose `Reached()` reads the kept count. Verify: the header still includes no libosmscout
  header, so the host suite compiles it.
- [x] 1.2 In `DoSearchLocations`, resolve the region handle's scope and derive its extent before the
  database walk. Verify: the name-based path still leaves the extent unset.
- [x] 1.3 Replace the `freeTextLimitReached` lambda with a `FreeTextBudget` and admit each built entry
  through `Admit(entry.lat, entry.lon)` in the collection loop. Verify: a refused hit is not pushed onto
  `freeTextEntries`.
- [x] 1.4 Delete the post-walk `std::remove_if` scope filter and move the dropped/scanned debug log into
  the collection block. Verify: no `scopeExtent.isSet` filter pass remains.

## 2. Host test (spec: search-free-text)

- [x] 2.1 Add `Tests/src/SearchScopeTest.cpp` case "A scoped free-text walk collects the requested
  candidates, not fewer": 60 reported hits, the last 30 in scope, deliver 30 candidates for a limit of 30.
- [x] 2.2 Add case "A refused hit does not spend the budget": the single allowed candidate survives an
  out-of-scope hit.
- [x] 2.3 Add case "Without a scope every hit spends the budget": an unset extent spends the budget on the
  hits themselves.

## 3. Verification (spec: search-free-text)

- [x] 3.1 Rebuild `SearchScopeTest` and run `ctest -R SearchScopeTest --output-on-failure` from `build`.
  Verify: 12 cases pass.
- [x] 3.2 Rebuild `osmscout_client_java`. Verify: the JNI module compiles and links.
- [x] 3.3 Falsification: make `Reached()` read the scanned count, rebuild and run the test. Verify: "A
  scoped free-text walk collects the requested candidates, not fewer" fails with `kept 0 of 30`; restore
  the accounting and verify it is green again.
- [x] 3.4 Run `openspec validate client-java-search-free-text-budget --strict`. Verify: it passes.

## 4. Change hygiene

- [x] 4.1 Verify the diff touches only the three source files the proposal's Impact section lists, and that
  no build file, database, map, style or file-format-version file changed.

## Workflow follow-up

- Archive the change once the review requirements are satisfied.
- Verify the archived result.
