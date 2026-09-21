# Tasks

Spec references: `poi-search-api` — requirement `POI result data class` (modified) and requirement
`POI results expose the operator and the brand of the found object` (added).

## 1. Native POI result building

- [ ] 1.1 Add the brand feature header to `libosmscout-client-java/src/OSMScoutClient.cpp`, next to
  the existing operator feature header, and verify the translation unit still compiles
  (spec: POI results expose the operator and the brand of the found object).

- [ ] 1.2 Extend the module-local POI result struct in `libosmscout-client-java/src/OSMScoutClient.cpp`
  with an operator and a brand member, default-constructed empty, and verify the file compiles
  (spec: POI results expose the operator and the brand of the found object).

- [ ] 1.3 Fill the two new members when a POI result is built in `libosmscout-client-java/src/OSMScoutClient.cpp`,
  from the found object's operator feature and brand feature independently of the existing label
  fallback chain, and verify the file compiles
  (spec: POI results expose the operator and the brand of the found object).

- [ ] 1.4 Resolve the two new fields and assign them when a result is converted for the caller in
  `libosmscout-client-java/src/OSMScoutClient.cpp`, leaving the surrounding object type, coordinate
  and distance assignments untouched, and verify the file compiles
  (spec: POI results expose the operator and the brand of the found object).

## 2. Java result type

- [ ] 2.1 Add the `operator` and `brand` fields to
  `libosmscout-client-java/java/com/framstag/libosmscout/client/PoiEntry.java` with javadoc stating
  the example value and that the field is empty when the found object carries no such information
  (not null, matching how the adjacent label and object type fields are delivered), and verify the
  Java client sources still compile
  (spec: POI result data class).

- [ ] 2.2 Extend `JavaScout/src/test/java/com/framstag/libosmscout/client/PoiEntryTest.java` so the
  default-constructor scenario asserts both new fields are null on a fresh instance, the populated
  scenario asserts both values round-trip, and a new case covers an entry that carries an operator
  and an empty brand, and verify the test passes with `mvn -Dtest=PoiEntryTest test`
  (spec: POI result data class).

## 3. End-to-end coverage

- [ ] 3.1 Add an assertion to
  `JavaScout/src/test/java/com/framstag/libosmscout/client/OSMScoutClientPoiSearchTest.java`, under
  the existing database-directory assumption, that at least one result of a search carries a
  non-empty operator, that no returned result exposes either attribute as null, and that a result
  whose object carries no such information still carries an empty value without raising an error,
  and verify the test passes when the database directory property is set
  (spec: POI results expose the operator and the brand of the found object).

- [ ] 3.2 Add an assertion to
  `JavaScout/src/test/java/com/framstag/libosmscout/client/OSMScoutClientPoiSearchTest.java` that a
  POI whose label is derived from its operator carries that operator as well, and verify the test
  passes when the database directory property is set
  (spec: POI results expose the operator and the brand of the found object).

- [ ] 3.3 Run the JavaScout test suite against a freshly built native client
  (`JavaScout/test.sh <build-dir>`) and verify it passes, including the new assertions, without
  changing the result count or the ordering assertions already present
  (spec: POI results expose the operator and the brand of the found object).

## 4. Build and regression verification

- [ ] 4.1 Configure and build with CMake, `OSMSCOUT_BUILD_CLIENT_JAVA=ON`, and verify the build
  completes with no errors and no new warnings in the touched translation unit and Java sources.

- [ ] 4.2 Build the same Java client targets with Meson and verify the build completes with no errors
  and no new warnings.

- [ ] 4.3 Run the complete C++ test suite (`ctest -j 2 --output-on-failure`) and verify every existing
  test still passes.

- [ ] 4.4 Run the complete JavaScout Maven test suite and verify every existing test still passes,
  with the database-driven scenarios skipping as before when no database directory is configured.

## 5. Conventions and documentation

- [ ] 5.1 Check every new or modified line against the documents in `guidelines/` and verify the
  touched files satisfy them, in particular the naming, comment and include-order rules.

- [ ] 5.2 Run the formatting and static analysis checks configured for the project on the touched
  files and verify they report no new findings.

- [ ] 5.3 Search the documentation and `AGENTS.md` for a description of the POI result fields and
  verify it either already accounts for the two new attributes or is updated in the same change.
