# Verification evidence

## 1. CMake

- `cmake --build build --target SearchScopeTest osmscout_client_java` — compiles the extended test, the
  bridge and the Java sources; no error and no compiler warning from the touched files.
- `cd build && ctest -R SearchScopeTest --output-on-failure` — `1/1 Test #62: SearchScopeTest ... Passed`,
  `100% tests passed out of 1`.
- `./build/Tests/SearchScopeTest` — `All tests passed (50 assertions in 9 test cases)`. The three cases that
  the earlier `test-search-scope-rule` change added are unchanged; the six new cases are the extent ones.
- `cmake --build build --target java_jar` followed by
  `javap -classpath build/libosmscout-client-java/libosmscoutclientjava.jar ...LocationEntry` — reports
  `public boolean inSearchScope`, so the field the bridge looks up by name and descriptor `Z` exists in the
  shipped class.

## 2. JNI signature parity

- `bash scripts/check-jni-signatures.sh` — `JNI signatures match: 55 native declarations checked, 6 dead JNI
  functions reported above.`; the six reported functions are pre-existing and none of them was touched by
  this change.

## 3. Meson

- `meson compile -C build-meson SearchScopeTest osmscout_client_java libosmscoutclientjava` — builds the
  test, the bridge and the jar without errors.
- `meson test -C build-meson "Check search scope"` — `OK`.
- `meson test -C build-meson "Check JNI signature parity"` — `OK`.

## 4. Superset property has teeth

- The containment case was made to fail on purpose: the region box in
  `SearchScopeTest.cpp` ("A region's bounding box never drops a position inside the region") was shrunk to
  `BoxFromCorners(51.60, 7.60, 51.70, 7.70)`, which no longer contains the point box standing in for an
  object inside the region. Rebuild and run gave
  `REQUIRE( naviveylin::GeoBoxContains(region, insideTheRegion) ) with expansion: false`,
  `9 test cases: 8 passed | 1 failed`. Restoring the box and running again gave
  `All tests passed (50 assertions in 9 test cases)`.

## 5. Spec scenario coverage

| Scenario | How it is verified |
|----------|--------------------|
| A hit inside the extent is admitted | Host test: `A scoped search admits a position inside the extent and rejects one outside` (the admitting half). The admission of a free-text hit by the bridge cannot be host-tested (see 6). |
| A hit outside the extent is rejected | Host test: the same case (the rejecting half). The bridge's filter is the same call, see 6. |
| The extent never drops a hit inside its own region | Host test: `A region's bounding box never drops a position inside the region`, plus the teeth check in 4. |
| A region represented only by a position gets a box around it | Host test: `The node-region fallback box contains its point at the documented size`. The bridge's node branch is not host-testable, see 6. |
| An unestablished extent admits every position | Host test: `An unavailable extent admits every position instead of emptying the search`; the bridge reaches the unset box from no region, no database, an unloadable object or an invalid box (`DeriveScopeExtent`). |
| A non-finite position is rejected | Host test: `A non-finite position is never inside a set extent`. |
| A scoped search reports the verdict per result | Not host-testable, see 6. The reported value is exactly the helper's answer for the entry's own position, which the coordinate and structured serialization paths show in the diff. |
| A search without a scope reports every result as inside | Not host-testable, see 6. It follows from the unset box admitting every position (host-tested) and from the field's default. |

## 6. What is not verified here, and why

- The bridge part — deriving the extent from a loaded region object, dropping free-text hits outside it and
  writing the verdict into each result — has no host test: it needs an open database and a resolved admin
  region. The map databases on this machine were format v26 while the library expects v27 when this was
  written (a v27 database, `maps/nordrhein-westfalen`, is available now and 4.4 was probed against it — see
  "Host verification 2026-10-16 (map-dependent items)"), and importing a v27 database was out of scope for
  this change. `tasks.md` 4.4 (verify on a real map) is therefore left unticked. No map-dependent test was
  invented for it.
- `tasks.md` 4.5 (the rest of the C++ and Java suites) was left unticked when this was written: the shared
  `build/` directory holds binaries from other branches and a full-suite run would need every target
  rebuilt, which the environment rules here do not allow; the `JavaScout` Maven suite needs the client jar
  installed into the local Maven repository and a usable map database. It is ticked now — see the "Host
  verification 2026-10-16" section above, which ran both suites from this revision.
- `scripts/format-check.sh` is red repo-wide on this machine, including pristine master files, so it is not
  used as a gate; the added lines follow the style of the code around them.

## 7. Change hygiene

- `git diff --stat` reports exactly the four files of the proposal's Impact section:
  `libosmscout-client-java/src/search_scope.h`, `libosmscout-client-java/src/OSMScoutClient.cpp`,
  `libosmscout-client-java/java/com/framstag/libosmscout/client/LocationEntry.java` and
  `Tests/src/SearchScopeTest.cpp`. No database, map, style-sheet, type-definition or file-format-version file
  is in the diff, so no `FileFormatVersion.md` bump applies.
- `Tests/CMakeLists.txt` and `Tests/meson.build` already register `SearchScopeTest` with the include path of
  `libosmscout-client-java/src`, so no build-file change was needed.
- The existing product-specific namespace and header guard (`naviveylin`, `NAVIVEYLIN_SEARCH_SCOPE_H`) are
  kept as master already carries them from the `test-search-scope-rule` change; the change introduces no new
  product name (see `tasks.md` 1.2).
- `openspec validate client-java-search-scope-extent --strict` — `Change 'client-java-search-scope-extent'
  is valid`.

## Host verification 2026-10-16

Run to close the suite item of the task list (the item now reads `RUN 2026-10-16`). Verified revision:
`38aaf7a18` on branch `todo-backlog-loop-2026-10-16`, whose only delta from `master` (`a79e239c9`) is `TODO.md`, so it carries
`master`'s code. The client jar was rebuilt from that revision
(`build/libosmscout-client-java/libosmscoutclientjava.jar`, sha256 `b6cb25802c513f1a2e448848129d7087acd4efb159eefc02f77714c9b4a2f3af`).

```
cmake --build build -j "$(nproc)"                                  # exit 0, no target failed
QT_QPA_PLATFORM=offscreen TESTS_TOP_DIR=../Tests TESTS_TMP_DIR=./tmp-suite \
  ctest -j 8 --output-on-failure                                   # 100% tests passed out of 158, exit 0, 23.68 s
cd JavaScout && ./mvnw install:install-file -Dfile=../build/libosmscout-client-java/libosmscoutclientjava.jar \
  -DgroupId=net.sf.libosmscout -DartifactId=libosmscout-client-java -Dversion=1.0-SNAPSHOT -Dpackaging=jar -q
./mvnw package -DskipTests                                         # BUILD SUCCESS
xvfb-run ./mvnw test                                               # Tests run: 205, Failures: 0, Errors: 0, Skipped: 38
```

Retained logs: `evidence/cpp-suite-2026-10-16.log`, `evidence/javascout-suite-2026-10-16.log`. Each names its own command and the revision it ran on.

Two facts about the environment, so the numbers can be reproduced:
- a reused `build/` directory whose test objects predate the installed Catch2 fails to link
  (`undefined reference to typeinfo for Catch::ITransientExpression`) until the test translation units are
  recompiled. That is a build-directory artefact, not a source defect; no source file was changed for it.
- the first full-suite run used the reused `build/Tests` as `TESTS_TMP_DIR` and failed `OptimizeAreaWayIdsTest`
  on its stale temp state (`Tests/src/OptimizeAreaWayIdsTest.cpp:383`, see the retained
  `ctest-full.log` beside this change's evidence). The same revision passes that test with a fresh temp
  directory, which is what the numbers above use. The dependence of the test on its temp directory is filed
  in `TODO.md`.

Not covered by this run, and still open in `tasks.md`: 4.4, the scoped search on a real map.

## Host verification 2026-10-16 (map-dependent items)

Task 4.4 was probed on the v27 database `maps/nordrhein-westfalen`. **It stays open, and the probe turned it
into a divergence** between this change's spec delta and what the shipped bridge does on real data.

```bash
# compiled once into build/loop-probes-2026-10-16
java -Djava.library.path=build/libosmscout-client-java \
  -cp build/libosmscout-client-java/libosmscoutclientjava.jar:build/loop-probes-2026-10-16 \
  SearchScopeProbe maps/nordrhein-westfalen stylesheets
```

The scope resolves from the Dortmund centre (51.514/7.465) to handle `1`, region `Innenstadt West`, scope
name `Regierungsbezirk Arnsberg`. Each query is run unscoped and then scoped by that handle:

| query | unscoped | scoped | scoped hits outside `>0.5 deg` of Dortmund | of those, flagged `inSearchScope=false` |
|---|---|---|---|---|
| `Bahnhof` | 9 | 50 | 1 | 0 |
| `Dom` | 2 | 50 | 10 | 0 |
| `Aldi` | 0 | 50 | 0 | 0 |

The scoped `Bahnhof` set spans lat `50.654 .. 51.986`, lon `6.831 .. 9.183` — hits in the Rhineland and
north of the Ruhr, far outside the Regierungsbezirk Arnsberg the scope names — and **every** hit of every
scoped search is flagged `inSearchScope=true` (0 of 150 entries out of scope). So on this data the scoped
search still returns hits from another part of the map's index, and the verdict does not mark them.

What this means for the divergence:

- This change's own spec delta claims a **filter**: "A search with a default admin region SHALL admit only
  hits whose position lies inside the geographic extent of that region, whichever loaded database produced
  the hit, including hits from the free-text index".
- The shipped `OSMScoutClientAdminRegionScopeTest` documents the opposite: "the scope is a preference
  for ranking, not a filter: entries from outside the scope may still be returned ... the scope's observable
  effect is the reported name", and it asserts only `scoped.length > 0`. Re-run on this v27 database on
  2026-10-16 it passes (`Tests run: 2, Failures: 0, Errors: 0, Skipped: 0`, log
  `evidence/admin-region-scope-test-v27-2026-10-16.log`).
- The probe agrees with the test's documentation, not with the spec delta: the observed scoped results are
  not narrowed to the scope's region and no result is reported out of scope. (The bridge does contain the
  free-text extent filter at `OSMScoutClient.cpp:4422-4441`, but its effect is not observable on this data —
  either the extent is not set, which is the documented fail-open case, or it spans the whole database. Its
  drop counter is debug-gated and off by default, so it cannot tell the two apart here.)
- The item's own comparison could not be established either: the unscoped searches return *fewer* or
  different results than the scoped ones (`9 / 2 / 0` vs `50 / 50 / 50`), so "an unscoped search returns it"
  has no observed counterpart.

A reviewer has to decide whether the scope is a filter (spec) or a ranking preference (test, and the observed
behaviour); the task is not ticked. No code was changed.

Log: `evidence/search-scope-probe-nrw-2026-10-16.log` (command, UTC timestamp, exit 0, revision
`06b197a35`). Probe source: `evidence/SearchScopeProbe.java`.

## Host verification 2026-10-27 (task 4.4, after the free-text merge was repaired)

**Superseding result: the task is satisfied, and the 2026-10-16 reading above was misattributed.**

What that earlier probe actually measured, established on 2026-10-27 with a temporarily instrumented bridge
(log lines reverted; see `openspec/changes/archive/2026-10-27-client-java-free-text-hits-survive`):

- Every free-text hit was erased by the bridge's structured-dedup pass before it reached Java
  (`resultsTxt=6`, `refs=3076`, `beforeDedup=50`, `freeText=0`). The extent filter therefore ran over an
  empty list, and the "50 scoped hits spanning the state" in the 2026-10-16 table were **structured**
  results, not text-index hits. The `objectType=boundary_administrative` on street labels already hinted at
  it.
- The 2026-10-16 probe also used `searchLocations(String,int,long)`, an overload that
  `client-java-search-request-lifecycle` removed; `evidence/SearchScopeProbe.java` now uses
  `beginSearch`/`cancelSearch` and compiles against the current jar.

With `2026-10-27-client-java-free-text-hits-survive` archived (two per-database offset sets: the free-text
loop no longer fills the set the dedup pass reads), the same probe on the same database returns
(`evidence/search-scope-probe-nrw-2026-10-27-postfix.log`):

| query | unscoped | scoped | unscoped span | scoped span | unscoped text-index hits outside the scoped span |
|---|---|---|---|---|---|
| `Bahnhof` | 61 | 64 | 50.37..52.24 / 6.46..9.15 | 50.75..51.67 / 7.14..8.51 | 36 of 50 |
| `Hauptstraße` | 50 | 53 | 50.38..52.27 / 6.25..8.77 | 50.88..51.69 / 7.16..8.45 | 47 of 50 |
| `Kirchstraße` | 50 | 51 | 50.39..52.15 / 6.24..8.62 | 50.79..51.67 / 7.15..8.85 | 49 of 50 |

- The extent is established: no `search scope: no extent` warning appears, and an instrumented run reports
  `scopeExtentSet=true` for the scoped calls and `false` for the unscoped ones.
- The scoped sets stay inside the region's extent (lat `50.75 .. 51.69`, lon `7.14 .. 8.51`) while the
  unscoped sets span the whole database (lat `50.37 .. 52.27`, lon `6.24 .. 9.15`), and every scoped hit
  reports `inSearchScope=true` — 0 out of scope, which now means "none was admitted from outside" because
  the filter actually runs.
- Unscoped text-index hits that lie outside the scoped set's span (36, 47 and 49 of 50 per query) are not
  part of the scoped result: the clause "a hit outside the extent is rejected" is observed.

Two limits of the measurement are recorded rather than papered over:

- The derived extent box itself is not exposed to Java, so "outside" is measured against the scoped set's
  own span. That span is necessarily inside the extent (the filter only admits inside hits), so it is a
  conservative proxy: a hit outside the span is outside the extent, a hit inside it is not necessarily.
- The free-text candidate page is capped at the requested limit **before** the extent filter runs
  (`OSMScoutClient.cpp`, the free-text loop breaks at `freeTextEntries.size() >= limit`), so a scoped search
  returns the text hits of that first page that lie inside the extent, not a full page of inside hits. That
  ordering is neither required nor forbidden by this change's delta and is left as it is.

Also unchanged by the re-probe: the shipped `OSMScoutClientAdminRegionScopeTest` documents the scope as a
"preference for ranking" and still passes; the ADDED requirement of this change (a filter with a fail-open
unset case and a per-result verdict) does not contradict that test — the ranking preference it describes is
what the structured path does, and the verdict field is what lets a caller rank below the scope's own hits.
