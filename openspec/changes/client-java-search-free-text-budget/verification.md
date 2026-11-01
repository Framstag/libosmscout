# Verification evidence

## 1. CMake

- `cmake --build build --target SearchScopeTest` — builds the case and the updated
  `search_scope.h`; 3 ninja steps, no error.
- `cd build && ctest -R SearchScopeTest --output-on-failure` — `1/1 Test #67: SearchScopeTest ...
  Passed`, `100% tests passed out of 1`.
- `cd build && ctest -R SearchScopeTest -V` — `All tests passed (69 assertions in 12 test cases)`, the
  reference's figure.
- `cmake --build build --target osmscout_client_java` — the JNI module compiles and links with the
  restructured call site: `[2/2] Linking CXX shared library
  libosmscout-client-java/libosmscout_client_java.so`.

## 2. Falsification: the budget must count kept hits

The pre-change accounting was restored by making `FreeTextBudget::Reached()` read the scanned count
(`return scanned >= limit;`) instead of the kept count, in
`libosmscout-client-java/src/search_scope.h`. Rebuilt and run:

- `cmake --build build --target SearchScopeTest`
- `cd build && ctest -R SearchScopeTest --output-on-failure` — `***Failed`, `0.00 sec`:

```
Tests/src/SearchScopeTest.cpp:208: FAILED:
  REQUIRE( budget.Kept()==30 )
with expansion:
  0 == 30
...
test cases: 12 | 10 passed | 2 failed
assertions: 61 | 59 passed | 2 failed
```

Exactly the failure the reference names: "A scoped free-text walk collects the requested candidates, not
fewer" fails with `kept 0 of 30`, because the budget stopped the walk after 30 scanned hits, all of them
out of scope. The accounting was then restored (`git diff --
libosmscout-client-java/src/search_scope.h` is empty apart from the intended addition to the base branch),
rebuilt and run green again: `All tests passed (69 assertions in 12 test cases)`.

## 3. Spec scenario coverage

| Scenario | How it is verified |
|----------|--------------------|
| A scoped walk collects the requested candidates | `A scoped free-text walk collects the requested candidates, not fewer` walks 60 hits whose last 30 are in scope and asserts `Kept()==30`, `Dropped()==30`, `Scanned()==60`. |
| An out-of-scope hit does not spend the budget | `A refused hit does not spend the budget` admits an out-of-scope hit, asserts `Reached()` is still false, then admits an in-scope hit and asserts `Kept()==1`. |
| Without a scope every hit spends the budget | `Without a scope every hit spends the budget` uses `UnsetGeoBox()` and asserts every hit is admitted and `Dropped()==0`. |

## 4. What is not verified here, and why

- The end-to-end Java search cannot run: `maps/` is data format v26 while the library expects v27, and no
  routable/usable map database is installed. No map database was imported for this change. The host test
  `SearchScopeTest` covers the budget rule, and the JNI call site in `DoSearchLocations` is
  compile-verified by the `osmscout_client_java` build above.
- `scripts/format-check.sh` is red repo-wide; it was not used as a gate. The added lines follow the style
  of the code around them.

## 5. Change hygiene

- `openspec validate client-java-search-free-text-budget --strict` — valid (see section 1 of the PR body).
- The diff touches only the files the proposal's Impact section lists: `search_scope.h`,
  `OSMScoutClient.cpp`, `Tests/src/SearchScopeTest.cpp` and this change. No build file, database, map,
  style or file-format-version file changed — the test file is already registered in `Tests/CMakeLists.txt`
  and `Tests/meson.build` with the `libosmscout-client-java/src` include path.
