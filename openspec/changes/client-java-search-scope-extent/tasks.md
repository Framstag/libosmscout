# Tasks

## 1. Extent helpers (spec: search-context-region)

- [x] 1.1 Extend `libosmscout-client-java/src/search_scope.h` with the geographic extent: a box value that
  can be "unset", a constructor from two corners that normalizes the corner order and clamps the latitude,
  a constructor for a box around a position with a documented default half-size, a containment test that
  admits every position for an unset box and rejects a non-finite position for a set one, and a
  "contains another box" test. Verify: the header keeps no libosmscout include, so it stays host-testable.
- [x] 1.2 Keep the existing product-specific namespace and header-guard name (`naviveylin`, the
  `NAVIVEYLIN_SEARCH_SCOPE_H` guard) instead of renaming them: master already carries the
  `test-search-scope-rule` change with that namespace in `search_scope.h` and in `SearchScopeTest.cpp`, so a
  rename here would diverge from the merged content of the same file and touch code this change does not
  own. The new extent helpers therefore live in the same namespace as the expansion rule they sit beside.
  Verify: the header guard and namespace are unchanged, and the change introduces no new product name.
- [x] 1.3 Extend `Tests/src/SearchScopeTest.cpp`: a position inside the box is admitted and one outside is
  rejected, the corner order is normalized, the latitude is clamped, an unset box admits everything, a
  non-finite position is rejected, the point box contains its point at the documented size and honours an
  explicit smaller half-size, and a smaller box inside a region's box is contained while the reverse is
  not. Verify: every scenario of the extent requirements has a case.
- [x] 1.4 Verify the superset property has teeth: make the derivation shrink the box and confirm the
  containment case fails, then restore it and confirm it passes again.

## 2. Derivation and filter in the bridge (spec: search-context-region)

- [x] 2.1 In `libosmscout-client-java/src/OSMScoutClient.cpp`, derive the extent from the resolved region's
  own object: an area or a way contributes its bounding box, a node region the documented fallback box, and
  every other outcome (no region, no database, object not loadable, invalid box) leaves it unset. Verify:
  no other code path invents an extent.
- [x] 2.2 Apply the extent as a filter to hits of every loaded database, including free-text hits, and leave
  the filter off when the extent is unset. Verify: with an unset extent the result set equals the unscoped
  result set.
- [x] 2.3 Keep the existing region-based scoping inside the owning database unchanged. Verify: the diff of
  the search parameter construction is limited to the new filter.

## 3. Reported verdict (spec: search-context-region)

- [x] 3.1 Add the verdict field to
  `libosmscout-client-java/java/com/framstag/libosmscout/client/LocationEntry.java`, documented, defaulting
  to "inside" so an older bridge and an older consumer keep working together.
- [x] 3.2 Set the field on every result the bridge serializes, and only from the extent filter's decision.
  Verify: a search without a scope reports every result as inside, and a scoped search reports the verdict
  per result.
- [x] 3.3 Run `scripts/check-jni-signatures.sh` and verify it passes (no JNI signature changed).

## 4. Build and regression verification (spec: search-context-region)

- [x] 4.1 Build the CMake build with the Java client library and the tests, and verify it compiles without
  errors and without warnings from the touched files.
- [x] 4.2 Build the Meson build the same way and verify it compiles without errors.
- [x] 4.3 Run the extended `SearchScopeTest` through both build systems and verify every case passes.
- [ ] 4.4 Verify on a real map: a scoped search no longer returns a hit from another map region's free-text
  index, and an unscoped search returns it. Verify: with a map directory the search test reports the
  narrowed and the unscoped result sets, and reports a skip without one.

  **Not done, and not feasible in this environment.** The map databases available here are format v26 while
  the library expects v27, so no database can be opened; importing a v27 database is out of scope for this
  change. The bridge part (deriving the extent from a loaded region object, applying the filter, serialising
  the verdict) therefore has no host test and no map test here; see `verification.md` for what was verified
  instead.
- [ ] 4.5 Run the rest of the C++ and Java suites and verify no existing test regresses.

  **Not done.** The shared `build/` directory holds binaries from other branches (a stale binary aborts),
  and a full-suite run would require rebuilding every target rather than the explicit targets this
  environment allows. The Java suite of `JavaScout` needs the client jar installed into the local Maven
  repository and a usable map database. What covers the touched surface instead: `SearchScopeTest`,
  `scripts/check-jni-signatures.sh` and both build systems compiling the bridge and the Java sources (see
  `verification.md`).
- [x] 4.6 Run `openspec validate "client-java-search-scope-extent" --strict` and verify the change validates.

## 5. Documentation and change hygiene

- [x] 5.1 Verify the header documents the fallback box size, the fail-open rule for an unset extent and the
  superset property, and that the result field is documented where a consumer reads it.
- [x] 5.2 Verify the diff touches only the files listed in the proposal's Impact section, and that no
  database, map or style file changed.

## 6. Verification evidence

See `verification.md` for the exact commands and their results.
