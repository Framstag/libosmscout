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
- [x] 4.4 Verify on a real map: a scoped search no longer returns a hit from another map region's free-text
  index, and an unscoped search returns it. Verify: with a map directory the search test reports the
  narrowed and the unscoped result sets, and reports a skip without one. — RE-PROBED 2026-10-27 after the
  free-text merge was repaired (archived change `2026-10-27-client-java-free-text-hits-survive`; before it,
  every free-text hit was erased by the bridge's dedup, so this task was not exercisable at all).
  Log `evidence/search-scope-probe-nrw-2026-10-27-postfix.log`, probe `evidence/SearchScopeProbe.java`
  (updated to the current `beginSearch`/`cancelSearch` API). On `maps/nordrhein-westfalen`:

  | query | unscoped | scoped | unscoped span | scoped span | unscoped text-index hits outside the scoped span |
  |---|---|---|---|---|---|
  | `Bahnhof` | 61 | 64 | 50.37..52.24 / 6.46..9.15 | 50.75..51.67 / 7.14..8.51 | 36 of 50 |
  | `Hauptstraße` | 50 | 53 | 50.38..52.27 / 6.25..8.77 | 50.88..51.69 / 7.16..8.45 | 47 of 50 |
  | `Kirchstraße` | 50 | 51 | 50.39..52.15 / 6.24..8.62 | 50.79..51.67 / 7.15..8.85 | 49 of 50 |

  All scoped hits report `inSearchScope=true` (0 out of scope), and the scoped sets stay inside the extent
  of `Regierungsbezirk Arnsberg` while the unscoped sets span the whole database — so a text-index hit
  outside the scope is no longer returned by a scoped search, and an unscoped search does return it.
  Two caveats are recorded rather than hidden: the true extent box is not exposed to Java, so "outside" is
  measured against the scoped set's own span (a conservative proxy; every scoped hit lies inside the real
  extent, which the verdict flag reports); and the free-text candidate page is capped at the limit *before*
  the extent filter runs, so a scoped search returns few text hits rather than a page of them. The task's
  own clause is satisfied; the probe's earlier 2026-10-16 numbers came from an API generation without the
  search generations and were structured hits, not text-index hits (see `verification.md`).
- [x] 4.5 Run the rest of the C++ and Java suites and verify no existing test regresses. RUN 2026-10-16: the
  C++ suite passed `100% tests passed out of 158` and the JavaScout suite `Tests run: 205, Failures: 0,
  Errors: 0, Skipped: 38`, both exit 0, with the tree and the client jar rebuilt from this revision, so the
  blockers this item named (stale binaries in the shared `build/`, a jar of another branch in the local
  Maven repository) no longer apply. Logs, commands and revision: `verification.md`, "Host verification
  2026-10-16".
- [x] 4.6 Run `openspec validate "client-java-search-scope-extent" --strict` and verify the change validates.

## 5. Documentation and change hygiene

- [x] 5.1 Verify the header documents the fallback box size, the fail-open rule for an unset extent and the
  superset property, and that the result field is documented where a consumer reads it.
- [x] 5.2 Verify the diff touches only the files listed in the proposal's Impact section, and that no
  database, map or style file changed.

## 6. Verification evidence

See `verification.md` for the exact commands and their results.
