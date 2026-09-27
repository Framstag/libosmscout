# Tasks

Spec references: `poi-search-api` — requirement `POI result data class` (modified) and requirement
`POI results expose the operator and the brand of the found object` (added).

## 1. Native POI result building

- [x] 1.1 Add the brand feature header to `libosmscout-client-java/src/OSMScoutClient.cpp`, next to
  the existing operator feature header, and verify the translation unit still compiles
  (spec: POI results expose the operator and the brand of the found object).

- [x] 1.2 Extend the module-local POI result struct in `libosmscout-client-java/src/OSMScoutClient.cpp`
  with an operator and a brand member, default-constructed empty, and verify the file compiles
  (spec: POI results expose the operator and the brand of the found object).

- [x] 1.3 Fill the two new members when a POI result is built in `libosmscout-client-java/src/OSMScoutClient.cpp`,
  from the found object's operator feature and brand feature independently of the existing label
  fallback chain, and verify the file compiles
  (spec: POI results expose the operator and the brand of the found object).

- [x] 1.4 Resolve the two new fields and assign them when a result is converted for the caller in
  `libosmscout-client-java/src/OSMScoutClient.cpp`, leaving the surrounding object type, coordinate
  and distance assignments untouched, and verify the file compiles
  (spec: POI results expose the operator and the brand of the found object).

## 2. Java result type

- [x] 2.1 Add the `operator` and `brand` fields to
  `libosmscout-client-java/java/com/framstag/libosmscout/client/PoiEntry.java` with javadoc stating
  the example value and that the field is empty when the found object carries no such information
  (not null, matching how the adjacent label and object type fields are delivered), and verify the
  Java client sources still compile
  (spec: POI result data class).

- [x] 2.2 Extend `JavaScout/src/test/java/com/framstag/libosmscout/client/PoiEntryTest.java` so the
  default-constructor scenario asserts both new fields are null on a fresh instance, the populated
  scenario asserts both values round-trip, and a new case covers an entry that carries an operator
  and an empty brand, and verify the test passes with `mvn -Dtest=PoiEntryTest test`
  (spec: POI result data class).

## 3. End-to-end coverage

- [x] 3.1 Add an assertion to
  `JavaScout/src/test/java/com/framstag/libosmscout/client/OSMScoutClientPoiSearchTest.java`, under
  the existing database-directory assumption, that at least one result of a search carries a
  non-empty operator, that no returned result exposes either attribute as null, and that a result
  whose object carries no such information still carries an empty value without raising an error,
  and verify the test passes when the database directory property is set
  (spec: POI results expose the operator and the brand of the found object).

- [x] 3.2 Add an assertion to
  `JavaScout/src/test/java/com/framstag/libosmscout/client/OSMScoutClientPoiSearchTest.java` that a
  POI whose label is derived from its operator carries that operator as well, and verify the test
  passes when the database directory property is set
  (spec: POI results expose the operator and the brand of the found object).

- [x] 3.3 Run the JavaScout test suite against a freshly built native client
  (`JavaScout/test.sh <build-dir>`) and verify it passes, including the new assertions, without
  changing the result count or the ordering assertions already present
  (spec: POI results expose the operator and the brand of the found object).

## 4. Build and regression verification

- [x] 4.1 Configure and build with CMake, `OSMSCOUT_BUILD_CLIENT_JAVA=ON`, and verify the build
  completes with no errors and no new warnings in the touched translation unit and Java sources.

- [x] 4.2 Build the same Java client targets with Meson and verify the build completes with no errors
  and no new warnings.

- [x] 4.3 Run the complete C++ test suite (`ctest -j 2 --output-on-failure`) and verify every existing
  test still passes.

- [x] 4.4 Run the complete JavaScout Maven test suite and verify every existing test still passes,
  with the database-driven scenarios skipping as before when no database directory is configured.

## 5. Conventions and documentation

- [x] 5.1 Check every new or modified line against the documents in `guidelines/` and verify the
  touched files satisfy them, in particular the naming, comment and include-order rules.

- [ ] 5.2 Run the formatting and static analysis checks configured for the project on the touched
  files and verify they report no new findings.

- [x] 5.3 Search the documentation and `AGENTS.md` for a description of the POI result fields and
  verify it either already accounts for the two new attributes or is updated in the same change.

## Verification Notes

Task 4.3 was closed on an argument that no longer holds. `ctest -j 4 --output-on-failure` reported 124
of 125 tests passing, the single failure being `MapPainterAreaVisibilityCullTest`, case "An area within
the border tolerance is not rejected" (`Tests/src/MapPainterAreaVisibilityCullTest.cpp:659`), which is
not attributable to this change: the change touches only `libosmscout-client-java/src/OSMScoutClient.cpp`
(JNI POI result construction), `PoiEntry.java` and two Java test files, none of which the map painter
loads. That failure is gone - `f0176205b` ("fix: apply the area border tolerance in pixels, not
millimetres") is an ancestor of master through PR #1825, not an unmerged branch, and the case passes.
The whole suite reports 136 of 136 passing, so 4.3 holds on its own terms rather than by exclusion; 5.2
remains unchecked.

Task 5.2 is left unchecked. Static analysis is clean: `clang-tidy -p build
libosmscout-client-java/src/OSMScoutClient.cpp` reports no finding on any added line (4475 pre-existing
diagnostics elsewhere in the file). Formatting cannot be satisfied, and it is not an enforced gate: no
workflow in `.github/workflows/` runs `scripts/format-check.sh`, which requires uncrustify 0.83.0 and
reports nearly the whole tracked tree as unformatted, including files this change never touched.
Measured with `uncrustify -c .uncrustify -l CPP`, the touched file carries 10277 diff lines against the
revision before this change and 10518 against the current one, so the drift is pre-existing and this
change adds no hunk class of its own. The only proposal affecting the added lines is the alignment of
the whole `jfieldID` declaration run, which the committed file does not follow either; aligning only the
two added lines would mix styles within one declaration run, and aligning the run is a whole-file pass.
The drift is recorded in `TODO.md`.

Runtime verification used the locally available extracts `maps/Dortmund` and
`maps/nordrhein-westfalen` (not part of the repository) with `-Dpoi.test.db.dir`. Charging stations
carry the attributes there: Dortmund gives 35 results with 30 operators and 27 operator-derived
labels, north rhine-westphalia gives 50 results with 45 operators, 16 brands and 34 operator-derived
labels. Continuous integration never sets `poi.test.db.dir`, so the database-driven scenarios skip
there, as they already did before this change.
